#include "yylex.h"

#include "commons.h"
#include "lexem.h"
#include "lexembuffer.h"
#include "lexer.h"
#include "globals.h"
#include "options.h"
#include "caching.h"
#include "extract.h"
#include "misc.h"
#include "semact.h"
#include "cxref.h"
#include "cxfile.h"
#include "jslsemact.h"          /* For s_jsl */
#include "reftab.h"
#include "jsemact.h"            /* For s_javaStat */

#include "c_parser.h"
#include "cexp_parser.h"
#include "yacc_parser.h"
#include "java_parser.h"

#include "parsers.h"
#include "protocol.h"

#include "hash.h"

#include "macroargumenttable.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "log.h"

static bool isProcessingPreprocessorIf = false;     /* flag for yylex, to not filter '\n' */



static void setYylvalsForIdentifier(char *name, Symbol *symbol, Position position) {
    uniyylval->ast_id.data = &yyIdBuffer[yyIdBufferIndex];
    yyIdBufferIndex ++; yyIdBufferIndex %= (YYIDBUFFER_SIZE);
    fillId(uniyylval->ast_id.data, name, symbol, position);
    yytext = name;
    uniyylval->ast_id.b = position;
    uniyylval->ast_id.e = position;
    uniyylval->ast_id.e.col += strlen(yytext);
}

static void setYylvalsForPosition(Position position, int length) {
        uniyylval->ast_position.data = position;
        uniyylval->ast_position.b = position;
        uniyylval->ast_position.e = position;
        uniyylval->ast_position.e.col += length;
}

static void setYylvalsForInteger(int val, Position position, int length) {
    uniyylval->ast_integer.data = val;
    uniyylval->ast_integer.b = position;
    uniyylval->ast_integer.e = position;
    uniyylval->ast_integer.e.col += length;
}


/* !!!!!!!!!!!!!!!!!!! to caching !!!!!!!!!!!!!!! */


/* Exceptions: */
#define END_OF_MACRO_ARGUMENT_EXCEPTION -1
#define END_OF_FILE_EXCEPTION -2
#define ON_LEXEM_EXCEPTION_GOTO(lexem, eof_label, eom_label)    \
    switch ((int)lexem) {                                       \
    case END_OF_FILE_EXCEPTION: goto eof_label;                 \
    case END_OF_MACRO_ARGUMENT_EXCEPTION: goto eom_label;       \
    default: ;                                                  \
    }


LexInput currentInput;
int macroStackIndex=0;
static LexInput macroInputStack[MACRO_INPUT_STACK_SIZE];

static char ppMemory[SIZE_ppMemory];
static int ppMemoryIndex=0;



static bool isIdentifierLexem(Lexem lexem) {
    return lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND  || lexem==IDENT_TO_COMPLETE;
}

static bool isConstantLexem(Lexem lexem) {
    return lexem == CONSTANT || lexem == LONG_CONSTANT || lexem == FLOAT_CONSTANT || lexem == DOUBLE_CONSTANT;
}

static bool expandMacroCall(Symbol *mdef, Position *mpos);


/* ************************************************************ */

void initAllInputs(void) {
    MB_INIT();
    includeStackPointer=0;
    macroStackIndex=0;
    isProcessingPreprocessorIf = false;
    resetMacroArgumentTable();
    ppMemoryIndex=0;
    s_olstring[0]=0;
    olstringFound = false;
    olstringServed = false;
    olstringInMacroBody = NULL;
    s_upLevelFunctionCompletionType = NULL;
    s_structRecordCompletionType = NULL;
}


static void fillMacroArgumentTableElement(MacroArgumentTableElement *macroArgTabElem, char *name, char *linkName,
                                int order) {
    macroArgTabElem->name = name;
    macroArgTabElem->linkName = linkName;
    macroArgTabElem->order = order;
}

void fillLexInput(LexInput *input, char *read, char *begin, char *write, char *macroName,
                  InputType inputType) {
    input->read      = read;
    input->begin     = begin;
    input->write     = write;
    input->macroName = macroName;
    input->inputType = inputType;
}

static void setCacheConsistency(Cache *cache, LexInput *input) {
    cache->nextLexemP = input->read;
}
static void setCurrentFileConsistency(FileDescriptor *file, LexInput *input) {
    file->lexemBuffer.read = input->read;
}
static void setCurrentInputConsistency(LexInput *input, FileDescriptor *file) {
    fillLexInput(input, file->lexemBuffer.read, file->lexemBuffer.lexemStream, file->lexemBuffer.write,
                 NULL, INPUT_NORMAL);
}

char *placeIdent(void) {
    static char tt[2*MAX_REF_LEN];
    char fn[MAX_FILE_NAME_SIZE];
    char mm[MAX_REF_LEN];
    int s;
    if (currentFile.fileName!=NULL) {
        if (options.xref2 && options.mode!=ServerMode) {
            strcpy(fn, getRealFileName_static(normalizeFileName(currentFile.fileName, cwd)));
            assert(strlen(fn) < MAX_FILE_NAME_SIZE);
            sprintf(mm, "%s:%d", simpleFileName(fn),currentFile.lineNumber);
            assert(strlen(mm) < MAX_REF_LEN);
            sprintf(tt, "<A HREF=\"file://%s#%d\" %s=%ld>%s</A>", fn, currentFile.lineNumber, PPCA_LEN, (unsigned long)strlen(mm), mm);
        } else {
            sprintf(tt,"%s:%d ",simpleFileName(getRealFileName_static(currentFile.fileName)),currentFile.lineNumber);
        }
        s = strlen(tt);
        assert(s<MAX_REF_LEN);
        return tt;
    }
    return "";
}

static bool isPreprocessorToken(Lexem lexem) {
    return lexem>CPP_TOKENS_START && lexem<CPP_TOKENS_END;
}

static void traceNewline(int lines) {
    for (int i=1; i<=lines; i++) {
        log_trace("Line %s:%d(%d+%d)", currentFile.fileName, currentFile.lineNumber+i, currentFile.lineNumber, i);
    }
}

static void setCurrentFileInfoFor(char *fileName) {
    char *name;
    int number;

    if (fileName==NULL) {
        number = noFileIndex;
        name = getFileItem(number)->name;
    } else {
        bool existed = existsInFileTable(fileName);
        number = addFileNameToFileTable(fileName);
        FileItem *fileItem = getFileItem(number);
        name = fileItem->name;
        checkFileModifiedTime(number);
        bool cxloading = fileItem->cxLoading;
        if (!existed) {
            cxloading = true;
        } else if (options.update==UPDATE_FAST) {
            if (fileItem->isScheduled) {
                // references from headers are not loaded on fast update !
                cxloading = true;
            }
        } else if (options.update==UPDATE_FULL) {
            if (fileItem->scheduledToUpdate) {
                cxloading = true;
            }
        } else {
            cxloading = true;
        }
        if (fileItem->cxSaved) {
            cxloading = false;
        }
        if (LANGUAGE(LANG_JAVA)) {
            if (s_jsl!=NULL || javaPreScanOnly) {
                // do not load (and save) references from jsl loaded files
                // nor during prescanning
                cxloading = fileItem->cxLoading;
            }
        }
        fileItem->cxLoading = cxloading;
    }
    currentFile.characterBuffer.fileNumber = number;
    currentFile.fileName = name;
}

/* ***************************************************************** */
/*                             Init Reading                          */
/* ***************************************************************** */

// it is supposed that one of file or buffer is NULL
void initInput(FILE *file, EditorBuffer *editorBuffer, char *prefix, char *fileName) {
    int     prefixLength, bufferSize, offset;
    char	*bufferStart;

    /* This can be called from various context where one or both are NULL, or we don't know which... */
    prefixLength = strlen(prefix);
    if (editorBuffer != NULL) {
        // read buffer
        assert(prefixLength < editorBuffer->allocation.allocatedFreePrefixSize);
        strncpy(editorBuffer->allocation.text-prefixLength, prefix, prefixLength);
        bufferStart = editorBuffer->allocation.text-prefixLength;
        bufferSize = editorBuffer->allocation.bufferSize+prefixLength;
        offset = editorBuffer->allocation.bufferSize;
        assert(bufferStart > editorBuffer->allocation.allocatedBlock);
    } else {
        // read file
        assert(prefixLength < CHARARACTER_BUFFER_SIZE);
        strcpy(currentFile.characterBuffer.chars, prefix);
        bufferStart = currentFile.characterBuffer.chars;
        bufferSize = prefixLength;
        offset = 0;
    }
    fillFileDescriptor(&currentFile, fileName, bufferStart, bufferSize, file, offset);
    setCurrentFileInfoFor(fileName);
    setCurrentInputConsistency(&currentInput, &currentFile);
    isProcessingPreprocessorIf = false;				/* TODO: WTF??? */
}

static void getAndSetOutPositionIfRequired(char **readPointerP, Position *outPosition) {
    if (outPosition != NULL)
        *outPosition = getLexPositionAt(readPointerP);
    else
        getLexPositionAt(readPointerP);
}

static void getAndSetOutLengthIfRequired(char **readPointerP, int *outLength) {
    if (outLength != NULL)
        *outLength = getLexIntAt(readPointerP);
    else
        getLexIntAt(readPointerP);
}

static void getAndSetOutValueIfRequired(char **readPointerP, int *outValue) {
    if (outValue != NULL)
        *outValue = getLexIntAt(readPointerP);
    else
        getLexIntAt(readPointerP);
}

/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

/* Depending on the passed Lexem will get/pass over subsequent lexems
 * like position, and advance the readPointerP. Allows any of the out
 * parameters to be NULL to ignore it. Might be too complex/general,
 * long argument list, when lex is known could be broken into parts
 * for specific lexem types. */
static void getExtraLexemInformationFor(Lexem lexem, char **readPointerP, int *outLineNumber, int *outValue,
                                        Position *outPosition, int *outLength, bool countLines) {
    if (lexem > MULTI_TOKENS_START) {
        if (isIdentifierLexem(lexem) || lexem == STRING_LITERAL) {
            /* Both are followed by a string and a position */
            *readPointerP = strchr(*readPointerP, '\0') + 1;
            getAndSetOutPositionIfRequired(readPointerP, outPosition);
        } else if (lexem == LINE_TOKEN) {
            int noOfLines = getLexShortAt(readPointerP);
            if (countLines) {
                traceNewline(noOfLines);
                currentFile.lineNumber += noOfLines;
            }
            if (outLineNumber != NULL)
                *outLineNumber = noOfLines;
        } else if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
            getAndSetOutValueIfRequired(readPointerP, outValue);
            getAndSetOutPositionIfRequired(readPointerP, outPosition);
            getAndSetOutLengthIfRequired(readPointerP, outLength);
        } else if (lexem == DOUBLE_CONSTANT || lexem == FLOAT_CONSTANT) {
            getAndSetOutPositionIfRequired(readPointerP, outPosition);
            getAndSetOutLengthIfRequired(readPointerP, outLength);
        } else if (lexem == CPP_MACRO_ARGUMENT) {
            getAndSetOutValueIfRequired(readPointerP, outValue);
            getAndSetOutPositionIfRequired(readPointerP, outPosition);
        } else if (lexem == CHAR_LITERAL) {
            getAndSetOutValueIfRequired(readPointerP, outValue);
            getAndSetOutPositionIfRequired(readPointerP, outPosition);
            getAndSetOutLengthIfRequired(readPointerP, outLength);
        }
    } else if (isPreprocessorToken(lexem)) {
        getAndSetOutPositionIfRequired(readPointerP, outPosition);
    } else if (lexem == '\n' && (countLines)) {
        getAndSetOutPositionIfRequired(readPointerP, outPosition);
        traceNewline(1);
        currentFile.lineNumber++;
    } else {
        getAndSetOutPositionIfRequired(readPointerP, outPosition);
    }
}

static Lexem getLexemSavePrevious(char **previousLexemP) {
    Lexem lexem;

    /* TODO This is weird, shouldn't this test for next @ end? Seems
     * backwards... */
    while (currentInput.read >= currentInput.write) {
        InputType inputType = currentInput.inputType;
        if (macroStackIndex > 0) {
            if (inputType == INPUT_MACRO_ARGUMENT) {
                return END_OF_MACRO_ARGUMENT_EXCEPTION;
            }
            MB_FREE_UNTIL(currentInput.begin);
            currentInput = macroInputStack[--macroStackIndex];
        } else if (inputType == INPUT_NORMAL) {
            setCurrentFileConsistency(&currentFile, &currentInput);
            if (!getLexemFromLexer(&currentFile.lexemBuffer)) {
                return END_OF_FILE_EXCEPTION;
            }
            setCurrentInputConsistency(&currentInput, &currentFile);
        } else {
            cache.nextLexemP = cache.lexemStreamEnd = NULL;
            cacheInput();
            cache.lexemStreamNext = currentFile.lexemBuffer.read;
            setCurrentInputConsistency(&currentInput, &currentFile);
        }
        if (previousLexemP != NULL)
            *previousLexemP = currentInput.read;
    }
    if (previousLexemP != NULL)
        *previousLexemP = currentInput.read;
    lexem = getLexTokenAt(&currentInput.read);
    return lexem;
}

static Lexem getLexem(void) {
    return getLexemSavePrevious(NULL);
}


/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

static void testCxrefCompletionId(Lexem *out_lexem, char *id, Position *pos) {
    Lexem lexem;

    lexem = *out_lexem;
    assert(options.mode);
    if (options.mode == ServerMode) {
        if (lexem==IDENT_TO_COMPLETE) {
            cache.cachingActive = false;
            olstringServed = true;
            if (currentLanguage == LANG_JAVA) {
                makeJavaCompletions(id, strlen(id), pos);
            }
            else if (currentLanguage == LANG_YACC) {
                makeYaccCompletions(id, strlen(id), pos);
            }
            else {
                makeCCompletions(id, strlen(id), pos);
            }
            /* here should be a longjmp to stop file processing !!!! */
            lexem = IDENTIFIER;
        }
    }
    *out_lexem = lexem;
}

/* ********************************** #LINE *********************** */
/* non-static only for unittesting */
protected void processLineDirective(void) {
    Lexem lexem;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, NULL, NULL, true);
    if (lexem != CONSTANT)
        return;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, NULL, NULL, true);
    return;

endOfMacroArgument:
    assert(0);
endOfFile:
    return;
}

/* ********************************* #INCLUDE ********************** */

static void fillIncludeSymbolItem(Symbol *symbol, Position *pos){
    fillSymbol(symbol, LINK_NAME_INCLUDE_REFS, LINK_NAME_INCLUDE_REFS, *pos);
    symbol->type = TypeCppInclude;
}


void addThisFileDefineIncludeReference(int fileIndex) {
    Position position;
    Symbol symbol;

    position = makePosition(fileIndex, 1, 0);
    fillIncludeSymbolItem(&symbol, &position);
    log_trace("adding reference on file %d==%s", fileIndex, getFileItem(fileIndex)->name);
    addCxReference(&symbol, &position, UsageDefined, fileIndex, fileIndex);
}

void addIncludeReference(int fileIndex, Position *position) {
    Symbol symbol;

    log_trace("adding reference on file %d==%s", fileIndex, getFileItem(fileIndex)->name);
    fillIncludeSymbolItem(&symbol, position);
    addCxReference(&symbol, position, UsageUsed, fileIndex, fileIndex);
}

static void addIncludeReferences(int fileIndex, Position *position) {
    addIncludeReference(fileIndex, position);
    addThisFileDefineIncludeReference(fileIndex);
}

void pushInclude(FILE *file, EditorBuffer *buffer, char *name, char *prepend) {
    if (currentInput.inputType == INPUT_CACHE) {
        setCacheConsistency(&cache, &currentInput);
    } else {
        setCurrentFileConsistency(&currentFile, &currentInput);
    }
    includeStack[includeStackPointer] = currentFile;		/* buffers are copied !!!!!!, burk */
    if (includeStackPointer+1 >= INCLUDE_STACK_SIZE) {
        FATAL_ERROR(ERR_ST,"too deep nesting in includes", XREF_EXIT_ERR);
    }
    includeStackPointer ++;
    initInput(file, buffer, prepend, name);
    cacheInclude(currentFile.characterBuffer.fileNumber);
}

void popInclude(void) {
    FileItem *fileItem = getFileItem(currentFile.characterBuffer.fileNumber);
    if (fileItem->cxLoading) {
        fileItem->cxLoaded = true;
    }
    closeCharacterBuffer(&currentFile.characterBuffer);
    if (includeStackPointer != 0) {
        currentFile = includeStack[--includeStackPointer];	/* buffers are copied !!!!!!, burk */
        if (includeStackPointer == 0 && cache.nextLexemP != NULL) {
            fillLexInput(&currentInput, cache.nextLexemP, cache.lexemStream, cache.lexemStreamEnd, NULL,
                         INPUT_CACHE);
        } else {
            setCurrentInputConsistency(&currentInput, &currentFile);
        }
    }
}

static bool openInclude(char includeType, char *name, char **fileName, bool is_include_next) {
    EditorBuffer *editorBuffer = NULL;
    FILE *file = NULL;
    StringList *includeDirP;
    char wildcardExpandedPaths[MAX_OPTION_LEN];
    char normalizedName[MAX_FILE_NAME_SIZE];
    char path[MAX_FILE_NAME_SIZE];

    extractPathInto(currentFile.fileName, path);

    StringList *start = options.includeDirs;

    if (is_include_next) {
        /* #include_next, so find the include path which matches this */
        for (StringList *p = options.includeDirs; p != NULL; p = p->next) {
            char normalizedIncludePath[MAX_FILE_NAME_SIZE];
            strcpy(normalizedIncludePath, normalizeFileName(p->string, cwd));
            int len = strlen(normalizedIncludePath);
            if (normalizedIncludePath[len-1] != FILE_PATH_SEPARATOR) {
                normalizedIncludePath[len] = FILE_PATH_SEPARATOR;
                normalizedIncludePath[len+1] = '\0';
            }
            if (strcmp(normalizedIncludePath, path) == 0) {
                /* And start search from the next */
                start = p->next;
                goto search;
            }
        }
    }

    /* If not an angle bracketed include, look first in the directory of the current file */
    if (includeType != '<') {
        strcpy(normalizedName, normalizeFileName(name, path));
        log_trace("trying to open %s", normalizedName);
        editorBuffer = editorFindFile(normalizedName);
        if (editorBuffer == NULL)
            file = openFile(normalizedName, "r");
    }

    /* If not found we need to walk the include paths... */
 search:
    for (includeDirP = start; includeDirP != NULL && editorBuffer == NULL && file == NULL; includeDirP = includeDirP->next) {
        strcpy(normalizedName, normalizeFileName(includeDirP->string, cwd));
        expandWildcardsInOnePath(normalizedName, wildcardExpandedPaths, MAX_OPTION_LEN);
        MapOverPaths(wildcardExpandedPaths, {
            int length;
            strcpy(normalizedName, currentPath);
            length = strlen(normalizedName);
            if (length > 0 && normalizedName[length - 1] != FILE_PATH_SEPARATOR) {
                normalizedName[length] = FILE_PATH_SEPARATOR;
                length++;
            }
            strcpy(normalizedName + length, name);
            log_trace("trying to open '%s'", normalizedName);
            editorBuffer = editorFindFile(normalizedName);
            if (editorBuffer == NULL)
                file = openFile(normalizedName, "r");
            if (editorBuffer != NULL || file != NULL)
                goto found;
        });
    }
    if (editorBuffer==NULL && file==NULL) {
        log_trace("failed to open '%s'", name);
        return false;
    }
 found:
    strcpy(normalizedName, normalizeFileName(normalizedName, cwd));
    log_debug("opened file '%s'", normalizedName);
    pushInclude(file, editorBuffer, normalizedName, "\n");
    return true;
}

static void processInclude2(Position *includePosition, char includeType, char *includedName, bool is_include_next) {
    char *actualFileName;
    Symbol symbol;
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "PragmaOnce-%s", includedName);

    fillSymbol(&symbol, tmpBuff, tmpBuff, noPosition);
    symbol.type = TypeMacro;
    symbol.storage = StorageNone;

    if (symbolTableIsMember(symbolTable, &symbol, NULL, NULL))
        return;
    if (!openInclude(includeType, includedName, &actualFileName, is_include_next)) {
        assert(options.mode);
        if (options.mode!=ServerMode)
            warningMessage(ERR_CANT_OPEN, includedName);
        else
            log_error("Can't open file '%s'", includedName);
    } else {
        addIncludeReferences(currentFile.characterBuffer.fileNumber, includePosition);
    }
}

/* Non-static only for unittests */
protected void processIncludeDirective(Position *includePosition, bool is_include_next) {
    char *beginningOfLexem, *previousLexemP;
    Lexem lexem;

    lexem = getLexemSavePrevious(&previousLexemP);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    beginningOfLexem = currentInput.read;
    if (lexem == STRING_LITERAL) {         /* Also bracketed "<something>" */
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, NULL, NULL, true);
        processInclude2(includePosition, *beginningOfLexem, beginningOfLexem+1, is_include_next);
    } else {
        // Not "abc" nor <abc>... Possibly a macro id that needs to be expanded?
        currentInput.read = previousLexemP;		/* unget lexem */
        lexem = yylex();
        // TODO Do yylex() expand macros? If so we can get almost anything here...
        if (lexem == STRING_LITERAL) {
            /* But it was a string, so try to include that file... */
            currentInput = macroInputStack[0];		// hack, cut everything pending
            macroStackIndex = 0;
            processInclude2(includePosition, '\"', yytext, is_include_next);
        } else if (lexem == '<') {
            /* Or possibly a bracketed include, don't know why
             * STRING_LITERAL doesn't cover this here as it normally
             * does... */
            warningMessage(ERR_ST,"Include <> after macro expansion not yet implemented, sorry\n\tuse \"\" instead");
        }
    }
    return;

 endOfMacroArgument:
    assert(0);
 endOfFile:
    return;
}

static void addMacroToTabs(Symbol *pp, char *name) {
    int index;
    bool isMember;

    isMember = symbolTableIsMember(symbolTable, pp, &index, NULL);
    if (isMember) {
        log_trace(": masking macro %s", name);
    } else {
        log_trace(": adding macro %s", name);
    }
    symbolTablePush(symbolTable, pp, index);
}

static void setMacroArgumentName(MacroArgumentTableElement *arg, void *at) {
    char **argTab;
    argTab = (char**)at;
    assert(arg->order>=0);
    argTab[arg->order] = arg->name;
}

static void handleMacroDefinitionParameterPositions(int argi, Position *macpos,
                                                    Position *parpos1,
                                                    Position *pos, Position *parpos2,
                                                    int final) {
    if ((options.serverOperation == OLO_GOTO_PARAM_NAME || options.serverOperation == OLO_GET_PARAM_COORDINATES)
        && positionsAreEqual(*macpos, cxRefPosition)) {
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(parpos1);
            } else if (argi < options.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(parpos2);
            }
        } else if (argi == options.olcxGotoVal) {
            parameterPosition = *pos;
            parameterBeginPosition = *parpos1;
            parameterEndPosition = *parpos2;
        }
    }
}

static void handleMacroUsageParameterPositions(int argi, Position *macroPosition,
                                               Position *parpos1, Position *parpos2,
                                               int final
    ) {
    if (options.serverOperation == OLO_GET_PARAM_COORDINATES
        && positionsAreEqual(*macroPosition, cxRefPosition)) {
        log_trace("checking param %d at %d,%d, final==%d", argi, parpos1->col, parpos2->col, final);
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(parpos1);
            } else if (argi < options.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(parpos2);
            }
        } else if (argi == options.olcxGotoVal) {
            parameterBeginPosition = *parpos1;
            parameterEndPosition = *parpos2;
//&fprintf(dumpOut,"regular setting to %d - %d\n", parpos1->col, parpos2->col);
        }
    }
}

static MacroBody *newMacroBody(int macroSize, int argCount, char *name, char *body, char **argumentNames) {
    MacroBody *macroBody;

    PPM_ALLOC(macroBody, MacroBody);
    macroBody->argCount = argCount;
    macroBody->size = macroSize;
    macroBody->name = name;
    macroBody->argumentNames = argumentNames;
    macroBody->body = body;

    return macroBody;
}


static Lexem getNonBlankLexemAndData(Position *position, int *lineNumber, int *value, int *length) {
    Lexem lexem;

    lexem = getLexem();
    while (lexem == LINE_TOKEN) {
        getExtraLexemInformationFor(lexem, &currentInput.read, lineNumber, value, position, length, true);
        lexem = getLexem();
    }
    return lexem;
}


/* Public only for unittesting */
protected void processDefineDirective(bool hasArguments) {
    Lexem lexem;
    Position macroPosition, ppb1, ppb2, *parpos1, *parpos2, *tmppp;
    char *currentLexemStart, *argumentName;
    char **argumentNames, *argLinkName;

    bool isReadingBody = false;
    char *macroName = NULL;
    int allocatedSize = 0;
    Symbol *symbol = NULL;
    char *body = NULL;

    SM_INIT(ppMemory);

    ppb1 = noPosition;
    ppb2 = noPosition;
    parpos1 = &ppb1;
    parpos2 = &ppb2;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    currentLexemStart = currentInput.read;

    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &macroPosition, NULL, true);

    testCxrefCompletionId(&lexem, currentLexemStart, &macroPosition);    /* for cross-referencing */
    if (lexem != IDENTIFIER)
        return;

    PPM_ALLOC(symbol, Symbol);
    fillSymbol(symbol, NULL, NULL, macroPosition);
    symbol->type = TypeMacro;
    symbol->storage = StorageNone;

    /* TODO: this is the only call to setGlobalFileDepNames() that doesn't do it in XX memory, why?
       PP == PreProcessor? */
    setGlobalFileDepNames(currentLexemStart, symbol, MEMORY_PP);
    macroName = symbol->name;

    /* process arguments */
    resetMacroArgumentTable();
    int argumentCount = -1;

    if (hasArguments) {
        Position position;
        lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, parpos2, NULL, true);
        *parpos1 = *parpos2;
        if (lexem != '(')
            goto errorlabel;
        argumentCount++;
        lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        if (lexem != ')') {
            for(;;) {
                Position position;
                currentLexemStart = argumentName = currentInput.read;
                getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
                bool ellipsis = false;
                if (lexem == IDENTIFIER ) {
                    argumentName = currentLexemStart;
                } else if (lexem == ELLIPSIS) {
                    argumentName = s_cppVarArgsName;
                    position = macroPosition;					// hack !!!
                    ellipsis = true;
                } else
                    goto errorlabel;

                char *name;
                PPM_ALLOCC(name, strlen(argumentName)+1, char);
                strcpy(name, argumentName);

                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "%x-%x%c%s", position.file, position.line,
                        LINK_NAME_SEPARATOR, argumentName);
                PPM_ALLOCC(argLinkName, strlen(tmpBuff)+1, char);
                strcpy(argLinkName, tmpBuff);

                MacroArgumentTableElement *macroArgumentTableElement;
                SM_ALLOC(ppMemory, macroArgumentTableElement, MacroArgumentTableElement);
                fillMacroArgumentTableElement(macroArgumentTableElement, name, argLinkName, argumentCount);
                int argumentIndex = addMacroArgument(macroArgumentTableElement);
                argumentCount++;
                lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
                ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

                tmppp=parpos1; parpos1=parpos2; parpos2=tmppp;
                getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, parpos2, NULL, true);
                if (!ellipsis) {
                    addTrivialCxReference(getMacroArgument(argumentIndex)->linkName, TypeMacroArg,StorageDefault,
                                          position, UsageDefined);
                    handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &position, parpos2, 0);
                }
                if (lexem == ELLIPSIS) {
                    lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
                    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
                    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, parpos2, NULL, true);
                }
                if (lexem == ')')
                    break;
                if (lexem != ',')
                    break;

                lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
                ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
            }
            handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &noPosition, parpos2, 1);
        } else {
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, parpos2, NULL, true);
            handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &noPosition, parpos2, 1);
        }
    }

    /* process macro body */
    allocatedSize = MACRO_BODY_BUFFER_SIZE;
    int macroSize = 0;
    PPM_ALLOCC(body, allocatedSize+MAX_LEXEM_SIZE, char);

    isReadingBody = true;

    Position position;
    lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    currentLexemStart = currentInput.read;
    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);

    while (lexem != '\n') {
        while(macroSize<allocatedSize && lexem != '\n') {
            char *lexemDestination = body+macroSize; /* TODO WTF Are we storing the lexems after the body?!?! */
            /* Create a MATE to be able to run ..IsMember() */
            MacroArgumentTableElement macroArgumentTableElement;
            fillMacroArgumentTableElement(&macroArgumentTableElement,currentLexemStart,NULL,0);

            int foundIndex;
            if (lexem==IDENTIFIER && isMemberInMacroArguments(&macroArgumentTableElement,&foundIndex)){
                /* macro argument */
                addTrivialCxReference(getMacroArgument(foundIndex)->linkName, TypeMacroArg, StorageDefault,
                                      position, UsageUsed);
                putLexTokenAt(CPP_MACRO_ARGUMENT, &lexemDestination);
                putLexIntAt(getMacroArgument(foundIndex)->order, &lexemDestination);
                putLexPositionAt(position, &lexemDestination);
            } else {
                if (lexem==IDENT_TO_COMPLETE
                    || (lexem == IDENTIFIER && positionsAreEqual(position, cxRefPosition))) {
                    cache.cachingActive = false;
                    olstringFound = true;
                    olstringInMacroBody = symbol->linkName;
                }
                putLexTokenAt(lexem, &lexemDestination);
                /* Copy from input to destination (which is in the body buffer...) */
                for (; currentLexemStart<currentInput.read; lexemDestination++,currentLexemStart++)
                    *lexemDestination = *currentLexemStart;
            }
            macroSize = lexemDestination - body;
            lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
            ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

            currentLexemStart = currentInput.read;
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        }
        if (lexem != '\n') {
            allocatedSize += MACRO_BODY_BUFFER_SIZE;
            PPM_REALLOCC(body, allocatedSize+MAX_LEXEM_SIZE, char,
                        allocatedSize+MAX_LEXEM_SIZE-MACRO_BODY_BUFFER_SIZE);
        }
    }

endOfBody:
    assert(macroSize>=0);
    PPM_REALLOCC(body, macroSize, char, allocatedSize+MAX_LEXEM_SIZE);
    allocatedSize = macroSize;
    if (argumentCount > 0) {
        PPM_ALLOCC(argumentNames, argumentCount, char*);
        memset(argumentNames, 0, argumentCount*sizeof(char*));
        mapOverMacroArgumentsWithPointer(setMacroArgumentName, argumentNames);
    } else
        argumentNames = NULL;
    MacroBody *macroBody = newMacroBody(allocatedSize, argumentCount, macroName, body, argumentNames);
    symbol->u.mbody = macroBody;

    addMacroToTabs(symbol, macroName);
    assert(options.mode);
    addCxReference(symbol, &macroPosition, UsageDefined, noFileIndex, noFileIndex);
    return;

endOfMacroArgument:
    assert(0);

endOfFile:
    if (isReadingBody)
        goto endOfBody;
    return;

errorlabel:
    return;
}

/* ************************** -D option ************************** */

void addMacroDefinedByOption(char *opt) {
    char *cc,*nopt;
    bool args = false;

    PPM_ALLOCC(nopt,strlen(opt)+3,char);
    strcpy(nopt,opt);
    cc = nopt;
    while (isalpha(*cc) || isdigit(*cc) || *cc == '_') cc++;
    if (*cc == '=') {
        *cc = ' ';
    } else if (*cc==0) {
        *cc++ = ' ';
        *cc++ = '1';
        *cc = 0;
    } else if (*cc=='(') {
        args = true;
    }
    initInput(NULL, NULL, nopt, NULL);
    currentFile.lineNumber = 1;
    processDefineDirective(args);
}

/* ****************************** #UNDEF ************************* */

static void processUndefineDirective(void) {
    Lexem lexem;
    char *cc;
    Position position;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    cc = currentInput.read;
    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
    testCxrefCompletionId(&lexem, cc, &position);
    if (isIdentifierLexem(lexem)) {
        Symbol symbol;
        Symbol *member;

        log_debug(": undef macro %s", cc);

        fillSymbol(&symbol, cc, cc, position);
        symbol.type = TypeMacro;
        symbol.storage = StorageNone;

        assert(options.mode);
        /* !!!!!!!!!!!!!! tricky, add macro with mbody == NULL !!!!!!!!!! */
        /* this is because of monotonicity for caching, just adding symbol */
        if (symbolTableIsMember(symbolTable, &symbol, NULL, &member)) {
            Symbol *pp;
            addCxReference(member, &position, UsageUndefinedMacro, noFileIndex, noFileIndex);

            PPM_ALLOC(pp, Symbol);
            fillSymbol(pp, member->name, member->linkName, position);
            pp->type = TypeMacro;
            pp->storage = StorageNone;

            addMacroToTabs(pp, member->name);
        }
    }
    while (lexem != '\n') {
        lexem = getLexem();
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
    }
    return;
endOfMacroArgument:
    assert(0);
endOfFile:
    return;
}

/* ********************************* #IFDEF ********************** */

static void processIfDirective();

enum deleteUntilReturn {
    UNTIL_NOTHING,
    UNTIL_ENDIF,
    UNTIL_ELIF,
    UNTIL_ELSE,
};

static void genCppIfElseReference(int level, Position *pos, int usage) {
    char                ttt[TMP_STRING_SIZE];
    Position			dp;
    CppIfStack       *ss;
    if (level > 0) {
      PPM_ALLOC(ss, CppIfStack);
      ss->position = *pos;
      ss->next = currentFile.ifStack;
      currentFile.ifStack = ss;
    }
    if (currentFile.ifStack!=NULL) {
      dp = currentFile.ifStack->position;
      sprintf(ttt,"CppIf%x-%x-%d", dp.file, dp.col, dp.line);
      addTrivialCxReference(ttt, TypeCppIfElse, StorageDefault, *pos, usage);
      if (level < 0) currentFile.ifStack = currentFile.ifStack->next;
    }
}

static int cppDeleteUntilEndElse(bool untilEnd) {
    Lexem lexem;
    int depth;
    Position position;

    depth = 1;
    while (depth > 0) {
        lexem = getLexem();
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        if (lexem==CPP_IF || lexem==CPP_IFDEF || lexem==CPP_IFNDEF) {
            genCppIfElseReference(1, &position, UsageDefined);
            depth++;
        } else if (lexem == CPP_ENDIF) {
            depth--;
            genCppIfElseReference(-1, &position, UsageUsed);
        } else if (lexem == CPP_ELIF) {
            genCppIfElseReference(0, &position, UsageUsed);
            if (depth == 1 && !untilEnd) {
                log_debug("#elif ");
                return UNTIL_ELIF;
            }
        } else if (lexem == CPP_ELSE) {
            genCppIfElseReference(0, &position, UsageUsed);
            if (depth == 1 && !untilEnd) {
                log_debug("#else");
                return UNTIL_ELSE;
            }
        }
    }
    log_debug("#endif");
    return UNTIL_ENDIF;

endOfMacroArgument:
    assert(0);
endOfFile:;
    if (options.mode!=ServerMode) {
        warningMessage(ERR_ST,"end of file in cpp conditional");
    }
    return UNTIL_ENDIF;
}

static void execCppIf(bool deleteSource) {
    int onElse;
    if (!deleteSource)
        currentFile.ifDepth ++;
    else {
        onElse = cppDeleteUntilEndElse(false);
        if (onElse==UNTIL_ELSE) {
            /* #if #else */
            currentFile.ifDepth ++;
        } else if (onElse==UNTIL_ELIF)
            processIfDirective();
    }
}

static void processIfdefDirective(bool isIfdef) {
    Lexem lexem;
    char *cp;
    Position position;
    bool deleteSrc;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    /* Then we are probably looking at the id... */
    cp = currentInput.read;
    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
    testCxrefCompletionId(&lexem, cp, &position);

    if (!isIdentifierLexem(lexem))
        return;

    Symbol symbol;
    fillSymbol(&symbol, cp, cp, noPosition);
    symbol.type = TypeMacro;
    symbol.storage = StorageNone;

    Symbol *member;
    bool isMember = symbolTableIsMember(symbolTable, &symbol, NULL, &member);
    if (isMember && member->u.mbody==NULL)
        isMember = false;	// undefined macro
    if (isMember) {
        addCxReference(member, &position, UsageUsed, noFileIndex, noFileIndex);
        if (isIfdef) {
            log_debug("#ifdef (true)");
            deleteSrc = false;
        } else {
            log_debug("#ifndef (false)");
            deleteSrc = true;
        }
    } else {
        if (isIfdef) {
            log_debug("#ifdef (false)");
            deleteSrc = true;
        } else {
            log_debug("#ifndef (true)");
            deleteSrc = false;
        }
    }
    execCppIf(deleteSrc);
    return;
endOfMacroArgument:
    assert(0);
endOfFile:
    return;
}

/* ********************************* #IF ************************** */

Lexem cexp_yylex(void) {
    int res, mm;
    Lexem lexem;
    char *cc;
    Symbol symbol, *foundMember;
    Position position;
    bool haveParenthesis;

    lexem = yylex();
    if (isIdentifierLexem(lexem)) {
        // this is useless, as it would be set to 0 anyway
        lexem = cexpTranslateToken(CONSTANT, 0);
    } else if (lexem == CPP_DEFINED_OP) {
        lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        cc = currentInput.read;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        if (lexem == '(') {
            haveParenthesis = true;

            lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
            ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

            cc = currentInput.read;
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        } else {
            haveParenthesis = false;
        }

        if (!isIdentifierLexem(lexem))
            return 0;

        fillSymbol(&symbol, cc, cc, noPosition);
        symbol.type = TypeMacro;
        symbol.storage = StorageNone;

        log_debug("(%s)", symbol.name);

        mm = symbolTableIsMember(symbolTable, &symbol, NULL, &foundMember);
        if (mm && foundMember->u.mbody == NULL)
            mm = 0;   // undefined macro
        assert(options.mode);
        if (mm)
            addCxReference(&symbol, &position, UsageUsed, noFileIndex, noFileIndex);

        /* following call sets uniyylval */
        res = cexpTranslateToken(CONSTANT, mm);
        if (haveParenthesis) {
            lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
            ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
            if (lexem != ')' && options.mode!=ServerMode) {
                warningMessage(ERR_ST,"missing ')' after defined( ");
            }
        }
        lexem = res;
    } else {
        lexem = cexpTranslateToken(lexem, uniyylval->ast_integer.data);
    }
    return lexem;
endOfMacroArgument:
    assert(0);
endOfFile:
    return 0;
}

static void processIfDirective(void) {
    int parseError, lexem;
    isProcessingPreprocessorIf = true;
    log_debug(": #if");
    parseError = cexp_yyparse();
    do
        lexem = yylex();
    while (lexem != '\n');
    isProcessingPreprocessorIf = false;
    execCppIf(!parseError);
}

static void processPragmaDirective(void) {
    Lexem lexem;
    char *mname, *fname;
    Position position;
    Symbol *pp;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    if (lexem == IDENTIFIER && strcmp(currentInput.read, "once")==0) {
        char tmpBuff[TMP_BUFF_SIZE];
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        fname = simpleFileName(getFileItem(position.file)->name);
        sprintf(tmpBuff, "PragmaOnce-%s", fname);
        PPM_ALLOCC(mname, strlen(tmpBuff)+1, char);
        strcpy(mname, tmpBuff);

        PPM_ALLOC(pp, Symbol);
        fillSymbol(pp, mname, mname, position);
        pp->type = TypeMacro;
        pp->storage = StorageNone;

        symbolTableAdd(symbolTable, pp);
    }
    while (lexem != '\n') {
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        lexem = getLexem();
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
    }
    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
    return;

endOfMacroArgument:
    assert(0);
endOfFile:
    return;
}

/* ***************************************************************** */
/*                                 CPP                               */
/* ***************************************************************** */

static bool processPreprocessorConstruct(Lexem lexem) {
    Position position;

    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
    log_debug("processing cpp-construct '%s' ", tokenNamesTable[lexem]);
    switch (lexem) {
    case CPP_INCLUDE:
        processIncludeDirective(&position, false);
        break;
    case CPP_INCLUDE_NEXT:
        processIncludeDirective(&position, true);
        break;
    case CPP_DEFINE0:
        processDefineDirective(false);
        break;
    case CPP_DEFINE:
        processDefineDirective(true);
        break;
    case CPP_UNDEF:
        processUndefineDirective();
        break;
    case CPP_IFDEF:
        genCppIfElseReference(1, &position, UsageDefined);
        processIfdefDirective(true);
        break;
    case CPP_IFNDEF:
        genCppIfElseReference(1, &position, UsageDefined);
        processIfdefDirective(false);
        break;
    case CPP_IF:
        genCppIfElseReference(1, &position, UsageDefined);
        processIfDirective();
        break;
    case CPP_ELIF:
        log_debug("#elif");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, &position, UsageUsed);
            currentFile.ifDepth --;
            cppDeleteUntilEndElse(true);
        } else if (options.mode!=ServerMode) {
            warningMessage(ERR_ST,"unmatched #elif");
        }
        break;
    case CPP_ELSE:
        log_debug("#else");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, &position, UsageUsed);
            currentFile.ifDepth --;
            cppDeleteUntilEndElse(true);
        } else if (options.mode!=ServerMode) {
            warningMessage(ERR_ST,"unmatched #else");
        }
        break;
    case CPP_ENDIF:
        log_debug("#endif");
        if (currentFile.ifDepth) {
            currentFile.ifDepth --;
            genCppIfElseReference(-1, &position, UsageUsed);
        } else if (options.mode!=ServerMode) {
            warningMessage(ERR_ST,"unmatched #endif");
        }
        break;
    case CPP_PRAGMA:
        log_debug("#pragma");
        processPragmaDirective();
        break;
    case CPP_LINE: {
        processLineDirective();

        lexem = getLexem();
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
        while (lexem != '\n') {
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
            lexem = getLexem();
            ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
        }
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        break;
    }
    default:
        log_fatal("Unknown lexem encountered");
    }
    return true;

endOfMacroArgument:
    assert(0);
endOfFile:
    return false;
}

/* ******************************************************************** */
/* *********************   MACRO CALL EXPANSION *********************** */
/* ******************************************************************** */

#define ExpandPreprocessorBufferIfOverflow(bcc,buf,size) {              \
        if (bcc >= buf+size) {                                          \
            size += MACRO_BODY_BUFFER_SIZE;                             \
            PPM_REALLOCC(buf,size+MAX_LEXEM_SIZE,char,                  \
                         size+MAX_LEXEM_SIZE-MACRO_BODY_BUFFER_SIZE);   \
        }                                                               \
    }
#define ExpandMacroBodyBufferIfOverflow(bcc,len,buf,size) {             \
        while (bcc + len >= buf + size) {                               \
            size += MACRO_BODY_BUFFER_SIZE;                             \
            MB_REALLOCC(buf,size+MAX_LEXEM_SIZE,char,                   \
                        size+MAX_LEXEM_SIZE-MACRO_BODY_BUFFER_SIZE);    \
        }                                                               \
    }

/* *********************************************************** */

static bool cyclicCall(MacroBody *macroBody) {
    char *name = macroBody->name;
    log_debug("Testing for cyclic call, '%s' against curr '%s'", name, currentInput.macroName);
    if (currentInput.macroName != NULL && strcmp(name,currentInput.macroName)==0)
        return true;
    for (int i=0; i<macroStackIndex; i++) {
        LexInput *lexInput = &macroInputStack[i];
        log_debug("Testing '%s' against '%s'", name, lexInput->macroName);
        if (lexInput->macroName != NULL && strcmp(name,lexInput->macroName)==0)
            return true;
    }
    return false;
}


static void prependMacroInput(LexInput *argumentBuffer) {
    assert(macroStackIndex < MACRO_INPUT_STACK_SIZE-1);
    macroInputStack[macroStackIndex++] = currentInput;
    currentInput = *argumentBuffer;
    currentInput.read = currentInput.begin;
    currentInput.inputType = INPUT_MACRO;
}


static void expandMacroArgument(LexInput *argumentBuffer) {
    Symbol sd, *memb;
    char *previousLexem, *nextLexemP, *tbcc;
    bool failedMacroExpansion;
    Lexem lexem;
    Position position;
    char *buf;
    char *bcc;
    int bsize;

    bsize = MACRO_BODY_BUFFER_SIZE;

    prependMacroInput(argumentBuffer);

    currentInput.inputType = INPUT_MACRO_ARGUMENT;
    PPM_ALLOCC(buf,bsize+MAX_LEXEM_SIZE,char);
    bcc = buf;

    for(;;) {
    nextLexem:
        lexem = getLexemSavePrevious(&previousLexem);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        nextLexemP = currentInput.read;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    macroStackIndex == 0);
        int length = ((char*)currentInput.read) - previousLexem;
        assert(length >= 0);
        memcpy(bcc, previousLexem, length);
        // a hack, it is copied, but bcc will be increased only if not
        // an expanding macro, this is because 'macroCallExpand' can
        // read new lexbuffer and destroy cInput, so copy it now.
        failedMacroExpansion = false;
        if (lexem == IDENTIFIER) {
            fillSymbol(&sd, nextLexemP, nextLexemP, noPosition);
            sd.type = TypeMacro;
            sd.storage = StorageNone;
            if (symbolTableIsMember(symbolTable, &sd, NULL, &memb)) {
                /* it is a macro, provide macro expansion */
                if (expandMacroCall(memb,&position))
                    goto nextLexem;
                else
                    failedMacroExpansion = true;
            }
        }
        if (failedMacroExpansion) {
            tbcc = bcc;
            assert(memb!=NULL);
            if (memb->u.mbody!=NULL && cyclicCall(memb->u.mbody))
                putLexTokenAt(IDENT_NO_CPP_EXPAND, &tbcc);
        }
        bcc += length;
        ExpandPreprocessorBufferIfOverflow(bcc, buf, bsize);
    }
endOfMacroArgument:
    currentInput = macroInputStack[--macroStackIndex];
    PPM_REALLOCC(buf, bcc-buf, char, bsize+MAX_LEXEM_SIZE);
    fillLexInput(argumentBuffer, buf, buf, bcc, NULL, INPUT_NORMAL);
    return;
endOfFile:
    assert(0);
}

static void cxAddCollateReference(char *sym, char *cs, Position *position) {
    char tempString[TMP_STRING_SIZE];
    strcpy(tempString,sym);
    assert(cs>=sym && cs-sym<TMP_STRING_SIZE);
    sprintf(tempString+(cs-sym), "%c%c%s", LINK_NAME_COLLATE_SYMBOL,
            LINK_NAME_COLLATE_SYMBOL, cs);
    addTrivialCxReference(tempString, TypeCppCollate,StorageDefault, *position, UsageDefined);
}


/* **************************************************************** */

static void collate(char **_lastBufferP, char **_currentBufferP, char *buffer, int *_bufferSize,
                    char **_currentBodyLexemP, LexInput *actualArgumentsInput) {
    char *lastBufferP,*currentBufferP,*currentInputLexemP,*endOfInputLexems,*currentBodyLexemP;
    int bufferSize;

    currentBodyLexemP = *_currentBodyLexemP;
    lastBufferP = *_lastBufferP;
    currentBufferP = *_currentBufferP;
    bufferSize = *_bufferSize;

    if (peekLexTokenAt(lastBufferP) == CPP_MACRO_ARGUMENT) {
        currentBufferP = lastBufferP;
        Lexem lexem = getLexTokenAt(&lastBufferP);
        int value;
        assert(lexem==CPP_MACRO_ARGUMENT);
        getExtraLexemInformationFor(lexem, &lastBufferP, NULL, &value, NULL, NULL, false);
        currentInputLexemP = actualArgumentsInput[value].begin;
        endOfInputLexems = actualArgumentsInput[value].write;
        lastBufferP = NULL;
        while (currentInputLexemP < endOfInputLexems) {
            char *lexemStart = currentInputLexemP;
            Lexem lexem = getLexTokenAt(&currentInputLexemP);
            getExtraLexemInformationFor(lexem, &currentInputLexemP, NULL, NULL, NULL, NULL, false);
            lastBufferP = currentBufferP;
            assert(currentInputLexemP>=lexemStart);
            int lexemLength = currentInputLexemP-lexemStart;
            memcpy(currentBufferP, lexemStart, lexemLength);
            currentBufferP += lexemLength;
            ExpandPreprocessorBufferIfOverflow(currentBufferP,buffer,bufferSize);
        }
    }

    if (peekLexTokenAt(currentBodyLexemP) == CPP_MACRO_ARGUMENT) {
        Lexem lexem = getLexTokenAt(&currentBodyLexemP);
        int value;
        getExtraLexemInformationFor(lexem, &currentBodyLexemP, NULL, &value, NULL, NULL, false);
        currentInputLexemP = actualArgumentsInput[value].begin;
        endOfInputLexems = actualArgumentsInput[value].write;
    } else {
        currentInputLexemP = currentBodyLexemP;
        Lexem lexem = getLexTokenAt(&currentBodyLexemP);
        getExtraLexemInformationFor(lexem, &currentBodyLexemP, NULL, NULL, NULL, NULL, false);
        endOfInputLexems = currentBodyLexemP;
    }

    /* now collate *lbcc and *cc */
    // berk, do not pre-compute, lbcc can be NULL!!!!
    if (lastBufferP != NULL && currentInputLexemP < endOfInputLexems && isIdentifierLexem(peekLexTokenAt(lastBufferP))) {
        Lexem nextLexem = peekLexTokenAt(currentInputLexemP);
        if (isIdentifierLexem(nextLexem) || isConstantLexem(nextLexem)) {
            Position position;
            /* TODO collation of all lexem pairs */
            int len  = strlen(lastBufferP + IDENT_TOKEN_SIZE);
            Lexem lexem = getLexTokenAt(&currentInputLexemP);
            int value;
            char *previousInputLexemP = currentInputLexemP;
            getExtraLexemInformationFor(lexem, &currentInputLexemP, NULL, &value, &position, NULL, false);
            currentBufferP = lastBufferP + IDENT_TOKEN_SIZE + len;
            assert(*currentBufferP == 0);
            if (isIdentifierLexem(lexem)) {
                /* TODO: WTF Here was a out-commented call to
                 * NextLexPosition(), maybe the following "hack"
                 * replaced that macro */
                strcpy(currentBufferP, previousInputLexemP);
                // the following is a hack as # is part of ## symbols
                position.col--;
                assert(position.col >= 0);
                cxAddCollateReference(lastBufferP + IDENT_TOKEN_SIZE, currentBufferP, &position);
                position.col++;
            } else /* isConstantLexem() */ {
                /* TODO: We should replace the NextLexPosition() macro
                 * with the C function nextLexPosition(). But the
                 * parameters here is weird (bcc+1). Also we have no
                 * coverage for this code. Why is that? Because we
                 * never have a constant in this position... */
                NextLexPosition(position, currentBufferP + 1); /* new identifier position*/
                sprintf(currentBufferP, "%d", value);
                cxAddCollateReference(lastBufferP + IDENT_TOKEN_SIZE, currentBufferP, &position);
            }
            currentBufferP += strlen(currentBufferP);
            assert(*currentBufferP == 0);
            currentBufferP++;
            putLexPositionAt(position, &currentBufferP);
        }
    }
    ExpandPreprocessorBufferIfOverflow(currentBufferP, buffer, bufferSize);
    while (currentInputLexemP < endOfInputLexems) {
        char *lexemStart   = currentInputLexemP;
        Lexem lexem = getLexTokenAt(&currentInputLexemP);
        getExtraLexemInformationFor(lexem, &currentInputLexemP, NULL, NULL, NULL, NULL, false);
        lastBufferP = currentBufferP;
        assert(currentInputLexemP >= lexemStart);
        memcpy(currentBufferP, lexemStart, currentInputLexemP - lexemStart);
        currentBufferP += currentInputLexemP - lexemStart;
        ExpandPreprocessorBufferIfOverflow(currentBufferP, buffer, bufferSize);
    }

    *_lastBufferP  = lastBufferP;
    *_currentBufferP   = currentBufferP;
    *_currentBodyLexemP   = currentBodyLexemP;
    *_bufferSize = bufferSize;
}

static void macroArgumentsToString(char *res, LexInput *lexInput) {
    char *cc, *lcc, *bcc;
    int value;

    bcc = res;
    *bcc = 0;
    cc = lexInput->begin;
    while (cc < lexInput->write) {
        Lexem lexem = getLexTokenAt(&cc);
        lcc = cc;
        getExtraLexemInformationFor(lexem, &cc, NULL, &value, NULL, NULL, false);
        if (isIdentifierLexem(lexem)) {
            sprintf(bcc, "%s", lcc);
            bcc+=strlen(bcc);
        } else if (lexem==STRING_LITERAL) {
            sprintf(bcc,"\"%s\"", lcc);
            bcc+=strlen(bcc);
        } else if (lexem==CONSTANT) {
            sprintf(bcc,"%d", value);
            bcc+=strlen(bcc);
        } else if (lexem < 256) {
            sprintf(bcc,"%c",lexem);
            bcc+=strlen(bcc);
        }
    }
}

static char *replaceMacroArguments(LexInput *actualArgumentsInput, char *readBuffer, char **_bcc) {
    char *currentLexemStart, *writePointer = *_bcc;
    int   bufferSize;
    char *writeBuffer;

    char *currentLexemP = readBuffer;
    char *endOfLexems   = writePointer;
    bufferSize          = MACRO_BODY_BUFFER_SIZE;
    MB_ALLOCC(writeBuffer, bufferSize + MAX_LEXEM_SIZE, char);
    writePointer = writeBuffer;
    while (currentLexemP < endOfLexems) {
        int      value;
        Position position;
        Lexem    lexem;

        currentLexemStart = currentLexemP;
        lexem = getLexTokenAt(&currentLexemP);
        getExtraLexemInformationFor(lexem, &currentLexemP, NULL, &value, &position, NULL, false);
        if (lexem == CPP_MACRO_ARGUMENT) {
            int len = actualArgumentsInput[value].write - actualArgumentsInput[value].begin;
            ExpandMacroBodyBufferIfOverflow(writePointer, len, writeBuffer, bufferSize);
            memcpy(writePointer, actualArgumentsInput[value].begin, len);
            writePointer += len;
        } else if (lexem == '#' && currentLexemP < endOfLexems
                   && peekLexTokenAt(currentLexemP) == CPP_MACRO_ARGUMENT) {
            lexem = getLexTokenAt(&currentLexemP);
            assert(lexem == CPP_MACRO_ARGUMENT);
            getExtraLexemInformationFor(lexem, &currentLexemP, NULL, &value, NULL, NULL, false);
            putLexTokenAt(STRING_LITERAL, &writePointer);
            ExpandMacroBodyBufferIfOverflow(writePointer, MACRO_BODY_BUFFER_SIZE, writeBuffer, bufferSize);
            macroArgumentsToString(writePointer, &actualArgumentsInput[value]);

            /* Move over something that looks like a string... */
            int len = strlen(writePointer) + 1;
            writePointer += len;

            /* TODO: This should really be putLexPosition() but that takes a LexemBuffer... */
            putLexPositionAt(position, &writePointer);
            if (len >= MACRO_BODY_BUFFER_SIZE - 15) {
                errorMessage(ERR_INTERNAL, "size of #macro_argument exceeded MACRO_BODY_BUFFER_SIZE");
            }
        } else {
            ExpandMacroBodyBufferIfOverflow(writePointer, (currentLexemStart - currentLexemP), writeBuffer, bufferSize);
            assert(currentLexemP >= currentLexemStart);
            memcpy(writePointer, currentLexemStart, currentLexemP - currentLexemStart);
            writePointer += currentLexemP - currentLexemStart;
        }
        ExpandMacroBodyBufferIfOverflow(writePointer, 0, writeBuffer, bufferSize);
    }
    MB_REALLOCC(writeBuffer, writePointer - writeBuffer, char, bufferSize + MAX_LEXEM_SIZE);
    *_bcc = writePointer;

    return writeBuffer;
}

/* ************************************************************** */
/* ********************* macro body replacement ***************** */
/* ************************************************************** */

static void createMacroBodyAsNewInput(LexInput *inputToSetup, MacroBody *macroBody, LexInput *actualArgumentsInput,
                                      int actualArgumentCount) {

    /* first make ## collations */
    char *currentBodyLexemP = macroBody->body;
    char *endOfBodyLexems   = macroBody->body + macroBody->size;
    int   bufferSize    = MACRO_BODY_BUFFER_SIZE;
    char *currentBufferP, *lastBufferP;
    char *buffer;
    PPM_ALLOCC(buffer, bufferSize + MAX_LEXEM_SIZE, char);
    currentBufferP  = buffer;
    lastBufferP = NULL;
    while (currentBodyLexemP < endOfBodyLexems) {
        char *lexemStart   = currentBodyLexemP;
        Lexem lexem = getLexTokenAt(&currentBodyLexemP);
        getExtraLexemInformationFor(lexem, &currentBodyLexemP, NULL, NULL, NULL, NULL, false);
        if (lexem == CPP_COLLATION && lastBufferP != NULL && currentBodyLexemP < endOfBodyLexems) {
            collate(&lastBufferP, &currentBufferP, buffer, &bufferSize, &currentBodyLexemP, actualArgumentsInput);
        } else {
            lastBufferP = currentBufferP;
            assert(currentBodyLexemP >= lexemStart);
            /* Copy this lexem over from body to buffer */
            int lexemLength = currentBodyLexemP - lexemStart;
            memcpy(currentBufferP, lexemStart, lexemLength);
            currentBufferP += lexemLength;
        }
        ExpandPreprocessorBufferIfOverflow(currentBufferP, buffer, bufferSize);
    }
    PPM_REALLOCC(buffer, currentBufferP - buffer, char, bufferSize + MAX_LEXEM_SIZE);

    /* expand arguments */
    for (int i = 0; i < actualArgumentCount; i++) {
        expandMacroArgument(&actualArgumentsInput[i]);
    }

    /* replace arguments */
    char *buf2 = replaceMacroArguments(actualArgumentsInput, buffer, &currentBufferP);

    fillLexInput(inputToSetup, buf2, buf2, currentBufferP, macroBody->name, INPUT_MACRO);
}

/* *************************************************************** */
/* ******************* MACRO CALL PROCESS ************************ */
/* *************************************************************** */

static Lexem getLexSkippingLines(char **previousLexemP, int *lineNumberP,
                                int *valueP, Position *positionP, int *lengthP) {
    Lexem lexem = getLexemSavePrevious(previousLexemP);
    while (lexem == LINE_TOKEN || lexem == '\n') {
        getExtraLexemInformationFor(lexem, &currentInput.read, lineNumberP, valueP, positionP, lengthP,
                                    macroStackIndex == 0);
        lexem = getLexemSavePrevious(previousLexemP);
    }
    return lexem;
}

static void getActualMacroArgument(
    char *previousLexem,
    Lexem *inOutLexem,
    Position *mpos,
    Position **positionOfFirstParenthesis,
    Position **positionOfSecondParenthesis,
    LexInput *actualArgumentsInput,
    MacroBody *macroBody,
    int actualArgumentIndex
) {
    int poffset;
    int depth;
    Lexem lexem;
    char *buf;
    char *bcc;
    int bufferSize;

    lexem = *inOutLexem;
    bufferSize = MACRO_ARGUMENTS_BUFFER_SIZE;
    depth = 0;
    PPM_ALLOCC(buf, bufferSize+MAX_LEXEM_SIZE, char);
    bcc = buf;
    /* if lastArgument, collect everything there */
    poffset = 0;
    while (((lexem != ',' || actualArgumentIndex+1==macroBody->argCount) && lexem != ')') || depth > 0) {
        // The following should be equivalent to the loop condition:
        //& if (lexem == ')' && depth <= 0) break;
        //& if (lexem == ',' && depth <= 0 && ! lastArgument) break;
        if (lexem == '(') depth ++;
        if (lexem == ')') depth --;
        for(;previousLexem < currentInput.read; previousLexem++, bcc++)
            *bcc = *previousLexem;
        if (bcc-buf >= bufferSize) {
            bufferSize += MACRO_ARGUMENTS_BUFFER_SIZE;
            PPM_REALLOCC(buf, bufferSize+MAX_LEXEM_SIZE, char,
                    bufferSize+MAX_LEXEM_SIZE-MACRO_ARGUMENTS_BUFFER_SIZE);
        }
        lexem = getLexSkippingLines(&previousLexem, NULL, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, (*positionOfSecondParenthesis),
                                    NULL, macroStackIndex == 0);
        if ((lexem == ',' || lexem == ')') && depth == 0) {
            poffset ++;
            handleMacroUsageParameterPositions(actualArgumentIndex+poffset, mpos, *positionOfFirstParenthesis, *positionOfSecondParenthesis, 0);
            **positionOfFirstParenthesis= **positionOfSecondParenthesis;
        }
    }
    if (0) {  /* skip the error message when finished normally */
endOfFile:;
endOfMacroArgument:;
        assert(options.mode);
        if (options.mode!=ServerMode) {
            warningMessage(ERR_ST,"[getActMacroArgument] unterminated macro call");
        }
    }
    PPM_REALLOCC(buf, bcc-buf, char, bufferSize+MAX_LEXEM_SIZE);
    fillLexInput(actualArgumentsInput, buf, buf, bcc, currentInput.macroName, INPUT_NORMAL);
    *inOutLexem = lexem;
    return;
}

static LexInput *getActualMacroArguments(MacroBody *macroBody, Position *macroPosition,
                                         Position lparPosition) {
    char *previousLexem;
    Lexem lexem;
    Position position, ppb1, ppb2, *parpos1, *parpos2;
    int argumentIndex = 0;
    LexInput *actualArgs;

    ppb1 = lparPosition;
    ppb2 = lparPosition;
    parpos1 = &ppb1;
    parpos2 = &ppb2;
    PPM_ALLOCC(actualArgs, macroBody->argCount, LexInput);
    lexem = getLexSkippingLines(&previousLexem, NULL, NULL, &position, NULL);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                macroStackIndex == 0);
    if (lexem == ')') {
        *parpos2 = position;
        handleMacroUsageParameterPositions(0, macroPosition, parpos1, parpos2, 1);
    } else {
        for(;;) {
            getActualMacroArgument(previousLexem, &lexem, macroPosition, &parpos1, &parpos2,
                                   &actualArgs[argumentIndex], macroBody, argumentIndex);
            argumentIndex ++ ;
            if (lexem != ',' || argumentIndex >= macroBody->argCount)
                break;
            lexem = getLexSkippingLines(&previousLexem, NULL, NULL, &position, NULL);
            ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                        macroStackIndex == 0);
        }
    }
    if (argumentIndex!=0) {
        handleMacroUsageParameterPositions(argumentIndex, macroPosition, parpos1, parpos2, 1);
    }
    /* fill mising arguments */
    for(;argumentIndex < macroBody->argCount; argumentIndex++) {
        fillLexInput(&actualArgs[argumentIndex], NULL, NULL, NULL, NULL,INPUT_NORMAL);
    }
    return actualArgs;

endOfMacroArgument:
    assert(0);
endOfFile:
    assert(options.mode);
    if (options.mode!=ServerMode) {
        warningMessage(ERR_ST,"[getActualMacroArguments] unterminated macro call");
    }
    return NULL;
}

/* **************************************************************** */

static void addMacroBaseUsageRef(Symbol *macroSymbol) {
    bool isMember;
    ReferencesItem ppp, *memb;
    Reference *r;
    Position basePos;

    basePos = makePosition(inputFileNumber, 0, 0);
    fillReferencesItem(&ppp, macroSymbol->linkName,
                       cxFileHashNumber(macroSymbol->linkName), // useless, put 0
                       noFileIndex, noFileIndex, TypeMacro, StorageDefault, ScopeGlobal,
                       macroSymbol->access, CategoryGlobal);
    isMember = isMemberInReferenceTable(&ppp, NULL, &memb);
    r = NULL;
    if (isMember) {
        // this is optimization to avoid multiple base references
        for (r=memb->references; r!=NULL; r=r->next) {
            if (r->usage.kind == UsageMacroBaseFileUsage)
                break;
        }
    }
    if (isMember==0 || r==NULL) {
        addCxReference(macroSymbol, &basePos, UsageMacroBaseFileUsage,
                       noFileIndex, noFileIndex);
    }
}


static bool expandMacroCall(Symbol *macroSymbol, Position *macroPosition) {
    Lexem lexem;
    Position lparPosition;
    LexInput *actualArgumentsInput, macroBodyInput;
    MacroBody *macroBody;
    char *previousLexemP;
    char *freeBase;

    macroBody = macroSymbol->u.mbody;
    if (macroBody == NULL)
        return false;	/* !!!!!         tricky,  undefined macro */
    if (macroStackIndex == 0) { /* call from source, init mem */
        MB_INIT();
    }
    log_trace("trying to expand macro '%s'", macroBody->name);
    if (cyclicCall(macroBody))
        return false;

    /* Make sure these are initialized */
    previousLexemP = currentInput.read;
    PPM_ALLOCC(freeBase, 0, char);

    if (macroBody->argCount >= 0) {
        lexem = getLexSkippingLines(&previousLexemP, NULL, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        if (lexem != '(') {
            currentInput.read = previousLexemP;		/* unget lexem */
            return false;
        }
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &lparPosition, NULL,
                                    macroStackIndex == 0);
        actualArgumentsInput = getActualMacroArguments(macroBody, macroPosition, lparPosition);
    } else {
        actualArgumentsInput = NULL;
    }
    assert(options.mode);
    addCxReference(macroSymbol, macroPosition, UsageUsed, noFileIndex, noFileIndex);
    if (options.mode == XrefMode)
        addMacroBaseUsageRef(macroSymbol);
    log_trace("create macro body '%s'", macroBody->name);
    createMacroBodyAsNewInput(&macroBodyInput,macroBody,actualArgumentsInput,macroBody->argCount);
    prependMacroInput(&macroBodyInput);
    log_trace("expanding macro '%s'", macroBody->name);
    PPM_FREE_UNTIL(freeBase);
    return true;

endOfMacroArgument:
    /* unterminated macro call in argument */
    /* TODO unread the argument that was read */
    currentInput.read = previousLexemP;
    PPM_FREE_UNTIL(freeBase);
    return false;

 endOfFile:
    assert(options.mode);
    if (options.mode!=ServerMode) {
        warningMessage(ERR_ST,"[macroCallExpand] unterminated macro call");
    }
    currentInput.read = previousLexemP;
    PPM_FREE_UNTIL(freeBase);
    return false;
}

void dumpLexemBuffer(LexemBuffer *lb) {
    char *cc;
    Lexem lexem;

    log_debug("lexbufdump [start] ");
    cc = lb->read;
    while (cc < (char *)getLexemStreamWrite(lb)) {
        lexem = getLexTokenAt(&cc);
        if (lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND) {
            log_debug("%s ",cc);
        } else if (lexem==IDENT_TO_COMPLETE) {
            log_debug("!%s! ",cc);
        } else if (lexem < 256) {
            log_debug("%c ",lexem);
        } else if (tokenNamesTable[lexem]==NULL){
            log_debug("?%d? ",lexem);
        } else {
            log_debug("%s ",tokenNamesTable[lexem]);
        }
        getExtraLexemInformationFor(lexem, &cc, NULL, NULL, NULL, NULL, false);
    }
    log_debug("lexbufdump [stop]");
}

/* ************************************************************** */
/*                   caching of input                             */
/* ************************************************************** */
int cachedInputPass(int cpoint, char **cfrom) {
    Lexem lexem;
    int lineNumber;
    Position position;
    unsigned lexemLength;
    char *previousLexem, *cto;
    char *cp;
    int res;

    assert(cpoint > 0);
    cto = cache.cachePoints[cpoint].nextLexemP;
    cp = *cfrom;
    res = 1;

    while (cp < cto) {
        lexem = getLexemSavePrevious(&previousLexem);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        getExtraLexemInformationFor(lexem, &currentInput.read, &lineNumber, NULL, &position, NULL, true);
        lexemLength = currentInput.read-previousLexem;
        assert(lexemLength >= 0);
        if (memcmp(previousLexem, cp, lexemLength)) {
            currentInput.read = previousLexem;			/* unget last lexem */
            res = 0;
            break;
        }
        if (isIdentifierLexem(lexem) || isPreprocessorToken(lexem)) {
            if (onSameLine(position, cxRefPosition)) {
                currentInput.read = previousLexem;			/* unget last lexem */
                res = 0;
                break;
            }
        }
        cp += lexemLength;
    }
endOfFile:
    setCurrentFileConsistency(&currentFile, &currentInput);
    *cfrom = cp;
    return res;
endOfMacroArgument:
    assert(0);
    return -1;
}

/* ***************************************************************** */
/*                                 yylex                             */
/* ***************************************************************** */

static char charText[2]={0,0};
static char constant[50];

static bool isIdAKeyword(Symbol *symbol, Position position) {
    if (symbol->type == TypeKeyword) {
        setYylvalsForIdentifier(symbol->name, symbol, position);
        return true;
    }
    return false;
}

/* TODO: Lookup for C and Java are similar, C has check for CPP and Typedef. Merge? */
/* And probably move to symboltable.c... */
static Lexem lookupCIdentifier(char *id, Position position) {
    Symbol *symbol = NULL;
    unsigned hash = hashFun(id) % symbolTable->size;

    log_trace("looking for C id '%s' in symbol table %p", id, symbolTable);
    for (Symbol *s=symbolTable->tab[hash]; s!=NULL; s=s->next) {
        if (strcmp(s->name, id) == 0) {
            if (symbol == NULL)
                symbol = s;
            if (isIdAKeyword(s, position))
                return s->u.keyword;
            if (s->type == TypeDefinedOp && isProcessingPreprocessorIf) {
                return CPP_DEFINED_OP;
            }
            if (s->type == TypeDefault) {
                setYylvalsForIdentifier(s->name, s, position);
                if (s->storage == StorageTypedef) {
                    return TYPE_NAME;
                } else {
                    return IDENTIFIER;
                }
            }
        }
    }
    if (symbol == NULL)
        id = stackMemoryPushString(id);
    else
        id = symbol->name;
    setYylvalsForIdentifier(id, symbol, position);
    return IDENTIFIER;
}


static Lexem lookupJavaIdentifier(char *id, Position position) {
    Symbol *symbol = NULL;
    unsigned hash = hashFun(id) % symbolTable->size;

    log_trace("looking for Java id '%s' in symbol table %p", id, symbolTable);
    for (Symbol *s=symbolTable->tab[hash]; s!=NULL; s=s->next) {
        if (strcmp(s->name, id) == 0) {
            if (symbol == NULL)
                symbol = s;
            if (isIdAKeyword(s, position))
                return s->u.keyword;
            if (s->type == TypeDefault) {
                setYylvalsForIdentifier(s->name, s, position);
                return IDENTIFIER;
            }
        }
    }
    if (symbol == NULL)
        id = stackMemoryPushString(id);
    else
        id = symbol->name;
    setYylvalsForIdentifier(id, symbol, position);
    return IDENTIFIER;
}


static void actionOnBlockMarker(void) {
    if (options.serverOperation == OLO_SET_MOVE_TARGET) {
        parsedInfo.setTargetAnswerClass[0] = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (parsedClassInfo.function == NULL) {
                if (javaStat!=NULL) {
                    if (javaStat->thisClass==NULL) {
                        sprintf(parsedInfo.setTargetAnswerClass, " %s", s_javaThisPackageName);
                    } else {
                        strcpy(parsedInfo.setTargetAnswerClass, javaStat->thisClass->linkName);
                    }
                }
            }
        }
    } else if (options.serverOperation == OLO_SET_MOVE_CLASS_TARGET) {
        parsedInfo.moveTargetApproved = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (parsedClassInfo.function == NULL) {
                if (javaStat!=NULL) {
                    parsedInfo.moveTargetApproved = 1;
                }
            }
        }
    } else if (options.serverOperation == OLO_SET_MOVE_METHOD_TARGET) {
        parsedInfo.moveTargetApproved = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (parsedClassInfo.function == NULL) {
                if (javaStat!=NULL) {
                    if (javaStat->thisClass!=NULL) {
                        parsedInfo.moveTargetApproved = 1;
                    }
                }
            }
        }
    } else if (options.serverOperation == OLO_EXTRACT) {
        extractActionOnBlockMarker();
    } else {
        parsedInfo.currentPackageAnswer[0] = 0;
        parsedInfo.currentClassAnswer[0] = 0;
        parsedInfo.currentSuperClassAnswer[0] = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (javaStat!=NULL) {
                strcpy(parsedInfo.currentPackageAnswer, s_javaThisPackageName);
                if (javaStat->thisClass!=NULL) {
                    assert(javaStat->thisClass->u.structSpec);
                    strcpy(parsedInfo.currentClassAnswer, javaStat->thisClass->linkName);
                    if (javaStat->thisClass->u.structSpec->super!=NULL) {
                        assert(javaStat->thisClass->u.structSpec->super->element);
                        strcpy(parsedInfo.currentSuperClassAnswer, javaStat->thisClass->u.structSpec->super->element->linkName);
                    }
                }
            }
        }
        parsedClassInfo.parserPassedMarker = 1;
    }
}

static Lexem lookupIdentifier(char *id, Position position) {
    if (LANGUAGE(LANG_C) || LANGUAGE(LANG_YACC))
        return lookupCIdentifier(id, position);
    else if (LANGUAGE(LANG_JAVA))
        return lookupJavaIdentifier(id, position);
    else {
        assert(0);
        return -1;              /* Will never happen */
    }
}

Lexem yylex(void) {
    Lexem lexem;
    char *previousLexem;

 nextYylex:
    lexem = getLexemSavePrevious(&previousLexem);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

 contYylex:
    if (lexem < 256) {          /* First non-single character symbol is 257 */
        if (lexem == '\n') {
            if (isProcessingPreprocessorIf) {
                currentInput.read = previousLexem;
            } else {
                getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, NULL, NULL,
                                            macroStackIndex == 0);
                for(;;) {
                    lexem = getLexem();
                    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

                    if (!isPreprocessorToken(lexem))
                        goto contYylex;
                    if (!processPreprocessorConstruct(lexem))
                        goto endOfFile;
                }
            }
        } else {
            Position position;
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                        macroStackIndex == 0);
            setYylvalsForPosition(position, tokenNameLengthsTable[lexem]);
        }
        yytext = charText;
        charText[0] = lexem;
        goto finish;
    }
    if (lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND) {
        char *id;
        Position position;
        Symbol symbol, *memberP;

        id = yytext = currentInput.read;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    macroStackIndex == 0);
        assert(options.mode);
        if (options.mode == ServerMode) {
            // TODO: ???????????? isn't this useless
            testCxrefCompletionId(&lexem,yytext,&position);
        }
        log_trace("id '%s' position %d, %d, %d", yytext, position.file, position.line, position.col);
        fillSymbol(&symbol, yytext, yytext, noPosition);
        symbol.type = TypeMacro;
        symbol.storage = StorageNone;

        if ((!LANGUAGE(LANG_JAVA))
            && lexem!=IDENT_NO_CPP_EXPAND
            && symbolTableIsMember(symbolTable, &symbol, NULL, &memberP)) {
            // following is because the macro check can read new lexBuf,
            // so id would be destroyed
            //&assert(strcmp(id,memberP->name)==0);
            id = memberP->name;
            if (expandMacroCall(memberP,&position))
                goto nextYylex;
        }

        lexem = lookupIdentifier(id, position);
        goto finish;
    }
    if (lexem == OL_MARKER_TOKEN) {
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, NULL, NULL, macroStackIndex == 0);
        actionOnBlockMarker();
        goto nextYylex;
    }
    if (lexem < MULTI_TOKENS_START) {
        Position position;
        yytext = tokenNamesTable[lexem];
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    macroStackIndex == 0);
        setYylvalsForPosition(position, tokenNameLengthsTable[lexem]);
        goto finish;
    }
    if (lexem == LINE_TOKEN) {
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, NULL, NULL, macroStackIndex == 0);
        goto nextYylex;
    }
    if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
        int value;
        int length;
        Position position;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, &value, &position, &length,
                                    macroStackIndex == 0);
        sprintf(constant,"%d",value);
        setYylvalsForInteger(value, position, length);
        yytext = constant;
        goto finish;
    }
    if (lexem == FLOAT_CONSTANT || lexem == DOUBLE_CONSTANT) {
        int length;
        Position position;
        yytext = "'fltp constant'";
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, &length,
                                    macroStackIndex == 0);
        setYylvalsForPosition(position, length);
        goto finish;
    }
    if (lexem == STRING_LITERAL) {
        Position position;
        yytext = currentInput.read;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    macroStackIndex == 0);
        setYylvalsForPosition(position, strlen(yytext));
        goto finish;
    }
    if (lexem == CHAR_LITERAL) {
        int value;
        int length;
        Position position;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, &value, &position, &length,
                                    macroStackIndex == 0);
        sprintf(constant,"'%c'",value);
        setYylvalsForInteger(value, position, length);
        yytext = constant;
        goto finish;
    }
    assert(options.mode);
    if (options.mode == ServerMode) {
        Position position;
        yytext = currentInput.read;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    macroStackIndex == 0);
        if (lexem == IDENT_TO_COMPLETE) {
            testCxrefCompletionId(&lexem,yytext,&position);
            while (includeStackPointer != 0)
                popInclude();
            /* while (getLexBuf(&cFile.lb)) cFile.lb.cc = cFile.lb.fin;*/
            goto endOfFile;
        }
    }
    log_trace("unknown lexem %d", lexem);
    goto endOfFile;

 finish:
    log_trace("!'%s'(%d)", yytext, cxMemory->index);
    s_lastReturnedLexem = lexem;
    return lexem;

 endOfMacroArgument:
    assert(0);

 endOfFile:
    if ((!LANGUAGE(LANG_JAVA)) && includeStackPointer != 0) {
        popInclude();
        placeCachePoint(true);
        goto nextYylex;
    }
    /* add the test whether in COMPLETION, communication string found */
    return 0;
}	/* end of yylex() */

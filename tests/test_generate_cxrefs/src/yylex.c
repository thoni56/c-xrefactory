#include "yylex.h"

#include <ctype.h>

#include "c_parser.h"
#include "caching.h"
#include "cexp_parser.h"
#include "commons.h"
#include "constants.h"
#include "cxref.h"
#include "extract.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "hash.h"
#include "input.h"
#include "lexem.h"
#include "lexembuffer.h"
#include "lexer.h"
#include "log.h"
#include "macroargumenttable.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "reftab.h"
#include "semact.h"
#include "stackmemory.h"
#include "storage.h"
#include "yacc_parser.h"


static bool isProcessingPreprocessorIf; /* Flag for yylex, to not filter '\n' */

static void setYylvalsForIdentifier(char *name, Symbol *symbol, Position position) {
    uniyylval->ast_id.data = &yyIdBuffer[yyIdBufferIndex];
    yyIdBufferIndex++;
    yyIdBufferIndex %= (YYIDBUFFER_SIZE);
    fillId(uniyylval->ast_id.data, name, symbol, position);
    yytext              = name;
    uniyylval->ast_id.begin = position;
    uniyylval->ast_id.end = position;
    uniyylval->ast_id.end.col += strlen(yytext);
}

static void setYylvalsForPosition(Position position, int length) {
    uniyylval->ast_position.data = position;
    uniyylval->ast_position.begin    = position;
    uniyylval->ast_position.end    = position;
    uniyylval->ast_position.end.col += length;
}

static void setYylvalsForInteger(int val, Position position, int length) {
    uniyylval->ast_integer.data = val;
    uniyylval->ast_integer.begin    = position;
    uniyylval->ast_integer.end    = position;
    uniyylval->ast_integer.end.col += length;
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


int macroStackIndex=0;
static LexInput macroInputStack[MACRO_INPUT_STACK_SIZE];

static Memory2 macroArgumentsMemory;

static void initMacroArgumentsMemory(void) {
    smInit(&macroArgumentsMemory, "macro arguments", MacroArgumentsMemorySize);
}

static void *mamAlloc(size_t size) {
    return smAlloc(&macroArgumentsMemory, size);
}


/* Macro body memory - MBM */

static Memory2 macroBodyMemory;

static void mbmInit(void) {
    smInit(&macroBodyMemory, "macro body", MacroBodyMemorySize);
}

static void *mbmAlloc(size_t size) {
    return smAlloc(&macroBodyMemory, size);
}

static void *mbmExpand(void *pointer, size_t oldSize, size_t newSize) {
    return smRealloc(&macroBodyMemory, pointer, oldSize, newSize);
}

static void mbmFreeUntil(void *pointer) {
    smFreeUntil(&macroBodyMemory, pointer);
}

int getMacroBodyMemoryIndex(void) {
    return macroBodyMemory.index;
}

void setMacroBodyMemoryIndex(int index) {
    macroBodyMemory.index = index;
}


static bool isIdentifierLexem(LexemCode lexem) {
    return lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND  || lexem==IDENT_TO_COMPLETE;
}

static bool isConstantLexem(LexemCode lexem) {
    return lexem == CONSTANT || lexem == LONG_CONSTANT || lexem == FLOAT_CONSTANT || lexem == DOUBLE_CONSTANT;
}

static bool expandMacroCall(Symbol *mdef, Position *mpos);


/* ************************************************************ */

void initAllInputs(void) {
    mbmInit();
    includeStack.pointer=0;
    macroStackIndex=0;
    isProcessingPreprocessorIf = false;
    resetMacroArgumentTable();
    olstringFound = false;
    olstringServed = false;
    olstringInMacroBody = NULL;
    upLevelFunctionCompletionType = NULL;
    structMemberCompletionType = NULL;
}


static void fillMacroArgumentTableElement(MacroArgumentTableElement *element, char *name, char *linkName,
                                int order) {
    element->name = name;
    element->linkName = linkName;
    element->order = order;
}

static void setCacheConsistency(Cache *cache, LexInput *input) {
    cache->read = input->read;
}
static void setCurrentFileConsistency(FileDescriptor *file, LexInput *input) {
    file->lexemBuffer.read = input->read;
}
static void setCurrentInputConsistency(LexInput *input, FileDescriptor *file) {
    fillLexInput(input, file->lexemBuffer.read, file->lexemBuffer.lexemStream, file->lexemBuffer.write,
                 NULL, INPUT_NORMAL);
}

static bool isPreprocessorToken(LexemCode lexem) {
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
        number = NO_FILE_NUMBER;
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
        fileItem->cxLoading = cxloading;
    }
    currentFile.characterBuffer.fileNumber = number;
    currentFile.fileName = name;
}

/* ***************************************************************** */
/*                             Init Reading                          */
/* ***************************************************************** */

/* Init input from file, editor buffer and/or prefix */
void initInput(FILE *file, EditorBuffer *editorBuffer, char *prefix, char *fileName) {
    int     prefixLength, bufferSize, offset;
    char	*bufferStart;

    /* This can be called from various context where one or both are
     * NULL, and we don't know which... */
    prefixLength = strlen(prefix);
    if (editorBuffer != NULL) {
        // Reading from buffer, prepare prefix
        assert(prefixLength < editorBuffer->allocation.allocatedFreePrefixSize);
        strncpy(editorBuffer->allocation.text-prefixLength, prefix, prefixLength);
        bufferStart = editorBuffer->allocation.text-prefixLength;
        bufferSize = editorBuffer->allocation.bufferSize+prefixLength;
        offset = editorBuffer->allocation.bufferSize;
        assert(bufferStart > editorBuffer->allocation.allocatedBlock);
    } else {
        // Reading from file or just a prefix
        assert(prefixLength < CHARACTER_BUFFER_SIZE);
        strcpy(currentFile.characterBuffer.chars, prefix);
        bufferStart = currentFile.characterBuffer.chars;
        bufferSize = prefixLength;
        offset = 0;
    }

    fillFileDescriptor(&currentFile, fileName, bufferStart, bufferSize, file, offset);
    setCurrentFileInfoFor(fileName);
    setCurrentInputConsistency(&currentInput, &currentFile);

    isProcessingPreprocessorIf = false;
}

static void getAndSetOutPositionIfRequired(char **readPointerP, Position *outPosition) {
    if (outPosition != NULL)
        *outPosition = getLexemPositionAt(readPointerP);
    else
        getLexemPositionAt(readPointerP);
}

static void getAndSetOutLengthIfRequired(char **readPointerP, int *outLength) {
    if (outLength != NULL)
        *outLength = getLexemIntAt(readPointerP);
    else
        getLexemIntAt(readPointerP);
}

static void getAndSetOutValueIfRequired(char **readPointerP, int *outValue) {
    if (outValue != NULL)
        *outValue = getLexemIntAt(readPointerP);
    else
        getLexemIntAt(readPointerP);
}

/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

/* Depending on the passed Lexem will get/pass over subsequent lexems
 * like position, and advance the readPointerP. Allows any of the out
 * parameters to be NULL to ignore it. Might be too complex/general,
 * long argument list, when lex is known could be broken into parts
 * for specific lexem types. */
static void getExtraLexemInformationFor(LexemCode lexem, char **readPointerP, int *outLineNumber, int *outValue,
                                        Position *outPosition, int *outLength, bool countLines) {
    if (lexem > MULTI_TOKENS_START) {
        if (isIdentifierLexem(lexem) || lexem == STRING_LITERAL) {
            /* Both are followed by a string and a position */
            log_trace("LEXEM '%s'", *readPointerP);
            *readPointerP = strchr(*readPointerP, '\0') + 1;
            getAndSetOutPositionIfRequired(readPointerP, outPosition);
        } else if (lexem == LINE_TOKEN) {
            int noOfLines = getLexemIntAt(readPointerP);
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
    } else if (lexem == '\n' && countLines) {
        getAndSetOutPositionIfRequired(readPointerP, outPosition);
        traceNewline(1);
        currentFile.lineNumber++;
    } else {
        getAndSetOutPositionIfRequired(readPointerP, outPosition);
    }
}

/* Returns next lexem from currentInput and saves a pointer to the previous lexem */
static LexemCode getLexemAndSavePointerToPrevious(char **previousLexemP) {
    LexemCode lexem;

    /* TODO This is weird, shouldn't this test for next @ end? Seems
     * backwards... */
    while (currentInput.read >= currentInput.write) {
        InputType inputType = currentInput.inputType;
        if (macroStackIndex > 0) {
            if (inputType == INPUT_MACRO_ARGUMENT) {
                return END_OF_MACRO_ARGUMENT_EXCEPTION;
            }
            mbmFreeUntil(currentInput.begin);
            currentInput = macroInputStack[--macroStackIndex];
        } else if (inputType == INPUT_NORMAL) {
            setCurrentFileConsistency(&currentFile, &currentInput);
            if (!buildLexemFromCharacters(&currentFile.characterBuffer, &currentFile.lexemBuffer)) {
                return END_OF_FILE_EXCEPTION;
            }
            setCurrentInputConsistency(&currentInput, &currentFile);
        } else {
            cache.read = cache.write = NULL;
            cacheInput(&currentInput);
            cache.nextToCache = currentFile.lexemBuffer.read;
            setCurrentInputConsistency(&currentInput, &currentFile);
        }
        if (previousLexemP != NULL)
            *previousLexemP = currentInput.read;
    }
    if (previousLexemP != NULL)
        *previousLexemP = currentInput.read;
    lexem = getLexemCodeAt(&currentInput.read);
    log_trace("LEXEM read: %s", lexemEnumNames[lexem]);
    return lexem;
}

static LexemCode getLexem(void) {
    return getLexemAndSavePointerToPrevious(NULL);
}


/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

static void testCxrefCompletionId(LexemCode *out_lexem, char *id, Position *pos) {
    LexemCode lexem;

    lexem = *out_lexem;
    assert(options.mode);
    if (options.mode == ServerMode) {
        if (lexem==IDENT_TO_COMPLETE) {
            deactivateCaching();
            olstringServed = true;
            if (currentLanguage == LANG_YACC) {
                makeYaccCompletions(id, strlen(id), pos);
            } else {
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
    LexemCode lexem;

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


void addFileAsIncludeReference(int fileNumber) {
    Position position;
    Symbol symbol;

    position = makePosition(fileNumber, 1, 0);
    fillIncludeSymbolItem(&symbol, &position);
    log_trace("adding reference on file %d==%s", fileNumber, getFileItem(fileNumber)->name);
    addCxReference(&symbol, &position, UsageDefined, fileNumber);
}

void addIncludeReference(int fileNumber, Position *position) {
    Symbol symbol;

    log_trace("adding reference on file %d==%s", fileNumber, getFileItem(fileNumber)->name);
    fillIncludeSymbolItem(&symbol, position);
    addCxReference(&symbol, position, UsageUsed, fileNumber);
}

static void addIncludeReferences(int fileNumber, Position *position) {
    addIncludeReference(fileNumber, position);
    addFileAsIncludeReference(fileNumber);
}

void pushInclude(FILE *file, EditorBuffer *buffer, char *name, char *prepend) {
    if (currentInput.inputType == INPUT_CACHE) {
        setCacheConsistency(&cache, &currentInput);
    } else {
        setCurrentFileConsistency(&currentFile, &currentInput);
    }
    includeStack.stack[includeStack.pointer++] = currentFile;		/* buffers are copied !!!!!!, burk */
    if (includeStack.pointer >= INCLUDE_STACK_SIZE) {
        FATAL_ERROR(ERR_ST,"too deep nesting in includes", XREF_EXIT_ERR);
    }
    initInput(file, buffer, prepend, name);
    cacheInclude(currentFile.characterBuffer.fileNumber);
}

void popInclude(void) {
    FileItem *fileItem = getFileItem(currentFile.characterBuffer.fileNumber);
    if (fileItem->cxLoading) {
        fileItem->cxLoaded = true;
    }
    closeCharacterBuffer(&currentFile.characterBuffer);
    if (includeStack.pointer != 0) {
        currentFile = includeStack.stack[--includeStack.pointer];	/* buffers are copied !!!!!!, burk */
        if (includeStack.pointer == 0 && cache.read != NULL) {
            fillLexInput(&currentInput, cache.read, cache.lexemStream, cache.write, NULL,
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
            strcpy(normalizedIncludePath, normalizeFileName_static(p->string, cwd));
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
        strcpy(normalizedName, normalizeFileName_static(name, path));
        log_trace("trying to open %s", normalizedName);
        editorBuffer = findEditorBufferForFile(normalizedName);
        if (editorBuffer == NULL)
            file = openFile(normalizedName, "r");
    }

    /* If not found we need to walk the include paths... */
 search:
    for (includeDirP = start; includeDirP != NULL && editorBuffer == NULL && file == NULL; includeDirP = includeDirP->next) {
        strcpy(normalizedName, normalizeFileName_static(includeDirP->string, cwd));
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
            editorBuffer = findEditorBufferForFile(normalizedName);
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
    strcpy(normalizedName, normalizeFileName_static(normalizedName, cwd));
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
    symbol.storage = StorageDefault;

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
    LexemCode lexem;

    lexem = getLexemAndSavePointerToPrevious(&previousLexemP);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    beginningOfLexem = currentInput.read;
    if (lexem == STRING_LITERAL) {         /* A bracketed "<something>" is also stored as a string literal */
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
                                                    Position *beginPosition,
                                                    Position *pos, Position *endPosition,
                                                    bool final) {
    if ((options.serverOperation == OLO_GOTO_PARAM_NAME || options.serverOperation == OLO_GET_PARAM_COORDINATES)
        && positionsAreEqual(*macpos, cxRefPosition)) {
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(beginPosition);
            } else if (argi < options.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(endPosition);
            }
        } else if (argi == options.olcxGotoVal) {
            parameterPosition = *pos;
            parameterBeginPosition = *beginPosition;
            parameterEndPosition = *endPosition;
        }
    }
}

static void handleMacroUsageParameterPositions(int argi, Position *macroPosition,
                                               Position *beginPosition, Position *endPosition,
                                               bool final
    ) {
    if (options.serverOperation == OLO_GET_PARAM_COORDINATES
        && positionsAreEqual(*macroPosition, cxRefPosition)) {
        log_trace("checking param %d at %d,%d, final==%d", argi, beginPosition->col, endPosition->col, final);
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(beginPosition);
            } else if (argi < options.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(endPosition);
            }
        } else if (argi == options.olcxGotoVal) {
            parameterBeginPosition = *beginPosition;
            parameterEndPosition = *endPosition;
//&fprintf(dumpOut,"regular setting to %d - %d\n", beginPosition->col, endPosition->col);
        }
    }
}

static MacroBody *newMacroBody(int macroSize, int argCount, char *name, char *body, char **argumentNames) {
    MacroBody *macroBody;

    macroBody = ppmAlloc(sizeof(MacroBody));
    macroBody->argCount = argCount;
    macroBody->size = macroSize;
    macroBody->name = name;
    macroBody->argumentNames = argumentNames;
    macroBody->body = body;

    return macroBody;
}


static LexemCode getNonBlankLexemAndData(Position *position, int *lineNumber, int *value, int *length) {
    LexemCode lexem;

    lexem = getLexem();
    while (lexem == LINE_TOKEN) {
        getExtraLexemInformationFor(lexem, &currentInput.read, lineNumber, value, position, length, true);
        lexem = getLexem();
    }
    return lexem;
}

static MacroArgumentTableElement *newMacroArgumentTableElement(char *argLinkName, int argumentCount, char *name) {
    MacroArgumentTableElement *macroArgumentTableElement = mamAlloc(sizeof(MacroArgumentTableElement));
    fillMacroArgumentTableElement(macroArgumentTableElement, name, argLinkName, argumentCount);
    return macroArgumentTableElement;
}

/* Public only for unittesting */
protected void processDefineDirective(bool hasArguments) {
    LexemCode lexem;
    Position macroPosition, ppb1, ppb2, *parpos1, *parpos2;
    char *currentLexemStart, *argumentName;
    char **argumentNames, *argLinkName;

    bool isReadingBody = false;
    char *macroName = NULL;
    int allocatedSize = 0;
    Symbol *symbol = NULL;
    char *body = NULL;

    initMacroArgumentsMemory();

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

    symbol = ppmAlloc(sizeof(Symbol));
    fillSymbol(symbol, NULL, NULL, macroPosition);
    symbol->type = TypeMacro;
    symbol->storage = StorageDefault;

    /* TODO: this is the only call to setGlobalFileDepNames() that
       doesn't do it in XX memory, why?  PPM == PreProcessor, but it's
       still a symbol... Changing this to XX makes the slow tests
       fail... */
    setGlobalFileDepNames(currentLexemStart, symbol, MEMORY_PPM);
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
                    argumentName = cppVarArgsName;
                    position = macroPosition;					// hack !!!
                    ellipsis = true;
                } else
                    goto errorlabel;

                char *name = ppmAllocc(strlen(argumentName)+1, sizeof(char));
                strcpy(name, argumentName);

                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "%x-%x%c%s", position.file, position.line,
                        LINK_NAME_SEPARATOR, argumentName);
                argLinkName = ppmAllocc(strlen(tmpBuff)+1, sizeof(char));
                strcpy(argLinkName, tmpBuff);

                MacroArgumentTableElement *macroArgumentTableElement = newMacroArgumentTableElement(argLinkName, argumentCount, name);
                int argumentIndex = addMacroArgument(macroArgumentTableElement);
                argumentCount++;

                lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
                ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

                /* Swap position pointers */
                Position *tmppos = parpos1;
                parpos1=parpos2;
                parpos2=tmppos;

                getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, parpos2, NULL, true);
                if (!ellipsis) {
                    addTrivialCxReference(getMacroArgument(argumentIndex)->linkName, TypeMacroArg,StorageDefault,
                                          position, UsageDefined);
                    handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &position, parpos2, false);
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
            handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &noPosition, parpos2, true);
        } else {
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, parpos2, NULL, true);
            handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &noPosition, parpos2, true);
        }
    }

    /* process macro body */
    allocatedSize = MACRO_BODY_BUFFER_SIZE;
    int macroSize = 0;
    body = ppmAllocc(allocatedSize+MAX_LEXEM_SIZE, sizeof(char));

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
            fillMacroArgumentTableElement(&macroArgumentTableElement, currentLexemStart, NULL, 0);

            int foundIndex;
            if (lexem==IDENTIFIER && isMemberInMacroArguments(&macroArgumentTableElement, &foundIndex)){
                /* macro argument */
                addTrivialCxReference(getMacroArgument(foundIndex)->linkName, TypeMacroArg, StorageDefault,
                                      position, UsageUsed);
                putLexemCodeAt(CPP_MACRO_ARGUMENT, &lexemDestination);
                putLexemIntAt(getMacroArgument(foundIndex)->order, &lexemDestination);
                putLexemPositionAt(position, &lexemDestination);
            } else {
                if (lexem==IDENT_TO_COMPLETE
                    || (lexem == IDENTIFIER && positionsAreEqual(position, cxRefPosition))) {
                    deactivateCaching();
                    olstringFound = true;
                    olstringInMacroBody = symbol->linkName;
                }
                putLexemCodeAt(lexem, &lexemDestination);
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
            body = ppmReallocc(body, allocatedSize+MAX_LEXEM_SIZE, sizeof(char),
                               allocatedSize+MAX_LEXEM_SIZE-MACRO_BODY_BUFFER_SIZE);
        }
    }

endOfBody:
    assert(macroSize>=0);
    body = ppmReallocc(body, macroSize, sizeof(char), allocatedSize+MAX_LEXEM_SIZE);
    allocatedSize = macroSize;
    if (argumentCount > 0) {
        argumentNames = ppmAllocc(argumentCount, sizeof(char*));
        memset(argumentNames, 0, argumentCount*sizeof(char*));
        mapOverMacroArgumentsWithPointer(setMacroArgumentName, argumentNames);
    } else
        argumentNames = NULL;
    MacroBody *macroBody = newMacroBody(allocatedSize, argumentCount, macroName, body, argumentNames);
    symbol->u.mbody = macroBody;

    addMacroToTabs(symbol, macroName);
    assert(options.mode);
    addCxReference(symbol, &macroPosition, UsageDefined, NO_FILE_NUMBER);
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

    nopt = ppmAllocc(strlen(opt)+3, sizeof(char));
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
    LexemCode lexem;
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
        symbol.storage = StorageDefault;

        assert(options.mode);
        /* !!!!!!!!!!!!!! tricky, add macro with mbody == NULL !!!!!!!!!! */
        /* this is because of monotonicity for caching, just adding symbol */
        if (symbolTableIsMember(symbolTable, &symbol, NULL, &member)) {
            Symbol *pp;
            addCxReference(member, &position, UsageUndefinedMacro, NO_FILE_NUMBER);

            pp = ppmAlloc(sizeof(Symbol));
            fillSymbol(pp, member->name, member->linkName, position);
            pp->type = TypeMacro;
            pp->storage = StorageDefault;

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
        ss = ppmAlloc(sizeof(CppIfStack));
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
    LexemCode lexem;
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
    LexemCode lexem;
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
    symbol.storage = StorageDefault;

    Symbol *member;
    bool isMember = symbolTableIsMember(symbolTable, &symbol, NULL, &member);
    if (isMember && member->u.mbody==NULL)
        isMember = false;	// undefined macro
    if (isMember) {
        addCxReference(member, &position, UsageUsed, NO_FILE_NUMBER);
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

LexemCode cexp_yylex(void) {
    int res, mm;
    LexemCode lexem;
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
        symbol.storage = StorageDefault;

        log_debug("(%s)", symbol.name);

        mm = symbolTableIsMember(symbolTable, &symbol, NULL, &foundMember);
        if (mm && foundMember->u.mbody == NULL)
            mm = 0;   // undefined macro
        assert(options.mode);
        if (mm)
            addCxReference(&symbol, &position, UsageUsed, NO_FILE_NUMBER);

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
    LexemCode lexem;
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
        mname = ppmAllocc(strlen(tmpBuff)+1, sizeof(char));
        strcpy(mname, tmpBuff);

        pp = ppmAlloc(sizeof(Symbol));
        fillSymbol(pp, mname, mname, position);
        pp->type = TypeMacro;
        pp->storage = StorageDefault;

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

static bool processPreprocessorConstruct(LexemCode lexem) {
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

static int expandPreprocessorBufferIfOverflow(char *pointer, char *buffer, int size) {
    if (pointer >= buffer+size) {
        size += MACRO_BODY_BUFFER_SIZE;
        buffer = ppmReallocc(buffer, size+MAX_LEXEM_SIZE, sizeof(char),
                             size+MAX_LEXEM_SIZE-MACRO_BODY_BUFFER_SIZE);
    }
    return size;
}

static void expandMacroBodyBufferIfOverflow(char *pointer, int len, char *buffer, int *size) {
    while (pointer + len >= buffer + *size) {
        *size += MACRO_BODY_BUFFER_SIZE;
        mbmExpand(buffer, *size+MAX_LEXEM_SIZE-MACRO_BODY_BUFFER_SIZE, *size+MAX_LEXEM_SIZE);
    }
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


static void expandMacroArgument(LexInput *argumentInput) {
    char *buffer;
    char *currentBufferP;
    int bufferSize;

    bufferSize = MACRO_BODY_BUFFER_SIZE;

    prependMacroInput(argumentInput);

    currentInput.inputType = INPUT_MACRO_ARGUMENT;
    buffer = ppmAllocc(bufferSize+MAX_LEXEM_SIZE, sizeof(char));
    currentBufferP = buffer;

    for(;;) {
        char *previousLexem;
        LexemCode lexem = getLexemAndSavePointerToPrevious(&previousLexem);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        char *nextLexemP = currentInput.read;

        Position position;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    macroStackIndex == 0);
        int length = ((char*)currentInput.read) - previousLexem;
        assert(length >= 0);
        memcpy(currentBufferP, previousLexem, length);

        // a hack, it is copied, but currentBufferP will be increased
        // only if not an expanding macro, this is because
        // 'macroCallExpand' can read new lexbuffer and destroy
        // cInput, so copy it now.
        bool failedMacroExpansion = false;
        Symbol *foundSymbol;
        if (lexem == IDENTIFIER) {
            Symbol symbol;
            fillSymbol(&symbol, nextLexemP, nextLexemP, noPosition);
            symbol.type = TypeMacro;
            symbol.storage = StorageDefault;
            if (symbolTableIsMember(symbolTable, &symbol, NULL, &foundSymbol)) {
                /* it is a macro, provide macro expansion */
                if (expandMacroCall(foundSymbol, &position))
                    continue; // with next lexem
                else
                    failedMacroExpansion = true;
            }
        }
        if (failedMacroExpansion) {
            char *tmp = currentBufferP;
            assert(foundSymbol!=NULL);
            if (foundSymbol->u.mbody!=NULL && cyclicCall(foundSymbol->u.mbody)) {
                assert(tmp == currentBufferP); /* Is tmp ever different from currentBufferP? */
                putLexemCodeAt(IDENT_NO_CPP_EXPAND, &tmp);
            }
        }
        currentBufferP += length;
        bufferSize = expandPreprocessorBufferIfOverflow(currentBufferP, buffer, bufferSize);
    }
endOfMacroArgument:
    currentInput = macroInputStack[--macroStackIndex];
    buffer = ppmReallocc(buffer, currentBufferP-buffer, sizeof(char), bufferSize+MAX_LEXEM_SIZE);
    fillLexInput(argumentInput, buffer, buffer, currentBufferP, NULL, INPUT_NORMAL);
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

    if (peekLexemCodeAt(lastBufferP) == CPP_MACRO_ARGUMENT) {
        currentBufferP = lastBufferP;
        LexemCode lexem = getLexemCodeAt(&lastBufferP);
        int argumentIndex;
        assert(lexem==CPP_MACRO_ARGUMENT);
        getExtraLexemInformationFor(lexem, &lastBufferP, NULL, &argumentIndex, NULL, NULL, false);
        currentInputLexemP = actualArgumentsInput[argumentIndex].begin;
        endOfInputLexems = actualArgumentsInput[argumentIndex].write;
        lastBufferP = NULL;
        while (currentInputLexemP < endOfInputLexems) {
            char *lexemStart = currentInputLexemP;
            LexemCode lexem = getLexemCodeAt(&currentInputLexemP);
            getExtraLexemInformationFor(lexem, &currentInputLexemP, NULL, NULL, NULL, NULL, false);
            lastBufferP = currentBufferP;
            assert(currentInputLexemP>=lexemStart);
            int lexemLength = currentInputLexemP-lexemStart;
            memcpy(currentBufferP, lexemStart, lexemLength);
            currentBufferP += lexemLength;
            bufferSize = expandPreprocessorBufferIfOverflow(currentBufferP, buffer, bufferSize);
        }
    }

    if (peekLexemCodeAt(currentBodyLexemP) == CPP_MACRO_ARGUMENT) {
        LexemCode lexem = getLexemCodeAt(&currentBodyLexemP);
        int value;
        getExtraLexemInformationFor(lexem, &currentBodyLexemP, NULL, &value, NULL, NULL, false);
        currentInputLexemP = actualArgumentsInput[value].begin;
        endOfInputLexems = actualArgumentsInput[value].write;
    } else {
        currentInputLexemP = currentBodyLexemP;
        LexemCode lexem = getLexemCodeAt(&currentBodyLexemP);
        getExtraLexemInformationFor(lexem, &currentBodyLexemP, NULL, NULL, NULL, NULL, false);
        endOfInputLexems = currentBodyLexemP;
    }

    /* now collate *lbcc and *cc */
    // berk, do not pre-compute, lbcc can be NULL!!!!
    if (lastBufferP != NULL && currentInputLexemP < endOfInputLexems && isIdentifierLexem(peekLexemCodeAt(lastBufferP))) {
        LexemCode nextLexem = peekLexemCodeAt(currentInputLexemP);
        if (isIdentifierLexem(nextLexem) || isConstantLexem(nextLexem)) {
            Position position;
            /* TODO collation of all lexem pairs */
            int len  = strlen(lastBufferP + LEXEMCODE_SIZE);
            LexemCode lexem = getLexemCodeAt(&currentInputLexemP);
            int value;
            char *previousInputLexemP = currentInputLexemP;
            getExtraLexemInformationFor(lexem, &currentInputLexemP, NULL, &value, &position, NULL, false);
            currentBufferP = lastBufferP + LEXEMCODE_SIZE + len;
            assert(*currentBufferP == 0);
            if (isIdentifierLexem(lexem)) {
                /* TODO: WTF Here was a out-commented call to
                 * NextLexPosition(), maybe the following "hack"
                 * replaced that macro */
                strcpy(currentBufferP, previousInputLexemP);
                // the following is a hack as # is part of ## symbols
                position.col--;
                assert(position.col >= 0);
                cxAddCollateReference(lastBufferP + LEXEMCODE_SIZE, currentBufferP, &position);
                position.col++;
            } else /* isConstantLexem() */ {
                position = peekLexemPositionAt(currentBufferP + 1); /* new identifier position */
                sprintf(currentBufferP, "%d", value);
                cxAddCollateReference(lastBufferP + LEXEMCODE_SIZE, currentBufferP, &position);
            }
            currentBufferP += strlen(currentBufferP);
            assert(*currentBufferP == 0);
            currentBufferP++;
            putLexemPositionAt(position, &currentBufferP);
        }
    }
    bufferSize = expandPreprocessorBufferIfOverflow(currentBufferP, buffer, bufferSize);
    while (currentInputLexemP < endOfInputLexems) {
        char *lexemStart   = currentInputLexemP;
        LexemCode lexem = getLexemCodeAt(&currentInputLexemP);
        getExtraLexemInformationFor(lexem, &currentInputLexemP, NULL, NULL, NULL, NULL, false);
        lastBufferP = currentBufferP;
        assert(currentInputLexemP >= lexemStart);
        int lexemLength = currentInputLexemP - lexemStart;
        memcpy(currentBufferP, lexemStart, lexemLength);
        currentBufferP += lexemLength;
        bufferSize = expandPreprocessorBufferIfOverflow(currentBufferP, buffer, bufferSize);
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
        LexemCode lexem = getLexemCodeAt(&cc);
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

static char *replaceMacroArguments(LexInput *actualArgumentsInput, char *readBuffer, char **_writePointerP) {
    char *currentLexemStart, *writePointer = *_writePointerP;
    int   bufferSize;
    char *writeBuffer;

    char *currentLexemP = readBuffer;
    char *endOfLexems   = writePointer;
    bufferSize          = MACRO_BODY_BUFFER_SIZE;
    writeBuffer = mbmAlloc(bufferSize + MAX_LEXEM_SIZE);
    writePointer = writeBuffer;
    while (currentLexemP < endOfLexems) {
        int      value;
        Position position;
        LexemCode    lexem;

        currentLexemStart = currentLexemP;
        lexem = getLexemCodeAt(&currentLexemP);
        getExtraLexemInformationFor(lexem, &currentLexemP, NULL, &value, &position, NULL, false);
        if (lexem == CPP_MACRO_ARGUMENT) {
            int len = actualArgumentsInput[value].write - actualArgumentsInput[value].begin;
            expandMacroBodyBufferIfOverflow(writePointer, len, writeBuffer, &bufferSize);
            memcpy(writePointer, actualArgumentsInput[value].begin, len);
            writePointer += len;
        } else if (lexem == '#' && currentLexemP < endOfLexems
                   && peekLexemCodeAt(currentLexemP) == CPP_MACRO_ARGUMENT) {
            lexem = getLexemCodeAt(&currentLexemP);
            assert(lexem == CPP_MACRO_ARGUMENT);
            getExtraLexemInformationFor(lexem, &currentLexemP, NULL, &value, NULL, NULL, false);
            putLexemCodeAt(STRING_LITERAL, &writePointer);
            expandMacroBodyBufferIfOverflow(writePointer, MACRO_BODY_BUFFER_SIZE, writeBuffer, &bufferSize);
            macroArgumentsToString(writePointer, &actualArgumentsInput[value]);

            /* Move over something that looks like a string... */
            int len = strlen(writePointer) + 1;
            writePointer += len;

            /* TODO: This should really be putLexPosition() but that takes a LexemBuffer... */
            putLexemPositionAt(position, &writePointer);
            if (len >= MACRO_BODY_BUFFER_SIZE - 15) {
                char tmpBuffer[TMP_BUFF_SIZE];
                sprintf(tmpBuffer, "size of #macro_argument exceeded MACRO_BODY_BUFFER_SIZE @%s:%d:%d",
                        getFileItem(position.file)->name, position.line, position.col);
                errorMessage(ERR_INTERNAL, tmpBuffer);
            }
        } else {
            expandMacroBodyBufferIfOverflow(writePointer, (currentLexemStart - currentLexemP), writeBuffer, &bufferSize);
            assert(currentLexemP >= currentLexemStart);
            memcpy(writePointer, currentLexemStart, currentLexemP - currentLexemStart);
            writePointer += currentLexemP - currentLexemStart;
        }
        expandMacroBodyBufferIfOverflow(writePointer, 0, writeBuffer, &bufferSize);
    }
    mbmExpand(writeBuffer, bufferSize + MAX_LEXEM_SIZE, writePointer - writeBuffer);
    *_writePointerP = writePointer;

    return writeBuffer;
}

/* ************************************************************** */
/* ********************* macro body replacement ***************** */
/* ************************************************************** */

static void createMacroBodyAsNewInput(LexInput *inputToSetup, MacroBody *macroBody, LexInput *actualArgumentsInput,
                                      int actualArgumentCount) {

    char *currentBodyLexemP = macroBody->body;
    char *endOfBodyLexems   = macroBody->body + macroBody->size;
    int   bufferSize    = MACRO_BODY_BUFFER_SIZE;
    char *buffer;
    char *currentBufferP, *lastBufferP;

    buffer = ppmAllocc(bufferSize + MAX_LEXEM_SIZE, sizeof(char));
    currentBufferP  = buffer;
    lastBufferP = NULL;
    while (currentBodyLexemP < endOfBodyLexems) {
        char *lexemStart   = currentBodyLexemP;
        LexemCode lexem = getLexemCodeAt(&currentBodyLexemP);
        getExtraLexemInformationFor(lexem, &currentBodyLexemP, NULL, NULL, NULL, NULL, false);

        /* first make ## collations, if any */
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
        bufferSize = expandPreprocessorBufferIfOverflow(currentBufferP, buffer, bufferSize);
    }
    buffer = ppmReallocc(buffer, currentBufferP - buffer, sizeof(char), bufferSize + MAX_LEXEM_SIZE);

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

static LexemCode getLexSkippingLines(char **saveLexemP, int *lineNumberP,
                                int *valueP, Position *positionP, int *lengthP) {
    LexemCode lexem = getLexemAndSavePointerToPrevious(saveLexemP);
    while (lexem == LINE_TOKEN || lexem == '\n') {
        getExtraLexemInformationFor(lexem, &currentInput.read, lineNumberP, valueP, positionP, lengthP,
                                    macroStackIndex == 0);
        lexem = getLexemAndSavePointerToPrevious(saveLexemP);
    }
    return lexem;
}

static void getActualMacroArgument(
    char *previousLexemP,
    LexemCode *inOutLexem,
    Position *macroPosition,
    Position **beginPosition,
    Position **endPosition,
    LexInput *actualArgumentsInput,
    MacroBody *macroBody,
    int actualArgumentIndex
) {
    int   offset;
    int   depth;
    LexemCode lexem;
    char *bufferP;
    char *buffer;
    int   bufferSize;

    lexem      = *inOutLexem;

    bufferSize = MACRO_ARGUMENTS_BUFFER_SIZE;
    depth      = 0;
    buffer = ppmAllocc(bufferSize + MAX_LEXEM_SIZE, sizeof(char));
    bufferP = buffer;

    /* if lastArgument, collect everything there */
    offset = 0;
    while (((lexem != ',' || actualArgumentIndex + 1 == macroBody->argCount) && lexem != ')')
           || depth > 0) {
        // The following should be equivalent to the loop condition:
        //& if (lexem == ')' && depth <= 0) break;
        //& if (lexem == ',' && depth <= 0 && ! lastArgument) break;
        if (lexem == '(')
            depth++;
        if (lexem == ')')
            depth--;

        /* Copy from source to the new buffer? */
        for (; previousLexemP < currentInput.read; previousLexemP++, bufferP++)
            *bufferP = *previousLexemP;

        if (bufferP - buffer >= bufferSize) {
            bufferSize += MACRO_ARGUMENTS_BUFFER_SIZE;
            buffer = ppmReallocc(buffer, bufferSize + MAX_LEXEM_SIZE, sizeof(char),
                                 bufferSize + MAX_LEXEM_SIZE - MACRO_ARGUMENTS_BUFFER_SIZE);
        }
        lexem = getLexSkippingLines(&previousLexemP, NULL, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile,
                                endOfMacroArgument); /* CAUTION! Contains goto:s! */

        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL,
                                    *endPosition, NULL, macroStackIndex == 0);
        if ((lexem == ',' || lexem == ')') && depth == 0) {
            offset++;
            handleMacroUsageParameterPositions(actualArgumentIndex + offset, macroPosition,
                                               *beginPosition,
                                               *endPosition, 0);
            **beginPosition = **endPosition;
        }
    }
    goto end;

endOfFile:;
endOfMacroArgument:;
    assert(options.mode);
    if (options.mode != ServerMode) {
        warningMessage(ERR_ST, "[getActualMacroArgument] unterminated macro call");
    }

end:
    buffer = ppmReallocc(buffer, bufferP - buffer, sizeof(char), bufferSize + MAX_LEXEM_SIZE);
    fillLexInput(actualArgumentsInput, buffer, buffer, bufferP, currentInput.macroName,
                 INPUT_NORMAL);
    *inOutLexem = lexem;
    return;
}

static LexInput *getActualMacroArguments(MacroBody *macroBody, Position *macroPosition,
                                         Position lparPosition) {
    char *previousLexem;
    LexemCode lexem;
    Position position, ppb1, ppb2, *beginPosition, *endPosition;
    int argumentIndex = 0;
    LexInput *actualArgs;

    ppb1 = lparPosition;
    ppb2 = lparPosition;
    beginPosition = &ppb1;
    endPosition = &ppb2;
    actualArgs = ppmAllocc(macroBody->argCount, sizeof(LexInput));
    lexem = getLexSkippingLines(&previousLexem, NULL, NULL, &position, NULL);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                macroStackIndex == 0);
    if (lexem == ')') {
        *endPosition = position;
        handleMacroUsageParameterPositions(0, macroPosition, beginPosition, endPosition, true);
    } else {
        for(;;) {
            getActualMacroArgument(previousLexem, &lexem, macroPosition, &beginPosition, &endPosition,
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
    /* fill missing arguments */
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
    ReferenceItem ppp, *memb;
    Reference *r;
    Position basePos;

    basePos = makePosition(inputFileNumber, 0, 0);
    fillReferenceItem(&ppp, macroSymbol->linkName,
                      NO_FILE_NUMBER, TypeMacro, StorageDefault, ScopeGlobal, CategoryGlobal);
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
        addCxReference(macroSymbol, &basePos, UsageMacroBaseFileUsage, NO_FILE_NUMBER);
    }
}


static bool expandMacroCall(Symbol *macroSymbol, Position *macroPosition) {
    LexemCode lexem;
    Position lparPosition;
    LexInput *actualArgumentsInput, macroBodyInput;
    MacroBody *macroBody;
    char *previousLexemP;
    char *freeBase;

    macroBody = macroSymbol->u.mbody;
    if (macroBody == NULL)
        return false;	/* !!!!!         tricky,  undefined macro */
    if (macroStackIndex == 0) { /* call from source, init mem */
        mbmInit();
    }
    log_trace("trying to expand macro '%s'", macroBody->name);
    if (cyclicCall(macroBody))
        return false;

    /* Make sure these are initialized */
    previousLexemP = currentInput.read;
    freeBase = ppmAllocc(0, sizeof(char));

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
    addCxReference(macroSymbol, macroPosition, UsageUsed, NO_FILE_NUMBER);
    if (options.mode == XrefMode)
        addMacroBaseUsageRef(macroSymbol);
    log_trace("create macro body '%s'", macroBody->name);
    createMacroBodyAsNewInput(&macroBodyInput,macroBody,actualArgumentsInput,macroBody->argCount);
    prependMacroInput(&macroBodyInput);
    log_trace("expanded macro '%s'", macroBody->name);
    ppmFreeUntil(freeBase);
    return true;

endOfMacroArgument:
    /* unterminated macro call in argument */
    /* TODO unread the argument that was read */
    currentInput.read = previousLexemP;
    ppmFreeUntil(freeBase);
    return false;

 endOfFile:
    assert(options.mode);
    if (options.mode!=ServerMode) {
        warningMessage(ERR_ST,"[macroCallExpand] unterminated macro call");
    }
    currentInput.read = previousLexemP;
    ppmFreeUntil(freeBase);
    return false;
}

void dumpLexemBuffer(LexemBuffer *lb) {
    char *cc;
    LexemCode lexem;

    log_debug("lexbufdump [start] ");
    cc = lb->read;
    while (cc < (char *)getLexemStreamWrite(lb)) {
        lexem = getLexemCodeAt(&cc);
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
    LexemCode lexem;
    int lineNumber;
    Position position;
    unsigned lexemLength;
    char *previousLexem, *cto;
    char *cp;
    int res;

    assert(cpoint > 0);
    cto = cache.points[cpoint].nextLexemP;
    cp = *cfrom;
    res = 1;

    while (cp < cto) {
        lexem = getLexemAndSavePointerToPrevious(&previousLexem);
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

/* TODO: probably move to symboltable.c... */
static LexemCode lookupCIdentifier(char *id, Position position) {
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


static void actionOnBlockMarker(void) {
    if (options.serverOperation == OLO_SET_MOVE_FUNCTION_TARGET) {
        parsedInfo.moveTargetAccepted = parsedInfo.function == NULL;
    } else if (options.serverOperation == OLO_EXTRACT) {
        extractActionOnBlockMarker();
    }
}

static LexemCode lookupIdentifier(char *id, Position position) {
    return lookupCIdentifier(id, position);
}

LexemCode yylex(void) {
    LexemCode lexem;
    char *previousLexem;

 nextYylex:
    lexem = getLexemAndSavePointerToPrevious(&previousLexem);
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
        symbol.storage = StorageDefault;

        if (lexem!=IDENT_NO_CPP_EXPAND && symbolTableIsMember(symbolTable, &symbol, NULL, &memberP)) {
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
            while (includeStack.pointer != 0)
                popInclude();
            /* while (getLexBuf(&cFile.lb)) cFile.lb.cc = cFile.lb.fin;*/
            goto endOfFile;
        }
    }
    log_trace("unknown lexem %d", lexem);
    goto endOfFile;

 finish:
    log_trace("!'%s'(%d)", yytext, cxMemory->index);
    return lexem;

 endOfMacroArgument:
    assert(0);

 endOfFile:
    if (includeStack.pointer != 0) {
        popInclude();
        placeCachePoint(true);
        goto nextYylex;
    }
    /* add the test whether in COMPLETION, communication string found */
    return 0;
}	/* end of yylex() */

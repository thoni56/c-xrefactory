#include "yylex.h"

#include "commons.h"
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
#include "utils.h"
#include "macroargumenttable.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "log.h"


static void setYylvalsForIdentifier(char *name, Symbol *symbol, Position position) {
    uniyylval->ast_id.d = &s_yyIdentBuf[s_yyIdentBufi];
    s_yyIdentBufi ++; s_yyIdentBufi %= (YYBUFFERED_ID_INDEX);
    fillId(uniyylval->ast_id.d, name, symbol, position);
    yytext = name;
    uniyylval->ast_id.b = position;
    uniyylval->ast_id.e = position;
    uniyylval->ast_id.e.col += strlen(yytext);
}

static void setYylvalsForPosition(Position position, int length) {
        uniyylval->ast_position.d = position;
        uniyylval->ast_position.b = position;
        uniyylval->ast_position.e = position;
        uniyylval->ast_position.e.col += length;
}

static void setYylvalsForInteger(int val, Position position, int length) {
    uniyylval->ast_integer.d = val;
    uniyylval->ast_integer.b = position;
    uniyylval->ast_integer.e = position;
    uniyylval->ast_integer.e.col += length;
}


/* !!!!!!!!!!!!!!!!!!! to caching !!!!!!!!!!!!!!! */


/* Exceptions: */
#define END_OF_MACRO_ARGUMENT_EXCEPTION -1
#define END_OF_FILE_EXCEPTION -2
#define EXCEPTION_CHECK(lexem, eof_label, eoma_label) switch ((int)lexem) {case END_OF_FILE_EXCEPTION: goto eof_label; case END_OF_MACRO_ARGUMENT_EXCEPTION: goto eoma_label; default: ; }


LexInput currentInput;
int macroStackIndex=0;
static LexInput macroInputStack[MACRO_INPUT_STACK_SIZE];

static char ppMemory[SIZE_ppMemory];
static int ppMemoryIndex=0;




static bool isIdentifierLexem(Lexem lexem) {
    return lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND  || lexem==IDENT_TO_COMPLETE;
}


static bool expandMacroCall(Symbol *mdef, Position *mpos);


/* ************************************************************ */

void initAllInputs(void) {
    MB_INIT();
    includeStackPointer=0;
    macroStackIndex=0;
    s_ifEvaluation = 0;
    s_cxRefFlag = 0;
    macroArgumentTableNoAllocInit(&macroArgumentTable, MAX_MACRO_ARGS);
    ppMemoryIndex=0;
    s_olstring[0]=0;
    s_olstringFound = false;
    s_olstringServed = false;
    s_olstringInMbody = NULL;
    s_upLevelFunctionCompletionType = NULL;
    s_structRecordCompletionType = NULL;
}


static void fillMacroArgTabElem(MacroArgumentTableElement *macroArgTabElem, char *name, char *linkName,
                                int order) {
    macroArgTabElem->name = name;
    macroArgTabElem->linkName = linkName;
    macroArgTabElem->order = order;
}

void fillLexInput(LexInput *lexInput, char *currentLexem, char *endOfBuffer,
                  char *beginningOfBuffer, char *macroName, InputType margExpFlag) {
    lexInput->currentLexemP = currentLexem;
    lexInput->endOfBuffer = endOfBuffer;
    lexInput->beginningOfBuffer = beginningOfBuffer;
    lexInput->macroName = macroName;
    lexInput->inputType = margExpFlag;
}

static void setCacheConsistency(void) {
    cache.cc = currentInput.currentLexemP;
}
static void setCFileConsistency(void) {
    currentFile.lexBuffer.next = currentInput.currentLexemP;
}
static void setCInputConsistency(void) {
    fillLexInput(&currentInput, currentFile.lexBuffer.next, currentFile.lexBuffer.end,
                 currentFile.lexBuffer.lexemStream, NULL, INPUT_NORMAL);
}

char *placeIdent(void) {
    static char tt[2*MAX_REF_LEN];
    char fn[MAX_FILE_NAME_SIZE];
    char mm[MAX_REF_LEN];
    int s;
    if (currentFile.fileName!=NULL) {
        if (options.xref2 && options.taskRegime!=RegimeEditServer) {
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
        bool cxloading = fileItem->bits.cxLoading;
        if (!existed) {
            cxloading = true;
        } else if (options.update==UPDATE_FAST) {
            if (fileItem->bits.scheduledToProcess) {
                // references from headers are not loaded on fast update !
                cxloading = true;
            }
        } else if (options.update==UPDATE_FULL) {
            if (fileItem->bits.scheduledToUpdate) {
                cxloading = true;
            }
        } else {
            cxloading = true;
        }
        if (fileItem->bits.cxSaved) {
            cxloading = false;
        }
        if (LANGUAGE(LANG_JAVA)) {
            if (s_jsl!=NULL || s_javaPreScanOnly) {
                // do not load (and save) references from jsl loaded files
                // nor during prescanning
                cxloading = fileItem->bits.cxLoading;
            }
        }
        fileItem->bits.cxLoading = cxloading;
    }
    currentFile.lexBuffer.buffer.fileNumber = number;
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
        assert(prefixLength < CHAR_BUFF_SIZE);
        strcpy(currentFile.lexBuffer.buffer.chars, prefix);
        bufferStart = currentFile.lexBuffer.buffer.chars;
        bufferSize = prefixLength;
        offset = 0;
    }
    fillFileDescriptor(&currentFile, fileName, bufferStart, bufferSize, file, offset);
    setCurrentFileInfoFor(fileName);
    setCInputConsistency();
    s_ifEvaluation = false;				/* TODO: WTF??? */
}

/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

/* maybe too complex/general, long argument list, when lex is known */
/* could be broken into parts for specific lexem types */
static void passLexem(char **input, Lexem lexem, int *lineNumberP, int *valueP, Position *positionP, int *lengthP, bool countLines) {
    if (lexem > MULTI_TOKENS_START) {
        if (isIdentifierLexem(lexem)){
            *input = strchr(*input, '\0')+1;
            *positionP = getLexPosition(input);
        } else if (lexem == STRING_LITERAL) {
            *input = strchr(*input, '\0')+1;
            *positionP = getLexPosition(input);
        } else if (lexem == LINE_TOKEN) {
            *lineNumberP = getLexToken(input);
            if (countLines) {
                traceNewline(*lineNumberP);
                currentFile.lineNumber += *lineNumberP;
            }
        } else if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
            *valueP = getLexInt(input);
            *positionP = getLexPosition(input);
            *lengthP = getLexInt(input);
        } else if (lexem == DOUBLE_CONSTANT || lexem == FLOAT_CONSTANT) {
            *positionP = getLexPosition(input);
            *lengthP = getLexInt(input);
        } else if (lexem == CPP_MACRO_ARGUMENT) {
            *valueP = getLexInt(input);
            *positionP = getLexPosition(input);
        } else if (lexem == CHAR_LITERAL) {
            *valueP = getLexInt(input);
            *positionP = getLexPosition(input);
            *lengthP = getLexInt(input);
        }
    } else if (isPreprocessorToken(lexem)) {
        *positionP = getLexPosition(input);
    } else if (lexem == '\n' && (countLines)) {
        *positionP = getLexPosition(input);
        traceNewline(1);
        currentFile.lineNumber ++;
    } else {
        *positionP = getLexPosition(input);
    }
}

static Lexem getLexemSavePrevious(char **previousLexem) {
    Lexem lexem;

    while (currentInput.currentLexemP >= currentInput.endOfBuffer) {
        InputType inputType;
        inputType = currentInput.inputType;
        if (macroStackIndex > 0) {
            if (inputType == INPUT_MACRO_ARGUMENT) {
                return END_OF_MACRO_ARGUMENT_EXCEPTION;
            }
            MB_FREE_UNTIL(currentInput.beginningOfBuffer);
            currentInput = macroInputStack[--macroStackIndex];
        } else if (inputType == INPUT_NORMAL) {
            setCFileConsistency();
            if (!getLexemFromLexer(&currentFile.lexBuffer)) {
                return END_OF_FILE_EXCEPTION;
            }
            setCInputConsistency();
        } else {
            cache.cc = cache.cfin = NULL;
            cacheInput();
            cache.lexcc = currentFile.lexBuffer.next;
            setCInputConsistency();
        }
        *previousLexem = currentInput.currentLexemP;
    }
    *previousLexem = currentInput.currentLexemP;
    lexem = getLexToken(&currentInput.currentLexemP);
    return lexem;
}

static int getLex() {
    char *previousLexem;
    UNUSED previousLexem;
    Lexem lexem = getLexemSavePrevious(&previousLexem);
    return lexem;
}


/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

static void testCxrefCompletionId(Lexem *out_lexem, char *idd, Position *pos) {
    Lexem lexem;

    lexem = *out_lexem;
    assert(options.taskRegime);
    if (options.taskRegime == RegimeEditServer) {
        if (lexem==IDENT_TO_COMPLETE) {
            cache.active = false;
            s_olstringServed = true;
            if (s_language == LANG_JAVA) {
                makeJavaCompletions(idd, strlen(idd), pos);
            }
            else if (s_language == LANG_YACC) {
                makeYaccCompletions(idd, strlen(idd), pos);
            }
            else {
                makeCCompletions(idd, strlen(idd), pos);
            }
            /* here should be a longjmp to stop file processing !!!! */
            lexem = IDENTIFIER;
        }
    }
    *out_lexem = lexem;
}



/* ********************************** #LINE *********************** */
/* non-static only for unittesting */
void processLineDirective(void) {
    Lexem lexem;
    int lineNumber, value, length; UNUSED length; UNUSED value; UNUSED lineNumber;
    Position position; UNUSED position;

    lexem = getLex();
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
    if (lexem != CONSTANT)
        return;

    lexem = getLex();
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
    return;

endOfMacroArgument:
    assert(0);
endOfFile:
    return;
}

/* ********************************* #INCLUDE ********************** */

static void fillIncludeSymbolItem(Symbol *ss, int filenum, Position *pos){
    fillSymbol(ss, LINK_NAME_INCLUDE_REFS, LINK_NAME_INCLUDE_REFS, *pos);
    fillSymbolBits(&ss->bits, AccessDefault, TypeCppInclude, StorageDefault);
}


void addThisFileDefineIncludeReference(int fileIndex) {
    Position position;
    Symbol symbol;

    position = makePosition(fileIndex, 1, 0);
    fillIncludeSymbolItem(&symbol,fileIndex, &position);
    log_trace("adding reference on file %d==%s", fileIndex, getFileItem(fileIndex)->name);
    addCxReference(&symbol, &position, UsageDefined, fileIndex, fileIndex);
}

void addIncludeReference(int fileIndex, Position *position) {
    Symbol symbol;

    log_trace("adding reference on file %d==%s", fileIndex, getFileItem(fileIndex)->name);
    fillIncludeSymbolItem( &symbol, fileIndex, position);
    addCxReference(&symbol, position, UsageUsed, fileIndex, fileIndex);
}

static void addIncludeReferences(int fileIndex, Position *position) {
    addIncludeReference(fileIndex, position);
    addThisFileDefineIncludeReference(fileIndex);
}

void pushInclude(FILE *file, EditorBuffer *buffer, char *name, char *prepend) {
    if (currentInput.inputType == INPUT_CACHE) {
        setCacheConsistency();
    } else {
        setCFileConsistency();
    }
    includeStack[includeStackPointer] = currentFile;		/* buffers are copied !!!!!!, burk */
    if (includeStackPointer+1 >= INCLUDE_STACK_SIZE) {
        fatalError(ERR_ST,"too deep nesting in includes", XREF_EXIT_ERR);
    }
    includeStackPointer ++;
    initInput(file, buffer, prepend, name);
    cacheInclude(currentFile.lexBuffer.buffer.fileNumber);
}

void popInclude(void) {
    FileItem *fileItem = getFileItem(currentFile.lexBuffer.buffer.fileNumber);
    if (fileItem->bits.cxLoading) {
        fileItem->bits.cxLoaded = true;
    }
    closeCharacterBuffer(&currentFile.lexBuffer.buffer);
    if (includeStackPointer != 0) {
        currentFile = includeStack[--includeStackPointer];	/* buffers are copied !!!!!!, burk */
        if (includeStackPointer == 0 && cache.cc!=NULL) {
            fillLexInput(&currentInput, cache.cc, cache.cfin, cache.lb, NULL, INPUT_CACHE);
        } else {
            setCInputConsistency();
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
        MapOnPaths(wildcardExpandedPaths, {
                int length;
                strcpy(normalizedName, currentPath);
                length = strlen(normalizedName);
                if (length > 0 && normalizedName[length-1] != FILE_PATH_SEPARATOR) {
                    normalizedName[length] = FILE_PATH_SEPARATOR;
                    length++;
                }
                strcpy(normalizedName+length, name);
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

static void processInclude2(Position *ipos, char pchar, char *iname, bool is_include_next) {
    char *fname;
    Symbol ss,*memb;
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "PragmaOnce-%s", iname);

    fillSymbol(&ss, tmpBuff, tmpBuff, noPosition);
    fillSymbolBits(&ss.bits, AccessDefault, TypeMacro, StorageNone);

    if (symbolTableIsMember(symbolTable, &ss, NULL, &memb))
        return;
    if (!openInclude(pchar, iname, &fname, is_include_next)) {
        assert(options.taskRegime);
        if (options.taskRegime!=RegimeEditServer)
            warningMessage(ERR_CANT_OPEN, iname);
        else
            log_error("Can't open file '%s'", fname);
    } else {
        addIncludeReferences(currentFile.lexBuffer.buffer.fileNumber, ipos);
    }
}

/* Public only for unittests */
void processIncludeDirective(Position *includePosition, bool is_include_next) {
    char *currentLexemP, *previousLexemP;
    Lexem lexem;
    int lineNumber, value, length; UNUSED length; UNUSED value; UNUSED lineNumber;
    Position position; UNUSED position;

    lexem = getLexemSavePrevious(&previousLexemP);
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    currentLexemP = currentInput.currentLexemP;
    if (lexem == STRING_LITERAL) {
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
        if (macroStackIndex != 0) {
            errorMessage(ERR_INTERNAL,"include directive in macro body?");
assert(0);
            currentInput = macroInputStack[0];
            macroStackIndex = 0;
        }
        processInclude2(includePosition, *currentLexemP, currentLexemP+1, is_include_next);
    } else {
        currentInput.currentLexemP = previousLexemP;		/* unget lexem */
        lexem = yylex();
        if (lexem == STRING_LITERAL) {
            currentInput = macroInputStack[0];		// hack, cut everything pending
            macroStackIndex = 0;
            processInclude2(includePosition, '\"', yytext, is_include_next);
        } else if (lexem == '<') {
            // TODO!!!!
            warningMessage(ERR_ST,"Include <> after macro expansion not yet implemented, sorry\n\tuse \"\" instead");
        }
        //do lex = yylex(); while (lex != '\n');
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
    Symbol *memb;

    isMember = symbolTableIsMember(symbolTable, pp, &index, &memb);
    if (isMember) {
        log_trace(": masking macro %s", name);
    } else {
        log_trace(": adding macro %s", name);
    }
    symbolTableSet(symbolTable, pp, index);
}

static void setMacroArgumentName(MacroArgumentTableElement *arg, void *at) {
    char ** argTab;
    argTab = (char**)at;
    assert(arg->order>=0);
    argTab[arg->order] = arg->name;
}

static void handleMacroDefinitionParameterPositions(int argi, Position *macpos,
                                                    Position *parpos1,
                                                    Position *pos, Position *parpos2,
                                                    int final) {
    if ((options.server_operation == OLO_GOTO_PARAM_NAME || options.server_operation == OLO_GET_PARAM_COORDINATES)
        && positionsAreEqual(*macpos, s_cxRefPos)) {
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(parpos1);
            } else if (argi < options.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(parpos2);
            }
        } else if (argi == options.olcxGotoVal) {
            s_paramPosition = *pos;
            s_paramBeginPosition = *parpos1;
            s_paramEndPosition = *parpos2;
        }
    }
}

static void handleMacroUsageParameterPositions(int argi, Position *macpos,
                                               Position *parpos1, Position *parpos2,
                                               int final
    ) {
    if (options.server_operation == OLO_GET_PARAM_COORDINATES
        && positionsAreEqual(*macpos, s_cxRefPos)) {
        log_trace("checking param %d at %d,%d, final==%d", argi, parpos1->col, parpos2->col, final);
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(parpos1);
            } else if (argi < options.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(parpos2);
            }
        } else if (argi == options.olcxGotoVal) {
            s_paramBeginPosition = *parpos1;
            s_paramEndPosition = *parpos2;
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


static Lexem getNonBlankLexem(Position *position, int *lineNumber, int *value, int *length) {
    Lexem lexem;

    lexem = getLex();
    while (lexem == LINE_TOKEN) {
        passLexem(&currentInput.currentLexemP, lexem, lineNumber, value, position, length, true);
        lexem = getLex();
    }
    return lexem;
}


/* Public only for unittesting */
void processDefineDirective(bool hasArguments) {
    Lexem lexem;
    int foundIndex;
    int ellipsis;
    MacroArgumentTableElement *maca, mmaca;
    MacroBody *macroBody;
    Position position, macroPosition, ppb1, ppb2, *parpos1, *parpos2, *tmppp;
    char *currentLexemStart, *argumentName;
    char *mm;
    char **argumentNames, *argLinkName;
    int lineNumber, value, length; UNUSED lineNumber; UNUSED value; UNUSED length;

    int argumentCount = 0;
    bool isReadingBody = false;
    int macroSize = 0;
    char *macroName = NULL;
    int allocatedSize = 0;
    Symbol *symbol = NULL;
    char *body = NULL;

    //symbol=NULL;macroBody=NULL;macroName=body=NULL; // to calm compiler

    argumentCount=0;
    macroSize = -1;

    SM_INIT(ppMemory);
    ppb1 = noPosition;
    ppb2 = noPosition;
    parpos1 = &ppb1;
    parpos2 = &ppb2;

    lexem = getLex();
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    currentLexemStart = currentInput.currentLexemP;

    /* There are "symbols" in the lexBuffer, like "\275\001".  Those
     * are compacted converted lexem codes. See lexembuffer.h for
     * explanation and functions.
     */
    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &macroPosition, &length, true);

    testCxrefCompletionId(&lexem, currentLexemStart, &macroPosition);    /* for cross-referencing */
    if (lexem != IDENTIFIER)
        return;

    PPM_ALLOC(symbol, Symbol);
    fillSymbol(symbol, NULL, NULL, macroPosition);
    fillSymbolBits(&symbol->bits, AccessDefault, TypeMacro, StorageNone);

    /* TODO: this is the only call to setGlobalFileDepNames() that doesn't do it in XX memory, why?
       PP == PreProcessor? */
    setGlobalFileDepNames(currentLexemStart, symbol, MEMORY_PP);
    macroName = symbol->name;
    /* process arguments */
    macroArgumentTableNoAllocInit(&macroArgumentTable, macroArgumentTable.size);
    argumentCount = -1;

    if (hasArguments) {
        lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, parpos2, &length, true);
        *parpos1 = *parpos2;
        if (lexem != '(')
            goto errorlab;
        argumentCount++;
        lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

        if (lexem != ')') {
            for(;;) {
                char tmpBuff[TMP_BUFF_SIZE];
                currentLexemStart = argumentName = currentInput.currentLexemP;
                passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
                ellipsis = 0;
                if (lexem == IDENTIFIER ) {
                    argumentName = currentLexemStart;
                } else if (lexem == ELLIPSIS) {
                    argumentName = s_cppVarArgsName;
                    position = macroPosition;					// hack !!!
                    ellipsis = 1;
                } else
                    goto errorlab;
                PPM_ALLOCC(mm, strlen(argumentName)+1, char);
                strcpy(mm, argumentName);
                sprintf(tmpBuff, "%x-%x%c%s", position.file, position.line,
                        LINK_NAME_SEPARATOR, argumentName);
                PPM_ALLOCC(argLinkName, strlen(tmpBuff)+1, char);
                strcpy(argLinkName, tmpBuff);
                SM_ALLOC(ppMemory, maca, MacroArgumentTableElement);
                fillMacroArgTabElem(maca, mm, argLinkName, argumentCount);
                foundIndex = macroArgumentTableAdd(&macroArgumentTable, maca);
                argumentCount++;
                lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
                EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

                tmppp=parpos1; parpos1=parpos2; parpos2=tmppp;
                passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, parpos2, &length, true);
                if (!ellipsis) {
                    addTrivialCxReference(macroArgumentTable.tab[foundIndex]->linkName, TypeMacroArg,StorageDefault,
                                          &position, UsageDefined);
                    handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &position, parpos2, 0);
                }
                if (lexem == ELLIPSIS) {
                    lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
                    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);
                    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, parpos2, &length, true);
                }
                if (lexem == ')')
                    break;
                if (lexem != ',')
                    break;

                lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
                EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);
            }
            handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &noPosition, parpos2, 1);
        } else {
            passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, parpos2, &length, true);
            handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &noPosition, parpos2, 1);
        }
    }
    /* process macro body */
    allocatedSize = MACRO_UNIT_SIZE;
    macroSize = 0;
    PPM_ALLOCC(body, allocatedSize+MAX_LEXEM_SIZE, char);
    isReadingBody = true;

    lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    currentLexemStart = currentInput.currentLexemP;
    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
    while (lexem != '\n') {
        while(macroSize<allocatedSize && lexem != '\n') {
            char *destination;
            fillMacroArgTabElem(&mmaca,currentLexemStart,NULL,0);
            if (lexem==IDENTIFIER && macroArgumentTableIsMember(&macroArgumentTable,&mmaca,&foundIndex)){
                /* macro argument */
                addTrivialCxReference(macroArgumentTable.tab[foundIndex]->linkName, TypeMacroArg,StorageDefault,
                                      &position, UsageUsed);
                destination = body+macroSize;
                putLexToken(CPP_MACRO_ARGUMENT, &destination);
                putLexInt(macroArgumentTable.tab[foundIndex]->order, &destination);
                putLexPosition(position.file, position.line,position.col, &destination);
                macroSize = destination - body;
            } else {
                if (lexem==IDENT_TO_COMPLETE
                    || (lexem == IDENTIFIER && positionsAreEqual(position, s_cxRefPos))) {
                    cache.active = false;
                    s_olstringFound = true;
                    s_olstringInMbody = symbol->linkName;
                }
                destination = body+macroSize;
                putLexToken(lexem, &destination);
                for (; currentLexemStart<currentInput.currentLexemP; destination++,currentLexemStart++)
                    *destination = *currentLexemStart;
                macroSize = destination - body;
            }
            lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
            EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

            currentLexemStart = currentInput.currentLexemP;
            passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
        }
        if (lexem != '\n') {
            allocatedSize += MACRO_UNIT_SIZE;
            PPM_REALLOCC(body, allocatedSize+MAX_LEXEM_SIZE, char,
                        allocatedSize+MAX_LEXEM_SIZE-MACRO_UNIT_SIZE);
        }
    }

endOfBody:
    assert(macroSize>=0);
    PPM_REALLOCC(body, macroSize, char, allocatedSize+MAX_LEXEM_SIZE);
    allocatedSize = macroSize;
    if (argumentCount > 0) {
        PPM_ALLOCC(argumentNames, argumentCount, char*);
        memset(argumentNames, 0, argumentCount*sizeof(char*));
        macroArgumentTableMap2(&macroArgumentTable, setMacroArgumentName, argumentNames);
    } else
        argumentNames = NULL;
    macroBody = newMacroBody(allocatedSize, argumentCount, macroName, body, argumentNames);
    symbol->u.mbody = macroBody;

    addMacroToTabs(symbol, macroName);
    assert(options.taskRegime);
    addCxReference(symbol, &macroPosition, UsageDefined, noFileIndex, noFileIndex);
    return;

endOfMacroArgument:
    assert(0);

endOfFile:
    if (isReadingBody)
        goto endOfBody;
    return;

errorlab:
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
    int lineNumber, value, length;
    UNUSED length; UNUSED value; UNUSED lineNumber;

    lexem = getLex();
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    cc = currentInput.currentLexemP;
    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
    testCxrefCompletionId(&lexem, cc, &position);
    if (isIdentifierLexem(lexem)) {
        Symbol symbol;
        Symbol *member;

        log_debug(": undef macro %s", cc);

        fillSymbol(&symbol, cc, cc, position);
        fillSymbolBits(&symbol.bits, AccessDefault, TypeMacro, StorageNone);

        assert(options.taskRegime);
        /* !!!!!!!!!!!!!! tricky, add macro with mbody == NULL !!!!!!!!!! */
        /* this is because of monotonicity for caching, just adding symbol */
        if (symbolTableIsMember(symbolTable, &symbol, NULL, &member)) {
            Symbol *pp;
            addCxReference(member, &position, UsageUndefinedMacro, noFileIndex, noFileIndex);

            PPM_ALLOC(pp, Symbol);
            fillSymbol(pp, member->name, member->linkName, position);
            fillSymbolBits(&pp->bits, AccessDefault, TypeMacro, StorageNone);

            addMacroToTabs(pp, member->name);
        }
    }
    while (lexem != '\n') {
        lexem = getLex();
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
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
    S_cppIfStack       *ss;
    if (level > 0) {
      PPM_ALLOC(ss, S_cppIfStack);
      ss->position = *pos;
      ss->next = currentFile.ifStack;
      currentFile.ifStack = ss;
    }
    if (currentFile.ifStack!=NULL) {
      dp = currentFile.ifStack->position;
      sprintf(ttt,"CppIf%x-%x-%d", dp.file, dp.col, dp.line);
      addTrivialCxReference(ttt, TypeCppIfElse,StorageDefault, pos, usage);
      if (level < 0) currentFile.ifStack = currentFile.ifStack->next;
    }
}

static int cppDeleteUntilEndElse(bool untilEnd) {
    Lexem lexem;
    int depth;
    Position position;
    int lineNumber, value, length; UNUSED length; UNUSED value;

    depth = 1;
    while (depth > 0) {
        lexem = getLex();
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
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
    if (options.taskRegime!=RegimeEditServer) {
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
    char *cc;
    Position position;
    bool deleteSrc;
    int lineNumber, value, length;
    UNUSED length; UNUSED value; UNUSED lineNumber;

    lexem = getLex();
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    cc = currentInput.currentLexemP;
    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
    testCxrefCompletionId(&lexem, cc, &position);

    if (!isIdentifierLexem(lexem))
        return;

    Symbol symbol;
    fillSymbol(&symbol, cc, cc, noPosition);
    fillSymbolBits(&symbol.bits, AccessDefault, TypeMacro, StorageNone);

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

int cexp_yylex(void) {
    int lineNumber, res, mm;
    Lexem lexem;
    char *cc;
    Symbol dd, *memb;
    Position position;
    bool haveParenthesis;
    int value, length;
    UNUSED length; UNUSED value;

    lexem = yylex();
    if (isIdentifierLexem(lexem)) {
        // this is useless, as it would be set to 0 anyway
        lexem = cexpTranslateToken(CONSTANT, 0);
    } else if (lexem == CPP_DEFINED_OP) {
        lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

        cc = currentInput.currentLexemP;
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
        if (lexem == '(') {
            haveParenthesis = true;

            lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
            EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

            cc = currentInput.currentLexemP;
            passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
        } else {
            haveParenthesis = false;
        }

        if (!isIdentifierLexem(lexem))
            return 0;

        fillSymbol(&dd, cc, cc, noPosition);
        fillSymbolBits(&dd.bits, AccessDefault, TypeMacro, StorageNone);

        log_debug("(%s)", dd.name);

        mm = symbolTableIsMember(symbolTable, &dd, NULL, &memb);
        if (mm && memb->u.mbody == NULL)
            mm = 0;   // undefined macro
        assert(options.taskRegime);
        if (mm)
            addCxReference(&dd, &position, UsageUsed, noFileIndex, noFileIndex);

        /* following call sets uniyylval */
        res = cexpTranslateToken(CONSTANT, mm);
        if (haveParenthesis) {
            lexem = getNonBlankLexem(&position, &lineNumber, &value, &length);
            EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

            passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
            if (lexem != ')' && options.taskRegime!=RegimeEditServer) {
                warningMessage(ERR_ST,"missing ')' after defined( ");
            }
        }
        lexem = res;
    } else {
        lexem = cexpTranslateToken(lexem, uniyylval->ast_integer.d);
    }
    return lexem;
endOfMacroArgument:
    assert(0);
endOfFile:
    return 0;
}

static void processIfDirective(void) {
    int res=1,lex;
    s_ifEvaluation = 1;
    log_debug(": #if");
    res = cexp_yyparse();
    do lex = yylex(); while (lex != '\n');
    s_ifEvaluation = 0;
    execCppIf(! res);
}

static void processPragmaDirective(void) {
    Lexem lexem;
    int lineNumber;
    char *mname, *fname;
    Position position;
    Symbol *pp;
    int value, length; UNUSED length; UNUSED value;

    lexem = getLex();
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    if (lexem == IDENTIFIER && strcmp(currentInput.currentLexemP, "once")==0) {
        char tmpBuff[TMP_BUFF_SIZE];
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
        fname = simpleFileName(getFileItem(position.file)->name);
        sprintf(tmpBuff, "PragmaOnce-%s", fname);
        PPM_ALLOCC(mname, strlen(tmpBuff)+1, char);
        strcpy(mname, tmpBuff);

        PPM_ALLOC(pp, Symbol);
        fillSymbol(pp, mname, mname, position);
        fillSymbolBits(&pp->bits, AccessDefault, TypeMacro, StorageNone);

        symbolTableAdd(symbolTable, pp);
    }
    while (lexem != '\n') {
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
        lexem = getLex();
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);
    }
    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
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
    int lineNumber;
    Position position;
    int value, length; UNUSED length; UNUSED value;

    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
    log_debug("processing cpp-construct '%s' ", tokenNamesTable[lexem]);
    switch (lexem) {
    case CPP_INCLUDE:
        processIncludeDirective(&position, false);
        break;
    case CPP_INCLUDE_NEXT:
        processIncludeDirective(&position, true);
        break;
    case CPP_DEFINE0:
        processDefineDirective(0);
        break;
    case CPP_DEFINE:
        processDefineDirective(1);
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
        } else if (options.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"unmatched #elif");
        }
        break;
    case CPP_ELSE:
        log_debug("#else");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, &position, UsageUsed);
            currentFile.ifDepth --;
            cppDeleteUntilEndElse(true);
        } else if (options.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"unmatched #else");
        }
        break;
    case CPP_ENDIF:
        log_debug("#endif");
        if (currentFile.ifDepth) {
            currentFile.ifDepth --;
            genCppIfElseReference(-1, &position, UsageUsed);
        } else if (options.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"unmatched #endif");
        }
        break;
    case CPP_PRAGMA:
        log_debug("#pragma");
        processPragmaDirective();
        break;
    case CPP_LINE: {
        processLineDirective();

        lexem = getLex();
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);
        while (lexem != '\n') {
            passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
            lexem = getLex();
            EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);
        }
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
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

#define TestPPBufOverflow(bcc,buf,bsize) {                      \
        if (bcc >= buf+bsize) {                                 \
            bsize += MACRO_UNIT_SIZE;                           \
            PPM_REALLOCC(buf,bsize+MAX_LEXEM_SIZE,char,         \
                         bsize+MAX_LEXEM_SIZE-MACRO_UNIT_SIZE); \
        }                                                       \
    }
#define TestMBBufOverflow(bcc,len,buf2,bsize) {                 \
        while (bcc + len >= buf2 + bsize) {                     \
            bsize += MACRO_UNIT_SIZE;                           \
            MB_REALLOCC(buf2,bsize+MAX_LEXEM_SIZE,char,         \
                        bsize+MAX_LEXEM_SIZE-MACRO_UNIT_SIZE);  \
        }                                                       \
    }

/* *********************************************************** */

static bool cyclicCall(MacroBody *mb) {
    char *name = mb->name;
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


static void prependMacroInput(LexInput *argb) {
    assert(macroStackIndex < MACRO_INPUT_STACK_SIZE-1);
    macroInputStack[macroStackIndex++] = currentInput;
    currentInput = *argb;
    currentInput.currentLexemP = currentInput.beginningOfBuffer;
    currentInput.inputType = INPUT_MACRO;
}


static void expandMacroArgument(LexInput *argb) {
    Symbol sd, *memb;
    char *previousLexem, *currentLexem, *tbcc;
    int length, lineNumber, failedMacroExpansion;
    Lexem lexem;
    Position position;
    int value;
    UNUSED value;
    char *buf;
    char *bcc;
    int bsize;

    bsize = MACRO_UNIT_SIZE;

    prependMacroInput(argb);

    currentInput.inputType = INPUT_MACRO_ARGUMENT;
    PPM_ALLOCC(buf,bsize+MAX_LEXEM_SIZE,char);
    bcc = buf;

    for(;;) {
    nextLexem:
        lexem = getLexemSavePrevious(&previousLexem);
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

        currentLexem = currentInput.currentLexemP;
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
        length = ((char*)currentInput.currentLexemP) - previousLexem;
        assert(length >= 0);
        memcpy(bcc, previousLexem, length);
        // a hack, it is copied, but bcc will be increased only if not
        // an expanding macro, this is because 'macroCallExpand' can
        // read new lexbuffer and destroy cInput, so copy it now.
        failedMacroExpansion = 0;
        if (lexem == IDENTIFIER) {
            fillSymbol(&sd, currentLexem, currentLexem, noPosition);
            fillSymbolBits(&sd.bits, AccessDefault, TypeMacro, StorageNone);
            if (symbolTableIsMember(symbolTable, &sd, NULL, &memb)) {
                /* it is a macro, provide macro expansion */
                if (expandMacroCall(memb,&position)) goto nextLexem;
                else failedMacroExpansion = 1;
            }
        }
        if (failedMacroExpansion) {
            tbcc = bcc;
            assert(memb!=NULL);
            if (memb->u.mbody!=NULL && cyclicCall(memb->u.mbody)) putLexToken(IDENT_NO_CPP_EXPAND, &tbcc);
            //& else putLexToken(IDENT_NO_CPP_EXPAND, &tbcc);
        }
        bcc += length;
        TestPPBufOverflow(bcc, buf, bsize);
    }
endOfMacroArgument:
    currentInput = macroInputStack[--macroStackIndex];
    PPM_REALLOCC(buf, bcc-buf, char, bsize+MAX_LEXEM_SIZE);
    fillLexInput(argb, buf, bcc, buf, NULL, INPUT_NORMAL);
    return;
endOfFile:
    assert(0);
}

static void cxAddCollateReference( char *sym, char *cs, Position *pos ) {
    char ttt[TMP_STRING_SIZE];
    Position pps;
    strcpy(ttt,sym);
    assert(cs>=sym && cs-sym<TMP_STRING_SIZE);
    sprintf(ttt+(cs-sym), "%c%c%s", LINK_NAME_COLLATE_SYMBOL,
            LINK_NAME_COLLATE_SYMBOL, cs);
    pps = *pos;
    addTrivialCxReference(ttt, TypeCppCollate,StorageDefault, &pps, UsageDefined);
}


/* **************************************************************** */

static void collate(char **albcc, char **abcc, char *buf, int *absize,
                    char **ancc, LexInput *actArgs) {
    char *lbcc,*bcc,*cc,*ccfin,*cc0,*ncc,*occ;
    int lineNumber, value, nlex, len1, bsize;
    Lexem lexem;
    Position respos;
    Position position; UNUSED position;
    int length; UNUSED length;

    ncc = *ancc;
    lbcc = *albcc;
    bcc = *abcc;
    bsize = *absize;
    value=0; // compiler
    if (nextLexToken(&lbcc) == CPP_MACRO_ARGUMENT) {
        bcc = lbcc;
        lexem = getLexToken(&lbcc);
        assert(lexem==CPP_MACRO_ARGUMENT);
        passLexem(&lbcc, lexem, &lineNumber, &value, &respos, &length, false);
        cc = actArgs[value].beginningOfBuffer; ccfin = actArgs[value].endOfBuffer;
        lbcc = NULL;
        while (cc < ccfin) {
            cc0 = cc;
            lexem = getLexToken(&cc);
            passLexem(&cc, lexem, &lineNumber, &value, &respos, &length, false);
            lbcc = bcc;
            assert(cc>=cc0);
            memcpy(bcc, cc0, cc-cc0);
            bcc += cc-cc0;
            TestPPBufOverflow(bcc,buf,bsize);
        }
    }
    if (nextLexToken(&ncc) == CPP_MACRO_ARGUMENT) {
        lexem = getLexToken(&ncc);
        passLexem(&ncc, lexem, &lineNumber, &value, &position, &length, false);
        cc = actArgs[value].beginningOfBuffer; ccfin = actArgs[value].endOfBuffer;
    } else {
        cc = ncc;
        lexem = getLexToken(&ncc);
        passLexem(&ncc, lexem, &lineNumber, &value, &position, &length, false);
        ccfin = ncc;
    }
    /* now collate *lbcc and *cc */
    // berk, do not pre-compute, lbcc can be NULL!!!!
    if (lbcc!=NULL && cc < ccfin && isIdentifierLexem(nextLexToken(&lbcc))) {
        nlex = nextLexToken(&cc);
        if (isIdentifierLexem(nlex) || nlex == CONSTANT
                    || nlex == LONG_CONSTANT || nlex == FLOAT_CONSTANT
                    || nlex == DOUBLE_CONSTANT ) {
            /* TODO collation of all lexem pairs */
            len1 = strlen(lbcc+IDENT_TOKEN_SIZE);
            lexem = getLexToken(&cc);
            occ = cc;
            passLexem(&cc, lexem, &lineNumber, &value, &respos, &length, false);
            bcc = lbcc + IDENT_TOKEN_SIZE + len1;
            assert(*bcc==0);
            if (isIdentifierLexem(lexem)) {
/*				NextLexPosition(respos,bcc+1);	*/ /* new identifier position*/
                strcpy(bcc,occ);
                // the following is a hack as # is part of ## symbols
                respos.col --;
                assert(respos.col>=0);
                cxAddCollateReference( lbcc+IDENT_TOKEN_SIZE, bcc, &respos);
                respos.col ++;
            } else {
                NextLexPosition(respos,bcc+1);	/* new identifier position*/
                sprintf(bcc,"%d",value);
                cxAddCollateReference( lbcc+IDENT_TOKEN_SIZE, bcc, &respos);
            }
            bcc += strlen(bcc);
            assert(*bcc==0);
            bcc++;
            putLexPosition(respos.file,respos.line,respos.col, &bcc);
        }
    }
    TestPPBufOverflow(bcc,buf,bsize);
    while (cc<ccfin) {
        cc0 = cc;
        lexem = getLexToken(&cc);
        passLexem(&cc, lexem, &lineNumber, &value, &position, &length, false);
        lbcc = bcc;
        assert(cc>=cc0);
        memcpy(bcc, cc0, cc-cc0);
        bcc += cc-cc0;
        TestPPBufOverflow(bcc,buf,bsize);
    }
    *albcc = lbcc;
    *abcc = bcc;
    *ancc = ncc;
    *absize = bsize;
}

static void macArgsToString(char *res, struct lexInput *lb) {
    char *cc, *lcc, *bcc;
    int value,lineNumber;
    Lexem lexem;
    Position position; UNUSED position;
    int length; UNUSED length;

    bcc = res;
    *bcc = 0;
    value=0;
    cc = lb->beginningOfBuffer;
    while (cc < lb->endOfBuffer) {
        lexem = getLexToken(&cc);
        lcc = cc;
        passLexem(&cc, lexem, &lineNumber, &value, &position, &length, false);
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


/* ************************************************************** */
/* ********************* macro body replacement ***************** */
/* ************************************************************** */

static void createMacroBody(LexInput *macroBody,
                            MacroBody *mb,
                            struct lexInput *actArgs,
                            int actArgn
) {
    char *cc,*cc0,*cfin,*bcc,*lbcc;
    int i,lineNumber,value,len,bsize;
    Lexem lexem;
    Position hpos;
    char *buf,*buf2;
    Position position; UNUSED position;
    int length; UNUSED length;

    value=0; //compiler
    /* first make ## collations */
    bsize = MACRO_UNIT_SIZE;
    PPM_ALLOCC(buf,bsize+MAX_LEXEM_SIZE, char);
    cc = mb->body;
    cfin = mb->body + mb->size;
    bcc = buf;
    lbcc = NULL;
    while (cc < cfin) {
        cc0 = cc;
        lexem = getLexToken(&cc);
        passLexem(&cc, lexem, &lineNumber, &value, &position, &length, false);
        if (lexem==CPP_COLLATION && lbcc!=NULL && cc<cfin) {
            collate(&lbcc,&bcc,buf,&bsize,&cc,actArgs);
        } else {
            lbcc = bcc;
            assert(cc>=cc0);
            memcpy(bcc, cc0, cc-cc0);
            bcc += cc-cc0;
        }
        TestPPBufOverflow(bcc,buf,bsize);
    }
    PPM_REALLOCC(buf,bcc-buf,char,bsize+MAX_LEXEM_SIZE);


    /* expand arguments */
    for(i=0;i<actArgn; i++) {
        expandMacroArgument(&actArgs[i]);
    }

    /* replace arguments */
    bsize = MACRO_UNIT_SIZE;
    MB_ALLOCC(buf2,bsize+MAX_LEXEM_SIZE,char);
    cc = buf;
    cfin = bcc;
    bcc = buf2;
    while (cc < cfin) {
        cc0 = cc;
        lexem = getLexToken(&cc);
        passLexem(&cc, lexem, &lineNumber, &value, &hpos, &length, false);
        if (lexem == CPP_MACRO_ARGUMENT) {
            len = actArgs[value].endOfBuffer - actArgs[value].beginningOfBuffer;
            TestMBBufOverflow(bcc,len,buf2,bsize);
            memcpy(bcc, actArgs[value].beginningOfBuffer, len);
            bcc += len;
        } else if (lexem=='#' && cc<cfin && nextLexToken(&cc)==CPP_MACRO_ARGUMENT) {
            lexem = getLexToken(&cc);
            passLexem(&cc, lexem, &lineNumber, &value, &position, &length, false);
            assert(lexem == CPP_MACRO_ARGUMENT);
            putLexToken(STRING_LITERAL, &bcc);
            TestMBBufOverflow(bcc,MACRO_UNIT_SIZE,buf2,bsize);
            macArgsToString(bcc, &actArgs[value]);
            len = strlen(bcc)+1;
            bcc += len;
            putLexPosition(hpos.file, hpos.line, hpos.col, &bcc);
            if (len >= MACRO_UNIT_SIZE-15) {
                errorMessage(ERR_INTERNAL,"size of #macro_argument exceeded MACRO_UNIT_SIZE");
            }
        } else {
            TestMBBufOverflow(bcc,(cc0-cc),buf2,bsize);
            assert(cc>=cc0);
            memcpy(bcc, cc0, cc-cc0);
            bcc += cc-cc0;
        }
        TestMBBufOverflow(bcc,0,buf2,bsize);
    }
    MB_REALLOCC(buf2,bcc-buf2,char,bsize+MAX_LEXEM_SIZE);

    fillLexInput(macroBody,buf2,bcc,buf2,mb->name,INPUT_MACRO);
}

/* *************************************************************** */
/* ******************* MACRO CALL PROCESS ************************ */
/* *************************************************************** */

static Lexem getLexSkippingLines(char **previousLexemP, int *lineNumberP,
                                int *valueP, Position *positionP, int *lengthP) {
    Lexem lexem = getLexemSavePrevious(previousLexemP);
    while (lexem == LINE_TOKEN || lexem == '\n') {
        passLexem(&currentInput.currentLexemP, lexem, lineNumberP, valueP, positionP, lengthP,
                  macroStackIndex == 0);
        lexem = getLexemSavePrevious(previousLexemP);
    }
    return lexem;
}

static void getActualMacroArgument(
    char *previousLexem,
    Lexem *out_lexem,
    Position *mpos,
    Position **positionOfFirstParenthesis,
    Position **positionOfSecondParenthesis,
    LexInput *actArg,
    MacroBody *macroBody,
    int actualArgumentIndex
) {
    int lineNumber,poffset;
    int depth;
    Position pos; UNUSED pos;
    int value,len; UNUSED len; UNUSED value;
    Lexem lexem;
    char *buf;
    char *bcc;
    int bufsize;

    lexem = *out_lexem;
    bufsize = MACRO_ARG_UNIT_SIZE;
    depth = 0;
    PPM_ALLOCC(buf, bufsize+MAX_LEXEM_SIZE, char);
    bcc = buf;
    /* if lastArgument, collect everything there */
    poffset = 0;
    while (((lexem != ',' || actualArgumentIndex+1==macroBody->argCount) && lexem != ')') || depth > 0) {
        // The following should be equivalent to the loop condition:
        //& if (lexem == ')' && depth <= 0) break;
        //& if (lexem == ',' && depth <= 0 && ! lastArgument) break;
        if (lexem == '(') depth ++;
        if (lexem == ')') depth --;
        for(;previousLexem < currentInput.currentLexemP; previousLexem++, bcc++)
            *bcc = *previousLexem;
        if (bcc-buf >= bufsize) {
            bufsize += MACRO_ARG_UNIT_SIZE;
            PPM_REALLOCC(buf, bufsize+MAX_LEXEM_SIZE, char,
                    bufsize+MAX_LEXEM_SIZE-MACRO_ARG_UNIT_SIZE);
        }
        lexem = getLexSkippingLines(&previousLexem, &lineNumber, &value, &pos, &len);
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, (*positionOfSecondParenthesis), &len,
                macroStackIndex == 0);
        if ((lexem == ',' || lexem == ')') && depth == 0) {
            poffset ++;
            handleMacroUsageParameterPositions(actualArgumentIndex+poffset, mpos, *positionOfFirstParenthesis, *positionOfSecondParenthesis, 0);
            **positionOfFirstParenthesis= **positionOfSecondParenthesis;
        }
    }
    if (0) {  /* skip the error message when finished normally */
endOfFile:;
endOfMacroArgument:;
        assert(options.taskRegime);
        if (options.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"[getActMacroArgument] unterminated macro call");
        }
    }
    PPM_REALLOCC(buf, bcc-buf, char, bufsize+MAX_LEXEM_SIZE);
    fillLexInput(actArg, buf, bcc, buf, currentInput.macroName, INPUT_NORMAL);
    *out_lexem = lexem;
    return;
}

static struct lexInput *getActualMacroArguments(MacroBody *macroBody, Position *macroPosition,
                                                Position lparPosition) {
    char *previousLexem;
    Lexem lexem;
    int lineNumber;
    Position position, ppb1, ppb2, *parpos1, *parpos2;
    int argumentIndex = 0;
    struct lexInput *actualArgs;
    int value,length; UNUSED length; UNUSED value;

    ppb1 = lparPosition;
    ppb2 = lparPosition;
    parpos1 = &ppb1;
    parpos2 = &ppb2;
    PPM_ALLOCC(actualArgs, macroBody->argCount, struct lexInput);
    lexem = getLexSkippingLines(&previousLexem, &lineNumber, &value, &position, &length);
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

    passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
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
            lexem = getLexSkippingLines(&previousLexem, &lineNumber, &value, &position, &length);
            EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);
            passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length,
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
    assert(options.taskRegime);
    if (options.taskRegime!=RegimeEditServer) {
        warningMessage(ERR_ST,"[getActualMacroArguments] unterminated macro call");
    }
    return NULL;
}

/* **************************************************************** */

static void addMacroBaseUsageRef(Symbol *mdef) {
    int rr;
    ReferencesItem ppp, *memb;
    Reference *r;
    Position basePos;

    basePos = makePosition(inputFileNumber, 0, 0);
    fillSymbolRefItem(&ppp, mdef->linkName,
                                cxFileHashNumber(mdef->linkName), // useless, put 0
                                noFileIndex, noFileIndex);
    fillSymbolRefItemBits(&ppp.bits,TypeMacro, StorageDefault, ScopeGlobal,
                           mdef->bits.access, CategoryGlobal);
    rr = refTabIsMember(&referenceTable, &ppp, NULL, &memb);
    r = NULL;
    if (rr) {
        // this is optimization to avoid multiple base references
        for (r=memb->references; r!=NULL; r=r->next) {
            if (r->usage.kind == UsageMacroBaseFileUsage)
                break;
        }
    }
    if (rr==0 || r==NULL) {
        addCxReference(mdef, &basePos, UsageMacroBaseFileUsage,
                       noFileIndex, noFileIndex);
    }
}


static bool expandMacroCall(Symbol *macroSymbol, Position *macroPosition) {
    Lexem lexem;
    int lineNumber;
    Position lparPosition;
    LexInput *actualArgumentsInput, macroBodyInput;
    MacroBody *macroBody;
    Position pos; UNUSED pos;
    int value,length; UNUSED length; UNUSED value;
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
    previousLexemP = currentInput.currentLexemP;
    PPM_ALLOCC(freeBase, 0, char);

    if (macroBody->argCount >= 0) {
        lexem = getLexSkippingLines(&previousLexemP, &lineNumber, &value, &pos, &length);
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

        if (lexem != '(') {
            currentInput.currentLexemP = previousLexemP;		/* unget lexem */
            return false;
        }
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &lparPosition, &length,
                           macroStackIndex == 0);
        actualArgumentsInput = getActualMacroArguments(macroBody, macroPosition, lparPosition);
    } else {
        actualArgumentsInput = NULL;
    }
    assert(options.taskRegime);
    addCxReference(macroSymbol, macroPosition, UsageUsed, noFileIndex, noFileIndex);
    if (options.taskRegime == RegimeXref)
        addMacroBaseUsageRef(macroSymbol);
    log_trace("create macro body '%s'", macroBody->name);
    createMacroBody(&macroBodyInput,macroBody,actualArgumentsInput,macroBody->argCount);
    prependMacroInput(&macroBodyInput);
    log_trace("expanding macro '%s'", macroBody->name);
    PPM_FREE_UNTIL(freeBase);
    return true;

endOfMacroArgument:
    /* unterminated macro call in argument */
    /* TODO unread argument read */
    currentInput.currentLexemP = previousLexemP;
    PPM_FREE_UNTIL(freeBase);
    return false;

 endOfFile:
    assert(options.taskRegime);
    if (options.taskRegime!=RegimeEditServer) {
        warningMessage(ERR_ST,"[macroCallExpand] unterminated macro call");
    }
    currentInput.currentLexemP = previousLexemP;
    PPM_FREE_UNTIL(freeBase);
    return false;
}

int dumpLexemBuffer(LexemBuffer *lb) {
    char *cc;
    int lineNumber;
    Lexem lexem;
    Position position; UNUSED position;
    int value,length; UNUSED length; UNUSED value;

    fprintf(dumpOut,"\nlexbufdump [start] \n"); fflush(dumpOut);
    cc = lb->next;
    while (cc < lb->end) {
        lexem = getLexToken(&cc);
        if (lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND) {
            fprintf(dumpOut,"%s ",cc);
        } else if (lexem==IDENT_TO_COMPLETE) {
            fprintf(dumpOut,"!%s! ",cc);
        } else if (lexem < 256) {
            fprintf(dumpOut,"%c ",lexem);fflush(dumpOut);
        } else if (tokenNamesTable[lexem]==NULL){
            fprintf(dumpOut,"?%d? ",lexem);fflush(dumpOut);
        } else {
            fprintf(dumpOut,"%s ",tokenNamesTable[lexem]);fflush(dumpOut);
        }
        passLexem(&cc, lexem, &lineNumber, &value, &position, &length, false);
    }
    fprintf(dumpOut,"lexbufdump [stop]\n");fflush(dumpOut);
    return 0;
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
    int value, length; UNUSED length; UNUSED value;
    char *ccc;
    int res;

    assert(cpoint > 0);
    cto = cache.cp[cpoint].lbcc;
    ccc = *cfrom;
    res = 1;

    while (ccc < cto) {
        lexem = getLexemSavePrevious(&previousLexem);
        EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, true);
        lexemLength = currentInput.currentLexemP-previousLexem;
        assert(lexemLength >= 0);
        if (memcmp(previousLexem, ccc, lexemLength)) {
            currentInput.currentLexemP = previousLexem;			/* unget last lexem */
            res = 0;
            break;
        }
        if (isIdentifierLexem(lexem) || isPreprocessorToken(lexem)) {
            if (onSameLine(position, s_cxRefPos)) {
                currentInput.currentLexemP = previousLexem;			/* unget last lexem */
                res = 0;
                break;
            }
        }
        ccc += lexemLength;
    }
endOfFile:
    setCFileConsistency();
    *cfrom = ccc;
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
    if (symbol->bits.symbolType == TypeKeyword) {
        setYylvalsForIdentifier(symbol->name, symbol, position);
        return true;
    }
    return false;
}

/* TODO: Lookup for C and Java are similar, C has check for CPP and Typedef. Merge? */
/* And probably move to symboltable.c... */
static int lookupCIdentifier(char *id, Position position) {
    Symbol *symbol = NULL;
    unsigned hash = hashFun(id) % symbolTable->size;

    log_trace("looking for C id '%s' in symbol table %p", id, symbolTable);
    for (Symbol *s=symbolTable->tab[hash]; s!=NULL; s=s->next) {
        if (strcmp(s->name, id) == 0) {
            if (symbol == NULL)
                symbol = s;
            if (isIdAKeyword(s, position))
                return s->u.keyword;
            if (s->bits.symbolType == TypeDefinedOp && s_ifEvaluation) {
                return CPP_DEFINED_OP;
            }
            if (s->bits.symbolType == TypeDefault) {
                setYylvalsForIdentifier(s->name, s, position);
                if (s->bits.storage == StorageTypedef) {
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


static int lookupJavaIdentifier(char *id, Position position) {
    Symbol *symbol = NULL;
    unsigned hash = hashFun(id) % symbolTable->size;

    log_trace("looking for Java id '%s' in symbol table %p", id, symbolTable);
    for (Symbol *s=symbolTable->tab[hash]; s!=NULL; s=s->next) {
        if (strcmp(s->name, id) == 0) {
            if (symbol == NULL)
                symbol = s;
            if (isIdAKeyword(s, position))
                return s->u.keyword;
            if (s->bits.symbolType == TypeDefault) {
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
    if (options.server_operation == OLO_SET_MOVE_TARGET) {
        s_cps.setTargetAnswerClass[0] = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (s_cp.function == NULL) {
                if (s_javaStat!=NULL) {
                    if (s_javaStat->thisClass==NULL) {
                        sprintf(s_cps.setTargetAnswerClass, " %s", s_javaThisPackageName);
                    } else {
                        strcpy(s_cps.setTargetAnswerClass, s_javaStat->thisClass->linkName);
                    }
                }
            }
        }
    } else if (options.server_operation == OLO_SET_MOVE_CLASS_TARGET) {
        s_cps.moveTargetApproved = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (s_cp.function == NULL) {
                if (s_javaStat!=NULL) {
                    s_cps.moveTargetApproved = 1;
                }
            }
        }
    } else if (options.server_operation == OLO_SET_MOVE_METHOD_TARGET) {
        s_cps.moveTargetApproved = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (s_cp.function == NULL) {
                if (s_javaStat!=NULL) {
                    if (s_javaStat->thisClass!=NULL) {
                        s_cps.moveTargetApproved = 1;
                    }
                }
            }
        }
    } else if (options.server_operation == OLO_EXTRACT) {
        extractActionOnBlockMarker();
    } else {
        s_cps.currentPackageAnswer[0] = 0;
        s_cps.currentClassAnswer[0] = 0;
        s_cps.currentSuperClassAnswer[0] = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (s_javaStat!=NULL) {
                strcpy(s_cps.currentPackageAnswer, s_javaThisPackageName);
                if (s_javaStat->thisClass!=NULL) {
                    assert(s_javaStat->thisClass->u.structSpec);
                    strcpy(s_cps.currentClassAnswer, s_javaStat->thisClass->linkName);
                    if (s_javaStat->thisClass->u.structSpec->super!=NULL) {
                        assert(s_javaStat->thisClass->u.structSpec->super->d);
                        strcpy(s_cps.currentSuperClassAnswer, s_javaStat->thisClass->u.structSpec->super->d->linkName);
                    }
                }
            }
        }
        s_cp.parserPassedMarker = 1;
    }
}


int yylex(void) {
    Lexem lexem;
    Position position, idpos;
    char *previousLexem;
    int lineNumber, value;
    int length = 0;

 nextYylex:
    lexem = getLexemSavePrevious(&previousLexem);
    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

 contYylex:
    if (lexem < 256) {          /* First non-single character symbol is 257 */
        if (lexem == '\n') {
            if (s_ifEvaluation) {
                currentInput.currentLexemP = previousLexem;
            } else {
                passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length,
                                   macroStackIndex == 0);
                for(;;) {
                    lexem = getLex();
                    EXCEPTION_CHECK(lexem, endOfFile, endOfMacroArgument);

                    if (!isPreprocessorToken(lexem))
                        goto contYylex;
                    if (!processPreprocessorConstruct(lexem))
                        goto endOfFile;
                }
            }
        } else {
            passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length,
                               macroStackIndex == 0);
            setYylvalsForPosition(position, tokenNameLengthsTable[lexem]);
        }
        yytext = charText;
        charText[0] = lexem;
        goto finish;
    }
    if (lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND) {
        char *id;
        Symbol symbol, *memberP;

        id = yytext = currentInput.currentLexemP;
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &idpos, &length, macroStackIndex == 0);
        assert(options.taskRegime);
        if (options.taskRegime == RegimeEditServer) {
//			???????????? isn't this useless
            testCxrefCompletionId(&lexem,yytext,&idpos);
        }
        log_trace("id '%s' position %d, %d, %d", yytext, idpos.file, idpos.line, idpos.col);
        fillSymbol(&symbol, yytext, yytext, noPosition);
        fillSymbolBits(&symbol.bits, AccessDefault, TypeMacro, StorageNone);

        if ((!LANGUAGE(LANG_JAVA))
            && lexem!=IDENT_NO_CPP_EXPAND
            && symbolTableIsMember(symbolTable, &symbol, NULL, &memberP)) {
            // following is because the macro check can read new lexBuf,
            // so id would be destroyed
            //&assert(strcmp(id,memberP->name)==0);
            id = memberP->name;
            if (expandMacroCall(memberP,&idpos))
                goto nextYylex;
        }

        /* TODO: Push down language check into a common lookupIdentifier() */
        if (LANGUAGE(LANG_C)||LANGUAGE(LANG_YACC))
            lexem=lookupCIdentifier(id, idpos);
        else if (LANGUAGE(LANG_JAVA))
            lexem = lookupJavaIdentifier(id, idpos);
        else
            assert(0);

        position = idpos;            /* To simplify debug - pos is always current at finish: */
        goto finish;
    }
    if (lexem == OL_MARKER_TOKEN) {
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
        actionOnBlockMarker();
        goto nextYylex;
    }
    if (lexem < MULTI_TOKENS_START) {
        yytext = tokenNamesTable[lexem];
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
        setYylvalsForPosition(position, tokenNameLengthsTable[lexem]);
        goto finish;
    }
    if (lexem == LINE_TOKEN) {
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
        goto nextYylex;
    }
    if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
        value=0;//compiler
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
        sprintf(constant,"%d",value);
        setYylvalsForInteger(value, position, length);
        yytext = constant;
        goto finish;
    }
    if (lexem == FLOAT_CONSTANT || lexem == DOUBLE_CONSTANT) {
        yytext = "'fltp constant'";
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
        setYylvalsForPosition(position, length);
        goto finish;
    }
    if (lexem == STRING_LITERAL) {
        yytext = currentInput.currentLexemP;
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
        setYylvalsForPosition(position, strlen(yytext));
        goto finish;
    }
    if (lexem == CHAR_LITERAL) {
        value=0;//compiler
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
        sprintf(constant,"'%c'",value);
        setYylvalsForInteger(value, position, length);
        yytext = constant;
        goto finish;
    }
    assert(options.taskRegime);
    if (options.taskRegime == RegimeEditServer) {
        yytext = currentInput.currentLexemP;
        passLexem(&currentInput.currentLexemP, lexem, &lineNumber, &value, &position, &length, macroStackIndex == 0);
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

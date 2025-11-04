#include "yylex.h"

#include <ctype.h>
#include <string.h>

#include "c_parser.h"
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
#include "head.h"
#include "lexemstream.h"
#include "lexem.h"
#include "lexembuffer.h"
#include "lexer.h"
#include "log.h"
#include "macroargumenttable.h"
#include "memory.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "reftab.h"
#include "semact.h"
#include "stackmemory.h"
#include "storage.h"
#include "type.h"
#include "yacc_parser.h"


static bool isProcessingPreprocessorIf; /* Flag for yylex, to not filter '\n' */

static void setYylvalsForIdentifier(char *name, Symbol *symbol, Position position) {
    uniyylval->ast_id.data = &yyIdBuffer[yyIdBufferIndex];
    *uniyylval->ast_id.data = makeId(name, symbol, position);
    yyIdBufferIndex++;
    yyIdBufferIndex %= (YYIDBUFFER_SIZE);
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


/* Exceptions when reading lexems: */
#define END_OF_MACRO_ARGUMENT_EXCEPTION -1
#define END_OF_FILE_EXCEPTION -2
#define ON_LEXEM_EXCEPTION_GOTO(lexem, eof_label, eom_label)    \
    switch ((int)lexem) {                                       \
    case END_OF_FILE_EXCEPTION: goto eof_label;                 \
    case END_OF_MACRO_ARGUMENT_EXCEPTION: goto eom_label;       \
    default: ;                                                  \
    }


int macroStackIndex=0;
static LexemStream macroInputStack[MACRO_INPUT_STACK_SIZE];

static Memory macroArgumentsMemory;

static void initMacroArgumentsMemory(void) {
    memoryInit(&macroArgumentsMemory, "macro arguments", NULL, MacroArgumentsMemorySize);
}

static void *mamAlloc(size_t size) {
    return memoryAlloc(&macroArgumentsMemory, size);
}


/* Macro body memory - MBM */

static Memory macroBodyMemory;

static void mbmInit(void) {
    memoryInit(&macroBodyMemory, "macro body", NULL, MacroBodyMemorySize);
}

static void *mbmAlloc(size_t size) {
    return memoryAlloc(&macroBodyMemory, size);
}

static void *mbmRealloc(void *pointer, size_t oldSize, size_t newSize) {
    return memoryRealloc(&macroBodyMemory, pointer, oldSize, newSize);
}

static void mbmFreeUntil(void *pointer) {
    memoryFreeUntil(&macroBodyMemory, pointer);
}

int getMacroBodyMemoryIndex(void) {
    return macroBodyMemory.index;
}

void setMacroBodyMemoryIndex(int index) {
    macroBodyMemory.index = index;
}


typedef struct {
    char *buffer;
    int size;
} LexemBufferDescriptor;

static bool isIdentifierLexem(LexemCode lexem) {
    return lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND  || lexem==IDENT_TO_COMPLETE;
}

static bool isConstantLexem(LexemCode lexem) {
    return lexem == CONSTANT || lexem == LONG_CONSTANT || lexem == FLOAT_CONSTANT || lexem == DOUBLE_CONSTANT;
}

static bool expandMacroCall(Symbol *macroSymbol, Position macroPosition);


/* ************************************************************ */

void initAllInputs(void) {
    mbmInit();
    includeStack.pointer=0;
    macroStackIndex=0;
    isProcessingPreprocessorIf = false;
    resetMacroArgumentTable();
    completionPositionFound = false;
    completionStringServed = false;
    completionStringInMacroBody = NULL;
    upLevelFunctionCompletionType = NULL;
    structMemberCompletionType = NULL;
}


static void setCurrentFileConsistency(FileDescriptor *file, LexemStream *input) {
    file->lexemBuffer.read = input->read;
}
static void setCurrentInputConsistency(LexemStream *input, FileDescriptor *file) {
    *input = makeLexemStream(file->lexemBuffer.lexemStream, file->lexemBuffer.read, file->lexemBuffer.write,
                          NULL, NORMAL_STREAM);
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
        name = getFileItemWithFileNumber(number)->name;
    } else {
        bool existed = existsInFileTable(fileName);
        number = addFileNameToFileTable(fileName);
        FileItem *fileItem = getFileItemWithFileNumber(number);
        name = fileItem->name;
        updateFileModificationTracking(number);
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

static void initInputFromEditorBuffer(EditorBuffer *editorBuffer, char *prefixString, char **bufferStartOut,
                                      int *bufferSizeOut, int *offsetOut) {
    int prefixLength = strlen(prefixString);
    // Reading from buffer, prepare prefix, does it fit?
    assert(prefixLength < editorBuffer->allocation.allocatedFreePrefixSize);
    // Use the editor buffer and placing the prefix infront of the existing text, so we
    // cannot copy the \0 from the end of the prefix
    memcpy(editorBuffer->allocation.text - prefixLength, prefixString, prefixLength);
    // Point to the start of the prefix in the reserved prefix area
    *bufferStartOut = editorBuffer->allocation.text - prefixLength;
    // and the size is the bufferSize + length of the prefix
    *bufferSizeOut = editorBuffer->allocation.bufferSize + prefixLength;
    *offsetOut     = editorBuffer->allocation.bufferSize;
    assert(*bufferStartOut > editorBuffer->allocation.allocatedBlock);
}

static void initInputFromFile(char *prefix, char **bufferStartOut, int *bufferSizeOut,
                              int *offsetOut) {
    int prefixLength = strlen(prefix);
    // Reading from file or just a prefix, does it fit?
    assert(prefixLength < CHARACTER_BUFFER_SIZE);
    // Ok, then just copy the prefix into the character buffer,
    // file reading will commence later
    strcpy(currentFile.characterBuffer.chars, prefix);
    *bufferStartOut   = currentFile.characterBuffer.chars;
    *bufferSizeOut    = prefixLength;
    *offsetOut        = 0;
}

/* ***************************************************************** */
/*                             Init Reading                          */
/* ***************************************************************** */

/* Init input from file, editor buffer and/or prefix */
void initInput(FILE *file, EditorBuffer *editorBuffer, char *prefixString, char *fileName) {
    char   *bufferStart;
    int     bufferSize, offset;

    /* This can be called from various context where one or both are
     * NULL, and we don't know which... */
    if (editorBuffer != NULL) {
        assert(file == NULL);
        initInputFromEditorBuffer(editorBuffer, prefixString, &bufferStart, &bufferSize, &offset);
    } else {
        assert(editorBuffer == NULL);
        initInputFromFile(prefixString, &bufferStart, &bufferSize, &offset);
    }

    fillFileDescriptor(&currentFile, fileName, bufferStart, bufferSize, file, offset);
    setCurrentFileInfoFor(fileName);
    setCurrentInputConsistency(&currentInput, &currentFile);

    isProcessingPreprocessorIf = false;
}

static void getAndSetOutPositionIfRequested(char **readPointerP, Position *outPositionP) {
    if (outPositionP != NULL)
        *outPositionP = getLexemPositionAt(readPointerP);
    else
        getLexemPositionAt(readPointerP);
}

static void getAndSetOutLengthIfRequested(char **readPointerP, int *outLengthP) {
    if (outLengthP != NULL)
        *outLengthP = getLexemIntAt(readPointerP);
    else
        getLexemIntAt(readPointerP);
}

static void getAndSetOutValueIfRequested(char **readPointerP, int *outValueP) {
    if (outValueP != NULL)
        *outValueP = getLexemIntAt(readPointerP);
    else
        getLexemIntAt(readPointerP);
}

/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

/* Depending on the passed Lexem will get or pass over subsequent lexems
 * like position, and advance the readPointerP. Allows any of the out
 * parameters to be NULL to ignore it. Might be too complex/general,
 * long argument list, when lex is known could be broken into parts
 * for specific lexem types. */
static void getExtraLexemInformationFor(LexemCode lexem, char **readPointerP, int *outLineNumber, int *outValue,
                                        Position *outPosition, int *outLength, bool countLines) {
    if (lexem > MULTI_TOKENS_START) {
        if (isIdentifierLexem(lexem) || lexem == STRING_LITERAL) {
            /* Both are followed by a string and a position */
            log_trace("Getting extra lexem information for LEXEM '%s'", *readPointerP);
            *readPointerP = strchr(*readPointerP, '\0') + 1;
            getAndSetOutPositionIfRequested(readPointerP, outPosition);
        } else if (lexem == LINE_TOKEN) {
            int noOfLines = getLexemIntAt(readPointerP);
            if (countLines) {
                traceNewline(noOfLines);
                currentFile.lineNumber += noOfLines;
            }
            if (outLineNumber != NULL)
                *outLineNumber = noOfLines;
        } else if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
            getAndSetOutValueIfRequested(readPointerP, outValue);
            getAndSetOutPositionIfRequested(readPointerP, outPosition);
            getAndSetOutLengthIfRequested(readPointerP, outLength);
        } else if (lexem == DOUBLE_CONSTANT || lexem == FLOAT_CONSTANT) {
            getAndSetOutPositionIfRequested(readPointerP, outPosition);
            getAndSetOutLengthIfRequested(readPointerP, outLength);
        } else if (lexem == CPP_MACRO_ARGUMENT) {
            getAndSetOutValueIfRequested(readPointerP, outValue);
            getAndSetOutPositionIfRequested(readPointerP, outPosition);
        } else if (lexem == CHAR_LITERAL) {
            getAndSetOutValueIfRequested(readPointerP, outValue);
            getAndSetOutPositionIfRequested(readPointerP, outPosition);
            getAndSetOutLengthIfRequested(readPointerP, outLength);
        }
    } else if (isPreprocessorToken(lexem)) {
        getAndSetOutPositionIfRequested(readPointerP, outPosition);
    } else if (lexem == '\n' && countLines) {
        getAndSetOutPositionIfRequested(readPointerP, outPosition);
        traceNewline(1);
        currentFile.lineNumber++;
    } else {
        getAndSetOutPositionIfRequested(readPointerP, outPosition);
    }
}

static void skipExtraLexemInformationWithCountLines(LexemCode lexem, char **readPointerP, bool countLines) {
    getExtraLexemInformationFor(lexem, readPointerP, NULL, NULL, NULL, NULL, countLines);
}

static bool insideMacro(void) {
    return macroStackIndex > 0;
}

static LexemCode refillInputIfEmpty(char **previousLexemP) {
    while (currentInput.read >= currentInput.write) {
        LexemStreamType inputType = currentInput.streamType;
        if (insideMacro()) {
            if (inputType == MACRO_ARGUMENT_STREAM) {
                return END_OF_MACRO_ARGUMENT_EXCEPTION;
            }
            mbmFreeUntil(currentInput.begin);
            currentInput = macroInputStack[--macroStackIndex];
        } else if (inputType == NORMAL_STREAM) {
            setCurrentFileConsistency(&currentFile, &currentInput);
            if (!buildLexemFromCharacters(&currentFile.characterBuffer, &currentFile.lexemBuffer)) {
                return END_OF_FILE_EXCEPTION;
            }
            setCurrentInputConsistency(&currentInput, &currentFile);
        }
        if (previousLexemP != NULL)
            *previousLexemP = currentInput.read;
    }
    return 0; /* Success */
}

/* Returns next lexem from currentInput and saves a pointer to the previous lexem */
static LexemCode getLexemAndSavePointerToPrevious(char **previousLexemP) {
    LexemCode lexem;

    lexem = refillInputIfEmpty(previousLexemP);
    if (lexem != 0)             /* END_OF_MACRO_ARGUMENT_EXCEPTION or END_OF_FILE_EXCEPTION */
        return lexem;

    if (previousLexemP != NULL)
        *previousLexemP = currentInput.read;
    lexem = getLexemCodeAndAdvance(&currentInput.read);
    if (options.lexemTrace)
        printf("LEXEM read: %s\n", lexemEnumNames[lexem]);
    log_trace("LEXEM read: %s", lexemEnumNames[lexem]);
    return lexem;
}

static LexemCode getLexem(void) {
    return getLexemAndSavePointerToPrevious(NULL);
}


/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

static void testCxrefCompletionId(LexemCode *out_lexem, char *id, Position position) {
    LexemCode lexem;

    lexem = *out_lexem;
    assert(options.mode);
    if (options.mode == ServerMode) {
        if (lexem==IDENT_TO_COMPLETE) {
            completionStringServed = true;
            if (currentLanguage == LANG_YACC) {
                makeYaccCompletions(id, strlen(id), position);
            } else {
                makeCCompletions(id, strlen(id), position);
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

    skipExtraLexemInformationWithCountLines(lexem, &currentInput.read, true);
    if (lexem != CONSTANT)
        return;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    skipExtraLexemInformationWithCountLines(lexem, &currentInput.read, true);
    return;

endOfMacroArgument:
    assert(0);
endOfFile:
    return;
}

/* ********************************* #INCLUDE ********************** */

static Symbol makeIncludeSymbolItem(Position pos) {
    Symbol symbol = makeSymbol(LINK_NAME_INCLUDE_REFS, TypeCppInclude, pos);
    return symbol;
}


void addFileAsIncludeReference(int fileNumber) {
    Position position = makePosition(fileNumber, 1, 0);
    Symbol symbol = makeIncludeSymbolItem(position);
    log_trace("adding reference on file %d==%s", fileNumber, getFileItemWithFileNumber(fileNumber)->name);
    addCxReference(&symbol, position, UsageDefined, fileNumber);
}

static void addIncludeReference(int fileNumber, Position position) {
    log_trace("adding reference on file %d==%s", fileNumber, getFileItemWithFileNumber(fileNumber)->name);
    Symbol symbol = makeIncludeSymbolItem(position);
    addCxReference(&symbol, position, UsageUsed, fileNumber);
}

static void addIncludeReferences(int fileNumber, Position position) {
    addIncludeReference(fileNumber, position);
    addFileAsIncludeReference(fileNumber);
}

void pushInclude(FILE *file, EditorBuffer *buffer, char *name, char *prepend) {
    setCurrentFileConsistency(&currentFile, &currentInput);
    includeStack.stack[includeStack.pointer++] = currentFile;		/* buffers are copied !!!!!!, burk */
    if (includeStack.pointer >= INCLUDE_STACK_SIZE) {
        FATAL_ERROR(ERR_ST,"too deep nesting in includes", XREF_EXIT_ERR);
    }
    initInput(file, buffer, prepend, name);
}

void popInclude(void) {
    FileItem *fileItem = getFileItemWithFileNumber(currentFile.characterBuffer.fileNumber);
    if (fileItem->cxLoading) {
        fileItem->cxLoaded = true;
    }
    closeCharacterBuffer(&currentFile.characterBuffer);
    if (includeStack.pointer != 0) {
        currentFile = includeStack.stack[--includeStack.pointer];	/* buffers are copied !!!!!!, burk */
        setCurrentInputConsistency(&currentInput, &currentFile);
    }
}

static bool openInclude(char includeType, char *name, bool is_include_next) {
    EditorBuffer *editorBuffer = NULL;
    FILE *file = NULL;
    StringList *includeDirP;
    char wildcardExpandedPaths[MAX_OPTION_LEN];
    char normalizedName[MAX_FILE_NAME_SIZE];
    char path[MAX_FILE_NAME_SIZE];

    log_trace("openInclude(%s)%s", name, is_include_next?" as include_next":"");

    extractPathInto(currentFile.fileName, path);

    StringList *start = options.includeDirs;

    if (is_include_next) {
        /* #include_next, (read this from the same directory) so find the include path which matches this */
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
        editorBuffer = findOrCreateAndLoadEditorBufferForFile(normalizedName);
        if (editorBuffer == NULL)
            file = openFile(normalizedName, "r");
    }

    /* If not found we need to walk the include paths... */
 search:
    for (includeDirP = start; includeDirP != NULL && editorBuffer == NULL && file == NULL; includeDirP = includeDirP->next) {
        strcpy(normalizedName, normalizeFileName_static(includeDirP->string, cwd));
        expandWildcardsInOnePath(normalizedName, wildcardExpandedPaths, MAX_OPTION_LEN);
        MAP_OVER_PATHS(wildcardExpandedPaths, {
            int length;
            strcpy(normalizedName, currentPath);
            length = strlen(normalizedName);
            if (length > 0 && normalizedName[length - 1] != FILE_PATH_SEPARATOR) {
                normalizedName[length] = FILE_PATH_SEPARATOR;
                length++;
            }
            strcpy(normalizedName + length, name);
            log_trace("trying to open '%s'", normalizedName);
            editorBuffer = findOrCreateAndLoadEditorBufferForFile(normalizedName);
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

static Symbol *findMacroSymbol(char *name) {
    Symbol symbol = makeMacroSymbol(name, noPosition);
    Symbol *memberP;
    if (symbolTableIsMember(symbolTable, &symbol, NULL, &memberP))
        return memberP;
    else
        return NULL;
}

static void processInclude2(Position includePosition, char includeType, char *includedName, bool is_include_next) {
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "PragmaOnce-%s", includedName);
    if (findMacroSymbol(tmpBuff) != NULL)
        return;

    if (!openInclude(includeType, includedName, is_include_next)) {
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
protected void processIncludeDirective(Position includePosition, bool is_include_next) {
    char *beginningOfLexem, *previousLexemP;
    LexemCode lexem;

    lexem = getLexemAndSavePointerToPrevious(&previousLexemP);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    beginningOfLexem = currentInput.read;
    if (lexem == STRING_LITERAL) {         /* A bracketed "<something>" is also stored as a string literal */
        skipExtraLexemInformationWithCountLines(lexem, &currentInput.read, true);
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

static void addMacroToTabs(Symbol *symbol, char *name) {
    int index;
    bool isMember;

    isMember = symbolTableIsMember(symbolTable, symbol, &index, NULL);
    if (isMember) {
        log_trace(": masking macro %s", name);
    } else {
        log_trace(": adding macro %s", name);
    }
    symbolTablePush(symbolTable, symbol, index);
}

static void setMacroArgumentName(MacroArgumentTableElement *arg, void *at) {
    char **argTab;
    argTab = (char**)at;
    assert(arg->order>=0);
    argTab[arg->order] = arg->name;
}

static void handleMacroDefinitionParameterPositions(int argi, Position macroPosition,
                                                    Position beginPosition,
                                                    Position position, Position endPosition,
                                                    bool final) {
    if ((options.serverOperation == OLO_GOTO_PARAM_NAME || options.serverOperation == OLO_GET_PARAM_COORDINATES)
        && positionsAreEqual(macroPosition, cxRefPosition)) {
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(beginPosition);
            } else if (argi < options.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(endPosition);
            }
        } else if (argi == options.olcxGotoVal) {
            parameterPosition = position;
            parameterBeginPosition = beginPosition;
            parameterEndPosition = endPosition;
        }
    }
}

static void handleMacroUsageParameterPositions(int argumentIndex, Position macroPosition,
                                               Position beginPosition, Position endPosition,
                                               bool final
) {
    if (options.serverOperation == OLO_GET_PARAM_COORDINATES
        && positionsAreEqual(macroPosition, cxRefPosition)) {
        log_trace("checking param %d at %d,%d, final==%d", argumentIndex, beginPosition.col, endPosition.col,
                  final);
        if (final) {
            if (argumentIndex==0) {
                setParamPositionForFunctionWithoutParams(beginPosition);
            } else if (argumentIndex < options.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(endPosition);
            }
        } else if (argumentIndex == options.olcxGotoVal) {
            parameterBeginPosition = beginPosition;
            parameterEndPosition = endPosition;
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


static LexemCode getNonBlankLexemAndData(Position *positionP, int *lineNumber, int *value, int *length) {
    LexemCode lexem;

    lexem = getLexem();
    while (lexem == LINE_TOKEN) {
        getExtraLexemInformationFor(lexem, &currentInput.read, lineNumber, value, positionP, length, true);
        lexem = getLexem();
    }
    return lexem;
}

static MacroArgumentTableElement *newMacroArgumentTableElement(char *argLinkName, int argumentCount, char *name) {
    MacroArgumentTableElement *element = mamAlloc(sizeof(MacroArgumentTableElement));
    *element = makeMacroArgumentTableElement(name, argLinkName, argumentCount);
    return element;
}

static char *allocateArgumentName(char *argumentName) {
    char *name;
    name = ppmAllocc(strlen(argumentName) + 1, sizeof(char));
    strcpy(name, argumentName);

    return name;
}

static char *allocateLinkName(char *argumentName, Position position) {
    char *argLinkName;
    char tmpBuff[TMP_BUFF_SIZE];
    sprintf(tmpBuff, "%x-%x%c%s", position.file, position.line, LINK_NAME_SEPARATOR, argumentName);
    argLinkName = ppmAllocc(strlen(tmpBuff) + 1, sizeof(char));
    strcpy(argLinkName, tmpBuff);

    return argLinkName;
}

static void swapPositions(Position **_parpos1, Position **_parpos2) {
    Position *parpos1 = *_parpos1, *parpos2 = *_parpos2;
    Position *tmppos = parpos1;
    parpos1 = parpos2;
    parpos2 = tmppos;
    *_parpos1 = parpos1;
    *_parpos2 = parpos2;
}

/* Public only for unittesting */
protected void processDefineDirective(bool hasArguments) {
    char **argumentNames;

    bool isReadingBody = false;
    char *macroName = NULL;
    int allocatedSize = 0;
    Symbol *symbol = NULL;

    initMacroArgumentsMemory();

    Position ppb1 = noPosition;
    Position ppb2 = noPosition;
    Position *parpos1 = &ppb1;
    Position *parpos2 = &ppb2;

    LexemCode lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    char *currentLexemStart = currentInput.read;

    Position macroPosition;
    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &macroPosition, NULL, true);

    testCxrefCompletionId(&lexem, currentLexemStart, macroPosition);    /* for cross-referencing */
    if (lexem != IDENTIFIER)
        return;

    symbol = ppmAlloc(sizeof(Symbol));
    *symbol = makeMacroSymbol(NULL, macroPosition);

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
                char *argumentName = currentLexemStart = currentInput.read;
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

                char *name = allocateArgumentName(argumentName);
                char *argLinkName = allocateLinkName(argumentName, position);

                MacroArgumentTableElement *macroArgumentTableElement = newMacroArgumentTableElement(argLinkName, argumentCount, name);
                int argumentIndex = addMacroArgument(macroArgumentTableElement);
                argumentCount++;

                lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
                ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

                swapPositions(&parpos1, &parpos2);

                getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, parpos2, NULL, true);
                if (!ellipsis) {
                    addTrivialCxReference(getMacroArgument(argumentIndex)->linkName, TypeMacroArg, StorageDefault,
                                          position, UsageDefined);
                    handleMacroDefinitionParameterPositions(argumentCount, macroPosition, *parpos1, position,
                                                            *parpos2, false);
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
            handleMacroDefinitionParameterPositions(argumentCount, macroPosition, *parpos1, noPosition, *parpos2,
                                                    true);
        } else {
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, parpos2, NULL, true);
            handleMacroDefinitionParameterPositions(argumentCount, macroPosition, *parpos1, noPosition, *parpos2,
                                                    true);
        }
    }

    /* process macro body */
    allocatedSize = MACRO_BODY_BUFFER_SIZE;
    int macroSize = 0;
    char *body = ppmAllocc(allocatedSize+MAX_LEXEM_SIZE, sizeof(char));

    isReadingBody = true;

    Position position;
    lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    currentLexemStart = currentInput.read;
    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);

    while (lexem != '\n') {
        while (macroSize<allocatedSize && lexem != '\n') {
            char *lexemDestination = body+macroSize; /* TODO WTF Are we storing the lexems after the body?!?! */
            /* Create a MATE to be able to run ..IsMember() */
            MacroArgumentTableElement macroArgumentTableElement = makeMacroArgumentTableElement(currentLexemStart,
                                                                                                NULL, 0);

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
                    completionPositionFound = true;
                    completionStringInMacroBody = symbol->linkName;
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
    symbol->mbody = macroBody;

    addMacroToTabs(symbol, macroName);
    assert(options.mode);
    addCxReference(symbol, macroPosition, UsageDefined, NO_FILE_NUMBER);
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
    char *ch;
    Position position;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    ch = currentInput.read;
    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
    testCxrefCompletionId(&lexem, ch, position);

    if (isIdentifierLexem(lexem)) {

        log_debug("Undefine macro %s", ch);
        Symbol *member;
        if ((member = findMacroSymbol(ch)) != NULL) {
            addCxReference(member, position, UsageUndefinedMacro, NO_FILE_NUMBER);

            Symbol *m = ppmAlloc(sizeof(Symbol));
            *m = makeMacroSymbol(member->linkName, position);
            addMacroToTabs(m, member->name);
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

static void genCppIfElseReference(int level, Position position, int usage) {
    char                ttt[TMP_STRING_SIZE];
    Position			dp;
    CppIfStack       *ss;
    if (level > 0) {
        ss = ppmAlloc(sizeof(CppIfStack));
        ss->position = position;
        ss->next = currentFile.ifStack;
        currentFile.ifStack = ss;
    }
    if (currentFile.ifStack!=NULL) {
        dp = currentFile.ifStack->position;
        sprintf(ttt,"CppIf%x-%x-%d", dp.file, dp.col, dp.line);
        addTrivialCxReference(ttt, TypeCppIfElse, StorageDefault, position, usage);
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
            genCppIfElseReference(1, position, UsageDefined);
            depth++;
        } else if (lexem == CPP_ENDIF) {
            depth--;
            genCppIfElseReference(-1, position, UsageUsed);
        } else if (lexem == CPP_ELIF) {
            genCppIfElseReference(0, position, UsageUsed);
            if (depth == 1 && !untilEnd) {
                log_debug("#elif ");
                return UNTIL_ELIF;
            }
        } else if (lexem == CPP_ELSE) {
            genCppIfElseReference(0, position, UsageUsed);
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
    char *ch;
    Position position;
    bool deleteSrc;

    lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    /* Then we are probably looking at the id... */
    ch = currentInput.read;
    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
    testCxrefCompletionId(&lexem, ch, position);

    if (!isIdentifierLexem(lexem))
        return;

    Symbol *macroSymbol = findMacroSymbol(ch);
    bool macroDefined = macroSymbol != NULL;

    if (macroSymbol != NULL && macroSymbol->mbody==NULL)
        macroDefined = false;	// undefined macro

    if (macroDefined) {
        addCxReference(macroSymbol, position, UsageUsed, NO_FILE_NUMBER);
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

    LexemCode lexem = yylex();
    if (isIdentifierLexem(lexem)) {
        // this is useless, as it would be set to 0 anyway
        lexem = cexpTranslateToken(CONSTANT, 0);
    } else if (lexem == CPP_DEFINED_OP) {
        Position position;
        lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        char *ch = currentInput.read;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);

        bool haveParenthesis;
        if (lexem == '(') {
            haveParenthesis = true;

            lexem = getNonBlankLexemAndData(&position, NULL, NULL, NULL);
            ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

            ch = currentInput.read;
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        } else {
            haveParenthesis = false;
        }

        if (!isIdentifierLexem(lexem))
            return 0;

        Symbol *macroSymbol = findMacroSymbol(ch);
        bool macroSymbolFound = macroSymbol != NULL;
        if (macroSymbol != NULL && macroSymbol->mbody == NULL)
            macroSymbolFound = false;   // undefined macro

        if (macroSymbolFound)
            addCxReference(macroSymbol, position, UsageUsed, NO_FILE_NUMBER);

        /* following call sets uniyylval */
        LexemCode res = cexpTranslateToken(CONSTANT, macroSymbolFound);
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

    LexemCode lexem = getLexem();
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    Position position;
    if (lexem == IDENTIFIER && strcmp(currentInput.read, "once")==0) {
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, true);
        char *fileName = simpleFileName(getFileItemWithFileNumber(position.file)->name);

        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "PragmaOnce-%s", fileName);
        char *mname = ppmAllocc(strlen(tmpBuff)+1, sizeof(char));
        strcpy(mname, tmpBuff);

        Symbol *symbol = ppmAlloc(sizeof(Symbol));
        *symbol = makeMacroSymbol(mname, position);
        symbolTableAdd(symbolTable, symbol);
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
        processIncludeDirective(position, false);
        break;
    case CPP_INCLUDE_NEXT:
        processIncludeDirective(position, true);
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
        genCppIfElseReference(1, position, UsageDefined);
        processIfdefDirective(true);
        break;
    case CPP_IFNDEF:
        genCppIfElseReference(1, position, UsageDefined);
        processIfdefDirective(false);
        break;
    case CPP_IF:
        genCppIfElseReference(1, position, UsageDefined);
        processIfDirective();
        break;
    case CPP_ELIF:
        log_debug("#elif");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, position, UsageUsed);
            currentFile.ifDepth --;
            cppDeleteUntilEndElse(true);
        } else if (options.mode!=ServerMode) {
            warningMessage(ERR_ST,"unmatched #elif");
        }
        break;
    case CPP_ELSE:
        log_debug("#else");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, position, UsageUsed);
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
            genCppIfElseReference(-1, position, UsageUsed);
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

// Will always keep the buffer an extra MAX_LEXEM_SIZE in size
static void expandPreprocessorBufferIfOverflow(LexemBufferDescriptor *desc, char *pointer) {
    if (pointer >= desc->buffer + desc->size) {
        desc->size += MACRO_BODY_BUFFER_SIZE;
        ppmReallocc(desc->buffer, desc->size+MAX_LEXEM_SIZE, sizeof(char),
                    desc->size+MAX_LEXEM_SIZE-MACRO_BODY_BUFFER_SIZE);
    }
}

// Will always keep the buffer an extra MAX_LEXEM_SIZE in size
static void expandMacroBodyBufferIfOverflow(LexemBufferDescriptor *desc, char *writePosition, int requiredSpace) {
    while (writePosition + requiredSpace >= desc->buffer + desc->size) {
        desc->size += MACRO_BODY_BUFFER_SIZE;
        mbmRealloc(desc->buffer, desc->size+MAX_LEXEM_SIZE-MACRO_BODY_BUFFER_SIZE, desc->size+MAX_LEXEM_SIZE);
    }
}

/* *********************************************************** */

static bool cyclicCall(MacroBody *macroBody) {
    char *name = macroBody->name;
    log_debug("Testing for cyclic call, '%s' against current '%s'", name, currentInput.macroName);
    if (currentInput.macroName != NULL && strcmp(name,currentInput.macroName)==0)
        return true;
    for (int i=0; i<macroStackIndex; i++) {
        LexemStream *lexInput = &macroInputStack[i];
        log_debug("Testing '%s' against '%s'", name, lexInput->macroName);
        if (lexInput->macroName != NULL && strcmp(name,lexInput->macroName)==0)
            return true;
    }
    return false;
}


static void prependMacroInput(LexemStream *argumentBuffer) {
    assert(macroStackIndex < MACRO_INPUT_STACK_SIZE-1);
    macroInputStack[macroStackIndex++] = currentInput;
    currentInput = *argumentBuffer;
    currentInput.read = currentInput.begin;
    currentInput.streamType = MACRO_STREAM;
}


static void expandMacroArgument(LexemStream *argumentInput) {
    char *currentBufferP;
    LexemBufferDescriptor bufferDesc;

    ENTER();

    bufferDesc.size = MACRO_BODY_BUFFER_SIZE;

    prependMacroInput(argumentInput);

    currentInput.streamType = MACRO_ARGUMENT_STREAM;
    bufferDesc.buffer = ppmAllocc(bufferDesc.size+MAX_LEXEM_SIZE, sizeof(char));
    currentBufferP = bufferDesc.buffer;

    for(;;) {
        char *previousLexem;
        LexemCode lexem = getLexemAndSavePointerToPrevious(&previousLexem);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        char *nextLexemP = currentInput.read;

        Position position;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL, !insideMacro());
        int length = ((char*)currentInput.read) - previousLexem;
        assert(length >= 0);
        memcpy(currentBufferP, previousLexem, length);

        // a hack, it is copied, but currentBufferP will be increased
        // only if not an expanding macro, this is because
        // 'macroCallExpand' can read new lexbuffer and destroy
        // cInput, so copy it now.
        if (lexem == IDENTIFIER) {
            Symbol *macroSymbol = findMacroSymbol(nextLexemP);
            if (macroSymbol != NULL) {
                /* it is a macro, provide macro expansion */
                log_trace("Macro found: '%s' (argument) -> Should be expanded", nextLexemP);
                if (expandMacroCall(macroSymbol, position))
                    continue; // with next lexem
                else {
                    /* Failed expansion... */
                    assert(macroSymbol!=NULL);
                    if (macroSymbol->mbody!=NULL && cyclicCall(macroSymbol->mbody)) {
                        putLexemCodeAt(IDENT_NO_CPP_EXPAND, &currentBufferP);
                    }
                }
            }
        }
        currentBufferP += length;
        expandPreprocessorBufferIfOverflow(&bufferDesc, currentBufferP);
    }
endOfMacroArgument:
    currentInput = macroInputStack[--macroStackIndex];
    ppmReallocc(bufferDesc.buffer, currentBufferP-bufferDesc.buffer, sizeof(char), bufferDesc.size+MAX_LEXEM_SIZE);
    *argumentInput = makeLexemStream(bufferDesc.buffer, bufferDesc.buffer, currentBufferP, NULL, NORMAL_STREAM);
    LEAVE();
    return;
endOfFile:
    assert(0);
}

static void cxAddCollateReference(char *sym, char *cs, Position position) {
    char tempString[TMP_STRING_SIZE];
    strcpy(tempString,sym);
    assert(cs>=sym && cs-sym<TMP_STRING_SIZE);
    sprintf(tempString+(cs-sym), "%c%c%s", LINK_NAME_COLLATE_SYMBOL,
            LINK_NAME_COLLATE_SYMBOL, cs);
    addTrivialCxReference(tempString, TypeCppCollate, StorageDefault, position, UsageDefined);
}

static char *resolveMacroArgument(char **nextLexemP, LexemStream *actualArgumentsInput) {
    LexemCode lexem = getLexemCodeAndAdvance(nextLexemP);
    log_trace("Lexem = '%s'", lexemEnumNames[lexem]);
    int argumentIndex;
    getExtraLexemInformationFor(lexem, nextLexemP, NULL, &argumentIndex, NULL, NULL, false);
    *nextLexemP = actualArgumentsInput[argumentIndex].begin;
    return actualArgumentsInput[argumentIndex].write;
}

static char *resolveRegularOperand(char **nextLexemP) {
    char *p = *nextLexemP;
    LexemCode lexem = getLexemCodeAndAdvance(&p);
     skipExtraLexemInformationFor(lexem, &p);
    return p;
}

static void copyRemainingLexems(LexemBufferDescriptor *bufferDesc, LexemStream *inputStream,
                                LexemStream *outputStream) {
    while (lexemStreamHasMore(inputStream)) {
        copyNextLexemFromStreamToStream(inputStream, outputStream);
        expandPreprocessorBufferIfOverflow(bufferDesc, outputStream->write);
    }
}

static char *resolveMacroArgumentAsLeftOperand(LexemBufferDescriptor *bufferDesc, char **bufferWriteP,
                                              char **nextLexemP, LexemStream *actualArgumentsInput) {
    *bufferWriteP = *nextLexemP;

    LexemCode lexem = getLexemCodeAndAdvance(nextLexemP);

    int argumentIndex;
    getExtraLexemInformationFor(lexem, nextLexemP, NULL, &argumentIndex, NULL, NULL, false);

    LexemStream inputStream = makeLexemStream(actualArgumentsInput[argumentIndex].begin,
                                              actualArgumentsInput[argumentIndex].begin,
                                              actualArgumentsInput[argumentIndex].write,
                                              NULL, NORMAL_STREAM);
    LexemStream outputStream = makeLexemStream(bufferDesc->buffer, *bufferWriteP, *bufferWriteP, NULL, NORMAL_STREAM);

    char *lastLexemP = NULL;
    while (lexemStreamHasMore(&inputStream)) {
        lastLexemP = outputStream.write;
        copyNextLexemFromStreamToStream(&inputStream, &outputStream);
        expandPreprocessorBufferIfOverflow(bufferDesc, outputStream.write);
    }
    *bufferWriteP = outputStream.write;
    return lastLexemP;
}

static bool nextLexemIsIdentifierOrConstant(char *nextInputLexemP) {
    LexemCode nextLexem = peekLexemCodeAt(nextInputLexemP);
    return isIdentifierLexem(nextLexem) || isConstantLexem(nextLexem);
}

static MacroBody *getMacroBody(Symbol *macroSymbol) {
    assert(macroSymbol->type == TypeMacro);
    return macroSymbol->mbody;
}

static LexemStream createMacroBodyAsNewStream(MacroBody *macroBody, LexemStream *actualArgumentsInput);

static void expandMacroInCollation(LexemBufferDescriptor *bufferDesc, LexemStream *outputStream,
                                   Symbol *macroSymbol, LexemStream *actualArgumentsInput) {
    MacroBody *macroBody = getMacroBody(macroSymbol);
    LexemStream macroExpansion = createMacroBodyAsNewStream(macroBody, actualArgumentsInput);

    LexemStream inputStream = makeLexemStream(macroExpansion.begin, macroExpansion.begin, macroExpansion.write,
                                              NULL, NORMAL_STREAM);
    copyRemainingLexems(bufferDesc, &inputStream, outputStream);
}

/* **************************************************************** */
/* Token pasting handlers for different lexem type combinations */

/* Lexem type classification for pattern matching */
typedef enum {
    LEX_ID       = 0x01,
    LEX_CONST    = 0x02,
    LEX_OTHER    = 0x04
} LexemTypeFlag;

#define PAIR(left, right) ((left << 4) | right)

static LexemTypeFlag classify_lexem(LexemCode code) {
    if (code == IDENTIFIER) return LEX_ID;
    if (code == CONSTANT || code == LONG_CONSTANT) return LEX_CONST;
    return LEX_OTHER;
}

/* Collate IDENTIFIER ## IDENTIFIER -> concatenated identifier */
static void collate_id_id(char **writeBufferWriteP, char *lhs, char **rhsP) {
    char *leftHandLexemString = lhs + LEXEMCODE_SIZE;
    *writeBufferWriteP = leftHandLexemString + strlen(leftHandLexemString);
    assert(**writeBufferWriteP == 0); /* Ensure at end of string */

    char *rhs = *rhsP;
    LexemCode rightHandLexem = getLexemCodeAndAdvance(&rhs);
    char *rightHandLexemString = rhs; /* For an ID the string follows, then extra info */
    Position position;
    getExtraLexemInformationFor(rightHandLexem, &rhs, NULL, NULL, &position, NULL, false);

    memmove(*writeBufferWriteP, rightHandLexemString, strlen(rightHandLexemString) + 1);

    position.col--;
    assert(position.col >= 0);
    cxAddCollateReference(leftHandLexemString, *writeBufferWriteP, position);
    position.col++;

    *writeBufferWriteP += strlen(*writeBufferWriteP);
    assert(**writeBufferWriteP == 0);
    (*writeBufferWriteP)++;
    putLexemPositionAt(position, writeBufferWriteP);

    *rhsP = rhs; /* Update rhs position after consuming */
}

/* Collate IDENTIFIER ## CONSTANT -> identifier with constant appended */
static void collate_id_const(char **writeBufferWriteP, char *lhs, char **rhsP) {
    char *leftHandLexemString = lhs + LEXEMCODE_SIZE;
    *writeBufferWriteP = leftHandLexemString + strlen(leftHandLexemString);
    assert(**writeBufferWriteP == 0); /* Ensure at end of string */

    char *rhs = *rhsP;
    LexemCode rightHandLexem = getLexemCodeAndAdvance(&rhs);
    int value;
    getExtraLexemInformationFor(rightHandLexem, &rhs, NULL, &value, NULL, NULL, false);

    /* Get position from lefthand id, immediately after its string */
    Position position = peekLexemPositionAt(*writeBufferWriteP + 1);

    sprintf(*writeBufferWriteP, "%d", value); /* Concat the value to the end of the LHS ID */
    cxAddCollateReference(leftHandLexemString, *writeBufferWriteP, position);

    *writeBufferWriteP += strlen(*writeBufferWriteP);
    assert(**writeBufferWriteP == 0);
    (*writeBufferWriteP)++;
    putLexemPositionAt(position, writeBufferWriteP);

    *rhsP = rhs; /* Update rhs position after consuming */
}

/* Collate CONSTANT ## IDENTIFIER -> identifier with constant prepended */
static void collate_const_id(char **writeBufferWriteP, char **lhsP, char **rhsP, LexemCode leftHandLexem) {
    char *lhs = *lhsP;
    /* Retrieve value and position from the LHS CONSTANT */
    char *leftHandLexemStart = lhs;

    int value;
    Position position;
    getLexemCodeAndAdvance(&lhs);
    getExtraLexemInformationFor(leftHandLexem, &lhs, NULL, &value, &position, NULL, false);

    /* Re-write to an IDENTIFIER */
    putLexemCodeAt(IDENTIFIER, &leftHandLexemStart);
    *writeBufferWriteP = leftHandLexemStart; /* We want to write the id next */

    char *rhs = *rhsP;
    LexemCode lexem = getLexemCodeAndAdvance(&rhs);
    char *rightHandLexemString = rhs; /* For an ID the string follows, then the position */
    skipExtraLexemInformationFor(lexem, &rhs);

    /* Calculate where RHS starts in the concatenated result */
    char valueStr[32];
    sprintf(valueStr, "%d", value);
    int leftPartLength = strlen(valueStr);

    sprintf(*writeBufferWriteP, "%d%s", value, rightHandLexemString);

    assert(position.col >= 0);
    cxAddCollateReference(*writeBufferWriteP, *writeBufferWriteP + leftPartLength, position);
    position.col++;

    *writeBufferWriteP += strlen(*writeBufferWriteP);
    assert(**writeBufferWriteP == 0);
    (*writeBufferWriteP)++;

    putLexemPositionAt(position, writeBufferWriteP);

    *rhsP = rhs; /* Update rhs position after consuming */
}

/* Collate CONSTANT ## CONSTANT (or CONSTANT ## ID) -> complex concatenation */
static void collate_const_const(char **writeBufferWriteP, char *lhs, char **rhsP) {
    char *rhs = *rhsP;
    if (nextLexemIsIdentifierOrConstant(rhs)) {
        /* Get left constant value */
        char *leftHandLexemStart = lhs;
        int leftValue;
        Position leftPosition;
        LexemCode leftHandLexem = getLexemCodeAndAdvance(&lhs);
        getExtraLexemInformationFor(leftHandLexem, &lhs, NULL, &leftValue, &leftPosition, NULL, false);

        /* Re-write as IDENTIFIER at the start position */
        putLexemCodeAt(IDENTIFIER, &leftHandLexemStart);
        *writeBufferWriteP = leftHandLexemStart;

        /* Get right operand */
        LexemCode rightHandLexem = getLexemCodeAndAdvance(&rhs);
        int rightValue;
        Position rightPosition;

        if (isIdentifierLexem(rightHandLexem)) {
            /* CONST ## ID: concatenate "leftValue" + "idString" */
            char *rightHandLexemString = rhs;
            getExtraLexemInformationFor(rightHandLexem, &rhs, NULL, NULL, &rightPosition, NULL, false);
            /* Calculate where RHS starts */
            char leftStr[32];
            sprintf(leftStr, "%d", leftValue);
            int leftPartLength = strlen(leftStr);

            sprintf(*writeBufferWriteP, "%d%s", leftValue, rightHandLexemString);
            cxAddCollateReference(*writeBufferWriteP, *writeBufferWriteP + leftPartLength, leftPosition);
        } else /* isConstantLexem() */ {
            /* CONST ## CONST: concatenate "leftValue" + "rightValue" as strings */
            getExtraLexemInformationFor(rightHandLexem, &rhs, NULL, &rightValue, &rightPosition, NULL, false);
            /* Calculate where RHS starts */
            char leftStr[32];
            sprintf(leftStr, "%d", leftValue);
            int leftPartLength = strlen(leftStr);

            sprintf(*writeBufferWriteP, "%d%d", leftValue, rightValue);
            /* Use left position for reference tracking */
            cxAddCollateReference(*writeBufferWriteP, *writeBufferWriteP + leftPartLength, leftPosition);
        }

        *writeBufferWriteP += strlen(*writeBufferWriteP);
        assert(**writeBufferWriteP == 0);
        (*writeBufferWriteP)++;
        putLexemPositionAt(leftPosition, writeBufferWriteP);
    }
    *rhsP = rhs; /* Update rhs position after consuming */
}

/* **************************************************************** */
// Returns updated position to continue reading from
static char *collate(LexemBufferDescriptor *writeBufferDesc, // Buffer descriptor for storing macro expansions
                     char **writeBufferWriteP, // Current position in write buffer (where we are writing)
                     char **leftHandLexemP, // Left-hand lexem already in the write buffer (token before ##)
                     char **rightHandLexemP, // Right-hand lexem to be processed (token after ##)
                     LexemStream *actualArgumentsInput // The argument values for the current macro expansion
) {
    ENTER();
    char *ppmMarker = ppmAllocc(0, sizeof(char));

    /* Macro argument resolution first for left ... */
    if (peekLexemCodeAt(*leftHandLexemP) == CPP_MACRO_ARGUMENT) {
        *leftHandLexemP = resolveMacroArgumentAsLeftOperand(writeBufferDesc, writeBufferWriteP,
                                                            leftHandLexemP, actualArgumentsInput);
        if (*leftHandLexemP == NULL) {
            log_warn("Token pasting skipped: Left operand is NULL after expansion.");
            LEAVE();
            return *rightHandLexemP;
        }
    }
    assert(*leftHandLexemP != NULL);

    /* Check for empty right operand */
    if (rightHandLexemP == NULL || *rightHandLexemP == NULL) {
        log_trace("Token pasting with empty right operand - using left operand as-is");
        LEAVE();
        return *rightHandLexemP; // or could be NULL
    }

    /* ... and the right hand operand */
    /* Save the position to continue reading from in the macro body */
    char *continueReadingFrom;
    char *endOfLexems = NULL;
    if (peekLexemCodeAt(*rightHandLexemP) == CPP_MACRO_ARGUMENT) {
        /* Save position before resolveMacroArgument overwrites *rightHandLexemP */
        char *argStart = *rightHandLexemP;
        endOfLexems = resolveMacroArgument(rightHandLexemP, actualArgumentsInput);
        /* Skip past the CPP_MACRO_ARGUMENT token and its extra info in the macro body */
        getLexemCodeAndAdvance(&argStart);
        int argumentIndex;
        getExtraLexemInformationFor(CPP_MACRO_ARGUMENT, &argStart, NULL, &argumentIndex, NULL, NULL, false);
        continueReadingFrom = argStart;
    } else {
        endOfLexems = resolveRegularOperand(rightHandLexemP);
        /* For non-macro arguments, continue reading from after the consumed lexem */
        continueReadingFrom = endOfLexems;
    }

    /* Check for empty right operand after possible expansion, or END_OF marker */
    bool rhsIsEmpty = (rightHandLexemP == NULL || *rightHandLexemP >= endOfLexems);
    bool rhsIsEndOfMarker = false;
    if (!rhsIsEmpty) {
        LexemCode rhsLexemCode = peekLexemCodeAt(*rightHandLexemP);
        rhsIsEndOfMarker = (rhsLexemCode == END_OF_FILE_EXCEPTION || rhsLexemCode == END_OF_MACRO_ARGUMENT_EXCEPTION);
    }

    if (rhsIsEmpty || rhsIsEndOfMarker) {
        /* GNU extension: delete comma if pasting with empty __VA_ARGS__ */
        if (peekLexemCodeAt(*leftHandLexemP) == COMMA) {
            log_trace("Token pasting: deleting comma before empty __VA_ARGS__");
            /* Don't write the comma - rewind write pointer to before it */
            *writeBufferWriteP = *leftHandLexemP;
        } else {
            log_trace("Token pasting with empty right operand - using left operand as-is");
        }
        LEAVE();
        return continueReadingFrom;
    }

    /* Macro invocations next */
    char *lhs = *leftHandLexemP;
    if (isIdentifierLexem(peekLexemCodeAt(lhs))) {
        char *lexemString = lhs + LEXEMCODE_SIZE;
        Symbol *macroSymbol = findMacroSymbol(lexemString);
        if (macroSymbol != NULL) {
            log_trace("Macro found: '%s' (left-hand) -> expanding it", lexemString);
            *writeBufferWriteP = lhs; /* We should write where current LHS, the macro, starts */
            LexemStream outputStream = makeLexemStream(writeBufferDesc->buffer, *writeBufferWriteP, *writeBufferWriteP,
                                                       NULL, NORMAL_STREAM);
            expandMacroInCollation(writeBufferDesc, &outputStream, macroSymbol, actualArgumentsInput);
            *writeBufferWriteP = outputStream.write;
        } else {
            log_trace("Identifier '%s' (left-hand) is NOT a macro", lexemString);
        }
    }

    char *rhs = *rightHandLexemP;
    if (isIdentifierLexem(peekLexemCodeAt(rhs))) {
        char *lexemString = rhs + LEXEMCODE_SIZE;
        Symbol *macroSymbol = findMacroSymbol(lexemString);
        if (macroSymbol != NULL) {
            log_trace("Macro found: '%s' (right-hand) -> expanding it", lexemString);
            rhs = *writeBufferWriteP; /* RHS now starts where we will write now */
            LexemStream outputStream = makeLexemStream(writeBufferDesc->buffer, *writeBufferWriteP, *writeBufferWriteP,
                                                       NULL, NORMAL_STREAM);
            expandMacroInCollation(writeBufferDesc, &outputStream, macroSymbol, actualArgumentsInput);
            *writeBufferWriteP = outputStream.write;
            endOfLexems = *writeBufferWriteP; /* End is now where we have just finished writing */
        } else {
            log_trace("Identifier '%s' (right-hand) is NOT a macro", lexemString);
        }
    }

    /* Now collate left and right hand tokens using pattern matching */
    LexemCode leftHandLexem = peekLexemCodeAt(lhs);
    LexemCode rightHandLexem = peekLexemCodeAt(rhs);

    if (rhs < endOfLexems) {
        LexemTypeFlag leftType = classify_lexem(leftHandLexem);
        LexemTypeFlag rightType = classify_lexem(rightHandLexem);
        int pattern = PAIR(leftType, rightType);

        switch (pattern) {
            case PAIR(LEX_ID, LEX_ID):
                collate_id_id(writeBufferWriteP, lhs, &rhs);
                break;

            case PAIR(LEX_ID, LEX_CONST):
                collate_id_const(writeBufferWriteP, lhs, &rhs);
                break;

            case PAIR(LEX_CONST, LEX_ID):
                collate_const_id(writeBufferWriteP, &lhs, &rhs, leftHandLexem);
                break;

            case PAIR(LEX_CONST, LEX_CONST):
                collate_const_const(writeBufferWriteP, lhs, &rhs);
                break;

            default:
                /* Unhandled token pasting combination */
                /* Note: COMMA cases handled earlier via GNU extension (comma deletion with empty __VA_ARGS__) */
                log_warn("Unhandled token pasting: %s ## %s",
                         lexemEnumNames[leftHandLexem], lexemEnumNames[rightHandLexem]);
                break;
        }
    }
    expandPreprocessorBufferIfOverflow(writeBufferDesc, *writeBufferWriteP);

    /* rhsLexem have moved over all tokens used in the collation and now points to any trailing ones */
    LexemStream inputStream = makeLexemStream(rhs, rhs, endOfLexems, NULL, NORMAL_STREAM);
    LexemStream outputStream = makeLexemStream(writeBufferDesc->buffer, *writeBufferWriteP, *writeBufferWriteP, NULL, NORMAL_STREAM);
    copyRemainingLexems(writeBufferDesc, &inputStream, &outputStream);
    *writeBufferWriteP = outputStream.write;

    ppmFreeUntil(ppmMarker); // Free any allocations done
    LEAVE();
    /* Return the position in the macro body to continue reading from, not from the argument buffer */
    return continueReadingFrom;
}

static void macroArgumentsToString(char *writeBuffer, LexemStream *lexInput) {
    char *lexemReadP, *lexemContentP, *stringWriteP;
    int value;

    stringWriteP = writeBuffer;
    *stringWriteP = 0;
    lexemReadP = lexInput->begin;
    while (lexemReadP < lexInput->write) {
        LexemCode lexem = getLexemCodeAndAdvance(&lexemReadP);
        lexemContentP = lexemReadP;
        getExtraLexemInformationFor(lexem, &lexemReadP, NULL, &value, NULL, NULL, false);
        if (isIdentifierLexem(lexem)) {
            sprintf(stringWriteP, "%s", lexemContentP);
            stringWriteP += strlen(stringWriteP);
        } else if (lexem==STRING_LITERAL) {
            sprintf(stringWriteP,"\"%s\"", lexemContentP);
            stringWriteP += strlen(stringWriteP);
        } else if (lexem==CONSTANT) {
            sprintf(stringWriteP,"%d", value);
            stringWriteP += strlen(stringWriteP);
        } else if (lexem < 256) {
            sprintf(stringWriteP,"%c",lexem);
            stringWriteP += strlen(stringWriteP);
        }
    }
}

static LexemStream replaceMacroArguments(LexemStream *actualArgumentsInput, char *readBuffer, char *readEnd) {
    LexemBufferDescriptor bufferDesc;
    bufferDesc.size = MACRO_BODY_BUFFER_SIZE;
    bufferDesc.buffer = mbmAlloc(bufferDesc.size + MAX_LEXEM_SIZE);

    LexemStream inputStream = makeLexemStream(readBuffer, readBuffer, readEnd, NULL, NORMAL_STREAM);
    LexemStream outputStream = makeLexemStream(bufferDesc.buffer, bufferDesc.buffer, bufferDesc.buffer, NULL, NORMAL_STREAM);

    while (lexemStreamHasMore(&inputStream)) {
        char *lexemStart = inputStream.read;
        LexemCode lexem = getLexemCodeAndAdvance(&inputStream.read);

        int argumentIndex;
        Position position;
        getExtraLexemInformationFor(lexem, &inputStream.read, NULL, &argumentIndex, &position, NULL, false);

        if (lexem == CPP_MACRO_ARGUMENT) {
            int len = actualArgumentsInput[argumentIndex].write - actualArgumentsInput[argumentIndex].begin;
            expandMacroBodyBufferIfOverflow(&bufferDesc, outputStream.write, len);
            memcpy(outputStream.write, actualArgumentsInput[argumentIndex].begin, len);
            outputStream.write += len;
        } else if (lexem == '#' && lexemStreamHasMore(&inputStream)
                   && peekLexemCodeAt(inputStream.read) == CPP_MACRO_ARGUMENT) {
            lexem = getLexemCodeAndAdvance(&inputStream.read);
            assert(lexem == CPP_MACRO_ARGUMENT);
            getExtraLexemInformationFor(lexem, &inputStream.read, NULL, &argumentIndex, NULL, NULL, false);

            putLexemCodeAt(STRING_LITERAL, &outputStream.write);
            expandMacroBodyBufferIfOverflow(&bufferDesc, outputStream.write, MACRO_BODY_BUFFER_SIZE);

            macroArgumentsToString(outputStream.write, &actualArgumentsInput[argumentIndex]);
            /* Skip over the string... */
            int len = strlen(outputStream.write) + 1;
            outputStream.write += len;

            /* TODO: This should really be putLexPosition() but that takes a LexemBuffer... */
            putLexemPositionAt(position, &outputStream.write);
            if (len >= MACRO_BODY_BUFFER_SIZE - 15) {
                char tmpBuffer[TMP_BUFF_SIZE];
                sprintf(tmpBuffer, "size of #macro_argument exceeded MACRO_BODY_BUFFER_SIZE @%s:%d:%d",
                        getFileItemWithFileNumber(position.file)->name, position.line, position.col);
                errorMessage(ERR_INTERNAL, tmpBuffer);
            }
        } else {
            expandMacroBodyBufferIfOverflow(&bufferDesc, outputStream.write, (lexemStart - inputStream.read));
            assert(inputStream.read >= lexemStart);
            int len = inputStream.read - lexemStart;
            memcpy(outputStream.write, lexemStart, len);
            outputStream.write += len;
        }
        expandMacroBodyBufferIfOverflow(&bufferDesc, outputStream.write, 0);
    }
    mbmRealloc(bufferDesc.buffer, bufferDesc.size + MAX_LEXEM_SIZE, outputStream.write - bufferDesc.buffer);

    return makeLexemStream(bufferDesc.buffer, bufferDesc.buffer, outputStream.write, NULL, NORMAL_STREAM);
}

/* ************************************************************** */
/* ********************* macro body replacement ***************** */
/* ************************************************************** */

static LexemStream createMacroBodyAsNewStream(MacroBody *macroBody, LexemStream *actualArgumentsInput) {
    // Allocate space for an extra lexem so that users can overwrite and *then* expand
    LexemBufferDescriptor bufferDesc;
    bufferDesc.size = MACRO_BODY_BUFFER_SIZE;
    bufferDesc.buffer = ppmAllocc(bufferDesc.size + MAX_LEXEM_SIZE, sizeof(char));
    char *bufferWrite  = bufferDesc.buffer;

    char *lastWrittenLexem = NULL;

    char *nextBodyLexemToRead = macroBody->body;
    char *endOfBodyLexems = macroBody->body + macroBody->size;

    while (nextBodyLexemToRead != NULL && nextBodyLexemToRead < endOfBodyLexems) {
        char *lexemStart   = nextBodyLexemToRead;

        LexemCode lexem = getLexemCodeAndAdvance(&nextBodyLexemToRead);
         skipExtraLexemInformationFor(lexem, &nextBodyLexemToRead);

        /* first make ## collations, if any */
        if (lexem == CPP_COLLATION && lastWrittenLexem != NULL && nextBodyLexemToRead < endOfBodyLexems) {
            nextBodyLexemToRead = collate(&bufferDesc, &bufferWrite, &lastWrittenLexem,
                                           &nextBodyLexemToRead, actualArgumentsInput);
        } else {
            lastWrittenLexem = bufferWrite;
            assert(nextBodyLexemToRead >= lexemStart);
            /* Copy this lexem over from body to buffer */
            int lexemLength = nextBodyLexemToRead - lexemStart;
            memcpy(bufferWrite, lexemStart, lexemLength);
            bufferWrite += lexemLength;
        }
        expandPreprocessorBufferIfOverflow(&bufferDesc, bufferWrite);
    }
    ppmReallocc(bufferDesc.buffer, bufferWrite - bufferDesc.buffer, sizeof(char), bufferDesc.size + MAX_LEXEM_SIZE);

    /* expand arguments */
    for (int i = 0; i < macroBody->argCount; i++) {
        expandMacroArgument(&actualArgumentsInput[i]);
    }

    /* replace arguments into a different buffer to be used as the input */
    LexemStream result = replaceMacroArguments(actualArgumentsInput, bufferDesc.buffer, bufferWrite);

    return makeLexemStream(result.begin, result.begin, result.write, macroBody->name, MACRO_STREAM);
}

/* *************************************************************** */
/* ******************* MACRO CALL PROCESSING ********************* */
/* *************************************************************** */

static LexemCode getLexSkippingLines(char **saveLexemP, int *lineNumberP,
                                     int *valueP, Position *positionP, int *lengthP) {
    LexemCode lexem = getLexemAndSavePointerToPrevious(saveLexemP);
    while (lexem == LINE_TOKEN || lexem == '\n') {
        getExtraLexemInformationFor(lexem, &currentInput.read, lineNumberP, valueP, positionP, lengthP,
                                    !insideMacro());
        lexem = getLexemAndSavePointerToPrevious(saveLexemP);
    }
    return lexem;
}

static LexemCode getActualMacroArgument(char *previousLexemP, LexemCode lexem, Position macroPosition,
                                        Position *beginPositionP, Position *endPositionP,
                                        LexemStream *actualArgumentsInput, int argumentCount,
                                        int actualArgumentIndex
) {
    char *bufferP;
    LexemBufferDescriptor bufferDesc;

    bufferDesc.size = MACRO_ARGUMENTS_BUFFER_SIZE;
    bufferDesc.buffer = ppmAllocc(bufferDesc.size + MAX_LEXEM_SIZE, sizeof(char));
    bufferP = bufferDesc.buffer;

    /* if lastArgument, collect everything there */
    bool isLastArgument = actualArgumentIndex + 1 == argumentCount;

    int depth = 0;
    int offset = 0;
    while (true) {
        if (lexem == '(')
            depth++;
        else if (lexem == ')') {
            if (depth == 0) break;
            depth--;
        } else if (lexem == ',' && depth == 0 && !isLastArgument)
            break;

        /* Copy from source to buffer */
        for (; previousLexemP < currentInput.read; previousLexemP++, bufferP++)
            *bufferP = *previousLexemP;

        if (bufferP - bufferDesc.buffer >= bufferDesc.size) {
            bufferDesc.size += MACRO_ARGUMENTS_BUFFER_SIZE;
            ppmReallocc(bufferDesc.buffer, bufferDesc.size + MAX_LEXEM_SIZE, sizeof(char),
                        bufferDesc.size + MAX_LEXEM_SIZE - MACRO_ARGUMENTS_BUFFER_SIZE);
        }
        lexem = getLexSkippingLines(&previousLexemP, NULL, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile,
                                endOfMacroArgument); /* CAUTION! Contains goto:s! */

        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL,
                                    endPositionP, NULL, !insideMacro());
        if ((lexem == ',' || lexem == ')') && depth == 0) {
            offset++;
            handleMacroUsageParameterPositions(actualArgumentIndex + offset, macroPosition,
                                               *beginPositionP,
                                               *endPositionP, 0);
            *beginPositionP = *endPositionP;
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
    ppmReallocc(bufferDesc.buffer, bufferP - bufferDesc.buffer, sizeof(char), bufferDesc.size + MAX_LEXEM_SIZE);
    *actualArgumentsInput = makeLexemStream(bufferDesc.buffer, bufferDesc.buffer, bufferP, currentInput.macroName,
                                         NORMAL_STREAM);
    return lexem;
}

static LexemStream *getActualMacroArguments(MacroBody *macroBody, Position macroPosition,
                                         Position lparPosition) {
    ENTER();

    LexemStream *actualArgs = ppmAllocc(macroBody->argCount, sizeof(LexemStream));

    char *previousLexemP;
    Position beginPosition = lparPosition;
    Position endPosition = lparPosition;

    LexemCode lexem = getLexSkippingLines(&previousLexemP, NULL, NULL, &endPosition, NULL);
    ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

    getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &endPosition, NULL,
                                !insideMacro());

    log_trace("getActualMacroArguments for %s: %s", macroBody->name, lexemEnumNames[lexem]);

    int argumentIndex = 0;

    if (lexem == ')') {
        handleMacroUsageParameterPositions(0, macroPosition, beginPosition, endPosition, true);
    } else {
        for (;;) {
            lexem = getActualMacroArgument(previousLexemP, lexem, macroPosition, &beginPosition, &endPosition,
                                           &actualArgs[argumentIndex], macroBody->argCount, argumentIndex);
            //ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
            log_trace("getActualMacroArgument: %s", lexemEnumNames[lexem]);
            //printf("Macroposition: %d:%d:%d\n", macroPosition.file, macroPosition.line, macroPosition.col);
            argumentIndex++;
            if (lexem != ',' || argumentIndex >= macroBody->argCount)
                break;
            lexem = getLexSkippingLines(&previousLexemP, NULL, NULL, NULL, NULL);
            ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */
            getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, NULL, NULL,
                                        !insideMacro());
        }
    }
    /* fill missing arguments */
    for (;argumentIndex < macroBody->argCount; argumentIndex++) {
        actualArgs[argumentIndex] = makeLexemStream(NULL, NULL, NULL, NULL, NORMAL_STREAM);
    }

    LEAVE();
    return actualArgs;

endOfMacroArgument:
    assert(0);
endOfFile:
    assert(options.mode);
    if (options.mode!=ServerMode) {
        warningMessage(ERR_ST,"[getActualMacroArguments] unterminated macro call");
    }
    LEAVE();
    return NULL;                /* WARNING this will probably crash the caller... */
}

/* **************************************************************** */

static void addMacroBaseUsageRef(Symbol *macroSymbol) {
    Position basePos = makePosition(inputFileNumber, 0, 0);
    ReferenceItem ppp = makeReferenceItem(macroSymbol->linkName, TypeMacro, StorageDefault,
                                          GlobalScope, GlobalVisibility, NO_FILE_NUMBER);
    ReferenceItem *memb;
    bool isMember = isMemberInReferenceTable(&ppp, NULL, &memb);
    Reference *r = NULL;
    if (isMember) {
        // this is optimization to avoid multiple base references
        for (r=memb->references; r!=NULL; r=r->next) {
            if (r->usage == UsageMacroBaseFileUsage)
                break;
        }
    }
    if (!isMember || r==NULL) {
        addCxReference(macroSymbol, basePos, UsageMacroBaseFileUsage, NO_FILE_NUMBER);
    }
}


static bool expandMacroCall(Symbol *macroSymbol, Position macroPosition) {

    MacroBody *macroBody = macroSymbol->mbody;
    if (macroBody == NULL)
        return false;	/* !!!!!         tricky,  undefined macro */
    if (!insideMacro()) { /* call from top level, init memory */
        mbmInit();
    }
    log_trace("trying to expand macro '%s'", macroBody->name);
    if (cyclicCall(macroBody))
        return false;

    /* Make sure these are initialized */
    char *previousLexemP = currentInput.read;
    char *ppmMarker = ppmAllocc(0, sizeof(char));

    LexemStream *actualArgumentsInput;
    if (macroBody->argCount >= 0) {
        LexemCode lexem = getLexSkippingLines(&previousLexemP, NULL, NULL, NULL, NULL);
        ON_LEXEM_EXCEPTION_GOTO(lexem, endOfFile, endOfMacroArgument); /* CAUTION! Contains goto:s! */

        if (lexem != '(') {
            currentInput.read = previousLexemP;		/* unget lexem */
            return false;
        }
        Position lparPosition;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &lparPosition, NULL,
                                    !insideMacro());
        actualArgumentsInput = getActualMacroArguments(macroBody, macroPosition, lparPosition);
    } else {
        actualArgumentsInput = NULL;
    }
    assert(options.mode);
    addCxReference(macroSymbol, macroPosition, UsageUsed, NO_FILE_NUMBER);
    if (options.mode == XrefMode)
        addMacroBaseUsageRef(macroSymbol);
    log_trace("create macro body '%s' as new input", macroBody->name);

    LexemStream macroBodyInput = createMacroBodyAsNewStream(macroBody, actualArgumentsInput);

    prependMacroInput(&macroBodyInput);
    log_trace("expanded macro '%s'", macroBody->name);

    ppmFreeUntil(ppmMarker);
    return true;

endOfMacroArgument:
    /* unterminated macro call in argument */
    /* TODO unread the argument that was read */
    currentInput.read = previousLexemP;
    ppmFreeUntil(ppmMarker);
    return false;

 endOfFile:
    assert(options.mode);
    if (options.mode!=ServerMode) {
        warningMessage(ERR_ST,"[macroCallExpand] unterminated macro call");
    }
    currentInput.read = previousLexemP;
    ppmFreeUntil(ppmMarker);
    return false;
}

void dumpLexemBuffer(LexemBuffer *lb) {
    char *cc;
    LexemCode lexem;

    log_debug("lexbufdump [start] ");
    cc = lb->read;
    while (cc < (char *)getLexemStreamWrite(lb)) {
        lexem = getLexemCodeAndAdvance(&cc);
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
         skipExtraLexemInformationFor(lexem, &cc);
    }
    log_debug("lexbufdump [stop]");
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
static LexemCode lookupIdentifier(char *id, Position position) {
    Symbol *symbol = NULL;
    unsigned hash = hashFun(id) % symbolTable->size;

    log_trace("looking for C id '%s' in symbol table %p", id, symbolTable);
    for (Symbol *s=symbolTable->tab[hash]; s!=NULL; s=s->next) {
        if (strcmp(s->name, id) == 0) {
            if (symbol == NULL)
                symbol = s;
            if (isIdAKeyword(s, position))
                return s->keyword;
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
                skipExtraLexemInformationWithCountLines(lexem, &currentInput.read, !insideMacro());
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
                                        !insideMacro());
            setYylvalsForPosition(position, tokenNameLengthsTable[lexem]);
        }
        yytext = charText;
        charText[0] = lexem;
        goto finish;
    }
    if (lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND) {

        char *id = yytext = currentInput.read;
        Position position;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    !insideMacro());
        assert(options.mode);
        if (options.mode == ServerMode) {
            // TODO: ???????????? isn't this useless
            testCxrefCompletionId(&lexem, yytext, position);
        }
        log_trace("id '%s' position %d, %d, %d", yytext, position.file, position.line, position.col);

        Symbol *foundMacroSymbol = findMacroSymbol(yytext);
        if (lexem != IDENT_NO_CPP_EXPAND && foundMacroSymbol != NULL) {
            id = foundMacroSymbol->name;
            if (expandMacroCall(foundMacroSymbol, position))
                goto nextYylex;
        }

        lexem = lookupIdentifier(id, position);
        goto finish;
    }
    if (lexem == OL_MARKER_TOKEN) {
        skipExtraLexemInformationWithCountLines(lexem, &currentInput.read, !insideMacro());
        actionOnBlockMarker();
        goto nextYylex;
    }
    if (lexem < MULTI_TOKENS_START) {
        Position position;
        yytext = tokenNamesTable[lexem];
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    !insideMacro());
        setYylvalsForPosition(position, tokenNameLengthsTable[lexem]);
        goto finish;
    }
    if (lexem == LINE_TOKEN) {
        skipExtraLexemInformationWithCountLines(lexem, &currentInput.read, !insideMacro());
        goto nextYylex;
    }
    if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
        int value;
        int length;
        Position position;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, &value, &position, &length,
                                    !insideMacro());
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
                                    !insideMacro());
        setYylvalsForPosition(position, length);
        goto finish;
    }
    if (lexem == STRING_LITERAL) {
        Position position;
        yytext = currentInput.read;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, NULL, &position, NULL,
                                    !insideMacro());
        setYylvalsForPosition(position, strlen(yytext));
        goto finish;
    }
    if (lexem == CHAR_LITERAL) {
        int value;
        int length;
        Position position;
        getExtraLexemInformationFor(lexem, &currentInput.read, NULL, &value, &position, &length,
                                    !insideMacro());
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
                                    !insideMacro());
        if (lexem == IDENT_TO_COMPLETE) {
            testCxrefCompletionId(&lexem, yytext, position);
            while (includeStack.pointer != 0)
                popInclude();
            /* while (getLexBuf(&cFile.lb)) cFile.lb.cc = cFile.lb.fin;*/
            goto endOfFile;
        }
    }
    log_trace("unknown lexem %d", lexem);
    goto endOfFile;

 finish:
    log_trace("yytext='%s'(@ index %d)", yytext, cxMemory.index);
    return lexem;

 endOfMacroArgument:
    assert(0);

 endOfFile:
    if (includeStack.pointer != 0) {
        popInclude();
        goto nextYylex;
    }
    /* add the test whether in COMPLETION, communication string found */
    return 0;
}	/* end of yylex() */

void yylexMemoryStatistics(void) {
    printMemoryStatisticsFor(&macroArgumentsMemory);
    printMemoryStatisticsFor(&macroBodyMemory);
}

void dumpLexem(char *lexemP) {
    LexemCode lexem = getLexemCodeAndAdvance(&lexemP);
    printf("%p : %s", lexemP, lexemEnumNames[lexem]);

    if (lexem == IDENTIFIER || lexem == STRING_LITERAL) {
        printf("(%s)", lexemP);
    } else {
        int lineNumber, value, length;
        Position position;
        getExtraLexemInformationFor(lexem, &lexemP, &lineNumber, &value, &position, &length, false);
        if (lexem == CPP_MACRO_ARGUMENT)
            printf("(%d)", value);
    }
    printf("\n");
}

void dumpLexemsWithEndPointer(char *lexemP, char *endP) {
    printf("-------------\n");
    while (lexemP < endP) {
        dumpLexem(lexemP);
        LexemCode lexem = getLexemCodeAndAdvance(&lexemP);
         skipExtraLexemInformationFor(lexem, &lexemP);
    }
    printf("-------------\n");
}

void dumpLexemsWithByteCount(char *lexemP, int byteCount) {
    dumpLexemsWithEndPointer(lexemP, lexemP + byteCount);
}

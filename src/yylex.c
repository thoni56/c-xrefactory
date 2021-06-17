#include "yylex.h"

#include "commons.h"
#include "lexer.h"
#include "globals.h"
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
#include "log.h"


#define SET_IDENTIFIER_YYLVAL(name, symb, pos) {\
    uniyylval->ast_id.d = &s_yyIdentBuf[s_yyIdentBufi];\
    s_yyIdentBufi ++; s_yyIdentBufi %= (YYBUFFERED_ID_INDEX);\
    fillId(uniyylval->ast_id.d, name, symb, pos);\
    yytext = name;\
    uniyylval->ast_id.b = pos;\
    uniyylval->ast_id.e = pos;\
    uniyylval->ast_id.e.col += strlen(yytext);\
}


/* !!!!!!!!!!!!!!!!!!! to caching !!!!!!!!!!!!!!! */


#define MB_INIT()				{SM_INIT(mbMemory);}
#define MB_ALLOC(p,t)           {SM_ALLOC(mbMemory,p,t);}
#define MB_ALLOCC(p,n,t)        {SM_ALLOCC(mbMemory,p,n,t);}
#define MB_REALLOCC(p,n,t,on)	{SM_REALLOCC(mbMemory,p,n,t,on);}
#define MB_FREE_UNTIL(p)        {SM_FREE_UNTIL(mbMemory,p);}


/* Exceptions: */
#define END_OF_MACRO_ARGUMENT_EXCEPTION -1
#define END_OF_FILE_EXCEPTION -2

LexInput currentInput;
int macroStackIndex=0;

static char ppMemory[SIZE_ppMemory];
static int ppMemoryIndex=0;
static LexInput macroStack[MACRO_STACK_SIZE];




static bool isIdentifierLexem(Lexem lexem) {
    return lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND  || lexem==IDENT_TO_COMPLETE;
}


static int expandMacroCall(Symbol *mdef, Position *mpos);


/* ************************************************************ */

void initAllInputs(void) {
    mbMemoryIndex=0;
    includeStackPointer=0;
    macroStackIndex=0;
    s_ifEvaluation = 0;
    s_cxRefFlag = 0;
    macroArgumentTableNoAllocInit(&s_macroArgumentTable, MAX_MACRO_ARGS);
    ppMemoryIndex=0;
    s_olstring[0]=0;
    s_olstringFound = 0;
    s_olstringServed = 0;
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
    s_cache.cc = currentInput.currentLexemP;
}
static void setCFileConsistency(void) {
    currentFile.lexBuffer.next = currentInput.currentLexemP;
}
static void setCInputConsistency(void) {
    fillLexInput(&currentInput, currentFile.lexBuffer.next, currentFile.lexBuffer.end,
                 currentFile.lexBuffer.lexemStream, NULL, INPUT_NORMAL);
}

char *placeIdent(void) {
    static char tt[2*MAX_HTML_REF_LEN];
    char fn[MAX_FILE_NAME_SIZE];
    char mm[MAX_HTML_REF_LEN];
    int s;
    if (currentFile.fileName!=NULL) {
        if (options.xref2 && options.taskRegime!=RegimeEditServer) {
            strcpy(fn, getRealFileNameStatic(normalizeFileName(currentFile.fileName, cwd)));
            assert(strlen(fn) < MAX_FILE_NAME_SIZE);
            sprintf(mm, "%s:%d", simpleFileName(fn),currentFile.lineNumber);
            assert(strlen(mm) < MAX_HTML_REF_LEN);
            sprintf(tt, "<A HREF=\"file://%s#%d\" %s=%ld>%s</A>", fn, currentFile.lineNumber, PPCA_LEN, (unsigned long)strlen(mm), mm);
        } else {
            sprintf(tt,"%s:%d ",simpleFileName(getRealFileNameStatic(currentFile.fileName)),currentFile.lineNumber);
        }
        s = strlen(tt);
        assert(s<MAX_HTML_REF_LEN);
        return(tt);
    }
    return("");
}

static bool isPreprocessorToken(Lexem lexem) {
    return lexem>CPP_TOKENS_START && lexem<CPP_TOKENS_END;
}

#ifdef DEBUG
static void traceNewline(int lines) {
    int i;
    if (options.trace) {
        for(i=1; i<=lines; i++) {
            log_trace("%s:%d", currentFile.fileName, currentFile.lineNumber+i);
        }
    }
}
#else
#define traceNewline(line) {}
#endif

/* *********************************************************** */
/* Always return the found index                               */
int addFileTabItem(char *name) {
    int fileIndex, len;
    char *fname, *normalizedFileName;
    struct fileItem *createdFileItem;

    /* Create a fileItem on the stack, with a static normalizedFileName, returned by normalizeFileName() */
    normalizedFileName = normalizeFileName(name, cwd);

    /* Does it already exist? */
    if (fileTableExists(&fileTable, normalizedFileName))
        return fileTableLookup(&fileTable, normalizedFileName);

    /* If not, add it, but then we need a filename and a fileitem in FT-memory  */
    len = strlen(normalizedFileName);
    FT_ALLOCC(fname, len+1, char);
    strcpy(fname, normalizedFileName);

    FT_ALLOC(createdFileItem, FileItem);
    fillFileItem(createdFileItem, fname, false);

    fileIndex = fileTableAdd(&fileTable, createdFileItem);
    checkFileModifiedTime(fileIndex); // it was too slow on load ?

    return fileIndex;
}

static void setCurrentFileInfoFor(char *fileName) {
    char *name;
    int number, cxloading;
    bool existed = false;

    if (fileName==NULL) {
        number = noFileIndex;
        name = fileTable.tab[number]->name;
    } else {
        existed = fileTableExists(&fileTable, fileName);
        number = addFileTabItem(fileName);
        name = fileTable.tab[number]->name;
        checkFileModifiedTime(number);
        cxloading = fileTable.tab[number]->b.cxLoading;
        if (!existed) {
            cxloading = 1;
        } else if (options.update==UP_FAST_UPDATE) {
            if (fileTable.tab[number]->b.scheduledToProcess) {
                // references from headers are not loaded on fast update !
                cxloading = 1;
            }
        } else if (options.update==UP_FULL_UPDATE) {
            if (fileTable.tab[number]->b.scheduledToUpdate) {
                cxloading = 1;
            }
        } else {
            cxloading = 1;
        }
        if (fileTable.tab[number]->b.cxSaved==1 && ! options.multiHeadRefsCare) {
            /* if multihead references care, load include refs each time */
            cxloading = 0;
        }
        if (LANGUAGE(LANG_JAVA)) {
            if (s_jsl!=NULL || s_javaPreScanOnly) {
                // do not load (and save) references from jsl loaded files
                // nor during prescanning
                cxloading = fileTable.tab[number]->b.cxLoading;
            }
        }
        fileTable.tab[number]->b.cxLoading = cxloading;
    }
    currentFile.lexBuffer.buffer.fileNumber = number;
    currentFile.fileName = name;
}

/* ***************************************************************** */
/*                             Init Reading                          */
/* ***************************************************************** */

void ppMemInit(void) {
    PP_ALLOCC(s_macroArgumentTable.tab, MAX_MACRO_ARGS, MacroArgumentTableElement *);
    macroArgumentTableNoAllocInit(&s_macroArgumentTable, MAX_MACRO_ARGS);
    ppMemoryIndex = 0;
}

// it is supposed that one of file or buffer is NULL
void initInput(FILE *file, EditorBuffer *editorBuffer, char *prefix, char *fileName) {
    int     prefixLength, bufferSize, offset;
    char	*bufferStart;

    /* TODO: perhaps this polymorphism should be handled some other way... */
    prefixLength = strlen(prefix);
    if (editorBuffer != NULL) {
        // read buffer
        assert(prefixLength < editorBuffer->a.allocatedFreePrefixSize);
        strncpy(editorBuffer->a.text-prefixLength, prefix, prefixLength);
        bufferStart = editorBuffer->a.text-prefixLength;
        bufferSize = editorBuffer->a.bufferSize+prefixLength;
        offset = editorBuffer->a.bufferSize;
        assert(bufferStart > editorBuffer->a.allocatedBlock);
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

/* maybe too time-consuming, when lex is known */
/* should be broken into parts */
#define PassLexem(input, lexem, lineval, val, pos, length, linecounting) { \
        if (lexem > MULTI_TOKENS_START) {                               \
            if (isIdentifierLexem(lexem)){                              \
                input = strchr(input, '\0')+1;                          \
                (pos) = getLexPosition(&input);                         \
            } else if (lexem == STRING_LITERAL) {                       \
                input = strchr(input, '\0')+1;                          \
                (pos) = getLexPosition(&input);                         \
            } else if (lexem == LINE_TOKEN) {                           \
                lineval = getLexToken(&input);                          \
                if (linecounting) {                                     \
                    traceNewline(lineval);                              \
                    currentFile.lineNumber += lineval;                  \
                }                                                       \
            } else if (lexem == CONSTANT || lexem == LONG_CONSTANT) {   \
                val = getLexInt(&input);                                \
                (pos) = getLexPosition(&input);                         \
                length = getLexInt(&input);                             \
            } else if (lexem == DOUBLE_CONSTANT || lexem == FLOAT_CONSTANT) { \
                (pos) = getLexPosition(&input);                         \
                length = getLexInt(&input);                             \
            } else if (lexem == CPP_MAC_ARG) {                          \
                val = getLexInt(&input);                                \
                (pos) = getLexPosition(&input);                         \
            } else if (lexem == CHAR_LITERAL) {                         \
                val = getLexInt(&input);                                \
                (pos) = getLexPosition(&input);                         \
                length = getLexInt(&input);                             \
            }                                                           \
        } else if (isPreprocessorToken(lexem)) {                        \
            (pos) = getLexPosition(&input);                             \
        } else if (lexem == '\n' && (linecounting)) {                      \
            (pos) = getLexPosition(&input);                             \
            traceNewline(1);                                            \
            currentFile.lineNumber ++;                                  \
        } else {                                                        \
            (pos) = getLexPosition(&input);                             \
        }                                                               \
    }

static Lexem getLexemSavePrevious(char **previousLexem, jmp_buf exceptionHandler) {
    Lexem lexem;

    while (currentInput.currentLexemP >= currentInput.endOfBuffer) {
        InputType inputType;
        inputType = currentInput.inputType;
        if (macroStackIndex > 0) {
            if (inputType == INPUT_MACRO_ARGUMENT) {
                if (exceptionHandler != NULL)
                    longjmp(exceptionHandler, END_OF_MACRO_ARGUMENT_EXCEPTION);
                else
                    return -1; //goto endOfMacroArgument; TODO: replace with setjmp()/longjmp()
            }
            MB_FREE_UNTIL(currentInput.beginningOfBuffer);
            currentInput = macroStack[--macroStackIndex];
        } else if (inputType == INPUT_NORMAL) {
            setCFileConsistency();
            if (!getLexemFromLexer(&currentFile.lexBuffer)) {
                if (exceptionHandler != NULL)
                    longjmp(exceptionHandler, END_OF_FILE_EXCEPTION);
                else
                    return -2; //goto endOfFile; TODO: replace with setjmp()/longjmp()
            }
            setCInputConsistency();
        } else {
            s_cache.cc = s_cache.cfin = NULL;
            cacheInput();
            s_cache.lexcc = currentFile.lexBuffer.next;
            setCInputConsistency();
        }
        *previousLexem = currentInput.currentLexemP;
    }
    *previousLexem = currentInput.currentLexemP;
    lexem = getLexToken(&currentInput.currentLexemP);
    return lexem;
}

/* All occurences are expanded
#define GetLex(lexem) {                                             \
    char *previousLexem;                                            \
    UNUSED previousLexem;                                           \
    lexem = getLexemSavePrevious(&previousLexem, NULL);             \
    if (lexem == -1) goto endOfMacroArgument;                       \
    if (lexem == -2) goto endOfFile;                                \
}
*/

/* Attempt to re-create a function that does what GetLex() macro does: */
static int getLex(jmp_buf exceptionHandler) {
    char *previousLexem;
    UNUSED previousLexem;
    Lexem lexem = getLexemSavePrevious(&previousLexem, exceptionHandler);
    return lexem; // -1 if endOfMacroArgument, -2 if endOfFile
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
            s_cache.activeCache = false;
            s_olstringServed = 1;
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
    int l, v, len; UNUSED len; UNUSED v; UNUSED l;
    Position pos; UNUSED pos;
    jmp_buf exceptionHandler;

    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

    lexem = getLex(exceptionHandler);

    PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
    if (lexem != CONSTANT)
        return;

    lexem = getLex(exceptionHandler);

    PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
    return;

endOfMacroArgument:	assert(0);
endOfFile:;
}

/* ********************************* #INCLUDE ********************** */

static void fillIncludeSymbolItem(Symbol *ss, int filenum, Position *pos){
    // should be different for HTML to be beatiful, however,
    // all includes needs to be in the same cxfile, because of
    // -update. On the other hand in HTML I wish them to split
    // by first letter of file name.
    fillSymbol(ss, LINK_NAME_INCLUDE_REFS, LINK_NAME_INCLUDE_REFS, *pos);
    fillSymbolBits(&ss->bits, AccessDefault, TypeCppInclude, StorageDefault);
}


void addThisFileDefineIncludeReference(int filenum) {
    Position dpos;
    Symbol ss;
    fillPosition(&dpos, filenum, 1, 0);
    fillIncludeSymbolItem(&ss,filenum, &dpos);
//&fprintf(dumpOut,"adding reference on file %d==%s\n",filenum, fileTable.tab[filenum]->name);
    addCxReference(&ss, &dpos, UsageDefined, filenum, filenum);
}

void addIncludeReference(int filenum, Position *pos) {
    Symbol ss;
//&fprintf(dumpOut,"adding reference on file %d==%s\n",filenum, fileTable.tab[filenum]->name);
    fillIncludeSymbolItem( &ss, filenum, pos);
    addCxReference(&ss, pos, UsageUsed, filenum, filenum);
}

static void addIncludeReferences(int filenum, Position *pos) {
    addIncludeReference(filenum, pos);
    addThisFileDefineIncludeReference(filenum);
}

void pushInclude(FILE *f, EditorBuffer *buffer, char *name, char *prepend) {
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
    initInput(f, buffer, prepend, name);
    cacheInclude(currentFile.lexBuffer.buffer.fileNumber);
}

void popInclude(void) {
    assert(fileTable.tab[currentFile.lexBuffer.buffer.fileNumber]);
    if (fileTable.tab[currentFile.lexBuffer.buffer.fileNumber]->b.cxLoading) {
        fileTable.tab[currentFile.lexBuffer.buffer.fileNumber]->b.cxLoaded = true;
    }
    closeCharacterBuffer(&currentFile.lexBuffer.buffer);
    if (includeStackPointer != 0) {
        currentFile = includeStack[--includeStackPointer];	/* buffers are copied !!!!!!, burk */
        if (includeStackPointer == 0 && s_cache.cc!=NULL) {
            fillLexInput(&currentInput, s_cache.cc, s_cache.cfin, s_cache.lb, NULL, INPUT_CACHE);
        } else {
            setCInputConsistency();
        }
    }
}

static FILE *openInclude(char includeType, char *name, char **fileName) {
    EditorBuffer *editorBuffer;
    FILE *file;
    StringList *ll;
    char wcp[MAX_OPTION_LEN];
    char normalizedName[MAX_FILE_NAME_SIZE];
    char rdir[MAX_FILE_NAME_SIZE];
    int nnlen, dlen, nmlen;

    editorBuffer = NULL; file = NULL;
    nmlen = strlen(name);
    extractPathInto(currentFile.fileName, rdir);
    if (includeType!='<') {
        strcpy(normalizedName, normalizeFileName(name, rdir));
        log_trace("try to open %s", normalizedName);
        editorBuffer = editorFindFile(normalizedName);
        if (editorBuffer == NULL)
            file = openFile(normalizedName, "r");
    }
    for (ll=options.includeDirs; ll!=NULL && editorBuffer==NULL && file==NULL; ll=ll->next) {
        strcpy(normalizedName, normalizeFileName(ll->d, rdir));
        expandWildcardsInOnePath(normalizedName, wcp, MAX_OPTION_LEN);
        JavaMapOnPaths(wcp, {
            strcpy(normalizedName, currentPath);
            dlen = strlen(normalizedName);
            if (dlen>0 && normalizedName[dlen-1]!=FILE_PATH_SEPARATOR) {
                normalizedName[dlen] = FILE_PATH_SEPARATOR;
                dlen++;
            }
            strcpy(normalizedName+dlen, name);
            nnlen = dlen+nmlen;
            normalizedName[nnlen]=0;
            log_trace("try to open <%s>", normalizedName);
            editorBuffer = editorFindFile(normalizedName);
            if (editorBuffer==NULL)
                file =
                    openFile(normalizedName, "r");
            if (editorBuffer!=NULL || file!=NULL)
                goto found;
        });
    }
    if (editorBuffer==NULL && file==NULL)
        return NULL;
 found:
    strcpy(normalizedName, normalizeFileName(normalizedName, cwd));
    log_trace("file '%s' opened, checking to %s", normalizedName, fileTable.tab[s_olOriginalFileNumber]->name);
    pushInclude(file, editorBuffer, normalizedName, "\n");
    return stdin;  // NOT NULL
}

static void processInclude2(Position *ipos, char pchar, char *iname) {
    char *fname;
    FILE *nyyin;
    Symbol ss,*memb;
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "PragmaOnce-%s", iname);

    fillSymbol(&ss, tmpBuff, tmpBuff, s_noPos);
    fillSymbolBits(&ss.bits, AccessDefault, TypeMacro, StorageNone);

    if (symbolTableIsMember(s_symbolTable, &ss, NULL, &memb))
        return;
    nyyin = openInclude(pchar, iname, &fname);
    if (nyyin == NULL) {
        assert(options.taskRegime);
        if (options.taskRegime!=RegimeEditServer)
            warningMessage(ERR_CANT_OPEN, iname);
    } else {
        addIncludeReferences(currentFile.lexBuffer.buffer.fileNumber, ipos);
    }
}

/* Public only for unittests */
void processIncludeDirective(Position *ipos) {
    char *currentLexemP, *previousLexemP;
    Lexem lexem;
    int l, v, len; UNUSED len; UNUSED v; UNUSED l;
    Position pos; UNUSED pos;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

    lexem = getLexemSavePrevious(&previousLexemP, exceptionHandler);

    currentLexemP = currentInput.currentLexemP;
    if (lexem == STRING_LITERAL) {
        PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        if (macroStackIndex != 0) {
            errorMessage(ERR_INTERNAL,"include directive in macro body?");
assert(0);
            currentInput = macroStack[0];
            macroStackIndex = 0;
        }
        processInclude2(ipos, *currentLexemP, currentLexemP+1);
    } else {
        currentInput.currentLexemP = previousLexemP;		/* unget lexem */
        lexem = yylex();
        if (lexem == STRING_LITERAL) {
            currentInput = macroStack[0];		// hack, cut everything pending
            macroStackIndex = 0;
            processInclude2(ipos, '\"', yytext);
        } else if (lexem == '<') {
            // TODO!!!!
            warningMessage(ERR_ST,"Include <> after macro expansion not yet implemented, sorry\n\tuse \"\" instead");
        }
        //do lex = yylex(); while (lex != '\n');
    }
    return;

 endOfMacroArgument:	assert(0);
 endOfFile:;
}

static void addMacroToTabs(Symbol *pp, char *name) {
    int index, mm;
    Symbol *memb;

    mm = symbolTableIsMember(s_symbolTable, pp, &index, &memb);
    if (mm) {
        log_trace(": masking macro %s", name);
    } else {
        log_trace(": adding macro %s", name);
    }
    symbolTableSet(s_symbolTable, pp, index);
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

static MacroBody *newMacroBody(int msize, int argi, char *name, char *body, char **argNames) {
    MacroBody *macroBody;

    PP_ALLOC(macroBody, MacroBody);
    macroBody->argn = argi;
    macroBody->size = msize;
    macroBody->name = name;
    macroBody->args = argNames;
    macroBody->body = body;

    return macroBody;
}


static Lexem getNonBlankLexem(jmp_buf exceptionHandler, Position *_pos, int *_l, int *_v, int *_len) {
    Lexem lexem;
    Position pos = *_pos;
    int l = *_l, v = *_v, len = *_len;

    lexem = getLex(exceptionHandler);
    while (lexem == LINE_TOKEN) {
        PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        lexem = getLex(exceptionHandler);
    }
    *_pos = pos;
    *_l = l;
    *_v = v;
    *_len = len;

    return lexem;
}


/* Make public only for unittesting */
void processDefineDirective(bool hasArguments) {
    Lexem lexem;
    bool isReadingBody = false;
    int foundIndex;
    int argumentCount, ellipsis;
    Symbol *symbol;
    MacroArgumentTableElement *maca, mmaca;
    MacroBody *macroBody;
    Position pos, macroPosition, ppb1, ppb2, *parpos1, *parpos2, *tmppp;
    char *currentLexemStart, *macroName, *argumentName;
    char *body, *mm;
    char **argumentNames, *argLinkName;
    int l, v, len; UNUSED l; UNUSED v; UNUSED len;

    /* These need to be static to survive longjmp */
    static int macroSize;
    static int allocatedSize;

    macroSize= -1;
    allocatedSize=0;argumentCount=0;symbol=NULL;macroBody=NULL;macroName=body=NULL; // to calm compiler
    SM_INIT(ppMemory);
    ppb1 = s_noPos;
    ppb2 = s_noPos;
    parpos1 = &ppb1;
    parpos2 = &ppb2;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

    lexem = getLex(exceptionHandler);
    currentLexemStart = currentInput.currentLexemP;

    /* There are "symbols" in the lexBuffer, like "\275\001".  Those
     * are compacted converted lexem codes. See lexembuffer.h for
     * explanation and functions.
     */
    PassLexem(currentInput.currentLexemP, lexem, l, v, macroPosition, len, true);

    testCxrefCompletionId(&lexem, currentLexemStart, &macroPosition);    /* for cross-referencing */
    if (lexem != IDENTIFIER)
        return;

    PP_ALLOC(symbol, Symbol);
    fillSymbol(symbol, NULL, NULL, macroPosition);
    fillSymbolBits(&symbol->bits, AccessDefault, TypeMacro, StorageNone);

    /* TODO: this is the only call to setGlobalFileDepNames() that doesn't do it in XX memory, why?
       PreProcessor? */
    setGlobalFileDepNames(currentLexemStart, symbol, MEMORY_PP);
    macroName = symbol->name;
    /* process arguments */
    macroArgumentTableNoAllocInit(&s_macroArgumentTable, s_macroArgumentTable.size);
    argumentCount = -1;


    if (hasArguments) {
        lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);
        PassLexem(currentInput.currentLexemP, lexem, l, v, *parpos2, len, true);
        *parpos1 = *parpos2;
        if (lexem != '(')
            goto errorlab;
        argumentCount++;
        lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);

        if (lexem != ')') {
            for(;;) {
                char tmpBuff[TMP_BUFF_SIZE];
                currentLexemStart = argumentName = currentInput.currentLexemP;
                PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
                ellipsis = 0;
                if (lexem == IDENTIFIER ) {
                    argumentName = currentLexemStart;
                } else if (lexem == ELIPSIS) {
                    argumentName = s_cppVarArgsName;
                    pos = macroPosition;					// hack !!!
                    ellipsis = 1;
                } else
                    goto errorlab;
                PP_ALLOCC(mm, strlen(argumentName)+1, char);
                strcpy(mm, argumentName);
                sprintf(tmpBuff, "%x-%x%c%s", pos.file, pos.line,
                        LINK_NAME_SEPARATOR, argumentName);
                PP_ALLOCC(argLinkName, strlen(tmpBuff)+1, char);
                strcpy(argLinkName, tmpBuff);
                SM_ALLOC(ppMemory, maca, MacroArgumentTableElement);
                fillMacroArgTabElem(maca, mm, argLinkName, argumentCount);
                foundIndex = macroArgumentTableAdd(&s_macroArgumentTable, maca);
                argumentCount++;
                lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);

                tmppp=parpos1; parpos1=parpos2; parpos2=tmppp;
                PassLexem(currentInput.currentLexemP, lexem, l, v, *parpos2, len, true);
                if (! ellipsis) {
                    addTrivialCxReference(s_macroArgumentTable.tab[foundIndex]->linkName, TypeMacroArg,StorageDefault,
                                          &pos, UsageDefined);
                    handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &pos, parpos2, 0);
                }
                if (lexem == ELIPSIS) {
                    // GNU ELLIPSIS ?????
                    lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);
                    PassLexem(currentInput.currentLexemP, lexem, l, v, *parpos2, len, true);
                }
                if (lexem == ')')
                    break;
                if (lexem != ',')
                    break;

                lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);
            }
            handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &s_noPos, parpos2, 1);
        } else {
            PassLexem(currentInput.currentLexemP, lexem, l, v, *parpos2, len, true);
            handleMacroDefinitionParameterPositions(argumentCount, &macroPosition, parpos1, &s_noPos, parpos2, 1);
        }
    }
    /* process macro body */
    allocatedSize = MACRO_UNIT_SIZE;
    macroSize = 0;
    PP_ALLOCC(body, allocatedSize+MAX_LEXEM_SIZE, char);
    isReadingBody = true;

    lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);
    currentLexemStart = currentInput.currentLexemP;
    PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
    while (lexem != '\n') {
        while(macroSize<allocatedSize && lexem != '\n') {
            char *destination;
            fillMacroArgTabElem(&mmaca,currentLexemStart,NULL,0);
            if (lexem==IDENTIFIER && macroArgumentTableIsMember(&s_macroArgumentTable,&mmaca,&foundIndex)){
                /* macro argument */
                addTrivialCxReference(s_macroArgumentTable.tab[foundIndex]->linkName, TypeMacroArg,StorageDefault,
                                      &pos, UsageUsed);
                destination = body+macroSize;
                putLexToken(CPP_MAC_ARG, &destination);
                putLexInt(s_macroArgumentTable.tab[foundIndex]->order, &destination);
                putLexPosition(pos.file, pos.line,pos.col, &destination);
                macroSize = destination - body;
            } else {
                if (lexem==IDENT_TO_COMPLETE
                    || (lexem == IDENTIFIER && positionsAreEqual(pos, s_cxRefPos))) {
                    s_cache.activeCache = false;
                    s_olstringFound = 1; s_olstringInMbody = symbol->linkName;
                }
                destination = body+macroSize;
                putLexToken(lexem, &destination);
                for (; currentLexemStart<currentInput.currentLexemP; destination++,currentLexemStart++)
                    *destination = *currentLexemStart;
                macroSize = destination - body;
            }
            lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);
            currentLexemStart = currentInput.currentLexemP;
            PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        }
        if (lexem != '\n') {
            allocatedSize += MACRO_UNIT_SIZE;
            PP_REALLOCC(body, allocatedSize+MAX_LEXEM_SIZE, char,
                        allocatedSize+MAX_LEXEM_SIZE-MACRO_UNIT_SIZE);
        }
    }

endOfBody:
    /* We might get here by a longjmp from getLex... so anything that is needed here needs to be static */
    assert(macroSize>=0);
    PP_REALLOCC(body, macroSize, char, allocatedSize+MAX_LEXEM_SIZE);
    allocatedSize = macroSize;
    if (argumentCount > 0) {
        PP_ALLOCC(argumentNames, argumentCount, char*);
        memset(argumentNames, 0, argumentCount*sizeof(char*));
        macroArgumentTableMap2(&s_macroArgumentTable, setMacroArgumentName, argumentNames);
    } else
        argumentNames = NULL;
    macroBody = newMacroBody(allocatedSize, argumentCount, macroName, body, argumentNames);
    symbol->u.mbody = macroBody;

    addMacroToTabs(symbol,macroName);
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

    PP_ALLOCC(nopt,strlen(opt)+3,char);
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
    Position pos;
    Symbol dd, *pp, *memb;
    int l, v, len;
    UNUSED len; UNUSED v; UNUSED l;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

    //& GetLex(lexem); // Expanded
    lexem = getLex(exceptionHandler);

    cc = currentInput.currentLexemP;
    PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
    testCxrefCompletionId(&lexem,cc,&pos);
    if (isIdentifierLexem(lexem)) {
        log_debug(": undef macro %s",cc);

        fillSymbol(&dd, cc, cc, pos);
        fillSymbolBits(&dd.bits, AccessDefault, TypeMacro, StorageNone);

        assert(options.taskRegime);
        /* !!!!!!!!!!!!!! tricky, add macro with mbody == NULL !!!!!!!!!! */
        /* this is because of monotonicity for caching, just adding symbol */
        if (symbolTableIsMember(s_symbolTable, &dd, NULL, &memb)) {
            addCxReference(memb, &pos, UsageUndefinedMacro, noFileIndex, noFileIndex);

            PP_ALLOC(pp, Symbol);
            fillSymbol(pp, memb->name, memb->linkName, pos);
            fillSymbolBits(&pp->bits, AccessDefault, TypeMacro, StorageNone);

            addMacroToTabs(pp,memb->name);
        }
    }
    while (lexem != '\n') {
        //& GetLex(lexem); // Expanded
        lexem = getLex(exceptionHandler);
        PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
    }
    return;
endOfMacroArgument:	assert(0);
endOfFile:;
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
      PP_ALLOC(ss, S_cppIfStack);
      ss->pos = *pos;
      ss->next = currentFile.ifStack;
      currentFile.ifStack = ss;
    }
    if (currentFile.ifStack!=NULL) {
      dp = currentFile.ifStack->pos;
      sprintf(ttt,"CppIf%x-%x-%d", dp.file, dp.col, dp.line);
      addTrivialCxReference(ttt, TypeCppIfElse,StorageDefault, pos, usage);
      if (level < 0) currentFile.ifStack = currentFile.ifStack->next;
    }
}

static int cppDeleteUntilEndElse(bool untilEnd) {
    Lexem lexem;
    int depth;
    Position pos;
    int l, v, len; UNUSED len; UNUSED v;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

    depth = 1;
    while (depth > 0) {
        //& GetLex(lexem); // Expanded
        lexem = getLex(exceptionHandler);

        PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        if (lexem==CPP_IF || lexem==CPP_IFDEF || lexem==CPP_IFNDEF) {
            genCppIfElseReference(1, &pos, UsageDefined);
            depth++;
        } else if (lexem == CPP_ENDIF) {
            depth--;
            genCppIfElseReference(-1, &pos, UsageUsed);
        } else if (lexem == CPP_ELIF) {
            genCppIfElseReference(0, &pos, UsageUsed);
            if (depth == 1 && !untilEnd) {
                log_debug("#elif ");
                return(UNTIL_ELIF);
            }
        } else if (lexem == CPP_ELSE) {
            genCppIfElseReference(0, &pos, UsageUsed);
            if (depth == 1 && !untilEnd) {
                log_debug("#else");
                return(UNTIL_ELSE);
            }
        }
    }
    log_debug("#endif");
    return(UNTIL_ENDIF);
endOfMacroArgument:	assert(0);
endOfFile:;
    if (options.taskRegime!=RegimeEditServer) {
        warningMessage(ERR_ST,"end of file in cpp conditional");
    }
    return(UNTIL_ENDIF);
}

static void execCppIf(int deleteSource) {
    int onElse;
    if (deleteSource==0) currentFile.ifDepth ++;
    else {
        onElse = cppDeleteUntilEndElse(false);
        if (onElse==UNTIL_ELSE) {
            /* #if #else */
            currentFile.ifDepth ++;
        } else if (onElse==UNTIL_ELIF) processIfDirective();
    }
}

static void processIfdefDirective(bool isIfdef) {
    Lexem lexem;
    int mm;
    Symbol pp, *memb;
    char *cc;
    Position pos;
    int deleteSrc;
    int l, v, len;
    UNUSED len; UNUSED v; UNUSED l;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

    //& GetLex(lexem); // Expanded
    lexem = getLex(exceptionHandler);

    cc = currentInput.currentLexemP;
    PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
    testCxrefCompletionId(&lexem,cc,&pos);

    if (! isIdentifierLexem(lexem)) return;

    fillSymbol(&pp, cc, cc, s_noPos);
    fillSymbolBits(&pp.bits, AccessDefault, TypeMacro, StorageNone);

    assert(options.taskRegime);
    mm = symbolTableIsMember(s_symbolTable, &pp, NULL, &memb);
    if (mm && memb->u.mbody==NULL) mm = 0;	// undefined macro
    if (mm) {
        addCxReference(memb, &pos, UsageUsed, noFileIndex, noFileIndex);
        if (isIfdef) {
            log_debug("#ifdef (true)");
            deleteSrc = 0;
        } else {
            log_debug("#ifndef (false)");
            deleteSrc = 1;
        }
    } else {
        if (isIfdef) {
            log_debug("#ifdef (false)");
            deleteSrc = 1;
        } else {
            log_debug("#ifndef (true)");
            deleteSrc = 0;
        }
    }
    execCppIf(deleteSrc);
    return;
endOfMacroArgument:	assert(0);
endOfFile:;
}

/* ********************************* #IF ************************** */

int cexp_yylex(void) {
    int l, par, res, mm;
    Lexem lexem;
    char *cc;
    Symbol dd, *memb;
    Position pos;
    int v, len;
    UNUSED len; UNUSED v;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

    lexem = yylex();
    if (isIdentifierLexem(lexem)) {
        // this is useless, as it would be set to 0 anyway
        lexem = cexpTranslateToken(CONSTANT, 0);
    } else if (lexem == CPP_DEFINED_OP) {
        lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);
        cc = currentInput.currentLexemP;
        PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        if (lexem == '(') {
            par = 1;
            lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);
            cc = currentInput.currentLexemP;
            PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        } else {
            par = 0;
        }

        if (!isIdentifierLexem(lexem))
            return(0);

        fillSymbol(&dd, cc, cc, s_noPos);
        fillSymbolBits(&dd.bits, AccessDefault, TypeMacro, StorageNone);

        log_debug("(%s)", dd.name);

        mm = symbolTableIsMember(s_symbolTable, &dd, NULL, &memb);
        if (mm && memb->u.mbody == NULL)
            mm = 0;   // undefined macro
        assert(options.taskRegime);
        if (mm)
            addCxReference(&dd, &pos, UsageUsed, noFileIndex, noFileIndex);

        /* following call sets uniyylval */
        res = cexpTranslateToken(CONSTANT, mm);
        if (par) {
            lexem = getNonBlankLexem(exceptionHandler, &pos, &l, &v, &len);
            PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
            if (lexem != ')' && options.taskRegime!=RegimeEditServer) {
                warningMessage(ERR_ST,"missing ')' after defined( ");
            }
        }
        lexem = res;
    } else {
        lexem = cexpTranslateToken(lexem, uniyylval->ast_integer.d);
    }
    return(lexem);
endOfMacroArgument:	assert(0);
endOfFile:;
    return(0);
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
    int l,ii;
    char *mname, *fname;
    Position pos;
    Symbol *pp;
    int v, len; UNUSED len; UNUSED v;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

    //& GetLex(lexem); // Expanded
    lexem = getLex(exceptionHandler);
    if (lexem == IDENTIFIER && strcmp(currentInput.currentLexemP, "once")==0) {
        char tmpBuff[TMP_BUFF_SIZE];
        PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        fname = simpleFileName(fileTable.tab[pos.file]->name);
        sprintf(tmpBuff, "PragmaOnce-%s", fname);
        PP_ALLOCC(mname, strlen(tmpBuff)+1, char);
        strcpy(mname, tmpBuff);

        PP_ALLOC(pp, Symbol);
        fillSymbol(pp, mname, mname, pos);
        fillSymbolBits(&pp->bits, AccessDefault, TypeMacro, StorageNone);

        symbolTableAdd(s_symbolTable,pp,&ii);
    }
    while (lexem != '\n') {
        PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        //& GetLex(lexem); // Expanded
        lexem = getLex(exceptionHandler);
    }
    PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
    return;
endOfMacroArgument:	assert(0);
endOfFile:;
}

/* ***************************************************************** */
/*                                 CPP                               */
/* ***************************************************************** */

#define AddHtmlCppReference(pos) {\
    if (options.taskRegime==RegimeHtmlGenerate && !options.htmlNoColors) {\
        addTrivialCxReference("_",TypeCppAny,StorageDefault,&pos,UsageUsed);\
    }\
}

static bool processPreprocessorConstruct(Lexem lexem) {
    int l;
    Position pos;
    int v, len; UNUSED len; UNUSED v;

    PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
    log_debug("processing cpp-construct '%s' ", s_tokenName[lexem]);
    switch (lexem) {
    case CPP_INCLUDE:
        processIncludeDirective(&pos);
        break;
    case CPP_DEFINE0:
        AddHtmlCppReference(pos);
        processDefineDirective(0);
        break;
    case CPP_DEFINE:
        AddHtmlCppReference(pos);
        processDefineDirective(1);
        break;
    case CPP_UNDEF:
        AddHtmlCppReference(pos);
        processUndefineDirective();
        break;
    case CPP_IFDEF:
        genCppIfElseReference(1, &pos, UsageDefined);
        processIfdefDirective(true);
        break;
    case CPP_IFNDEF:
        genCppIfElseReference(1, &pos, UsageDefined);
        processIfdefDirective(false);
        break;
    case CPP_IF:
        genCppIfElseReference(1, &pos, UsageDefined);
        processIfDirective();
        break;
    case CPP_ELIF:
        log_debug("#elif");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, &pos, UsageUsed);
            currentFile.ifDepth --;
            cppDeleteUntilEndElse(true);
        } else if (options.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"unmatched #elif");
        }
        break;
    case CPP_ELSE:
        log_debug("#else");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, &pos, UsageUsed);
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
            genCppIfElseReference(-1, &pos, UsageUsed);
        } else if (options.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"unmatched #endif");
        }
        break;
    case CPP_PRAGMA:
        log_debug("#pragma");
        AddHtmlCppReference(pos);
        processPragmaDirective();
        break;
    case CPP_LINE: {
        jmp_buf exceptionHandler;

        switch(setjmp(exceptionHandler)) {
        case END_OF_FILE_EXCEPTION:
            goto endOfFile;
        case END_OF_MACRO_ARGUMENT_EXCEPTION:
            goto endOfMacroArgument;
        }

        AddHtmlCppReference(pos);
        processLineDirective();

        lexem = getLex(exceptionHandler);
        while (lexem != '\n') {
            PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
            lexem = getLex(exceptionHandler);
        }
        PassLexem(currentInput.currentLexemP, lexem, l, v, pos, len, true);
        break;
    }
    default:
        log_fatal("Unknown lexem encountered");
    }
    return true;

endOfMacroArgument: assert(0);
endOfFile:
    return false;
}

/* ******************************************************************** */
/* *********************   MACRO CALL EXPANSION *********************** */
/* ******************************************************************** */

#define TestPPBufOverflow(bcc,buf,bsize) {\
    if (bcc >= buf+bsize) {\
        bsize += MACRO_UNIT_SIZE;\
        PP_REALLOCC(buf,bsize+MAX_LEXEM_SIZE,char,\
                        bsize+MAX_LEXEM_SIZE-MACRO_UNIT_SIZE);\
    }\
}
#define TestMBBufOverflow(bcc,len,buf2,bsize) {\
    while (bcc + len >= buf2 + bsize) {\
        bsize += MACRO_UNIT_SIZE;\
        MB_REALLOCC(buf2,bsize+MAX_LEXEM_SIZE,char,\
                    bsize+MAX_LEXEM_SIZE-MACRO_UNIT_SIZE);\
    }\
}

/* *********************************************************** */

static int cyclicCall(MacroBody *mb) {
    struct lexInput *ll;
    char *name;
    int i;
    name = mb->name;
/*fprintf(dumpOut,"testing '%s' against curr '%s'\n",name,cInput.macname);*/
    if (currentInput.macroName != NULL && strcmp(name,currentInput.macroName)==0) return(1);
    for(i=0; i<macroStackIndex; i++) {
        ll = &macroStack[i];
/*fprintf(dumpOut,"testing '%s' against '%s'\n",name,ll->macname);*/
        if (ll->macroName != NULL && strcmp(name,ll->macroName)==0) return(1);
    }
    return(0);
}


static void prependMacroInput(LexInput *argb) {
    assert(macroStackIndex < MACRO_STACK_SIZE-1);
    macroStack[macroStackIndex++] = currentInput;
    currentInput = *argb;
    currentInput.currentLexemP = currentInput.beginningOfBuffer;
    currentInput.inputType = INPUT_MACRO;
}


static void expandMacroArgument(LexInput *argb) {
    Symbol sd, *memb;
    char *buf, *previousLexem, *currentLexem, *bcc, *tbcc;
    int length, line, failedMacroExpansion;
    Lexem lexem;
    Position pos;
    int len, val;
    UNUSED val; UNUSED len;
    int bsize = MACRO_UNIT_SIZE;

    prependMacroInput(argb);

    currentInput.inputType = INPUT_MACRO_ARGUMENT;
    PP_ALLOCC(buf,bsize+MAX_LEXEM_SIZE,char);
    bcc = buf;

    for(;;) {
    nextLexem:
        lexem = getLexemSavePrevious(&previousLexem, NULL);
        if (lexem == -1)
            goto endOfMacroArgument;
        if (lexem == -2)
            goto endOfFile;

        currentLexem = currentInput.currentLexemP;
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        length = ((char*)currentInput.currentLexemP) - previousLexem;
        assert(length >= 0);
        memcpy(bcc, previousLexem, length);
        // a hack, it is copied, but bcc will be increased only if not
        // an expanding macro, this is because 'macroCallExpand' can
        // read new lexbuffer and destroy cInput, so copy it now.
        failedMacroExpansion = 0;
        if (lexem == IDENTIFIER) {
            fillSymbol(&sd, currentLexem, currentLexem, s_noPos);
            fillSymbolBits(&sd.bits, AccessDefault, TypeMacro, StorageNone);
            if (symbolTableIsMember(s_symbolTable, &sd, NULL, &memb)) {
                /* it is a macro, provide macro expansion */
                if (expandMacroCall(memb,&pos)) goto nextLexem;
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
    currentInput = macroStack[--macroStackIndex];
    PP_REALLOCC(buf, bcc-buf, char, bsize+MAX_LEXEM_SIZE);
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
    int line, val, nlex, len1, bsize;
    Lexem lexem;
    Position respos;
    Position pos; UNUSED pos;
    int len; UNUSED len;

    ncc = *ancc;
    lbcc = *albcc;
    bcc = *abcc;
    bsize = *absize;
    val=0; // compiler
    if (nextLexToken(&lbcc) == CPP_MAC_ARG) {
        bcc = lbcc;
        lexem = getLexToken(&lbcc);
        assert(lexem==CPP_MAC_ARG);
        PassLexem(lbcc, lexem, line, val, respos, len, false);
        cc = actArgs[val].beginningOfBuffer; ccfin = actArgs[val].endOfBuffer;
        lbcc = NULL;
        while (cc < ccfin) {
            cc0 = cc;
            lexem = getLexToken(&cc);
            PassLexem(cc, lexem, line, val, respos, len, false);
            lbcc = bcc;
            assert(cc>=cc0);
            memcpy(bcc, cc0, cc-cc0);
            bcc += cc-cc0;
            TestPPBufOverflow(bcc,buf,bsize);
        }
    }
    if (nextLexToken(&ncc) == CPP_MAC_ARG) {
        lexem = getLexToken(&ncc);
        PassLexem(ncc, lexem, line, val, pos, len, false);
        cc = actArgs[val].beginningOfBuffer; ccfin = actArgs[val].endOfBuffer;
    } else {
        cc = ncc;
        lexem = getLexToken(&ncc);
        PassLexem(ncc, lexem, line, val, pos, len, false);
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
            PassLexem(cc, lexem, line, val, respos, len, false);
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
                sprintf(bcc,"%d",val);
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
        PassLexem(cc, lexem, line, val, pos, len, false);
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
    int v,c,lv;
    Lexem lexem;
    Position pos; UNUSED pos;
    int len; UNUSED len;

    bcc = res;
    *bcc = 0;
    c=0; v=0;
    cc = lb->beginningOfBuffer;
    while (cc < lb->endOfBuffer) {
        lexem = getLexToken(&cc);
        lcc = cc;
        PassLexem(cc, lexem, lv, v, pos, len, c);
        if (isIdentifierLexem(lexem)) {
            sprintf(bcc, "%s", lcc);
            bcc+=strlen(bcc);
        } else if (lexem==STRING_LITERAL) {
            sprintf(bcc,"\"%s\"", lcc);
            bcc+=strlen(bcc);
        } else if (lexem==CONSTANT) {
            sprintf(bcc,"%d", v);
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
    int i,line,val,len,bsize;
    Lexem lexem;
    Position hpos;
    char *buf,*buf2;
    Position pos; UNUSED pos;
    int lexlen; UNUSED lexlen;

    val=0; //compiler
    /* first make ## collations */
    bsize = MACRO_UNIT_SIZE;
    PP_ALLOCC(buf,bsize+MAX_LEXEM_SIZE, char);
    cc = mb->body;
    cfin = mb->body + mb->size;
    bcc = buf;
    lbcc = NULL;
    while (cc < cfin) {
        cc0 = cc;
        lexem = getLexToken(&cc);
        PassLexem(cc, lexem, line, val, pos, lexlen, false);
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
    PP_REALLOCC(buf,bcc-buf,char,bsize+MAX_LEXEM_SIZE);


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
        PassLexem(cc, lexem, line, val, hpos, lexlen, false);
        if (lexem == CPP_MAC_ARG) {
            len = actArgs[val].endOfBuffer - actArgs[val].beginningOfBuffer;
            TestMBBufOverflow(bcc,len,buf2,bsize);
            memcpy(bcc, actArgs[val].beginningOfBuffer, len);
            bcc += len;
        } else if (lexem=='#' && cc<cfin && nextLexToken(&cc)==CPP_MAC_ARG) {
            lexem = getLexToken(&cc);
            PassLexem(cc, lexem, line, val, pos, lexlen, false);
            assert(lexem == CPP_MAC_ARG);
            putLexToken(STRING_LITERAL, &bcc);
            TestMBBufOverflow(bcc,MACRO_UNIT_SIZE,buf2,bsize);
            macArgsToString(bcc, &actArgs[val]);
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
#define GetLexASkippingLines(lexem, previousLexem, line, val, pos, len) \
    {                                                                   \
        lexem = getLexemSavePrevious(&previousLexem, NULL);             \
        if (lexem == -1)                                                \
            goto endOfMacroArgument;                                    \
        if (lexem == -2)                                                \
            goto endOfFile;                                             \
        while (lexem == LINE_TOKEN || lexem == '\n') {                  \
            PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, \
                    macroStackIndex == 0);                              \
            lexem = getLexemSavePrevious(&previousLexem, NULL);         \
            if (lexem == -1)                                            \
                goto endOfMacroArgument;                                \
            if (lexem == -2)                                            \
                goto endOfFile;                                         \
        }                                                               \
    }

static void getActualMacroArgument(
    char *previousLexem,
    Lexem *out_lexem,
    Position *mpos,
    Position **parpos1,
    Position **parpos2,
    LexInput *actArg,
    MacroBody *macroBody,
    int actualArgumentIndex
) {
    char *buf,*bcc;
    int line,poffset;
    int bufsize,depth;
    Lexem lexem;
    Position pos; UNUSED pos;
    int val,len; UNUSED len; UNUSED val;

    lexem = *out_lexem;
    bufsize = MACRO_ARG_UNIT_SIZE;
    depth = 0;
    PP_ALLOCC(buf, bufsize+MAX_LEXEM_SIZE, char);
    bcc = buf;
    /* if lastArgument, collect everything there */
    poffset = 0;
    while (((lexem != ',' || actualArgumentIndex+1==macroBody->argn) && lexem != ')') || depth > 0) {
        // The following should be equivalent to the loop condition:
        //& if (lexem == ')' && depth <= 0) break;
        //& if (lexem == ',' && depth <= 0 && ! lastArgument) break;
        if (lexem == '(') depth ++;
        if (lexem == ')') depth --;
        for(;previousLexem < currentInput.currentLexemP; previousLexem++, bcc++)
            *bcc = *previousLexem;
        if (bcc-buf >= bufsize) {
            bufsize += MACRO_ARG_UNIT_SIZE;
            PP_REALLOCC(buf, bufsize+MAX_LEXEM_SIZE, char,
                    bufsize+MAX_LEXEM_SIZE-MACRO_ARG_UNIT_SIZE);
        }
        GetLexASkippingLines(lexem, previousLexem, line, val, pos, len);

        PassLexem(currentInput.currentLexemP, lexem, line, val, (**parpos2), len,
                macroStackIndex == 0);
        if ((lexem == ',' || lexem == ')') && depth == 0) {
            poffset ++;
            handleMacroUsageParameterPositions(actualArgumentIndex+poffset, mpos, *parpos1, *parpos2, 0);
            **parpos1= **parpos2;
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
    PP_REALLOCC(buf, bcc-buf, char, bufsize+MAX_LEXEM_SIZE);
    fillLexInput(actArg,buf,bcc,buf,currentInput.macroName,INPUT_NORMAL);
    *out_lexem = lexem;
    return;
}

static struct lexInput *getActualMacroArguments(MacroBody *mb, Position *mpos,
                                                Position *lparpos) {
    char *previousLexem;
    Lexem lexem;
    int line;
    Position pos, ppb1, ppb2, *parpos1, *parpos2;
    int actArgi = 0;
    struct lexInput *actArgs;
    int val,len; UNUSED len; UNUSED val;

    ppb1 = *lparpos;
    ppb2 = *lparpos;
    parpos1 = &ppb1;
    parpos2 = &ppb2;
    PP_ALLOCC(actArgs,mb->argn,struct lexInput);
    GetLexASkippingLines(lexem, previousLexem, line, val, pos, len);
    PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
    if (lexem == ')') {
        *parpos2 = pos;
        handleMacroUsageParameterPositions(0, mpos, parpos1, parpos2, 1);
    } else {
        for(;;) {
            getActualMacroArgument(previousLexem, &lexem, mpos, &parpos1, &parpos2,
                                &actArgs[actArgi], mb, actArgi);
            actArgi ++ ;
            if (lexem != ',' || actArgi >= mb->argn) break;
            GetLexASkippingLines(lexem, previousLexem, line, val, pos, len);
            PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len,
                    macroStackIndex == 0);
        }
    }
    if (actArgi!=0) {
        handleMacroUsageParameterPositions(actArgi, mpos, parpos1, parpos2, 1);
    }
    /* fill mising arguments */
    for(;actArgi < mb->argn; actArgi++) {
        fillLexInput(&actArgs[actArgi], NULL, NULL, NULL, NULL,INPUT_NORMAL);
    }
    return(actArgs);
endOfMacroArgument:	assert(0);
endOfFile:
    assert(options.taskRegime);
    if (options.taskRegime!=RegimeEditServer) {
        warningMessage(ERR_ST,"[getActualMacroArguments] unterminated macro call");
    }
    return(NULL);
}

/* **************************************************************** */

static void addMacroBaseUsageRef(Symbol *mdef) {
    int rr;
    SymbolReferenceItem ppp, *memb;
    Reference *r;
    Position basePos;

    fillPosition(&basePos, s_input_file_number, 0, 0);
    fillSymbolRefItem(&ppp, mdef->linkName,
                                cxFileHashNumber(mdef->linkName), // useless, put 0
                                noFileIndex, noFileIndex);
    fillSymbolRefItemBits(&ppp.b,TypeMacro, StorageDefault, ScopeGlobal,
                           mdef->bits.access, CategoryGlobal, 0);
    rr = refTabIsMember(&referenceTable, &ppp, NULL, &memb);
    r = NULL;
    if (rr) {
        // this is optimization to avoid multiple base references
        for(r=memb->refs; r!=NULL; r=r->next) {
            if (r->usage.base == UsageMacroBaseFileUsage) break;
        }
    }
    if (rr==0 || r==NULL) {
        addCxReference(mdef,&basePos,UsageMacroBaseFileUsage,
                              noFileIndex, noFileIndex);
    }
}


static int expandMacroCall(Symbol *mdef, Position *mpos) {
    Lexem lexem;
    int line;
    char *previousLexem,*freeBase;
    Position lparpos;
    LexInput *actualArguments, macroBody;
    MacroBody *mb;
    Position pos; UNUSED pos;
    int val,len; UNUSED len; UNUSED val;

    previousLexem = currentInput.currentLexemP;
    mb = mdef->u.mbody;
    if (mb == NULL) return(0);	/* !!!!!         tricky,  undefined macro */
    if (macroStackIndex == 0) { /* call from source, init mem */
        MB_INIT();
    }
    log_trace("try to expand macro '%s'", mb->name);
    if (cyclicCall(mb))
        return(0);
    PP_ALLOCC(freeBase,0,char);
    if (mb->argn >= 0) {
        GetLexASkippingLines(lexem, previousLexem, line, val, pos, len);
        if (lexem != '(') {
            currentInput.currentLexemP = previousLexem;		/* unget lexem */
            return(0);
        }
        PassLexem(currentInput.currentLexemP, lexem, line, val, lparpos, len,
                           macroStackIndex == 0);
        actualArguments = getActualMacroArguments(mb, mpos, &lparpos);
    } else {
        actualArguments = NULL;
    }
    assert(options.taskRegime);
    addCxReference(mdef, mpos, UsageUsed, noFileIndex, noFileIndex);
    if (options.taskRegime == RegimeXref)
        addMacroBaseUsageRef(mdef);
    log_trace("create macro body '%s'", mb->name);
    createMacroBody(&macroBody,mb,actualArguments,mb->argn);
    prependMacroInput(&macroBody);
    log_trace("expanding macro '%s'", mb->name);
    PP_FREE_UNTIL(freeBase);
    return(1);
endOfMacroArgument:
    /* unterminated macro call in argument */
    /* TODO unread readed argument */
    currentInput.currentLexemP = previousLexem;
    PP_FREE_UNTIL(freeBase);
    return(0);
endOfFile:
    assert(options.taskRegime);
    if (options.taskRegime!=RegimeEditServer) {
        warningMessage(ERR_ST,"[macroCallExpand] unterminated macro call");
    }
    currentInput.currentLexemP = previousLexem;
    PP_FREE_UNTIL(freeBase);
    return(0);
}

#ifdef DEBUG
int lexBufDump(LexemBuffer *lb) {
    char *cc;
    int c,lv;
    Lexem lexem;
    Position pos; UNUSED pos;
    int v,len; UNUSED len; UNUSED v;

    c=0;
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
        } else if (s_tokenName[lexem]==NULL){
            fprintf(dumpOut,"?%d? ",lexem);fflush(dumpOut);
        } else {
            fprintf(dumpOut,"%s ",s_tokenName[lexem]);fflush(dumpOut);
        }
        PassLexem(cc, lexem, lv, v, pos, len, c);
    }
    fprintf(dumpOut,"lexbufdump [stop]\n");fflush(dumpOut);
    return(0);
}
#endif

/* ************************************************************** */
/*                   caching of input                             */
/* ************************************************************** */
int cachedInputPass(int cpoint, char **cfrom) {
    Lexem lexem;
    int line, res;
    Position pos;
    unsigned lexemLength;
    char *previousLexem, *cto, *ccc;
    int val, len; UNUSED len; UNUSED val;

    assert(cpoint > 0);
    cto = s_cache.cp[cpoint].lbcc;
    ccc = *cfrom;
    res = 1;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }


    while (ccc < cto) {
        lexem = getLexemSavePrevious(&previousLexem, exceptionHandler);
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, true);
        lexemLength = currentInput.currentLexemP-previousLexem;
        assert(lexemLength >= 0);
        if (memcmp(previousLexem, ccc, lexemLength)) {
            currentInput.currentLexemP = previousLexem;			/* unget last lexem */
            res = 0;
            break;
        }
        if (isIdentifierLexem(lexem) || isPreprocessorToken(lexem)) {
            if (onSameLine(pos, s_cxRefPos)) {
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
    return(res);
endOfMacroArgument:
    assert(0);
    return -1;                  /* The assert should protect this from executing */
}

/* ***************************************************************** */
/*                                 yylex                             */
/* ***************************************************************** */

static char charText[2]={0,0};
static char constant[50];

#define CHECK_ID_FOR_KEYWORD(sd,idposa) {\
    if (sd->bits.symbolType == TypeKeyword) {\
        SET_IDENTIFIER_YYLVAL(sd->name, sd, *idposa);\
        if (options.taskRegime==RegimeHtmlGenerate && !options.htmlNoColors) {\
            char ttt[TMP_STRING_SIZE];\
            sprintf(ttt,"%s-%x", sd->name, idposa->file);\
            addTrivialCxReference(ttt, TypeKeyword,StorageDefault, idposa, UsageUsed);\
            /*&addCxReference(sd, idposa, UsageUsed, noFileIndex, noFileIndex);&*/\
        }\
        return(sd->u.keyword);\
    }\
}

static int processCIdent(unsigned hashval, char *id, Position *idposa) {
    Symbol *sd,*memb;
    memb = NULL;
/*fprintf(dumpOut,"looking for %s in %d\n",id,s_symTab);*/
    for(sd=s_symbolTable->tab[hashval]; sd!=NULL; sd=sd->next) {
        if (strcmp(sd->name, id) == 0) {
            if (memb == NULL) memb = sd;
            CHECK_ID_FOR_KEYWORD(sd, idposa);
            if (sd->bits.symbolType == TypeDefinedOp && s_ifEvaluation) {
                return(CPP_DEFINED_OP);
            }
            if (sd->bits.symbolType == TypeDefault) {
                SET_IDENTIFIER_YYLVAL(sd->name, sd, *idposa);
                if (sd->bits.storage == StorageTypedef) {
                    return(TYPE_NAME);
                } else {
                    return(IDENTIFIER);
                }
            }
        }
    }
    if (memb == NULL) id = stackMemoryPushString(id);
    else id = memb->name;
    SET_IDENTIFIER_YYLVAL(id, memb, *idposa);
    return(IDENTIFIER);
}


static int processJavaIdent(unsigned hashval, char *id, Position *idposa) {
    Symbol *sd,*memb;
    memb = NULL;
/*fprintf(dumpOut,"looking for %s in %d\n",id,s_symTab);*/
    for(sd=s_symbolTable->tab[hashval]; sd!=NULL; sd=sd->next) {
        if (strcmp(sd->name, id) == 0) {
            if (memb == NULL) memb = sd;
            CHECK_ID_FOR_KEYWORD(sd, idposa);
            if (sd->bits.symbolType == TypeDefault) {
                SET_IDENTIFIER_YYLVAL(sd->name, sd, *idposa);
                return(IDENTIFIER);
            }
        }
    }
    if (memb == NULL) id = stackMemoryPushString(id);
    else id = memb->name;
    SET_IDENTIFIER_YYLVAL(id, memb, *idposa);
    return(IDENTIFIER);
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
                    assert(s_javaStat->thisClass->u.s);
                    strcpy(s_cps.currentClassAnswer, s_javaStat->thisClass->linkName);
                    if (s_javaStat->thisClass->u.s->super!=NULL) {
                        assert(s_javaStat->thisClass->u.s->super->d);
                        strcpy(s_cps.currentSuperClassAnswer, s_javaStat->thisClass->u.s->super->d->linkName);
                    }
                }
            }
        }
        s_cp.parserPassedMarker = 1;
    }
}


#define SET_POSITION_YYLVAL(pos, len) {\
    uniyylval->ast_position.d = pos;\
    uniyylval->ast_position.b = pos;\
    uniyylval->ast_position.e = pos;\
    uniyylval->ast_position.e.col += len;\
}

#define SET_INTEGER_YYLVAL(val, pos, len) {\
        uniyylval->ast_integer.d = val;\
        uniyylval->ast_integer.b = pos;\
        uniyylval->ast_integer.e = pos;\
        uniyylval->ast_integer.e.col += len;\
}

int yylex(void) {
    Lexem lexem;
    Position pos, idpos;
    char *previousLexem;
    int line, val;
    int len = 0;

    jmp_buf exceptionHandler;
    switch(setjmp(exceptionHandler)) {
    case END_OF_FILE_EXCEPTION:
        goto endOfFile;
    case END_OF_MACRO_ARGUMENT_EXCEPTION:
        goto endOfMacroArgument;
    }

 nextYylex:
    lexem = getLexemSavePrevious(&previousLexem, exceptionHandler);

 contYylex:
    if (lexem < 256) {
        if (lexem == '\n') {
            if (s_ifEvaluation) {
                currentInput.currentLexemP = previousLexem;
            } else {
                PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len,
                                   macroStackIndex == 0);
                for(;;) {
                    //& GetLex(lexem); // Expanded
                    lexem = getLex(exceptionHandler);

                    if (!isPreprocessorToken(lexem))
                        goto contYylex;
                    if (!processPreprocessorConstruct(lexem))
                        goto endOfFile;
                }
            }
        } else {
            PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len,
                               macroStackIndex == 0);
            SET_POSITION_YYLVAL(pos, s_tokenLength[lexem]);
        }
        yytext = charText;
        charText[0] = lexem;
        goto finish;
    }
    if (lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND) {
        char *id;
        unsigned hash;
        Symbol symbol, *memberP;

        id = yytext = currentInput.currentLexemP;
        PassLexem(currentInput.currentLexemP, lexem, line, val, idpos, len, macroStackIndex == 0);
        assert(options.taskRegime);
        if (options.taskRegime == RegimeEditServer) {
//			???????????? isn't this useless
            testCxrefCompletionId(&lexem,yytext,&idpos);
        }
        log_trace("id %s position %d %d %d",yytext,idpos.file,idpos.line,idpos.col);
        fillSymbol(&symbol, yytext, yytext, s_noPos);
        fillSymbolBits(&symbol.bits, AccessDefault, TypeMacro, StorageNone);

        if ((!LANGUAGE(LANG_JAVA))
            && lexem!=IDENT_NO_CPP_EXPAND
            && symbolTableIsMember(s_symbolTable, &symbol, NULL, &memberP)) {
            // following is because the macro check can read new lexBuf,
            // so id would be destroyed
            //&assert(strcmp(id,memberP->name)==0);
            id = memberP->name;
            if (expandMacroCall(memberP,&idpos)) goto nextYylex;
        }
        hash = hashFun(id) % s_symbolTable->size;
        if(LANGUAGE(LANG_C)||LANGUAGE(LANG_YACC)) lexem=processCIdent(hash,id,&idpos);
        else if (LANGUAGE(LANG_JAVA)) lexem = processJavaIdent(hash, id, &idpos);
        else assert(0);
        pos = idpos;            /* To simplify debug - pos is always current at finish: */
        goto finish;
    }
    if (lexem == OL_MARKER_TOKEN) {
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        actionOnBlockMarker();
        goto nextYylex;
    }
    if (lexem < MULTI_TOKENS_START) {
        yytext = s_tokenName[lexem];
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, s_tokenLength[lexem]);
        goto finish;
    }
    if (lexem == LINE_TOKEN) {
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        goto nextYylex;
    }
    if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
        val=0;//compiler
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        sprintf(constant,"%d",val);
        SET_INTEGER_YYLVAL(val, pos, len);
        yytext = constant;
        goto finish;
    }
    if (lexem == FLOAT_CONSTANT || lexem == DOUBLE_CONSTANT) {
        yytext = "'fltp constant'";
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, len);
        goto finish;
    }
    if (lexem == STRING_LITERAL) {
        yytext = currentInput.currentLexemP;
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, strlen(yytext));
        goto finish;
    }
    if (lexem == CHAR_LITERAL) {
        val=0;//compiler
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        sprintf(constant,"'%c'",val);
        SET_INTEGER_YYLVAL(val, pos, len);
        yytext = constant;
        goto finish;
    }
    assert(options.taskRegime);
    if (options.taskRegime == RegimeEditServer) {
        yytext = currentInput.currentLexemP;
        PassLexem(currentInput.currentLexemP, lexem, line, val, pos, len, macroStackIndex == 0);
        if (lexem == IDENT_TO_COMPLETE) {
            testCxrefCompletionId(&lexem,yytext,&pos);
            while (includeStackPointer != 0) popInclude();
            /* while (getLexBuf(&cFile.lb)) cFile.lb.cc = cFile.lb.fin;*/
            goto endOfFile;
        }
    }
/*fprintf(stderr,"unknown lexem %d\n",lex);*/
    goto endOfFile;

 finish:
    log_trace("!'%s'(%d)", yytext, cxMemory->index);
    s_lastReturnedLexem = lexem;
    return(lexem);

 endOfMacroArgument:
    assert(0);

 endOfFile:
    if ((!LANGUAGE(LANG_JAVA)) && includeStackPointer != 0) {
        popInclude();
        placeCachePoint(true);
        goto nextYylex;
    }
    /* add the test whether in COMPLETION, communication string found */
    return(0);
}	/* end of yylex() */

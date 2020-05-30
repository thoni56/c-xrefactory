#include "yylex.h"

#include "commons.h"
#include "lexem.h"
#include "lex.h"
#include "lexmac.h"
#include "globals.h"
#include "caching.h"
#include "extract.h"
#include "misc.h"
#include "semact.h"
#include "cxref.h"
#include "cxfile.h"
#include "jslsemact.h"
#include "editor.h"
#include "reftab.h"
#include "characterbuffer.h"
#include "symbol.h"
#include "jsemact.h"

#include "c_parser.x"
#include "cexp_parser.x"
#include "yacc_parser.x"
#include "java_parser.x"

#include "parsers.h"
#include "memory.h"
#include "protocol.h"

#include "hash.h"
#include "log.h"
#include "utils.h"
#include "macroargumenttable.h"
#include "filedescriptor.h"
#include "fileio.h"


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

static char ppMemory[SIZE_ppMemory];
static int ppMemoryi=0;

int macroStackIndex=0;
static S_lexInput macroStack[MACRO_STACK_SIZE];

S_lexInput cInput;


static bool isIdentifierLexem(Lexem lex) {
    return lex==IDENTIFIER || lex==IDENT_NO_CPP_EXPAND  || lex==IDENT_TO_COMPLETE;
}


static int expandMacroCall(Symbol *mdef, Position *mpos);


/* ************************************************************ */

void initAllInputs(void) {
    mbMemoryi=0;
    inStacki=0;
    macroStackIndex=0;
    s_ifEvaluation = 0;
    s_cxRefFlag = 0;
    macroArgumentTableNoAllocInit(&s_macroArgumentTable, MAX_MACRO_ARGS);
    ppMemoryi=0;
    s_olstring[0]=0;
    s_olstringFound = 0;
    s_olstringServed = 0;
    s_olstringInMbody = NULL;
    s_upLevelFunctionCompletionType = NULL;
    s_structRecordCompletionType = NULL;
}


static void fillMacroArgTabElem(S_macroArgumentTableElement *macroArgTabElem, char *name, char *linkName,
                                int order) {
    macroArgTabElem->name = name;
    macroArgTabElem->linkName = linkName;
    macroArgTabElem->order = order;
}

void fillLexInput(S_lexInput *lexInput, char *currentLexem, char *endOfBuffer,
                  char *beginningOfBuffer, char *macroName, InputType margExpFlag) {
    lexInput->currentLexem = currentLexem;
    lexInput->endOfBuffer = endOfBuffer;
    lexInput->beginningOfBuffer = beginningOfBuffer;
    lexInput->macroName = macroName;
    lexInput->inputType = margExpFlag;
}

static void setCacheConsistency(void) {
    s_cache.cc = cInput.currentLexem;
}
static void setCFileConsistency(void) {
    currentFile.lexBuffer.next = cInput.currentLexem;
}
static void setCInputConsistency(void) {
    fillLexInput(&cInput, currentFile.lexBuffer.next, currentFile.lexBuffer.end,
                 currentFile.lexBuffer.chars, NULL, INPUT_NORMAL);
}

char *placeIdent(void) {
    static char tt[2*MAX_HTML_REF_LEN];
    char fn[MAX_FILE_NAME_SIZE];
    char mm[MAX_HTML_REF_LEN];
    int s;
    if (currentFile.fileName!=NULL) {
        if (s_opt.xref2 && s_opt.taskRegime!=RegimeEditServer) {
            strcpy(fn, getRealFileNameStatic(normalizeFileName(currentFile.fileName, s_cwd)));
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
    if (s_opt.debug) {
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
    normalizedFileName = normalizeFileName(name,s_cwd);

    /* Does it already exist? */
    if (fileTabExists(&s_fileTab, normalizedFileName))
        return fileTabLookup(&s_fileTab, normalizedFileName);

    /* If not, add it, but then we need a filename and a fileitem in FT-memory  */
    len = strlen(normalizedFileName);
    FT_ALLOCC(fname, len+1, char);
    strcpy(fname, normalizedFileName);

    FT_ALLOC(createdFileItem, S_fileItem);
    fillFileItem(createdFileItem, fname, false);

    fileIndex = fileTabAdd(&s_fileTab, createdFileItem);
    checkFileModifiedTime(fileIndex); // it was too slow on load ?

    return fileIndex;
}

static int getOrCreateFileNumberFor(char *fileName) {
    int fileNumber, cxloading;
    bool existed = false;

    if (fileName==NULL) {
        fileNumber = s_noneFileIndex;
    } else {
        existed = fileTabExists(&s_fileTab, fileName);
        fileNumber = addFileTabItem(fileName);
        checkFileModifiedTime(fileNumber);
        cxloading = s_fileTab.tab[fileNumber]->b.cxLoading;
        if (!existed) {
            cxloading = 1;
        } else if (s_opt.update==UP_FAST_UPDATE) {
            if (s_fileTab.tab[fileNumber]->b.scheduledToProcess) {
                // references from headers are not loaded on fast update !
                cxloading = 1;
            }
        } else if (s_opt.update==UP_FULL_UPDATE) {
            if (s_fileTab.tab[fileNumber]->b.scheduledToUpdate) {
                cxloading = 1;
            }
        } else {
            cxloading = 1;
        }
        if (s_fileTab.tab[fileNumber]->b.cxSaved==1 && ! s_opt.multiHeadRefsCare) {
            /* if multihead references care, load include refs each time */
            cxloading = 0;
        }
        if (LANGUAGE(LANG_JAVA)) {
            if (s_jsl!=NULL || s_javaPreScanOnly) {
                // do not load (and save) references from jsl loaded files
                // nor during prescanning
                cxloading = s_fileTab.tab[fileNumber]->b.cxLoading;
            }
        }
        s_fileTab.tab[fileNumber]->b.cxLoading = cxloading;
    }
    return fileNumber;
}

/* ***************************************************************** */
/*                             Init Reading                          */
/* ***************************************************************** */

void ppMemInit(void) {
    PP_ALLOCC(s_macroArgumentTable.tab, MAX_MACRO_ARGS, S_macroArgumentTableElement *);
    macroArgumentTableNoAllocInit(&s_macroArgumentTable, MAX_MACRO_ARGS);
    ppMemoryi = 0;
}

// it is supposed that one of file or buffer is NULL
void initInput(FILE *file, S_editorBuffer *editorBuffer, char *prefix, char *fileName) {
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
    currentFile.lexBuffer.buffer.fileNumber = getOrCreateFileNumberFor(fileName);
    setCInputConsistency();
    s_ifEvaluation = false;				/* TODO: WTF??? */
}

/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

/* maybe too time-consuming, when lex is known */
/* should be broken into parts */
#define PassLex(input, lexem, lineval, val, pos, length, linecount) { \
        if (lexem > MULTI_TOKENS_START) {                               \
            if (isIdentifierLexem(lexem)){                              \
                input = strchr(input, '\0')+1;                          \
                GetLexPosition((pos),input);                            \
            } else if (lexem == STRING_LITERAL) {                       \
                input = strchr(input, '\0')+1;                          \
                GetLexPosition((pos),input);                            \
            } else if (lexem == LINE_TOK) {                             \
                GetLexToken(lineval,input);                             \
                if (linecount) {                                        \
                    traceNewline(lineval);                              \
                    currentFile.lineNumber += lineval;                  \
                }                                                       \
            } else if (lexem == CONSTANT || lexem == LONG_CONSTANT) {   \
                GetLexInt(val,input);                                   \
                GetLexPosition((pos),input);                            \
                GetLexInt(length,input);                                \
            } else if (lexem == DOUBLE_CONSTANT || lexem == FLOAT_CONSTANT) { \
                GetLexPosition((pos),input);                            \
                GetLexInt(length,input);                                \
            } else if (lexem == CPP_MAC_ARG) {                          \
                GetLexInt(val,input);                                   \
                GetLexPosition((pos),input);                            \
            } else if (lexem == CHAR_LITERAL) {                         \
                GetLexInt(val,input);                                   \
                GetLexPosition((pos),input);                            \
                GetLexInt(length,input);                                \
            }                                                           \
        } else if (isPreprocessorToken(lexem)) {                        \
            GetLexPosition((pos),input);                                \
        } else if (lexem == '\n' && (linecount)) {                      \
            GetLexPosition((pos),input);                                \
            traceNewline(1);                                            \
            currentFile.lineNumber ++;                                  \
        } else {                                                        \
            GetLexPosition((pos),input);                                \
        }                                                               \
    }

/* getLexA() - a functional facsimile to the old, now removed, macro GetLexA()
 * Returns either of:
 * the found lexem
 * -1 for end of macro argument
 * -2 for end of file
*/
static Lexem getLexA(char **previousLexem) {
    Lexem lexem;

    while (cInput.currentLexem >= cInput.endOfBuffer) {
        InputType inputType;
        inputType = cInput.inputType;
        if (macroStackIndex > 0) {
            if (inputType == INPUT_MACRO_ARGUMENT)
                return -1; //goto endOfMacArg;
            MB_FREE_UNTIL(cInput.beginningOfBuffer);
            cInput = macroStack[--macroStackIndex];
        } else if (inputType == INPUT_NORMAL) {
            setCFileConsistency();
            if (!getLexem(&currentFile.lexBuffer))
                return -2; //goto endOfFile;
            setCInputConsistency();
        } else {
            s_cache.cc = s_cache.cfin = NULL;
            cacheInput();
            s_cache.lexcc = currentFile.lexBuffer.next;
            setCInputConsistency();
        }
        *previousLexem = cInput.currentLexem;
    }
    *previousLexem = cInput.currentLexem;
    GetLexToken(lexem, cInput.currentLexem);
    return lexem;
}


#define GetLex(lexem) {                                             \
    char *previousLexem;                                            \
    UNUSED previousLexem;                                           \
    lexem = getLexA(&previousLexem);                                \
    if (lexem == -1) goto endOfMacArg;                              \
    if (lexem == -2) goto endOfFile;                                \
}

/* Attempt to re-create a function that does what GetLex() macro does: */
static int getLex(void) {
    char *previousLexem;
    Lexem lexem = getLexA(&previousLexem);
    return lexem; // -1 if endOfMacroArgument, -2 if endOfFile
}


/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

static void testCxrefCompletionId(Lexem *out_lexem, char *idd, Position *pos) {
    Lexem lexem;

    lexem = *out_lexem;
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime == RegimeEditServer) {
        if (lexem==IDENT_TO_COMPLETE) {
            s_cache.activeCache = 0;
            s_olstringServed = 1;
            if (s_language == LANG_JAVA) {
                makeJavaCompletions(idd, strlen(idd), pos);
            }
            else if (s_language == LANG_YACC) {
                makeYaccCompletions(idd, strlen(idd), pos);
            }
#			ifdef CCC_ALLOWED
            else if (s_language == LAN_CCC) {
                makeCccCompletions(idd, strlen(idd), pos);
            }
#			endif
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

static void processLine(void) {
    Lexem lexem;
    int l,v=0,len;
    Position pos;

    //& GetLex(lexem);
    lexem = getLex();
    if (lexem == -1) goto endOfMacArg;
    if (lexem == -2) goto endOfFile;

    PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
    if (lexem != CONSTANT) return;

    //& GetLex(lexem);
    lexem = getLex();
    if (lexem == -1) goto endOfMacArg;
    if (lexem == -2) goto endOfFile;

    PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
    return;
endOfMacArg:	assert(0);
endOfFile:;
}

/* ********************************* #INCLUDE ********************** */

static void fillIncludeSymbolItem(Symbol *ss, int filenum, Position *pos){
    // should be different for HTML to be beatiful, however,
    // all includes needs to be in the same cxfile, because of
    // -update. On the other hand in HTML I wish them to split
    // by first letter of file name.
    fillSymbol(ss, LINK_NAME_INCLUDE_REFS, LINK_NAME_INCLUDE_REFS, *pos);
    fillSymbolBits(&ss->bits, ACCESS_DEFAULT, TypeCppInclude, StorageDefault);
}


void addThisFileDefineIncludeReference(int filenum) {
    Position dpos;
    Symbol        ss;
    fillPosition(&dpos, filenum, 1, 0);
    fillIncludeSymbolItem( &ss,filenum, &dpos);
//&fprintf(dumpOut,"adding reference on file %d==%s\n",filenum, s_fileTab.tab[filenum]->name);
    addCxReference(&ss, &dpos, UsageDefined, filenum, filenum);
}

void addIncludeReference(int filenum, Position *pos) {
    Symbol        ss;
//&fprintf(dumpOut,"adding reference on file %d==%s\n",filenum, s_fileTab.tab[filenum]->name);
    fillIncludeSymbolItem( &ss, filenum, pos);
    addCxReference(&ss, pos, UsageUsed, filenum, filenum);
}

static void addIncludeReferences(int filenum, Position *pos) {
    addIncludeReference(filenum, pos);
    addThisFileDefineIncludeReference(filenum);
}

void pushNewInclude(FILE *f, S_editorBuffer *buffer, char *name, char *prepend) {
    if (cInput.inputType == INPUT_CACHE) {
        setCacheConsistency();
    } else {
        setCFileConsistency();
    }
    inStack[inStacki] = currentFile;		/* buffers are copied !!!!!!, burk */
    if (inStacki+1 >= INCLUDE_STACK_SIZE) {
        fatalError(ERR_ST,"too deep nesting in includes", XREF_EXIT_ERR);
    }
    inStacki ++;
    initInput(f, buffer, prepend, name);
    cacheInclude(currentFile.lexBuffer.buffer.fileNumber);
}

void popInclude(void) {
    assert(s_fileTab.tab[currentFile.lexBuffer.buffer.fileNumber]);
    if (s_fileTab.tab[currentFile.lexBuffer.buffer.fileNumber]->b.cxLoading) {
        s_fileTab.tab[currentFile.lexBuffer.buffer.fileNumber]->b.cxLoaded = true;
    }
    closeCharacterBuffer(&currentFile.lexBuffer.buffer);
    if (inStacki != 0) {
        currentFile = inStack[--inStacki];	/* buffers are copied !!!!!!, burk */
        if (inStacki == 0 && s_cache.cc!=NULL) {
            fillLexInput(&cInput, s_cache.cc, s_cache.cfin, s_cache.lb, NULL, INPUT_CACHE);
        } else {
            setCInputConsistency();
        }
    }
}

static FILE *openInclude(char includeType, char *name, char **fileName) {
    S_editorBuffer  *er;
    FILE			*r;
    S_stringList    *ll;
    char            wcp[MAX_OPTION_LEN];
    char            nn[MAX_FILE_NAME_SIZE];
    char            rdir[MAX_FILE_NAME_SIZE];
    char            *nnn;
    int             nnlen,dlen,nmlen;

    er = NULL; r = NULL;
    nmlen = strlen(name);
    extractPathInto(currentFile.fileName, rdir);
    if (includeType!='<') {
        strcpy(nn, normalizeFileName(name, rdir));
        log_trace("try to open %s", nn);
        er = editorFindFile(nn);
        if (er==NULL) r = openFile(nn,"r");
    }
    for (ll=s_opt.includeDirs; ll!=NULL && er==NULL && r==NULL; ll=ll->next) {
        strcpy(nn, normalizeFileName(ll->d, rdir));
        expandWildcardsInOnePath(nn, wcp, MAX_OPTION_LEN);
        JavaMapOnPaths(wcp, {
            strcpy(nn, currentPath);
            dlen = strlen(nn);
            if (dlen>0 && nn[dlen-1]!=FILE_PATH_SEPARATOR) {
                nn[dlen] = FILE_PATH_SEPARATOR;
                dlen++;
            }
            strcpy(nn+dlen, name);
            nnlen = dlen+nmlen;
            nn[nnlen]=0;
            log_trace("try to open <%s>", nn);
            er = editorFindFile(nn);
            if (er==NULL) r = openFile(nn,"r");
            if (er!=NULL || r!=NULL) goto found;
        });
    }
    if (er==NULL && r==NULL) return(NULL);
 found:
    nnn = normalizeFileName(nn, s_cwd);
    strcpy(nn,nnn);
    log_trace("file '%s' opened, checking to %s", nn, s_fileTab.tab[s_olOriginalFileNumber]->name);
    pushNewInclude(r, er, nn, "\n");
    return(stdin);  // NOT NULL
}

static void processInclude2(Position *ipos, char pchar, char *iname) {
    char *fname;
    FILE *nyyin;
    Symbol ss,*memb;
    int ii;
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "PragmaOnce-%s", iname);

    fillSymbol(&ss, tmpBuff, tmpBuff, s_noPos);
    fillSymbolBits(&ss.bits, ACCESS_DEFAULT, TypeMacro, StorageNone);

    if (symbolTableIsMember(s_symbolTable, &ss, &ii, &memb)) return;
    nyyin = openInclude(pchar, iname, &fname);
    if (nyyin == NULL) {
        assert(s_opt.taskRegime);
        if (s_opt.taskRegime!=RegimeEditServer) warningMessage(ERR_CANT_OPEN, iname);
    } else if (CX_REGIME()) {
        addIncludeReferences(currentFile.lexBuffer.buffer.fileNumber, ipos);
    }
}


static void processInclude(Position *ipos) {
    char *currentLexem, *previousLexem;
    Lexem lexem;
    int l, v, len;
    Position pos;

    //& GetLexA(lexem, previousLexem);
    lexem = getLexA(&previousLexem);
    if (lexem == -1) goto endOfMacArg;
    if (lexem == -2) goto endOfFile;

    currentLexem = cInput.currentLexem;
    if (lexem == STRING_LITERAL) {
        PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
        if (macroStackIndex != 0) {
            errorMessage(ERR_INTERNAL,"include directive in macro body?");
assert(0);
            cInput = macroStack[0];
            macroStackIndex = 0;
        }
        processInclude2(ipos, *currentLexem, currentLexem+1);
    } else {
        cInput.currentLexem = previousLexem;		/* unget lexem */
        lexem = yylex();
        if (lexem == STRING_LITERAL) {
            cInput = macroStack[0];		// hack, cut everything pending
            macroStackIndex = 0;
            processInclude2(ipos, '\"', yytext);
        } else if (lexem == '<') {
            // TODO!!!!
            warningMessage(ERR_ST,"Include <> after macro expansion not yet implemented, sorry\n\tuse \"\" instead");
        }
        //do lex = yylex(); while (lex != '\n');
    }
    return;
 endOfMacArg:	assert(0);
 endOfFile:;
}

/* ********************************* #DEFINE ********************** */

#define GetNonBlankMaybeLexem(lexem, l, v, pos, len) {   \
    GetLex(lexem);\
    while (lexem == LINE_TOK) {\
        PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);\
        GetLex(lexem);\
    }\
}

static void addMacroToTabs(Symbol *pp, char *name) {
    int ii,mm;
    Symbol *memb;
    mm = symbolTableIsMember(s_symbolTable,pp,&ii,&memb);
    if (mm) {
        log_trace(": masking macro %s",name);
    } else {
        log_trace(": adding macro %s",name);
    }
    symbolTableSet(s_symbolTable,pp,ii);
}

static void setMacroArgumentName(S_macroArgumentTableElement *arg, void *at) {
    char ** argTab;
    argTab = (char**)at;
    assert(arg->order>=0);
    argTab[arg->order] = arg->name;
}

static void handleMacroDefinitionParameterPositions(int argi, Position *macpos,
                                                    Position *parpos1,
                                                    Position *pos, Position *parpos2,
                                                    int final) {
    if ((s_opt.server_operation == OLO_GOTO_PARAM_NAME || s_opt.server_operation == OLO_GET_PARAM_COORDINATES)
        && POSITION_EQ(*macpos, s_cxRefPos)) {
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(parpos1);
            } else if (argi < s_opt.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(parpos2);
            }
        } else if (argi == s_opt.olcxGotoVal) {
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
    if (s_opt.server_operation == OLO_GET_PARAM_COORDINATES
        && POSITION_EQ(*macpos, s_cxRefPos)) {
        log_trace("checking param %d at %d,%d, final==%d", argi, parpos1->col, parpos2->col, final);
        if (final) {
            if (argi==0) {
                setParamPositionForFunctionWithoutParams(parpos1);
            } else if (argi < s_opt.olcxGotoVal) {
                setParamPositionForParameterBeyondRange(parpos2);
            }
        } else if (argi == s_opt.olcxGotoVal) {
            s_paramBeginPosition = *parpos1;
            s_paramEndPosition = *parpos2;
//&fprintf(dumpOut,"regular setting to %d - %d\n", parpos1->col, parpos2->col);
        }
    }
}

static S_macroBody *newMacroBody(int msize, int argi, char *name, char *body, char **argNames) {
    S_macroBody *macroBody;

    PP_ALLOC(macroBody, S_macroBody);
    macroBody->argn = argi;
    macroBody->size = msize;
    macroBody->name = name;
    macroBody->args = argNames;
    macroBody->body = body;

    return macroBody;
}

/* Make public only for unittesting */
void processDefine(bool argFlag) {
    Lexem lexem;
    int l, v;
    bool bodyReadingFlag = false;
    int sizei, foundIndex, msize, argCount, ellipsis, len;
    Symbol *pp;
    S_macroArgumentTableElement *maca, mmaca;
    S_macroBody *macroBody;
    Position pos, macpos, ppb1, ppb2, *parpos1, *parpos2, *tmppp;
    char *cc, *mname, *aname, *body, *mm, *ddd;
    char **argNames, *argLinkName;

    sizei= -1;
    msize=0;argCount=0;pp=NULL;macroBody=NULL;mname=body=NULL; // to calm compiler
    SM_INIT(ppMemory);
    ppb1 = s_noPos;
    ppb2 = s_noPos;
    parpos1 = &ppb1;
    parpos2 = &ppb2;

    lexem = getLex();
    if (lexem == -1) goto endOfMacArg;
    if (lexem == -2) goto endOfFile;

    cc = cInput.currentLexem;

    /* TODO: WTF there are some "symbols" in the lexBuffer, like "\275\001".
     * Those are compacted converted lexem codes. See lexmac.h for explanation.
     */
    PassLex(cInput.currentLexem, lexem, l, v, macpos, len, 1);

    testCxrefCompletionId(&lexem, cc, &macpos);    /* for cross-referencing */
    if (lexem != IDENTIFIER)
        return;

    PP_ALLOC(pp, Symbol);
    fillSymbol(pp, NULL, NULL, macpos);
    fillSymbolBits(&pp->bits, ACCESS_DEFAULT, TypeMacro, StorageNone);

    setGlobalFileDepNames(cc, pp, MEMORY_PP);
    mname = pp->name;
    /* process arguments */
    macroArgumentTableNoAllocInit(&s_macroArgumentTable, s_macroArgumentTable.size);
    argCount = -1;
    if (argFlag) {
        GetNonBlankMaybeLexem(lexem, l, v, pos, len);
        PassLex(cInput.currentLexem, lexem, l, v, *parpos2, len, 1);
        *parpos1 = *parpos2;
        if (lexem != '(') goto errorlab;
        argCount ++;
        GetNonBlankMaybeLexem(lexem, l, v, pos, len);
        if (lexem != ')') {
            for(;;) {
                char tmpBuff[TMP_BUFF_SIZE];
                cc = aname = cInput.currentLexem;
                PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
                ellipsis = 0;
                if (lexem == IDENTIFIER ) {
                    aname = cc;
                } else if (lexem == ELIPSIS) {
                    aname = s_cppVarArgsName;
                    pos = macpos;					// hack !!!
                    ellipsis = 1;
                } else goto errorlab;
                PP_ALLOCC(mm, strlen(aname)+1, char);
                strcpy(mm, aname);
                sprintf(tmpBuff, "%x-%x%c%s", pos.file, pos.line,
                        LINK_NAME_SEPARATOR, aname);
                PP_ALLOCC(argLinkName, strlen(tmpBuff)+1, char);
                strcpy(argLinkName, tmpBuff);
                SM_ALLOC(ppMemory, maca, S_macroArgumentTableElement);
                fillMacroArgTabElem(maca, mm, argLinkName, argCount);
                foundIndex = macroArgumentTableAdd(&s_macroArgumentTable, maca);
                argCount ++;
                GetNonBlankMaybeLexem(lexem, l, v, pos, len);
                tmppp=parpos1; parpos1=parpos2; parpos2=tmppp;
                PassLex(cInput.currentLexem, lexem, l, v, *parpos2, len, 1);
                if (! ellipsis) {
                    addTrivialCxReference(s_macroArgumentTable.tab[foundIndex]->linkName, TypeMacroArg,StorageDefault,
                                          &pos, UsageDefined);
                    handleMacroDefinitionParameterPositions(argCount, &macpos, parpos1, &pos, parpos2, 0);
                }
                if (lexem == ELIPSIS) {
                    // GNU ELLIPSIS ?????
                    GetNonBlankMaybeLexem(lexem, l, v, pos, len);
                    PassLex(cInput.currentLexem, lexem, l, v, *parpos2, len, 1);
                }
                if (lexem == ')') break;
                if (lexem != ',') break;
                GetNonBlankMaybeLexem(lexem, l, v, pos, len);
            }
            handleMacroDefinitionParameterPositions(argCount, &macpos, parpos1, &s_noPos, parpos2, 1);
        } else {
            PassLex(cInput.currentLexem, lexem, l, v, *parpos2, len, 1);
            handleMacroDefinitionParameterPositions(argCount, &macpos, parpos1, &s_noPos, parpos2, 1);
        }
    }
    /* process macro body */
    msize = MACRO_UNIT_SIZE;
    sizei = 0;
    PP_ALLOCC(body, msize+MAX_LEXEM_SIZE, char);
    bodyReadingFlag = true;
    GetNonBlankMaybeLexem(lexem, l, v, pos, len);
    cc = cInput.currentLexem;
    PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
    while (lexem != '\n') {
        while(sizei<msize && lexem != '\n') {
            fillMacroArgTabElem(&mmaca,cc,NULL,0);
            if (lexem==IDENTIFIER && macroArgumentTableIsMember(&s_macroArgumentTable,&mmaca,&foundIndex)){
                /* macro argument */
                addTrivialCxReference(s_macroArgumentTable.tab[foundIndex]->linkName, TypeMacroArg,StorageDefault,
                                      &pos, UsageUsed);
                ddd = body+sizei;
                PutLexToken(CPP_MAC_ARG, ddd);
                PutLexInt(s_macroArgumentTable.tab[foundIndex]->order, ddd);
                PutLexPosition(pos.file, pos.line,pos.col,ddd);
                sizei = ddd - body;
            } else {
                if (lexem==IDENT_TO_COMPLETE
                    || (lexem == IDENTIFIER &&
                        POSITION_EQ(pos, s_cxRefPos))) {
                    s_cache.activeCache = 0;
                    s_olstringFound = 1; s_olstringInMbody = pp->linkName;
                }
                ddd = body+sizei;
                PutLexToken(lexem, ddd);
                for(; cc<cInput.currentLexem; ddd++,cc++)*ddd= *cc;
                sizei = ddd - body;
            }
            GetNonBlankMaybeLexem(lexem, l, v, pos, len);
            cc = cInput.currentLexem;
            PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
        }
        if (lexem != '\n') {
            msize += MACRO_UNIT_SIZE;
            PP_REALLOCC(body,msize+MAX_LEXEM_SIZE,char,
                        msize+MAX_LEXEM_SIZE-MACRO_UNIT_SIZE);
        }
    }

endOfBody:
    assert(sizei>=0);
    PP_REALLOCC(body, sizei, char, msize+MAX_LEXEM_SIZE);
    msize = sizei;
    if (argCount > 0) {
        PP_ALLOCC(argNames, argCount, char*);
        memset(argNames, 0, argCount*sizeof(char*));
        macroArgumentTableMap2(&s_macroArgumentTable, setMacroArgumentName, argNames);
    } else
        argNames = NULL;
    macroBody = newMacroBody(msize, argCount, mname, body, argNames);
    pp->u.mbody = macroBody;

    addMacroToTabs(pp,mname);
    assert(s_opt.taskRegime);
    if (CX_REGIME()) {
        addCxReference(pp, &macpos, UsageDefined,s_noneFileIndex, s_noneFileIndex);
    }
    return;

endOfMacArg:
    assert(0);

endOfFile:
    if (bodyReadingFlag)
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
    processDefine(args);
}

/* ****************************** #UNDEF ************************* */

static void processUnDefine(void) {
    Lexem lexem;
    int l,v,ii,len;
    char *cc;
    Position pos;
    Symbol dd,*pp,*memb;

    //& GetLex(lexem);
    lexem = getLex();
    if (lexem == -1) goto endOfMacArg;
    if (lexem == -2) goto endOfFile;

    cc = cInput.currentLexem;
    PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
    testCxrefCompletionId(&lexem,cc,&pos);
    if (isIdentifierLexem(lexem)) {
        log_debug(": undef macro %s",cc);

        fillSymbol(&dd, cc, cc, pos);
        fillSymbolBits(&dd.bits, ACCESS_DEFAULT, TypeMacro, StorageNone);

        assert(s_opt.taskRegime);
        /* !!!!!!!!!!!!!! tricky, add macro with mbody == NULL !!!!!!!!!! */
        /* this is because of monotonicity for caching, just adding symbol */
        if (symbolTableIsMember(s_symbolTable, &dd, &ii, &memb)) {
            if (CX_REGIME()) {
                addCxReference(memb, &pos, UsageUndefinedMacro,s_noneFileIndex, s_noneFileIndex);
            }

            PP_ALLOC(pp, Symbol);
            fillSymbol(pp, memb->name, memb->linkName, pos);
            fillSymbolBits(&pp->bits, ACCESS_DEFAULT, TypeMacro, StorageNone);

            addMacroToTabs(pp,memb->name);
        }
    }
    while (lexem != '\n') {
        //& GetLex(lexem);
        lexem = getLex();
        if (lexem == -1) goto endOfMacArg;
        if (lexem == -2) goto endOfFile;
        PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
    }
    return;
endOfMacArg:	assert(0);
endOfFile:;
    return;
}

/* ********************************* #IFDEF ********************** */

static void processIf();

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
    int l,v,len;
    int depth;
    Position pos;

    depth = 1;
    while (depth > 0) {
        //& GetLex(lexem);
        lexem = getLex();
        if (lexem == -1) goto endOfMacArg;
        if (lexem == -2) goto endOfFile;

        PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
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
endOfMacArg:	assert(0);
endOfFile:;
    if (s_opt.taskRegime!=RegimeEditServer) {
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
        } else if (onElse==UNTIL_ELIF) processIf();
    }
}

static void processIfdef(bool isIfdef) {
    Lexem lexem;
    int l,v;
    int ii,mm,len;
    Symbol pp,*memb;
    char *cc;
    Position pos;
    int deleteSrc;

    //& GetLex(lexem);
    lexem = getLex();
    if (lexem == -1) goto endOfMacArg;
    if (lexem == -2) goto endOfFile;

    cc = cInput.currentLexem;
    PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
    testCxrefCompletionId(&lexem,cc,&pos);

    if (! isIdentifierLexem(lexem)) return;

    fillSymbol(&pp, cc, cc, s_noPos);
    fillSymbolBits(&pp.bits, ACCESS_DEFAULT, TypeMacro, StorageNone);

    assert(s_opt.taskRegime);
    mm = symbolTableIsMember(s_symbolTable,&pp,&ii,&memb);
    if (mm && memb->u.mbody==NULL) mm = 0;	// undefined macro
    if (mm) {
        if (CX_REGIME()) {
            addCxReference(memb,&pos,UsageUsed,s_noneFileIndex,s_noneFileIndex);
        }
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
endOfMacArg:	assert(0);
endOfFile:;
}

/* ********************************* #IF ************************** */

int cexp_yylex(void) {
    int l,v,par,ii,res,mm,len;
    Lexem lexem;
    char *cc;
    Symbol dd,*memb;
    Position pos;

    lexem = yylex();
    if (isIdentifierLexem(lexem)) {
        // this is useless, as it would be set to 0 anyway
        lexem = cexpTranslateToken(CONSTANT, 0);
    } else if (lexem == CPP_DEFINED_OP) {
        GetNonBlankMaybeLexem(lexem, l, v, pos, len);
        cc = cInput.currentLexem;
        PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
        if (lexem == '(') {
            par = 1;
            GetNonBlankMaybeLexem(lexem, l, v, pos, len);
            cc = cInput.currentLexem;
            PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
        } else {
            par = 0;
        }

        if (! isIdentifierLexem(lexem)) return(0);

        fillSymbol(&dd, cc, cc, s_noPos);
        fillSymbolBits(&dd.bits, ACCESS_DEFAULT, TypeMacro, StorageNone);

        log_debug("(%s)", dd.name);

        mm = symbolTableIsMember(s_symbolTable,&dd,&ii,&memb);
        if (mm && memb->u.mbody == NULL) mm = 0;   // undefined macro
        assert(s_opt.taskRegime);
        if (CX_REGIME()) {
            if (mm) addCxReference(&dd, &pos, UsageUsed,s_noneFileIndex, s_noneFileIndex);
        }
        /* following call sets uniyylval */
        res = cexpTranslateToken(CONSTANT, mm);
        if (par) {
            GetNonBlankMaybeLexem(lexem, l, v, pos, len);
            PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
            if (lexem != ')' && s_opt.taskRegime!=RegimeEditServer) {
                warningMessage(ERR_ST,"missing ')' after defined( ");
            }
        }
        lexem = res;
    } else {
        lexem = cexpTranslateToken(lexem, uniyylval->ast_integer.d);
    }
    return(lexem);
endOfMacArg:	assert(0);
endOfFile:;
    return(0);
}

static void processIf(void) {
    int res=1,lex;
    s_ifEvaluation = 1;
    log_debug(": #if");
    res = cexp_yyparse();
    do lex = yylex(); while (lex != '\n');
    s_ifEvaluation = 0;
    execCppIf(! res);
}

static void processPragma(void) {
    Lexem lexem;
    int l,v,len,ii;
    char *mname, *fname;
    Position pos;
    Symbol *pp;

    //& GetLex(lexem);
    lexem = getLex();
    if (lexem == -1) goto endOfMacArg;
    if (lexem == -2) goto endOfFile;

    if (lexem == IDENTIFIER && strcmp(cInput.currentLexem, "once")==0) {
        char tmpBuff[TMP_BUFF_SIZE];
        PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
        fname = simpleFileName(s_fileTab.tab[pos.file]->name);
        sprintf(tmpBuff, "PragmaOnce-%s", fname);
        PP_ALLOCC(mname, strlen(tmpBuff)+1, char);
        strcpy(mname, tmpBuff);

        PP_ALLOC(pp, Symbol);
        fillSymbol(pp, mname, mname, pos);
        fillSymbolBits(&pp->bits, ACCESS_DEFAULT, TypeMacro, StorageNone);

        symbolTableAdd(s_symbolTable,pp,&ii);
    }
    while (lexem != '\n') {
        PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
        //& GetLex(lexem);
        lexem = getLex();
        if (lexem == -1) goto endOfMacArg;
        if (lexem == -2) goto endOfFile;
    }
    PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
    return;
endOfMacArg:	assert(0);
endOfFile:;
}

/* ***************************************************************** */
/*                                 CPP                               */
/* ***************************************************************** */

#define AddHtmlCppReference(pos) {\
    if (s_opt.taskRegime==RegimeHtmlGenerate && !s_opt.htmlNoColors) {\
        addTrivialCxReference("_",TypeCppAny,StorageDefault,&pos,UsageUsed);\
    }\
}

static bool processPreprocessorConstruct(Lexem lexem) {
    int l,v,len;
    Position pos;

    PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
    log_debug("processing cpp-construct '%s' ", s_tokenName[lexem]);
    switch (lexem) {
    case CPP_INCLUDE:
        processInclude(&pos);
        break;
    case CPP_DEFINE0:
        AddHtmlCppReference(pos);
        processDefine(0);
        break;
    case CPP_DEFINE:
        AddHtmlCppReference(pos);
        processDefine(1);
        break;
    case CPP_UNDEF:
        AddHtmlCppReference(pos);
        processUnDefine();
        break;
    case CPP_IFDEF:
        genCppIfElseReference(1, &pos, UsageDefined);
        processIfdef(true);
        break;
    case CPP_IFNDEF:
        genCppIfElseReference(1, &pos, UsageDefined);
        processIfdef(false);
        break;
    case CPP_IF:
        genCppIfElseReference(1, &pos, UsageDefined);
        processIf();
        break;
    case CPP_ELIF:
        log_debug("#elif");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, &pos, UsageUsed);
            currentFile.ifDepth --;
            cppDeleteUntilEndElse(true);
        } else if (s_opt.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"unmatched #elif");
        }
        break;
    case CPP_ELSE:
        log_debug("#else");
        if (currentFile.ifDepth) {
            genCppIfElseReference(0, &pos, UsageUsed);
            currentFile.ifDepth --;
            cppDeleteUntilEndElse(true);
        } else if (s_opt.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"unmatched #else");
        }
        break;
    case CPP_ENDIF:
        log_debug("#endif");
        if (currentFile.ifDepth) {
            currentFile.ifDepth --;
            genCppIfElseReference(-1, &pos, UsageUsed);
        } else if (s_opt.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"unmatched #endif");
        }
        break;
    case CPP_PRAGMA:
        log_debug("#pragma");
        AddHtmlCppReference(pos);
        processPragma();
        break;
    case CPP_LINE:
        AddHtmlCppReference(pos);
        processLine();

        //& GetLex(lexem);
        lexem = getLex();
        if (lexem == -1) goto endOfMacArg;
        if (lexem == -2) goto endOfFile;

        while (lexem != '\n') {
            PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);

            //& GetLex(lexem);
            lexem = getLex();
            if (lexem == -1) goto endOfMacArg;
            if (lexem == -2) goto endOfFile;
        }
        PassLex(cInput.currentLexem, lexem, l, v, pos, len, 1);
        break;
    default: assert(0);
    }
    return true;

endOfMacArg: assert(0);
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

static int cyclicCall(S_macroBody *mb) {
    struct lexInput *ll;
    char *name;
    int i;
    name = mb->name;
/*fprintf(dumpOut,"testing '%s' against curr '%s'\n",name,cInput.macname);*/
    if (cInput.macroName != NULL && strcmp(name,cInput.macroName)==0) return(1);
    for(i=0; i<macroStackIndex; i++) {
        ll = &macroStack[i];
/*fprintf(dumpOut,"testing '%s' against '%s'\n",name,ll->macname);*/
        if (ll->macroName != NULL && strcmp(name,ll->macroName)==0) return(1);
    }
    return(0);
}


static void prependMacroInput(S_lexInput *argb) {
    assert(macroStackIndex < MACRO_STACK_SIZE-1);
    macroStack[macroStackIndex++] = cInput;
    cInput = *argb;
    cInput.currentLexem = cInput.beginningOfBuffer;
    cInput.inputType = INPUT_MACRO;
}


static void expandMacroArgument(S_lexInput *argb) {
    Symbol sd,*memb;
    char *buf,*previousLexem,*currentLexem,*bcc, *tbcc;
    int length,ii,line,val,bsize,failedMacroExpansion,len;
    Lexem lexem;
    Position pos;

    prependMacroInput(argb);

    cInput.inputType = INPUT_MACRO_ARGUMENT;
    bsize = MACRO_UNIT_SIZE;
    PP_ALLOCC(buf,bsize+MAX_LEXEM_SIZE,char);
    bcc = buf;
    for(;;) {
    nextLexem:
        //& GetLexA(lexem, previousLexem);
        lexem = getLexA(&previousLexem);
        if (lexem == -1) goto endOfMacArg;
        if (lexem == -2) goto endOfFile;

        currentLexem = cInput.currentLexem;
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        length = ((char*)cInput.currentLexem) - previousLexem;
        assert(length >= 0);
        memcpy(bcc, previousLexem, length);
        // a hack, it is copied, but bcc will be increased only if not
        // an expanding macro, this is because 'macroCallExpand' can
        // read new lexbuffer and destroy cInput, so copy it now.
        failedMacroExpansion = 0;
        if (lexem == IDENTIFIER) {
            fillSymbol(&sd, currentLexem, currentLexem, s_noPos);
            fillSymbolBits(&sd.bits, ACCESS_DEFAULT, TypeMacro, StorageNone);
            if (symbolTableIsMember(s_symbolTable, &sd, &ii, &memb)) {
                /* it is a macro, provide macro expansion */
                if (expandMacroCall(memb,&pos)) goto nextLexem;
                else failedMacroExpansion = 1;
            }
        }
        if (failedMacroExpansion) {
            tbcc = bcc;
            assert(memb!=NULL);
            if (memb->u.mbody!=NULL && cyclicCall(memb->u.mbody)) PutLexToken(IDENT_NO_CPP_EXPAND, tbcc);
            //& else PutLexToken(IDENT_NO_CPP_EXPAND, tbcc);
        }
        bcc += length;
        TestPPBufOverflow(bcc, buf, bsize);
    }
endOfMacArg:
    cInput = macroStack[--macroStackIndex];
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
                    char **ancc, S_lexInput *actArgs) {
    char *lbcc,*bcc,*cc,*ccfin,*cc0,*ncc,*occ;
    int line, val, nlex, len1, bsize, len;
    Lexem lexem;
    Position pos,respos;

    ncc = *ancc;
    lbcc = *albcc;
    bcc = *abcc;
    bsize = *absize;
    val=0; // compiler
    if (NextLexToken(lbcc) == CPP_MAC_ARG) {
        bcc = lbcc;
        GetLexToken(lexem,lbcc);
        assert(lexem==CPP_MAC_ARG);
        PassLex(lbcc, lexem, line, val, respos, len, 0);
        cc = actArgs[val].beginningOfBuffer; ccfin = actArgs[val].endOfBuffer;
        lbcc = NULL;
        while (cc < ccfin) {
            cc0 = cc;
            GetLexToken(lexem, cc);
            PassLex(cc, lexem, line, val, respos, len, 0);
            lbcc = bcc;
            assert(cc>=cc0);
            memcpy(bcc, cc0, cc-cc0);
            bcc += cc-cc0;
            TestPPBufOverflow(bcc,buf,bsize);
        }
    }
    if (NextLexToken(ncc) == CPP_MAC_ARG) {
        GetLexToken(lexem, ncc);
        PassLex(ncc, lexem, line, val, pos, len, 0);
        cc = actArgs[val].beginningOfBuffer; ccfin = actArgs[val].endOfBuffer;
    } else {
        cc = ncc;
        GetLexToken(lexem, ncc);
        PassLex(ncc, lexem, line, val, pos, len, 0);
        ccfin = ncc;
    }
    /* now collate *lbcc and *cc */
    // berk, do not pre-compute, lbcc can be NULL!!!!
    //& nlt = NextLexToken(lbcc);
    if (lbcc!=NULL && cc < ccfin && isIdentifierLexem(NextLexToken(lbcc))) {
        nlex = NextLexToken(cc);
        if (isIdentifierLexem(nlex) || nlex == CONSTANT
                    || nlex == LONG_CONSTANT || nlex == FLOAT_CONSTANT
                    || nlex == DOUBLE_CONSTANT ) {
            /* TODO collation of all lexem pairs */
            len1 = strlen(lbcc+IDENT_TOKEN_SIZE);
            GetLexToken(lexem, cc);
            occ = cc;
            PassLex(cc, lexem, line, val, respos, len, 0);
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
            PutLexPosition(respos.file,respos.line,respos.col,bcc);
        }
    }
    TestPPBufOverflow(bcc,buf,bsize);
    while (cc<ccfin) {
        cc0 = cc;
        GetLexToken(lexem, cc);
        PassLex(cc, lexem, line, val, pos, len, 0);
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
    int v,c,lv,len;
    Lexem lexem;
    Position pos;

    bcc = res;
    *bcc = 0;
    c=0; v=0;
    cc = lb->beginningOfBuffer;
    while (cc < lb->endOfBuffer) {
        GetLexToken(lexem,cc);
        lcc = cc;
        PassLex(cc, lexem, lv, v, pos, len, c);
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

static void createMacroBody(S_lexInput *macBody,
                        S_macroBody *mb,
                        struct lexInput *actArgs,
                        int actArgn
                        ) {
    char *cc,*cc0,*cfin,*bcc,*lbcc;
    int i,line,val,len,bsize,lexlen;
    Lexem lexem;
    Position pos, hpos;
    char *buf,*buf2;

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
        GetLexToken(lexem, cc);
        PassLex(cc, lexem, line, val, pos, lexlen, 0);
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
        GetLexToken(lexem, cc);
        PassLex(cc, lexem, line, val, hpos, lexlen, 0);
        if (lexem == CPP_MAC_ARG) {
            len = actArgs[val].endOfBuffer - actArgs[val].beginningOfBuffer;
            TestMBBufOverflow(bcc,len,buf2,bsize);
            memcpy(bcc, actArgs[val].beginningOfBuffer, len);
            bcc += len;
        } else if (lexem=='#' && cc<cfin && NextLexToken(cc)==CPP_MAC_ARG) {
            GetLexToken(lexem, cc);
            PassLex(cc, lexem, line, val, pos, lexlen, 0);
            assert(lexem == CPP_MAC_ARG);
            PutLexToken(STRING_LITERAL, bcc);
            TestMBBufOverflow(bcc,MACRO_UNIT_SIZE,buf2,bsize);
            macArgsToString(bcc, &actArgs[val]);
            len = strlen(bcc)+1;
            bcc += len;
            PutLexPosition(hpos.file, hpos.line, hpos.col, bcc);
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

    fillLexInput(macBody,buf2,bcc,buf2,mb->name,INPUT_MACRO);

}

/* *************************************************************** */
/* ******************* MACRO CALL PROCESS ************************ */
/* *************************************************************** */
/* TODO: another macro that forces invocation context to have
   particular variables. Introduce them as arguments */
#define GetLexASkippingLines(lexem, previousLexem)                      \
    {                                                                   \
        lexem = getLexA(&previousLexem);                                \
        if (lexem == -1) goto endOfMacArg;                              \
        if (lexem == -2) goto endOfFile;                                \
                                                                        \
        while (lexem == LINE_TOK || lexem == '\n') {                    \
            PassLex(cInput.currentLexem, lexem, line, val, pos, len, \
                               macroStackIndex == 0);                   \
            lexem = getLexA(&previousLexem);                            \
            if (lexem == -1) goto endOfMacArg;                          \
            if (lexem == -2) goto endOfFile;                            \
        }                                                               \
    }

static void getActMacroArgument(char *previousLexem,
                                Lexem *out_lexem,
                                Position *mpos,
                                Position **parpos1,
                                Position **parpos2,
                                S_lexInput *actArg,
                                S_macroBody *mb,
                                int actArgi
                                ) {
    char *buf,*bcc;
    int line,val,len, poffset;
    Position pos;
    int bufsize,depth;
    Lexem lexem;

    lexem = *out_lexem;
    bufsize = MACRO_ARG_UNIT_SIZE;
    depth = 0;
    PP_ALLOCC(buf, bufsize+MAX_LEXEM_SIZE, char);
    bcc = buf;
    /* if lastArgument, collect everything there */
    poffset = 0;
    while (((lexem != ',' || actArgi+1==mb->argn) && lexem != ')') || depth > 0) {
        // The following should be equivalent to the loop condition:
        //& if (lexem == ')' && depth <= 0) break;
        //& if (lexem == ',' && depth <= 0 && ! lastArgument) break;
        if (lexem == '(') depth ++;
        if (lexem == ')') depth --;
        for(;previousLexem < cInput.currentLexem; previousLexem++, bcc++)
            *bcc = *previousLexem;
        if (bcc-buf >= bufsize) {
            bufsize += MACRO_ARG_UNIT_SIZE;
            PP_REALLOCC(buf, bufsize+MAX_LEXEM_SIZE, char,
                    bufsize+MAX_LEXEM_SIZE-MACRO_ARG_UNIT_SIZE);
        }
        //& GetLexASkippingLines(lexem, previousLexem);
        {
            lexem = getLexA(&previousLexem);
            if (lexem == -1) goto end;
            if (lexem == -2) goto end;

            while (lexem == LINE_TOK || lexem == '\n') {
                PassLex(cInput.currentLexem, lexem, line, val, pos, len,
                        macroStackIndex == 0);
                lexem = getLexA(&previousLexem);
                if (lexem == -1) goto end;
                if (lexem == -2) goto end;
            }
        end:
            if (lexem == -1) goto endOfMacArg;
            if (lexem == -2) goto endOfFile;
        }

        PassLex(cInput.currentLexem, lexem, line, val, (**parpos2), len,
                macroStackIndex == 0);
        if ((lexem == ',' || lexem == ')') && depth == 0) {
            poffset ++;
            handleMacroUsageParameterPositions(actArgi+poffset, mpos, *parpos1, *parpos2, 0);
            **parpos1= **parpos2;
        }
    }
    if (0) {  /* skip the error message when finished normally */
endOfFile:;
endOfMacArg:;
        assert(s_opt.taskRegime);
        if (s_opt.taskRegime!=RegimeEditServer) {
            warningMessage(ERR_ST,"[getActMacroArgument] unterminated macro call");
        }
    }
    PP_REALLOCC(buf, bcc-buf, char, bufsize+MAX_LEXEM_SIZE);
    fillLexInput(actArg,buf,bcc,buf,cInput.macroName,INPUT_NORMAL);
    *out_lexem = lexem;
    return;
}

static struct lexInput *getActualMacroArguments(S_macroBody *mb, Position *mpos,
                                                Position *lparpos) {
    char *previousLexem;
    Lexem lexem;
    int line,val,len;
    Position pos, ppb1, ppb2, *parpos1, *parpos2;
    int actArgi = 0;
    struct lexInput *actArgs;

    ppb1 = *lparpos;
    ppb2 = *lparpos;
    parpos1 = &ppb1;
    parpos2 = &ppb2;
    PP_ALLOCC(actArgs,mb->argn,struct lexInput);
    GetLexASkippingLines(lexem, previousLexem);
    PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
    if (lexem == ')') {
        *parpos2 = pos;
        handleMacroUsageParameterPositions(0, mpos, parpos1, parpos2, 1);
    } else {
        for(;;) {
            getActMacroArgument(previousLexem,&lexem, mpos, &parpos1, &parpos2,
                                &actArgs[actArgi], mb, actArgi);
            actArgi ++ ;
            if (lexem != ',' || actArgi >= mb->argn) break;
            GetLexASkippingLines(lexem, previousLexem);
            PassLex(cInput.currentLexem, lexem, line, val, pos, len,
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
endOfMacArg:	assert(0);
endOfFile:
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime!=RegimeEditServer) {
        warningMessage(ERR_ST,"[getActualMacroArguments] unterminated macro call");
    }
    return(NULL);
}

/* **************************************************************** */

static void addMacroBaseUsageRef(Symbol *mdef) {
    int                 ii,rr;
    SymbolReferenceItem     ppp,*memb;
    Reference			*r;
    Position          basePos;
    fillPosition(&basePos, s_input_file_number, 0, 0);
    fillSymbolRefItemExceptBits(&ppp, mdef->linkName,
                                cxFileHashNumber(mdef->linkName), // useless, put 0
                                s_noneFileIndex, s_noneFileIndex);
    fillSymbolRefItemBits(&ppp.b,TypeMacro, StorageDefault, ScopeGlobal,
                           mdef->bits.access, CategoryGlobal, 0);
    rr = refTabIsMember(&s_cxrefTab, &ppp, &ii, &memb);
    r = NULL;
    if (rr) {
        // this is optimization to avoid multiple base references
        for(r=memb->refs; r!=NULL; r=r->next) {
            if (r->usage.base == UsageMacroBaseFileUsage) break;
        }
    }
    if (rr==0 || r==NULL) {
        addCxReference(mdef,&basePos,UsageMacroBaseFileUsage,
                              s_noneFileIndex, s_noneFileIndex);
    }
}


static int expandMacroCall(Symbol *mdef, Position *mpos) {
    Lexem lexem;
    int line,val,len;
    char *previousLexem,*freeBase;
    Position pos, lparpos;
    S_lexInput *actArgs, macroBody;
    S_macroBody *mb;

    previousLexem = cInput.currentLexem;
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
        GetLexASkippingLines(lexem, previousLexem);
        if (lexem != '(') {
            cInput.currentLexem = previousLexem;		/* unget lexem */
            return(0);
        }
        PassLex(cInput.currentLexem, lexem, line, val, lparpos, len,
                           macroStackIndex == 0);
        actArgs = getActualMacroArguments(mb, mpos, &lparpos);
    } else {
        actArgs = NULL;
    }
    assert(s_opt.taskRegime);
    if (CX_REGIME()) {
        addCxReference(mdef,mpos,UsageUsed,s_noneFileIndex, s_noneFileIndex);
        if (s_opt.taskRegime == RegimeXref) addMacroBaseUsageRef(mdef);
    }
    log_trace("create macro body '%s'", mb->name);
    createMacroBody(&macroBody,mb,actArgs,mb->argn);
    prependMacroInput(&macroBody);
    log_trace("expanding macro '%s'", mb->name);
    PP_FREE_UNTIL(freeBase);
    return(1);
endOfMacArg:
    /* unterminated macro call in argument */
    /* TODO unread readed argument */
    cInput.currentLexem = previousLexem;
    PP_FREE_UNTIL(freeBase);
    return(0);
endOfFile:
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime!=RegimeEditServer) {
        warningMessage(ERR_ST,"[macroCallExpand] unterminated macro call");
    }
    cInput.currentLexem = previousLexem;
    PP_FREE_UNTIL(freeBase);
    return(0);
}

#ifdef DEBUG
int lexBufDump(LexemBuffer *lb) {
    char *cc;
    int v,c,lv,len;
    Lexem lexem;
    Position pos;

    c=0;
    fprintf(dumpOut,"\nlexbufdump [start] \n"); fflush(dumpOut);
    cc = lb->next;
    while (cc < lb->end) {
        GetLexToken(lexem,cc);
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
        PassLex(cc, lexem, lv, v, pos, len, c);
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
    int line, val, res, len;
    Position pos;
    unsigned lexemLength;
    char *previousLexem, *cto, *ccc;

    assert(cpoint > 0);
    cto = s_cache.cp[cpoint].lbcc;
    ccc = *cfrom;
    res = 1;
    while (ccc < cto) {
        //& GetLexA(lexem, previousLexem);
        lexem = getLexA(&previousLexem);
        if (lexem == -1) goto endOfMacArg;
        if (lexem == -2) goto endOfFile;

        PassLex(cInput.currentLexem, lexem, line, val, pos, len, 1);
        lexemLength = cInput.currentLexem-previousLexem;
        assert(lexemLength >= 0);
        if (memcmp(previousLexem, ccc, lexemLength)) {
            cInput.currentLexem = previousLexem;			/* unget last lexem */
            res = 0;
            break;
        }
        if (isIdentifierLexem(lexem) || isPreprocessorToken(lexem)) {
            if (onSameLine(pos, s_cxRefPos)) {
                cInput.currentLexem = previousLexem;			/* unget last lexem */
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
endOfMacArg:
    assert(0);
    return -1;                  /* The assert should protect this from executing */
}

/* ***************************************************************** */
/*                                 yylex                             */
/* ***************************************************************** */

static char charText[2]={0,0};
static char constant[50];

#define CHECK_ID_FOR_KEYWORD(sd,idposa) {\
    if (sd->bits.symType == TypeKeyword) {\
        SET_IDENTIFIER_YYLVAL(sd->name, sd, *idposa);\
        if (s_opt.taskRegime==RegimeHtmlGenerate && !s_opt.htmlNoColors) {\
            char ttt[TMP_STRING_SIZE];\
            sprintf(ttt,"%s-%x", sd->name, idposa->file);\
            addTrivialCxReference(ttt, TypeKeyword,StorageDefault, idposa, UsageUsed);\
            /*&addCxReference(sd, idposa, UsageUsed,s_noneFileIndex, s_noneFileIndex);&*/\
        }\
        return(sd->u.keyWordVal);\
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
            if (sd->bits.symType == TypeDefinedOp && s_ifEvaluation) {
                return(CPP_DEFINED_OP);
            }
            if (sd->bits.symType == TypeDefault) {
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
            if (sd->bits.symType == TypeDefault) {
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
    if (s_opt.server_operation == OLO_SET_MOVE_TARGET) {
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
    } else if (s_opt.server_operation == OLO_SET_MOVE_CLASS_TARGET) {
        s_cps.moveTargetApproved = 0;
        if (LANGUAGE(LANG_JAVA)) {
            if (s_cp.function == NULL) {
                if (s_javaStat!=NULL) {
                    s_cps.moveTargetApproved = 1;
                }
            }
        }
    } else if (s_opt.server_operation == OLO_SET_MOVE_METHOD_TARGET) {
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
    } else if (s_opt.server_operation == OLO_EXTRACT) {
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
    int line, val, len;
    Position pos, idpos;
    char *previousLexem;

    len = 0;
 nextYylex:
    //& GetLexA(lexem, previousLexem); /* Expand and extract into: */
    lexem = getLexA(&previousLexem);
    if (lexem == -1) goto endOfMacArg;
    if (lexem == -2) goto endOfFile;

 contYylex:
    if (lexem < 256) {
        if (lexem == '\n') {
            if (s_ifEvaluation) {
                cInput.currentLexem = previousLexem;
            } else {
                PassLex(cInput.currentLexem, lexem, line, val, pos, len,
                                   macroStackIndex == 0);
                for(;;) {
                    //& GetLex(lexem);
                    lexem = getLex();
                    if (lexem == -1) goto endOfMacArg;
                    if (lexem == -2) goto endOfFile;

                    if (!isPreprocessorToken(lexem))
                        goto contYylex;
                    if (!processPreprocessorConstruct(lexem))
                        goto endOfFile;
                }
            }
        } else {
            PassLex(cInput.currentLexem, lexem, line, val, pos, len,
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
        int ii;
        Symbol symbol, *memberP;

        id = yytext = cInput.currentLexem;
        PassLex(cInput.currentLexem, lexem, line, val, idpos, len, macroStackIndex == 0);
        assert(s_opt.taskRegime);
        if (s_opt.taskRegime == RegimeEditServer) {
//			???????????? isn't this useless
            testCxrefCompletionId(&lexem,yytext,&idpos);
        }
        log_trace("id %s position %d %d %d",yytext,idpos.file,idpos.line,idpos.col);
        fillSymbol(&symbol, yytext, yytext, s_noPos);
        fillSymbolBits(&symbol.bits, ACCESS_DEFAULT, TypeMacro, StorageNone);

        if ((!LANGUAGE(LANG_JAVA))
            && lexem!=IDENT_NO_CPP_EXPAND
            && symbolTableIsMember(s_symbolTable,&symbol,&ii,&memberP)) {
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
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        actionOnBlockMarker();
        goto nextYylex;
    }
    if (lexem < MULTI_TOKENS_START) {
        yytext = s_tokenName[lexem];
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, s_tokenLength[lexem]);
        goto finish;
    }
    if (lexem == LINE_TOK) {
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        goto nextYylex;
    }
    if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
        val=0;//compiler
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        sprintf(constant,"%d",val);
        SET_INTEGER_YYLVAL(val, pos, len);
        yytext = constant;
        goto finish;
    }
    if (lexem == FLOAT_CONSTANT || lexem == DOUBLE_CONSTANT) {
        yytext = "'fltp constant'";
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, len);
        goto finish;
    }
    if (lexem == STRING_LITERAL) {
        yytext = cInput.currentLexem;
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, strlen(yytext));
        goto finish;
    }
    if (lexem == CHAR_LITERAL) {
        val=0;//compiler
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        sprintf(constant,"'%c'",val);
        SET_INTEGER_YYLVAL(val, pos, len);
        yytext = constant;
        goto finish;
    }
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime == RegimeEditServer) {
        yytext = cInput.currentLexem;
        PassLex(cInput.currentLexem, lexem, line, val, pos, len, macroStackIndex == 0);
        if (lexem == IDENT_TO_COMPLETE) {
            testCxrefCompletionId(&lexem,yytext,&pos);
            while (inStacki != 0) popInclude();
            /* while (getLexBuf(&cFile.lb)) cFile.lb.cc = cFile.lb.fin;*/
            goto endOfFile;
        }
    }
/*fprintf(stderr,"unknown lexem %d\n",lex);*/
    goto endOfFile;

 finish:
    log_trace("!'%s'(%d)", yytext, cxMemory->i);
    s_lastReturnedLexem = lexem;
    return(lexem);

 endOfMacArg:
    assert(0);

 endOfFile:
    if ((!LANGUAGE(LANG_JAVA)) && inStacki != 0) {
        popInclude();
        placeCachePoint(1);
        goto nextYylex;
    }
    /* add the test whether in COMPLETION, communication string found */
    return(0);
}	/* end of yylex() */

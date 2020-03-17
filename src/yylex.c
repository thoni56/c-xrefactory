#include "yylex.h"

#include "commons.h"
#include "lex.h"
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
#include "charbuf.h"
#include "symbol.h"
#include "strFill.h"

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


#define SET_IDENTIFIER_YYLVAL(name, symb, pos) {\
    uniyylval->bbidIdent.d = &s_yyIdentBuf[s_yyIdentBufi];\
    s_yyIdentBufi ++; s_yyIdentBufi %= (YYBUFFERED_ID_INDEX);\
    FILL_id(uniyylval->bbidIdent.d, name, symb, pos, NULL);\
    yytext = name;\
    uniyylval->bbidIdent.b = pos;\
    uniyylval->bbidIdent.e = pos;\
    uniyylval->bbidIdent.e.col += strlen(yytext);\
}


/* !!!!!!!!!!!!!!!!!!! to caching !!!!!!!!!!!!!!! */

#include "matab.h"

#define MB_INIT()				{SM_INIT(mbMemory);}
#define MB_ALLOC(p,t)           {SM_ALLOC(mbMemory,p,t);}
#define MB_ALLOCC(p,n,t)        {SM_ALLOCC(mbMemory,p,n,t);}
#define MB_REALLOCC(p,n,t,on)	{SM_REALLOCC(mbMemory,p,n,t,on);}
#define MB_FREE_UNTIL(p)        {SM_FREE_UNTIL(mbMemory,p);}

static char ppMemory[SIZE_ppMemory];
static int ppMemoryi=0;

S_position s_yyPositionBuf[YYBUFFERED_ID_INDEX];
int s_yyPositionBufi = 0;

#define SetCacheConsistency() {s_cache.cc = cInput.cc;}
#define SetCFileConsistency() {\
    cFile.lb.next = cInput.cc;\
}
#define SetCInputConsistency() {\
    FILL_lexInput(&cInput,cFile.lb.next,cFile.lb.end,cFile.lb.chars,NULL,II_NORMAL);\
}

#define IS_IDENTIFIER_LEXEM(lex) (lex==IDENTIFIER || lex==IDENT_NO_CPP_EXPAND  || lex==IDENT_TO_COMPLETE)

static int macroCallExpand(Symbol *mdef, S_position *mpos);

/* ************************************************************ */

void initAllInputs(void) {
    mbMemoryi=0;
    inStacki=0;
    macroStackIndex=0;
    s_ifEvaluation = 0;
    s_cxRefFlag = 0;
    maTabNoAllocInit(&s_maTab, MAX_MACRO_ARGS);
    ppMemoryi=0;
    s_olstring[0]=0;
    s_olstringFound = 0;
    s_olstringServed = 0;
    s_olstringInMbody = NULL;
    s_upLevelFunctionCompletionType = NULL;
    s_structRecordCompletionType = NULL;
}


char *placeIdent(void) {
    static char tt[MAX_HTML_REF_LEN];
    char fn[MAX_FILE_NAME_SIZE];
    char mm[MAX_HTML_REF_LEN];
    int s;
    if (cFile.fileName!=NULL) {
        if (s_opt.xref2 && s_opt.taskRegime!=RegimeEditServer) {
            strcpy(fn, getRealFileNameStatic(normalizeFileName(cFile.fileName, s_cwd)));
            assert(strlen(fn) < MAX_FILE_NAME_SIZE);
            sprintf(mm, "%s:%d", simpleFileName(fn),cFile.lineNumber);
            assert(strlen(mm) < MAX_HTML_REF_LEN);
            sprintf(tt, "<A HREF=\"file://%s#%d\" %s=%ld>%s</A>", fn, cFile.lineNumber, PPCA_LEN, (unsigned long)strlen(mm), mm);
        } else {
            sprintf(tt,"%s:%d ",simpleFileName(getRealFileNameStatic(cFile.fileName)),cFile.lineNumber);
        }
        s = strlen(tt);
        assert(s<MAX_HTML_REF_LEN);
        return(tt);
    }
    return("");
}

#ifdef DEBUG
static void dpnewline(int n) {
    int i;
    if (s_opt.debug) {
        for(i=1; i<=n; i++) {
            log_trace("%s:%d", cFile.fileName, cFile.lineNumber+i);
        }
    }
}
#else
#define dpnewline(line) {}
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

static void getOrCreateFileInfo(char *ss, int *fileNumber, char **fileName) {
    int fileIndex, cxloading;
    bool existed = false;

    if (ss==NULL) {
        *fileNumber = fileIndex = s_noneFileIndex;
        *fileName = s_fileTab.tab[fileIndex]->name;
    } else {
        existed = fileTabExists(&s_fileTab, ss);
        fileIndex = addFileTabItem(ss);
        *fileNumber = fileIndex;
        *fileName = s_fileTab.tab[fileIndex]->name;
        checkFileModifiedTime(fileIndex);
        cxloading = s_fileTab.tab[fileIndex]->b.cxLoading;
        if (!existed) {
            cxloading = 1;
        } else if (s_opt.update==UP_FAST_UPDATE) {
            if (s_fileTab.tab[fileIndex]->b.scheduledToProcess) {
                // references from headers are not loaded on fast update !
                cxloading = 1;
            }
        } else if (s_opt.update==UP_FULL_UPDATE) {
            if (s_fileTab.tab[fileIndex]->b.scheduledToUpdate) {
                cxloading = 1;
            }
        } else {
            cxloading = 1;
        }
        if (s_fileTab.tab[fileIndex]->b.cxSaved==1 && ! s_opt.multiHeadRefsCare) {
            /* if multihead references care, load include refs each time */
            cxloading = 0;
        }
        if (LANGUAGE(LANG_JAVA)) {
            if (s_jsl!=NULL || s_javaPreScanOnly) {
                // do not load (and save) references from jsl loaded files
                // nor during prescanning
                cxloading = s_fileTab.tab[fileIndex]->b.cxLoading;
            }
        }
        s_fileTab.tab[fileIndex]->b.cxLoading = cxloading;
    }
}

/* this function is redundant, remove it in future */
static void setOpenFileInfo(char *ss) {
    int ii;
    char *ff;
    getOrCreateFileInfo(ss, &ii, &ff);
    cFile.lb.buffer.fileNumber = ii;
    cFile.fileName = ff;
}

/* ***************************************************************** */
/*                             Init Reading                          */
/* ***************************************************************** */

void ppMemInit(void) {
    PP_ALLOCC(s_maTab.tab, MAX_MACRO_ARGS, struct macroArgTabElem *);
    maTabNoAllocInit(&s_maTab, MAX_MACRO_ARGS);
    ppMemoryi = 0;
}

// it is supposed that one of ff or buffer is NULL
void initInput(FILE *ff, S_editorBuffer *buffer, char *prepend, char *name) {
    int     plen, bsize, filepos;
    char	*bbase;
    plen = strlen(prepend);
    if (buffer!=NULL) {
        // read buffer
        assert(plen < buffer->a.allocatedFreePrefixSize);
        strncpy(buffer->a.text-plen, prepend, plen);
        bbase = buffer->a.text-plen;
        bsize = buffer->a.bufferSize+plen;
        filepos = buffer->a.bufferSize;
        assert(bbase > buffer->a.allocatedBlock);
    } else {
        // read file
        assert(plen < CHAR_BUFF_SIZE);
        strcpy(cFile.lb.buffer.chars,prepend);
        bbase = cFile.lb.buffer.chars;
        bsize = plen;
        filepos = 0;
    }
    FILLF_fileDesc(&cFile,name,0,0,NULL,
                   /* lex buf */
                   NULL,NULL,0,
                   /* char buf */
                   bbase, bbase+bsize, ff,
                   filepos, s_noneFileIndex, 0, bbase,0,
                   0, INPUT_DIRECT,
                   /* z_stream */
                   NULL, 0, 0,		// in
                   NULL, 0, 0,		// out
                   NULL, NULL,
                   NULL, NULL,		// zlibAlloc, zlibFree,
                   NULL, 0, 0, 0
                   );
    setOpenFileInfo(name);
    SetCInputConsistency();
    s_ifEvaluation = 0;				/* ??? */

    /*	while (yylex());	exit(0); */

}

/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

/* maybe too time-consuming, when lex is known */
/* should be broken into parts */
#define PassLex(Input,lex,lineval,val,hash,pos, length,linecount) {     \
        if (lex > MULTI_TOKENS_START) {                                 \
            if (IS_IDENTIFIER_LEXEM(lex)){                              \
                register char *tmpcc,tmpch;                             \
                hash = 0;                                               \
                for(tmpcc=Input,tmpch= *tmpcc; tmpch; tmpch = *++tmpcc) { \
                    SYMTAB_HASH_FUN_INC(hash, tmpch);                  \
                }                                                       \
                SYMTAB_HASH_FUN_FINAL(hash);                           \
                tmpcc ++;                                               \
                GetLexPosition((pos),tmpcc);                            \
                Input = tmpcc;                                          \
            } else if (lex == STRING_LITERAL) {                         \
                register char *tmpcc,tmpch;                             \
                for(tmpcc=Input,tmpch= *tmpcc; tmpch; tmpch = *++tmpcc); \
                tmpcc ++;                                               \
                GetLexPosition((pos),tmpcc);                            \
                Input = tmpcc;                                          \
            } else if (lex == LINE_TOK) {                               \
                GetLexToken(lineval,Input);                             \
                if (linecount) {                                        \
                    if(s_opt.debug) dpnewline(lineval);                 \
                    cFile.lineNumber += lineval;                        \
                }                                                       \
            } else if (lex == CONSTANT || lex == LONG_CONSTANT) {       \
                GetLexInt(val,Input);                                   \
                GetLexPosition((pos),Input);                            \
                GetLexInt(length,Input);                                \
            } else if (lex == DOUBLE_CONSTANT || lex == FLOAT_CONSTANT) { \
                GetLexPosition((pos),Input);                            \
                GetLexInt(length,Input);                                \
            } else if (lex == CPP_MAC_ARG) {                            \
                GetLexInt(val,Input);                                   \
                GetLexPosition((pos),Input);                            \
            } else if (lex == CHAR_LITERAL) {                           \
                GetLexInt(val,Input);                                   \
                GetLexPosition((pos),Input);                            \
                GetLexInt(length,Input);                                \
            }                                                           \
        } else if (lex>CPP_TOKENS_START && lex<CPP_TOKENS_END) {        \
            GetLexPosition((pos),Input);                                \
        } else if (lex == '\n' && (linecount)) {                        \
            GetLexPosition((pos),Input);                                \
            if (s_opt.debug) dpnewline(1);                              \
            cFile.lineNumber ++;                                        \
        } else {                                                        \
            GetLexPosition((pos),Input);                                \
        }                                                               \
    }

#define PrependMacroInput(macInput) {             \
        assert(macroStackIndex < MACSTACK_SIZE-1);    \
        macStack[macroStackIndex++] = cInput;         \
        cInput = macInput;                      \
        cInput.cc = cInput.a;                   \
        cInput.margExpFlag = II_MACRO;          \
    }

#define GetLexA(lex,lastlexadd) {                                   \
        while (cInput.cc >= cInput.fin) {                           \
            char margFlag;                                          \
            margFlag = cInput.margExpFlag;                          \
            if (macroStackIndex > 0) {                                    \
                if (margFlag == II_MACRO_ARG) goto endOfMacArg;     \
                MB_FREE_UNTIL(cInput.a);                            \
                cInput = macStack[--macroStackIndex];                     \
            } else if (margFlag == II_NORMAL) {                     \
                SetCFileConsistency();                              \
                getLexBuf(&cFile.lb);                               \
                if (cFile.lb.next >= cFile.lb.end) goto endOfFile;    \
                SetCInputConsistency();                             \
            } else {                                                \
                /*			s_cache.recoveringFromCache = 0;*/      \
                s_cache.cc = s_cache.cfin = NULL;                   \
                cacheInput();                                       \
                s_cache.lexcc = cFile.lb.next;                        \
                SetCInputConsistency();                             \
            }                                                       \
            lastlexadd = cInput.cc;                                 \
        }                                                           \
        lastlexadd = cInput.cc;                                     \
        GetLexToken(lex, cInput.cc);                                \
    }

#define GetLex(lex) {                                                \
        char *lastlexcc; UNUSED lastlexcc; GetLexA(lex, lastlexcc);  \
    }


/* ***************************************************************** */
/*                                                                   */
/* ***************************************************************** */

static void testCxrefCompletionId(int *llex, char *idd, S_position *pos) {
    int lex;

    lex = *llex;
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime == RegimeEditServer) {
        if (lex==IDENT_TO_COMPLETE) {
            s_cache.activeCache = 0;
            s_olstringServed = 1;
            if (s_language == LANG_JAVA) {
                makeJavaCompletions(idd, strlen(idd), pos);
            }
            else if (s_language == LAN_YACC) {
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
            lex = IDENTIFIER;
        }
    }
    *llex = lex;
}



/* ********************************** #LINE *********************** */

static  void processLine(void) {
    int lex,l,h,v=0,len;
    S_position pos;

    GetLex(lex);
    PassLex(cInput.cc,lex,l,v,h,pos, len,1);
    if (lex != CONSTANT) return;
    //& cFile.lineNumber = v-1;      // ignore this directive
    GetLex(lex);
/*	this should be moved to the  getLexBuf routine,!!!!!!!!!!!!!!!!!!! TODO */
/*&
    if (lex == STRING_LITERAL) {
        i = 0;
        cc = cInput.cc;
        ch = *cc;
        if (ch != SLASH) copyDir(ss,s_opt.originalDir,&i);
        for(; ch; ch= *++cc) {
            ss[i++] = ch;
            assert(i+2<TMP_STRING_SIZE);
        }
        ss[i] = 0;
        setOpenFileInfo(ss);
    }
&*/
    PassLex(cInput.cc,lex,l,v,h,pos, len,1);
    return;
endOfMacArg:	assert(0);
endOfFile:;
}

/* ********************************* #INCLUDE ********************** */

static void fillIncludeSymbolItem( Symbol *ss,
                                   int filenum, S_position *pos
    ){
    // should be different for HTML to be beatiful, however,
    // all includes needs to be in the same cxfile, because of
    // -update. On the other hand in HTML I wish them to split
    // by first letter of file name.
    fillSymbol(ss, LINK_NAME_INCLUDE_REFS, LINK_NAME_INCLUDE_REFS, *pos);
    fillSymbolBits(&ss->bits, ACC_DEFAULT, TypeCppInclude, StorageDefault);
}


void addThisFileDefineIncludeReference(int filenum) {
    S_position dpos;
    Symbol        ss;
    FILL_position(&dpos, filenum, 1, 0);
    fillIncludeSymbolItem( &ss,filenum, &dpos);
//&fprintf(dumpOut,"adding reference on file %d==%s\n",filenum, s_fileTab.tab[filenum]->name);
    addCxReference(&ss, &dpos, UsageDefined, filenum, filenum);
}

void addIncludeReference(int filenum, S_position *pos) {
    Symbol        ss;
//&fprintf(dumpOut,"adding reference on file %d==%s\n",filenum, s_fileTab.tab[filenum]->name);
    fillIncludeSymbolItem( &ss, filenum, pos);
    addCxReference(&ss, pos, UsageUsed, filenum, filenum);
}

static void addIncludeReferences(int filenum, S_position *pos) {
    addIncludeReference(filenum, pos);
    addThisFileDefineIncludeReference(filenum);
}

void pushNewInclude(FILE *f, S_editorBuffer *buffer, char *name, char *prepend) {
    if (cInput.margExpFlag == II_CACHE) {
        SetCacheConsistency();
    } else {
        SetCFileConsistency();
    }
    inStack[inStacki] = cFile;		/* buffers are copied !!!!!!, burk */
    if (inStacki+1 >= INSTACK_SIZE) {
        fatalError(ERR_ST,"too deep nesting in includes", XREF_EXIT_ERR);
    }
    inStacki ++;
    initInput(f, buffer, prepend, name);
    cacheInclude(cFile.lb.buffer.fileNumber);
}

void popInclude(void) {
    assert(s_fileTab.tab[cFile.lb.buffer.fileNumber]);
    if (s_fileTab.tab[cFile.lb.buffer.fileNumber]->b.cxLoading) {
        s_fileTab.tab[cFile.lb.buffer.fileNumber]->b.cxLoaded = 1;
    }
    charBuffClose(&cFile.lb.buffer);
    if (inStacki != 0) {
        cFile = inStack[--inStacki];	/* buffers are copied !!!!!!, burk */
        if (inStacki == 0 && s_cache.cc!=NULL) {
            FILL_lexInput(&cInput, s_cache.cc, s_cache.cfin, s_cache.lb,
                        NULL, II_CACHE);
        } else {
            SetCInputConsistency();
        }
    }
}

static FILE *openInclude(char pchar, char *name, char **fileName) {
    S_editorBuffer  *er;
    FILE			*r;
    S_stringList    *ll;
    char            wcp[MAX_OPTION_LEN];
    char            nn[MAX_FILE_NAME_SIZE];
    char            rdir[MAX_FILE_NAME_SIZE];
    char            *nnn;
    int             nnlen,dlen,fdlen,nmlen;

    er = NULL; r = NULL;
    nmlen = strlen(name);
    copyDir(rdir, cFile.fileName, &fdlen);
    if (pchar!='<') {
        log_trace("fdlen == %d", fdlen);
        strcpy(nn, normalizeFileName(name, rdir));
        log_trace("try to open %s", nn);
        er = editorFindFile(nn);
        if (er==NULL) r = fopen(nn,"r");
    }
    for (ll=s_opt.includeDirs; ll!=NULL && er==NULL && r==NULL; ll=ll->next) {
        strcpy(nn, normalizeFileName(ll->d, rdir));
        expandWildCharactersInOnePath(nn, wcp, MAX_OPTION_LEN);
        JavaMapOnPaths(wcp, {
            strcpy(nn, currentPath);
            dlen = strlen(nn);
            if (dlen>0 && nn[dlen-1]!=SLASH) {
                nn[dlen] = SLASH;
                dlen++;
            }
            strcpy(nn+dlen, name);
            nnlen = dlen+nmlen;
            nn[nnlen]=0;
            log_trace("try to open <%s>", nn);
            er = editorFindFile(nn);
            if (er==NULL) r = fopen(nn,"r");
            if (er!=NULL || r!=NULL) goto found;
        });
    }
    if (er==NULL && r==NULL) return(NULL);
 found:
    nnn = normalizeFileName(nn, s_cwd);
    strcpy(nn,nnn);
    log_trace("file '%s' opened", nn);
    log_trace("checking to  %s", s_fileTab.tab[s_olOriginalFileNumber]->name);
    pushNewInclude(r, er, nn, "\n");
    return(stdin);  // NOT NULL
}

static void processInclude2(S_position *ipos, char pchar, char *iname) {
    char *fname;
    FILE *nyyin;
    Symbol ss,*memb;
    int ii;
    sprintf(tmpBuff, "PragmaOnce-%s", iname);

    fillSymbol(&ss, tmpBuff, tmpBuff, s_noPos);
    fillSymbolBits(&ss.bits, ACC_DEFAULT, TypeMacro, StorageNone);

    if (symTabIsMember(s_symTab, &ss, &ii, &memb)) return;
    nyyin = openInclude(pchar, iname, &fname);
    if (nyyin == NULL) {
        assert(s_opt.taskRegime);
        if (s_opt.taskRegime!=RegimeEditServer) warning(ERR_CANT_OPEN, iname);
    } else if (CX_REGIME()) {
        addIncludeReferences(cFile.lb.buffer.fileNumber, ipos);
    }
}


static void processInclude(S_position *ipos) {
    char *ccc, *cc2;
    int lex,l,h,v,len;
    S_position pos;
    GetLexA(lex, cc2);
    ccc = cInput.cc;
    if (lex == STRING_LITERAL) {
        PassLex(cInput.cc,lex,l,v,h,pos, len,1);
        if (macroStackIndex != 0) {
            error(ERR_INTERNAL,"include directive in macro body?");
assert(0);
            cInput = macStack[0];
            macroStackIndex = 0;
        }
        processInclude2(ipos, *ccc, ccc+1);
    } else {
        cInput.cc = cc2;		/* unget lexem */
        lex = yylex();
        if (lex == STRING_LITERAL) {
            cInput = macStack[0];		// hack, cut everything pending
            macroStackIndex = 0;
            processInclude2(ipos, '\"', yytext);
        } else if (lex == '<') {
            // TODO!!!!
            warning(ERR_ST,"Include <> after macro expansion not yet implemented, sorry\n\tuse \"\" instead");
        }
        //do lex = yylex(); while (lex != '\n');
    }
    return;
 endOfMacArg:	assert(0);
 endOfFile:;
}

/* ********************************* #DEFINE ********************** */

#define GetNonBlankMaybeLexem(lex) {\
    GetLex(lex);\
    while (lex == LINE_TOK) {\
        PassLex(cInput.cc,lex,l,v,h,pos, len,1);\
        GetLex(lex);\
    }\
}

static void addMacroToTabs(Symbol *pp, char *name) {
    int ii,mm;
    Symbol *memb;
    mm = symTabIsMember(s_symTab,pp,&ii,&memb);
    if (mm) {
        log_trace(": masking macro %s",name);
    } else {
        log_trace(": adding macro %s",name);
    }
    symTabSet(s_symTab,pp,ii);
}

static void setMacroArgumentName(S_macroArgTabElem *arg, void *at) {
    char ** argTab;
    argTab = (char**)at;
    assert(arg->order>=0);
    argTab[arg->order] = arg->name;
}

static void handleMacroDefinitionParameterPositions(int argi, S_position *macpos,
                                                    S_position *parpos1,
                                                    S_position *pos, S_position *parpos2,
                                                    int final
    ) {
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

static void handleMacroUsageParameterPositions(int argi, S_position *macpos,
                                               S_position *parpos1, S_position *parpos2,
                                               int final
    ) {
    if (s_opt.server_operation == OLO_GET_PARAM_COORDINATES
        && POSITION_EQ(*macpos, s_cxRefPos)) {
//&sprintf(tmpBuff,"checking param %d at %d,%d, final==%d", argi, parpos1->col, parpos2->col, final);ppcGenTmpBuff();
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

static void processDefine(int argFlag) {
    int lex, l, h, v;
    int bodyReadingFlag = 0;
    int sizei, foundIndex, msize, argi, ellipsis, len;
    Symbol *pp;
    S_macroArgTabElem *maca, mmaca;
    S_macroBody *macbody;
    S_position pos, macpos, ppb1, ppb2, *parpos1, *parpos2, *tmppp;
    char *cc, *mname, *aname, *mbody, *mm, *ddd;
    char **argNames, *argLinkName;

    sizei= -1;
    msize=0;argi=0;pp=NULL;macbody=NULL;mname=mbody=NULL; // to calm compiler
    SM_INIT(ppMemory);
    ppb1 = s_noPos;
    ppb2 = s_noPos;
    parpos1 = &ppb1;
    parpos2 = &ppb2;
    GetLex(lex);
    cc = cInput.cc;
    PassLex(cInput.cc,lex,l,v,h,macpos, len,1);
    testCxrefCompletionId(&lex,cc,&macpos);    /* for cross-referencing */
    if (lex != IDENTIFIER) return;

    PP_ALLOC(pp, Symbol);
    fillSymbol(pp, NULL, NULL, macpos);
    fillSymbolBits(&pp->bits, ACC_DEFAULT, TypeMacro, StorageNone);

    setGlobalFileDepNames(cc, pp, MEM_PP);
    mname = pp->name;
    /* process arguments */
    maTabNoAllocInit(&s_maTab, s_maTab.size);
    argi = -1;
    if (argFlag) {
        GetNonBlankMaybeLexem(lex);
        PassLex(cInput.cc, lex, l, v, h, *parpos2, len, 1);
        *parpos1 = *parpos2;
        if (lex != '(') goto errorlab;
        argi ++;
        GetNonBlankMaybeLexem(lex);
        if (lex != ')') {
            for(;;) {
                cc = aname = cInput.cc;
                PassLex(cInput.cc, lex, l, v, h, pos, len, 1);
                ellipsis = 0;
                if (lex == IDENTIFIER ) {
                    aname = cc;
                } else if (lex == ELIPSIS) {
                    aname = s_cppVarArgsName;
                    pos = macpos;					// hack !!!
                    ellipsis = 1;
                } else goto errorlab;
                PP_ALLOCC(mm, strlen(aname)+1, char);
                strcpy(mm, aname);
                sprintf(tmpBuff, "%x-%x%c%s", pos.file, pos.line,
                        LINK_NAME_CUT_SYMBOL, aname);
                PP_ALLOCC(argLinkName, strlen(tmpBuff)+1, char);
                strcpy(argLinkName, tmpBuff);
                SM_ALLOC(ppMemory, maca, S_macroArgTabElem);
                FILL_macroArgTabElem(maca, mm, argLinkName, argi);
                foundIndex = maTabAdd(&s_maTab, maca);
                argi ++;
                GetNonBlankMaybeLexem(lex);
                tmppp=parpos1; parpos1=parpos2; parpos2=tmppp;
                PassLex(cInput.cc, lex, l, v, h, *parpos2, len, 1);
                if (! ellipsis) {
                    addTrivialCxReference(s_maTab.tab[foundIndex]->linkName, TypeMacroArg,StorageDefault,
                                          &pos, UsageDefined);
                    handleMacroDefinitionParameterPositions(argi, &macpos, parpos1, &pos, parpos2, 0);
                }
                if (lex == ELIPSIS) {
                    // GNU ELLIPSIS ?????
                    GetNonBlankMaybeLexem(lex);
                    PassLex(cInput.cc, lex, l, v, h, *parpos2, len, 1);
                }
                if (lex == ')') break;
                if (lex != ',') break;
                GetNonBlankMaybeLexem(lex);
            }
            handleMacroDefinitionParameterPositions(argi, &macpos, parpos1, &s_noPos, parpos2, 1);
        } else {
            PassLex(cInput.cc,lex,l,v,h,*parpos2, len,1); // added 12.5.?????
            handleMacroDefinitionParameterPositions(argi, &macpos, parpos1, &s_noPos, parpos2, 1);
        }
    }
    /* process macro body */
    msize = MACRO_UNIT_SIZE;
    sizei = 0;
    PP_ALLOC(macbody, S_macroBody);
    PP_ALLOCC(mbody, msize+MAX_LEXEM_SIZE, char);
    bodyReadingFlag = 1;
    GetNonBlankMaybeLexem(lex);
    cc = cInput.cc;
    PassLex(cInput.cc, lex, l, v, h, pos, len, 1);
    while (lex != '\n') {
        while(sizei<msize && lex != '\n') {
            FILL_macroArgTabElem(&mmaca,cc,NULL,0);
            if (lex==IDENTIFIER && maTabIsMember(&s_maTab,&mmaca,&foundIndex)){
                /* macro argument */
                addTrivialCxReference(s_maTab.tab[foundIndex]->linkName, TypeMacroArg,StorageDefault,
                                      &pos, UsageUsed);
                ddd = mbody+sizei;
                PutLexToken(CPP_MAC_ARG, ddd);
                PutLexInt(s_maTab.tab[foundIndex]->order, ddd);
                PutLexPosition(pos.file, pos.line,pos.col,ddd);
                sizei = ddd - mbody;
            } else {
                if (lex==IDENT_TO_COMPLETE
                    || (lex == IDENTIFIER &&
                        POSITION_EQ(pos, s_cxRefPos))) {
                    s_cache.activeCache = 0;
                    s_olstringFound = 1; s_olstringInMbody = pp->linkName;
                }
                ddd = mbody+sizei;
                PutLexToken(lex, ddd);
                for(; cc<cInput.cc; ddd++,cc++)*ddd= *cc;
                sizei = ddd - mbody;
            }
            GetNonBlankMaybeLexem(lex);
            cc = cInput.cc;
            PassLex(cInput.cc,lex,l,v,h,pos, len,1);
        }
        if (lex != '\n') {
            msize += MACRO_UNIT_SIZE;
            PP_REALLOCC(mbody,msize+MAX_LEXEM_SIZE,char,
                        msize+MAX_LEXEM_SIZE-MACRO_UNIT_SIZE);
        }
    }
endOfBody:
    assert(sizei>=0);
    PP_REALLOCC(mbody, sizei, char, msize+MAX_LEXEM_SIZE);
    msize = sizei;
    if (argi > 0) {
//{static int c=0;fprintf(dumpOut,"margs0#%d\n",c+=argi);}
        PP_ALLOCC(argNames, argi, char*);
        memset(argNames, 0, argi*sizeof(char*));
        maTabMap2(&s_maTab, setMacroArgumentName, argNames);
    } else argNames = NULL;
    FILL_macroBody(macbody, argi, msize, mname, argNames, mbody);
    pp->u.mbody = macbody;
    //{static int c=0;fprintf(dumpOut,"#def#%d\n",c++);}
    addMacroToTabs(pp,mname);
    assert(s_opt.taskRegime);
    if (CX_REGIME()) {
        addCxReference(pp, &macpos, UsageDefined,s_noneFileIndex, s_noneFileIndex);
    }
    return;
endOfMacArg:	assert(0);
endOfFile:
    if (bodyReadingFlag) goto endOfBody;
    return;
errorlab:
    return;
}

/* ************************** -D option ************************** */

void addMacroDefinedByOption(char *opt) {
    char *cc,*nopt;
    int args;
    PP_ALLOCC(nopt,strlen(opt)+3,char);
    strcpy(nopt,opt);
    cc = nopt;
    while (isalpha(*cc) || isdigit(*cc) || *cc == '_') cc++;
    args = 0;
    if (*cc == '=') {
        *cc = ' ';
    } else if (*cc==0) {
        *cc++ = ' ';
        *cc++ = '1';
        *cc = 0;
    } else if (*cc=='(') {
        args = 1;
    }
    initInput(NULL, NULL,nopt,NULL);
    cFile.lineNumber = 1;
    processDefine(args);
}

/* ****************************** #UNDEF ************************* */

static void processUnDefine(void) {
    int lex,l,h,v,ii,len;
    char *cc;
    S_position pos;
    Symbol dd,*pp,*memb;
    GetLex(lex);
    cc = cInput.cc;
    PassLex(cInput.cc,lex,l,v,h,pos, len,1);
    testCxrefCompletionId(&lex,cc,&pos);
    if (IS_IDENTIFIER_LEXEM(lex)) {
        log_debug(": undef macro %s",cc);

        fillSymbol(&dd, cc, cc, pos);
        fillSymbolBits(&dd.bits, ACC_DEFAULT, TypeMacro, StorageNone);

        assert(s_opt.taskRegime);
        /* !!!!!!!!!!!!!! tricky, add macro with mbody == NULL !!!!!!!!!! */
        /* this is because of monotonicity for caching, just adding symbol */
        if (symTabIsMember(s_symTab, &dd, &ii, &memb)) {
            if (CX_REGIME()) {
                addCxReference(memb, &pos, UsageUndefinedMacro,s_noneFileIndex, s_noneFileIndex);
            }

            PP_ALLOC(pp, Symbol);
            fillSymbol(pp, memb->name, memb->linkName, pos);
            fillSymbolBits(&pp->bits, ACC_DEFAULT, TypeMacro, StorageNone);

            addMacroToTabs(pp,memb->name);
        }
    }
    while (lex != '\n') {GetLex(lex); PassLex(cInput.cc,lex,l,v,h,pos, len,1);}
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

static void genCppIfElseReference(int level, S_position *pos, int usage) {
    char                ttt[TMP_STRING_SIZE];
    S_position			dp;
    S_cppIfStack       *ss;
    if (level > 0) {
      PP_ALLOC(ss, S_cppIfStack);
      FILL_cppIfStack(ss, *pos, cFile.ifstack);
      cFile.ifstack = ss;
    }
    if (cFile.ifstack!=NULL) {
      dp = cFile.ifstack->pos;
      sprintf(ttt,"CppIf%x-%x-%d", dp.file, dp.col, dp.line);
      addTrivialCxReference(ttt, TypeCppIfElse,StorageDefault, pos, usage);
      if (level < 0) cFile.ifstack = cFile.ifstack->next;
    }
}

static int cppDeleteUntilEndElse(int untilEnd) {
    int lex,l,h,v,len;
    int deep;
    S_position pos;
    deep = 1;
    while (deep > 0) {
        GetLex(lex);
        PassLex(cInput.cc,lex,l,v,h,pos, len,1);
        if (lex==CPP_IF || lex==CPP_IFDEF || lex==CPP_IFNDEF) {
            genCppIfElseReference(1, &pos, UsageDefined);
            deep++;
        } else if (lex == CPP_ENDIF) {
            deep--;
            genCppIfElseReference(-1, &pos, UsageUsed);
        } else if (lex == CPP_ELIF) {
            genCppIfElseReference(0, &pos, UsageUsed);
            if (deep == 1 && !untilEnd) {
                log_debug("#elif ");
                return(UNTIL_ELIF);
            }
        } else if (lex == CPP_ELSE) {
            genCppIfElseReference(0, &pos, UsageUsed);
            if (deep == 1 && !untilEnd) {
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
        warning(ERR_ST,"end of file in cpp conditional");
    }
    return(UNTIL_ENDIF);
}

static void execCppIf(int deleteSource) {
    int onElse;
    if (deleteSource==0) cFile.ifDeep ++;
    else {
        onElse = cppDeleteUntilEndElse(0);
        if (onElse==UNTIL_ELSE) {
            /* #if #else */
            cFile.ifDeep ++;
        } else if (onElse==UNTIL_ELIF) processIf();
    }
}

static void processIfdef(int isIfdef) {
    int lex,l,h,v;
    int ii,mm,len;
    Symbol pp,*memb;
    char *cc;
    S_position pos;
    int deleteSrc;
    GetLex(lex);
    cc = cInput.cc;
    PassLex(cInput.cc,lex,l,v,h,pos, len,1);
    testCxrefCompletionId(&lex,cc,&pos);

    if (! IS_IDENTIFIER_LEXEM(lex)) return;

    fillSymbol(&pp, cc, cc, s_noPos);
    fillSymbolBits(&pp.bits, ACC_DEFAULT, TypeMacro, StorageNone);

    assert(s_opt.taskRegime);
    mm = symTabIsMember(s_symTab,&pp,&ii,&memb);
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

int cexpyylex(void) {
    int l,v,lex,par,ii,res,mm,len;
    char *cc;
    unsigned h;
    Symbol dd,*memb;
    S_position pos;
    lex = yylex();
    if (IS_IDENTIFIER_LEXEM(lex)) {
        // this is useless, as it would be set to 0 anyway
        lex = cexpTranslateToken(CONSTANT, 0);
    } else if (lex == CPP_DEFINED_OP) {
        GetNonBlankMaybeLexem(lex);
        cc = cInput.cc;
        PassLex(cInput.cc,lex,l,v,h,pos, len,1);
        if (lex == '(') {
            par = 1;
            GetNonBlankMaybeLexem(lex);
            cc = cInput.cc;
            PassLex(cInput.cc,lex,l,v,h,pos, len,1);
        } else {
            par = 0;
        }

        if (! IS_IDENTIFIER_LEXEM(lex)) return(0);

        fillSymbol(&dd, cc, cc, s_noPos);
        fillSymbolBits(&dd.bits, ACC_DEFAULT, TypeMacro, StorageNone);

        log_debug("(%s)", dd.name);

        mm = symTabIsMember(s_symTab,&dd,&ii,&memb);
        if (mm && memb->u.mbody == NULL) mm = 0;   // undefined macro
        assert(s_opt.taskRegime);
        if (CX_REGIME()) {
            if (mm) addCxReference(&dd, &pos, UsageUsed,s_noneFileIndex, s_noneFileIndex);
        }
        /* following call sets uniyylval */
        res = cexpTranslateToken(CONSTANT, mm);
        if (par) {
            GetNonBlankMaybeLexem(lex);
            PassLex(cInput.cc,lex,l,v,h,pos, len,1);
            if (lex != ')' && s_opt.taskRegime!=RegimeEditServer) {
                warning(ERR_ST,"missing ')' after defined( ");
            }
        }
        lex = res;
    } else {
        lex = cexpTranslateToken(lex, uniyylval->bbinteger.data);
    }
    return(lex);
endOfMacArg:	assert(0);
endOfFile:;
    return(0);
}

static void processIf(void) {
    int res=1,lex;
    s_ifEvaluation = 1;
    log_debug(": #if");
    res = cexpyyparse();
    do lex = yylex(); while (lex != '\n');
    s_ifEvaluation = 0;
    execCppIf(! res);
}

static void processPragma(void) {
    int lex,l,v,len,ii;
    unsigned h;
    char *mname, *fname;
    S_position pos;
    Symbol *pp;

    GetLex(lex);
    if (lex == IDENTIFIER && !strcmp(cInput.cc, "once")) {
        PassLex(cInput.cc,lex,l,v,h,pos, len,1);
        fname = simpleFileName(s_fileTab.tab[pos.file]->name);
        sprintf(tmpBuff, "PragmaOnce-%s", fname);
        PP_ALLOCC(mname, strlen(tmpBuff)+1, char);
        strcpy(mname, tmpBuff);

        PP_ALLOC(pp, Symbol);
        fillSymbol(pp, mname, mname, pos);
        fillSymbolBits(&pp->bits, ACC_DEFAULT, TypeMacro, StorageNone);

        symTabAdd(s_symTab,pp,&ii);
    }
    while (lex != '\n') {PassLex(cInput.cc,lex,l,v,h,pos, len,1); GetLex(lex);}
    PassLex(cInput.cc,lex,l,v,h,pos, len,1);
    return;
endOfMacArg:	assert(0);
endOfFile:;
}

/* ***************************************************************** */
/*                                 CPP                               */
/* ***************************************************************** */

#define AddHtmlCppReference(pos) {\
    if (s_opt.taskRegime==RegimeHtmlGenerate && s_opt.htmlNoColors==0) {\
        addTrivialCxReference("_",TypeCppAny,StorageDefault,&pos,UsageUsed);\
    }\
}

static int processCppConstruct(int lex) {
    int l,v,len;
    unsigned h;
    S_position pos;

    PassLex(cInput.cc,lex,l,v,h,pos, len,1);
/*	if (s_opt.debug) fprintf(dumpOut,"%s ",s_tokenName[lex]); */
    switch (lex) {
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
        processIfdef(1);
        break;
    case CPP_IFNDEF:
        genCppIfElseReference(1, &pos, UsageDefined);
        processIfdef(0);
        break;
    case CPP_IF:
        genCppIfElseReference(1, &pos, UsageDefined);
        processIf();
        break;
    case CPP_ELIF:
        log_debug("#elif");
        if (cFile.ifDeep) {
            genCppIfElseReference(0, &pos, UsageUsed);
            cFile.ifDeep --;
            cppDeleteUntilEndElse(1);
        } else if (s_opt.taskRegime!=RegimeEditServer) {
            warning(ERR_ST,"unmatched #elif");
        }
        break;
    case CPP_ELSE:
        log_debug("#else");
        if (cFile.ifDeep) {
            genCppIfElseReference(0, &pos, UsageUsed);
            cFile.ifDeep --;
            cppDeleteUntilEndElse(1);
        } else if (s_opt.taskRegime!=RegimeEditServer) {
            warning(ERR_ST,"unmatched #else");
        }
        break;
    case CPP_ENDIF:
        log_debug("#endif");
        if (cFile.ifDeep) {
            cFile.ifDeep --;
            genCppIfElseReference(-1, &pos, UsageUsed);
        } else if (s_opt.taskRegime!=RegimeEditServer) {
            warning(ERR_ST,"unmatched #endif");
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
        GetLex(lex);
        while (lex != '\n') {PassLex(cInput.cc,lex,l,v,h,pos, len,1); GetLex(lex);}
        PassLex(cInput.cc,lex,l,v,h,pos, len,1);
        break;
    default: assert(0);
    }
    return(1);
endOfMacArg:	assert(0);
endOfFile:
    return(0);
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
    if (cInput.macname != NULL && !strcmp(name,cInput.macname)) return(1);
    for(i=0; i<macroStackIndex; i++) {
        ll = &macStack[i];
/*fprintf(dumpOut,"testing '%s' against '%s'\n",name,ll->macname);*/
        if (ll->macname != NULL && !strcmp(name,ll->macname)) return(1);
    }
    return(0);
}

static void expandMacroArgument(S_lexInput *argb) {
    Symbol sd,*memb;
    char *buf,*cc,*cc2,*bcc, *tbcc;
    int nn,ii,lex,line,val,bsize,failedMacroExpansion,len;
    S_position pos;
    unsigned hash;
    PrependMacroInput(*argb);
    cInput.margExpFlag = II_MACRO_ARG;
    bsize = MACRO_UNIT_SIZE;
    PP_ALLOCC(buf,bsize+MAX_LEXEM_SIZE,char);
    bcc = buf;
    for(;;) {
    nextLexem:
        GetLexA(lex,cc);
        cc2 = cInput.cc;
        PassLex(cInput.cc,lex, line, val, hash, pos, len, macroStackIndex == 0);
        nn = ((char*)cInput.cc) - cc;
        assert(nn >= 0);
        memcpy(bcc, cc, nn);
        // a hack, it is copied, but bcc will be increased only if not
        // an expanding macro, this is because 'macroCallExpand' can
        // read new lexbuffer and destroy cInput, so copy it now.
        failedMacroExpansion = 0;
        if (lex == IDENTIFIER) {
            fillSymbol(&sd, cc2, cc2, s_noPos);
            fillSymbolBits(&sd.bits, ACC_DEFAULT, TypeMacro, StorageNone);
            if (symTabIsMember(s_symTab,&sd,&ii,&memb)) {
                /* it is a macro, provide macro expansion */
                if (macroCallExpand(memb,&pos)) goto nextLexem;
                else failedMacroExpansion = 1;
            }
        }
        if (failedMacroExpansion) {
            tbcc = bcc;
            assert(memb!=NULL);
            if (memb->u.mbody!=NULL && cyclicCall(memb->u.mbody)) PutLexToken(IDENT_NO_CPP_EXPAND, tbcc);
            //& else PutLexToken(IDENT_NO_CPP_EXPAND, tbcc);
        }
        bcc += nn;
        TestPPBufOverflow(bcc,buf,bsize);
    }
endOfMacArg:
    cInput = macStack[--macroStackIndex];
    PP_REALLOCC(buf,bcc-buf,char,bsize+MAX_LEXEM_SIZE);
    FILL_lexInput(argb,buf,bcc,buf,NULL,II_NORMAL);
    return;
endOfFile:
    assert(0);
}

static void cxAddCollateReference( char *sym, char *cs, S_position *pos ) {
    char ttt[TMP_STRING_SIZE];
    S_position pps;
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
    int line, val, lex, nlex, len1, bsize, len;
    S_position pos,respos;
    unsigned hash;
    ncc = *ancc;
    lbcc = *albcc;
    bcc = *abcc;
    bsize = *absize;
    val=0; // compiler
    if (NextLexToken(lbcc) == CPP_MAC_ARG) {
        bcc = lbcc;
        GetLexToken(lex,lbcc);
        assert(lex==CPP_MAC_ARG);
        PassLex(lbcc, lex, line, val, hash, respos, len, 0);
        cc = actArgs[val].a; ccfin = actArgs[val].fin;
        lbcc = NULL;
        while (cc < ccfin) {
            cc0 = cc;
            GetLexToken(lex, cc);
            PassLex(cc, lex, line, val, hash, respos, len, 0);
            lbcc = bcc;
            assert(cc>=cc0);
            memcpy(bcc, cc0, cc-cc0);
            bcc += cc-cc0;
            TestPPBufOverflow(bcc,buf,bsize);
        }
    }
    if (NextLexToken(ncc) == CPP_MAC_ARG) {
        GetLexToken(lex, ncc);
        PassLex(ncc, lex, line, val, hash, pos, len, 0);
        cc = actArgs[val].a; ccfin = actArgs[val].fin;
    } else {
        cc = ncc;
        GetLexToken(lex, ncc);
        PassLex(ncc, lex, line, val, hash, pos, len, 0);
        ccfin = ncc;
    }
    /* now collate *lbcc and *cc */
    // berk, do not pre-compute, lbcc can be NULL!!!!
    //& nlt = NextLexToken(lbcc);
    if (lbcc!=NULL && cc < ccfin && IS_IDENTIFIER_LEXEM(NextLexToken(lbcc))) {
        nlex = NextLexToken(cc);
        if (IS_IDENTIFIER_LEXEM(nlex) || nlex == CONSTANT
                    || nlex == LONG_CONSTANT || nlex == FLOAT_CONSTANT
                    || nlex == DOUBLE_CONSTANT ) {
            /* TODO collation of all lexem pairs */
            len1 = strlen(lbcc+IDENT_TOKEN_SIZE);
            GetLexToken(lex, cc);
            occ = cc;
            PassLex(cc, lex, line, val, hash, respos, len, 0);
            bcc = lbcc + IDENT_TOKEN_SIZE + len1;
            assert(*bcc==0);
            if (IS_IDENTIFIER_LEXEM(lex)) {
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
        GetLexToken(lex, cc);
        PassLex(cc, lex, line, val, hash, pos, len, 0);
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
    int v,h,c,lv,lex,len;
    S_position pos;
    bcc = res;
    *bcc = 0;
    c=0; v=0;
    cc = lb->a;
    while (cc < lb->fin) {
        GetLexToken(lex,cc);
        lcc = cc;
        PassLex(cc, lex, lv,v,h,pos, len,c);
        if (IS_IDENTIFIER_LEXEM(lex)) {
            sprintf(bcc, "%s", lcc);
            bcc+=strlen(bcc);
        } else if (lex==STRING_LITERAL) {
            sprintf(bcc,"\"%s\"", lcc);
            bcc+=strlen(bcc);
        } else if (lex==CONSTANT) {
            sprintf(bcc,"%d", v);
            bcc+=strlen(bcc);
        } else if (lex < 256) {
            sprintf(bcc,"%c",lex);
            bcc+=strlen(bcc);
        }
    }
}


/* ************************************************************** */
/* ********************* macro body replacement ***************** */
/* ************************************************************** */

static void crMacroBody(S_lexInput *macBody,
                        S_macroBody *mb,
                        struct lexInput *actArgs,
                        int actArgn
                        ) {
    char *cc,*cc0,*cfin,*bcc,*lbcc;
    int i,lex,line,val,len,bsize,lexlen;
    S_position pos, hpos;
    unsigned hash;
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
        GetLexToken(lex, cc);
        PassLex(cc, lex, line, val, hash, pos, lexlen, 0);
        if (lex==CPP_COLLATION && lbcc!=NULL && cc<cfin) {
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
        GetLexToken(lex, cc);
        PassLex(cc, lex, line, val, hash, hpos, lexlen, 0);
        if (lex == CPP_MAC_ARG) {
            len = actArgs[val].fin - actArgs[val].a;
            TestMBBufOverflow(bcc,len,buf2,bsize);
            memcpy(bcc, actArgs[val].a, len);
            bcc += len;
        } else if (lex=='#' && cc<cfin && NextLexToken(cc)==CPP_MAC_ARG) {
            GetLexToken(lex, cc);
            PassLex(cc, lex, line, val, hash, pos, lexlen, 0);
            assert(lex == CPP_MAC_ARG);
            PutLexToken(STRING_LITERAL, bcc);
            TestMBBufOverflow(bcc,MACRO_UNIT_SIZE,buf2,bsize);
            macArgsToString(bcc, &actArgs[val]);
            len = strlen(bcc)+1;
            bcc += len;
            PutLexPosition(hpos.file, hpos.line, hpos.col, bcc);
            if (len >= MACRO_UNIT_SIZE-15) {
                error(ERR_INTERNAL,"size of #macro_argument exceeded MACRO_UNIT_SIZE");
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

    FILL_lexInput(macBody,buf2,bcc,buf2,mb->name,II_MACRO);

}

/* *************************************************************** */
/* ******************* MACRO CALL PROCESS ************************ */
/* *************************************************************** */

#define GetNotLineLexA(lex,lastlexaddr) {\
    GetLexA(lex,lastlexaddr);\
    while (lex == LINE_TOK || lex == '\n') {\
        PassLex(cInput.cc,lex,line,val,h,pos, len, macroStackIndex == 0);\
        GetLexA(lex,lastlexaddr);\
    }\
}
#define GetNotLineLex(lex) {\
    char *lastlexaddr; GetNotLineLexA(lex,lastlexaddr);\
}

static void getActMacroArgument(char *cc,
                                int *llex,
                                S_position *mpos,
                                S_position **parpos1,
                                S_position **parpos2,
                                S_lexInput *actArg,
                                S_macroBody *mb,
                                int actArgi
                                ) {
    char *buf,*bcc;
    int line,val,len, poffset;
    S_position pos;
    unsigned h;
    int bufsize,lex,deep;
    lex = *llex;
    bufsize = MACRO_ARG_UNIT_SIZE;
    deep = 0;
    PP_ALLOCC(buf, bufsize+MAX_LEXEM_SIZE, char);
    bcc = buf;
    /* if lastArgument, collect everything there */
    poffset = 0;
    while (((lex != ',' || actArgi+1==mb->argn) && lex != ')') || deep > 0) {
/* The following should be equivalent to the loop condition
        if (lex == ')' && deep <= 0) break;
        if (lex == ',' && deep <= 0 && ! lastArgument) break;
*/
        if (lex == '(') deep ++;
        if (lex == ')') deep --;
        for(;cc < cInput.cc; cc++,bcc++) *bcc = *cc;
        if (bcc-buf >= bufsize) {
            bufsize += MACRO_ARG_UNIT_SIZE;
            PP_REALLOCC(buf, bufsize+MAX_LEXEM_SIZE, char,
                    bufsize+MAX_LEXEM_SIZE-MACRO_ARG_UNIT_SIZE);
        }
        GetNotLineLexA(lex,cc);
        PassLex(cInput.cc,lex,line,val,h, (**parpos2), len, macroStackIndex == 0);
        if ((lex == ',' || lex == ')') && deep == 0) {
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
            warning(ERR_ST,"[getActMacroArgument] unterminated macro call");
        }
    }
    PP_REALLOCC(buf, bcc-buf, char, bufsize+MAX_LEXEM_SIZE);
    FILL_lexInput(actArg,buf,bcc,buf,cInput.macname,II_NORMAL);
    *llex = lex;
    return;
}

static struct lexInput *getActualMacroArguments(S_macroBody *mb, S_position *mpos,
                                                S_position *lparpos) {
    char *cc;
    int lex,line,val,len;
    S_position pos, ppb1, ppb2, *parpos1, *parpos2;
    unsigned h;
    int actArgi = 0;
    struct lexInput *actArgs;
    ppb1 = *lparpos;
    ppb2 = *lparpos;
    parpos1 = &ppb1;
    parpos2 = &ppb2;
    PP_ALLOCC(actArgs,mb->argn,struct lexInput);
    GetNotLineLexA(lex,cc);
    PassLex(cInput.cc,lex,line,val,h, pos, len, macroStackIndex == 0);
    if (lex == ')') {
        *parpos2 = pos;
        handleMacroUsageParameterPositions(0, mpos, parpos1, parpos2, 1);
    } else {
        for(;;) {
            getActMacroArgument(cc,&lex, mpos, &parpos1, &parpos2,
                                &actArgs[actArgi], mb, actArgi);
            actArgi ++ ;
            if (lex != ',' || actArgi >= mb->argn) break;
            GetNotLineLexA(lex,cc);
            PassLex(cInput.cc,lex,line,val,h, pos, len, macroStackIndex == 0);
        }
    }
    if (actArgi!=0) {
        handleMacroUsageParameterPositions(actArgi, mpos, parpos1, parpos2, 1);
    }
    /* fill mising arguments */
    for(;actArgi < mb->argn; actArgi++) {
        FILL_lexInput(&actArgs[actArgi], NULL, NULL, NULL, NULL,II_NORMAL);
    }
    return(actArgs);
endOfMacArg:	assert(0);
endOfFile:
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime!=RegimeEditServer) {
        warning(ERR_ST,"[getActualMacroArguments] unterminated macro call");
    }
    return(NULL);
}

/* **************************************************************** */

static void addMacroBaseUsageRef( Symbol *mdef) {
    int                 ii,rr;
    S_symbolRefItem     ppp,*memb;
    S_reference			*r;
    S_position          basePos;
    FILL_position(&basePos, s_input_file_number, 0, 0);
    FILL_symbolRefItemBits(&ppp.b,TypeMacro,StorageDefault,ScopeGlobal,
                           mdef->bits.accessFlags, CatGlobal,0);
    FILL_symbolRefItem(&ppp,mdef->linkName,
                       cxFileHashNumber(mdef->linkName), // useless, put 0
                       s_noneFileIndex,s_noneFileIndex,
                       ppp.b,NULL,NULL);
    rr = refTabIsMember(&s_cxrefTab, &ppp, &ii, &memb);
    r = NULL;
    if (rr) {
        // this is optimization to avoid multiple base references
        for(r=memb->refs; r!=NULL; r=r->next) {
            if (r->usg.base == UsageMacroBaseFileUsage) break;
        }
    }
    if (rr==0 || r==NULL) {
        addCxReference(mdef,&basePos,UsageMacroBaseFileUsage,
                              s_noneFileIndex, s_noneFileIndex);
    }
}


static int macroCallExpand(Symbol *mdef, S_position *mpos) {
    int lex,line,val,len;
    char *cc2,*freeBase;
    S_position pos, lparpos;
    unsigned h;
    struct lexInput *actArgs,macBody;
    S_macroBody *mb;
    cc2 = cInput.cc;
    mb = mdef->u.mbody;
    if (mb == NULL) return(0);	/* !!!!!         tricky,  undefined macro */
    if (macroStackIndex == 0) { /* call from source, init mem */
        MB_INIT();
    }
//&fprintf(dumpOut,"try to expand macro '%s'\n", mb->name);fflush(dumpOut);
    if (cyclicCall(mb)) return(0);
    PP_ALLOCC(freeBase,0,char);
    if (mb->argn >= 0) {
        GetNotLineLexA(lex,cc2);
        if (lex != '(') {
            cInput.cc = cc2;		/* unget lexem */
            return(0);
        }
        PassLex(cInput.cc,lex,line,val,h, lparpos, len, macroStackIndex == 0);
        actArgs = getActualMacroArguments(mb, mpos, &lparpos);
    } else {
        actArgs = NULL;
    }
    assert(s_opt.taskRegime);
    if (CX_REGIME()) {
        addCxReference(mdef,mpos,UsageUsed,s_noneFileIndex, s_noneFileIndex);
        if (s_opt.taskRegime == RegimeXref) addMacroBaseUsageRef(mdef);
    }
//&fprintf(dumpOut,"cr mbody '%s'\n", mb->name);fflush(dumpOut);
    crMacroBody(&macBody,mb,actArgs,mb->argn);
    PrependMacroInput(macBody);
/*fprintf(dumpOut,"expanding macro '%s'\n", mb->name);fflush(dumpOut);*/
    PP_FREE_UNTIL(freeBase);
    return(1);
endOfMacArg:
    /* unterminated macro call in argument */
    /* TODO unread readed argument */
    cInput.cc = cc2;
    PP_FREE_UNTIL(freeBase);
    return(0);
endOfFile:
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime!=RegimeEditServer) {
        warning(ERR_ST,"[macroCallExpand] unterminated macro call");
    }
    cInput.cc = cc2;
    PP_FREE_UNTIL(freeBase);
    return(0);
}

#ifdef DEBUG
int lexBufDump(struct lexBuf *lb) {
    char *cc;
    int v,h,c,lv,lex,len;
    S_position pos;
    c=0;
    fprintf(dumpOut,"\nlexbufdump [start] \n"); fflush(dumpOut);
    cc = lb->next;
    while (cc < lb->end) {
        GetLexToken(lex,cc);
        if (lex==IDENTIFIER || lex==IDENT_NO_CPP_EXPAND) {
            fprintf(dumpOut,"%s ",cc);
        } else if (lex==IDENT_TO_COMPLETE) {
            fprintf(dumpOut,"!%s! ",cc);
        } else if (lex < 256) {
            fprintf(dumpOut,"%c ",lex);fflush(dumpOut);
        } else if (s_tokenName[lex]==NULL){
            fprintf(dumpOut,"?%d? ",lex);fflush(dumpOut);
        } else {
            fprintf(dumpOut,"%s ",s_tokenName[lex]);fflush(dumpOut);
        }
        PassLex(cc, lex, lv,v,h,pos, len,c);
    }
    fprintf(dumpOut,"lexbufdump [stop]\n");fflush(dumpOut);
    return(0);
}
#endif

/* ************************************************************** */
/*                   caching of input                             */
/* ************************************************************** */
int cachedInputPass(int cpoint, char **cfrom) {
    int lex,line,val,res,len;				// ??redeclaration of len ??
    S_position pos;
    unsigned h,lsize,compsize;
    char *cc,*cto,*ccc;
    assert(cpoint > 0);
    cto = s_cache.cp[cpoint].lbcc;
    ccc = *cfrom;
    res = 1;
    while (ccc < cto) {
        GetLexA(lex,cc);
        PassLex(cInput.cc,lex,line,val,h,pos, len,1);
        compsize = lsize = cInput.cc-cc;
        assert(compsize >= 0);
/*		if (lex == IDENTIFIER) compsize = TOKEN_SIZE+strlen(cc+TOKEN_SIZE)+1;*/
        if (memcmp(cc, ccc, compsize)) {
            cInput.cc = cc;			/* unget last lexem */
            res = 0;
            break;
        }
        if (IS_IDENTIFIER_LEXEM(lex) || (lex>CPP_TOKENS_START&&lex<CPP_TOKENS_END)) {
            if (pos.file==s_cxRefPos.file && pos.line >= s_cxRefPos.line) {
                cInput.cc = cc;			/* unget last lexem */
                res = 0;
                break;
            }
        }
        ccc += lsize;
    }
endOfFile:
    SetCFileConsistency();
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
        if (s_opt.taskRegime==RegimeHtmlGenerate && s_opt.htmlNoColors==0) {\
            char ttt[TMP_STRING_SIZE];\
            sprintf(ttt,"%s-%x", sd->name, idposa->file);\
            addTrivialCxReference(ttt, TypeKeyword,StorageDefault, idposa, UsageUsed);\
            /*&addCxReference(sd, idposa, UsageUsed,s_noneFileIndex, s_noneFileIndex);&*/\
        }\
        return(sd->u.keyWordVal);\
    }\
}

static int processCIdent(unsigned hashval, char *id, S_position *idposa) {
    Symbol *sd,*memb;
    memb = NULL;
/*fprintf(dumpOut,"looking for %s in %d\n",id,s_symTab);*/
    for(sd=s_symTab->tab[hashval]; sd!=NULL; sd=sd->next) {
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


static int processJavaIdent(unsigned hashval, char *id, S_position *idposa) {
    Symbol *sd,*memb;
    memb = NULL;
/*fprintf(dumpOut,"looking for %s in %d\n",id,s_symTab);*/
    for(sd=s_symTab->tab[hashval]; sd!=NULL; sd=sd->next) {
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


static int processCccIdent(unsigned hashval, char *id, S_position *idposa) {
    Symbol *sd,*memb;
    memb = NULL;
/*fprintf(dumpOut,"looking for %s in %d\n",id,s_symTab);*/
    for(sd=s_symTab->tab[hashval]; sd!=NULL; sd=sd->next) {
        if (strcmp(sd->name, id) == 0) {
            if (memb == NULL) memb = sd;
            CHECK_ID_FOR_KEYWORD(sd, idposa);
            if (sd->bits.symType == TypeDefinedOp && s_ifEvaluation) {
                return(CPP_DEFINED_OP);
            }
            break;	// I hope it will work.
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
#if ZERO
    } else if (s_opt.cxrefs == OLO_GET_CURRENT_CLASS ||
               s_opt.cxrefs == OLO_GET_CURRENT_SUPER_CLASS ||
               s_opt.cxrefs == OLO_GET_ENV_VALUE) {
#endif
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
    uniyylval->bbposition.d = pos;\
    uniyylval->bbposition.b = pos;\
    uniyylval->bbposition.e = pos;\
    uniyylval->bbposition.e.col += len;\
}

#define SET_INTEGER_YYLVAL(val, pos, len) {\
        uniyylval->bbinteger.data = val;\
        uniyylval->bbinteger.b = pos;\
        uniyylval->bbinteger.e = pos;\
        uniyylval->bbinteger.e.col += len;\
}

int yylex(void) {
    int         lexem, line, val, len;
    S_position  pos, idpos;
    char		*ch;
    unsigned h;

    len = 0;
 nextYylex:
    GetLexA(lexem, ch);
 contYylex:
    if (lexem < 256) {
        if (lexem == '\n') {
            if (s_ifEvaluation) {
                cInput.cc = ch;
            } else {
                PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
                for(;;) {
                    GetLex(lexem);
                    if (lexem<=CPP_TOKENS_START || lexem>=CPP_TOKENS_END) goto contYylex;
                    if (processCppConstruct(lexem) == 0) goto endOfFile;
                }
            }
        } else {
            PassLex(cInput.cc, lexem, line, val, h, pos, len, macroStackIndex == 0);
            SET_POSITION_YYLVAL(pos, s_tokenLength[lexem]);
        }
        yytext = charText;
        charText[0] = lexem;
        goto finish;
    }
    if (lexem==IDENTIFIER || lexem==IDENT_NO_CPP_EXPAND) {
        register char *id;
        register unsigned h;
        int ii;
        Symbol symbol, *memberP;

        h = 0;//compiler
        id = yytext = cInput.cc;
        PassLex(cInput.cc,lexem,line,val,h,idpos, len,macroStackIndex == 0);
        assert(s_opt.taskRegime);
        if (s_opt.taskRegime == RegimeEditServer) {
//			???????????? isn't this useless
            testCxrefCompletionId(&lexem,yytext,&idpos);
        }
        log_trace("id %s position %d %d %d",yytext,idpos.file,idpos.line,idpos.col);

        fillSymbol(&symbol, yytext, yytext, s_noPos);
        fillSymbolBits(&symbol.bits, ACC_DEFAULT, TypeMacro, StorageNone);

        if ((!LANGUAGE(LANG_JAVA))
            && lexem!=IDENT_NO_CPP_EXPAND
            && symTabIsMember(s_symTab,&symbol,&ii,&memberP)) {
            // following is because the macro check can read new lexBuf,
            // so id would be destroyed
            //&assert(strcmp(id,memberP->name)==0);
            id = memberP->name;
            if (macroCallExpand(memberP,&idpos)) goto nextYylex;
        }
        h = h % s_symTab->size;
        if(LANGUAGE(LANG_C)||LANGUAGE(LAN_YACC)) lexem=processCIdent(h,id,&idpos);
        else if (LANGUAGE(LANG_JAVA)) lexem = processJavaIdent(h, id, &idpos);
        else if (LANGUAGE(LANG_CCC)) lexem = processCccIdent(h, id, &idpos);
        else assert(0);
        goto finish;
    }
    if (lexem == OL_MARKER_TOKEN) {
        PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
        actionOnBlockMarker();
        goto nextYylex;
    }
    if (lexem < MULTI_TOKENS_START) {
        yytext = s_tokenName[lexem];
        PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, s_tokenLength[lexem]);
        goto finish;
    }
    if (lexem == LINE_TOK) {
        PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
        goto nextYylex;
    }
    if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
        val=0;//compiler
        PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
        sprintf(constant,"%d",val);
        SET_INTEGER_YYLVAL(val, pos, len);
        yytext = constant;
        goto finish;
    }
    if (lexem == FLOAT_CONSTANT || lexem == DOUBLE_CONSTANT) {
        yytext = "'fltp constant'";
        PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, len);
        goto finish;
    }
    if (lexem == STRING_LITERAL) {
        yytext = cInput.cc;
        PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
        SET_POSITION_YYLVAL(pos, strlen(yytext));
        goto finish;
    }
    if (lexem == CHAR_LITERAL) {
        val=0;//compiler
        PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
        sprintf(constant,"'%c'",val);
        SET_INTEGER_YYLVAL(val, pos, len);
        yytext = constant;
        goto finish;
    }
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime == RegimeEditServer) {
        yytext = cInput.cc;
        PassLex(cInput.cc,lexem,line,val,h,pos, len,macroStackIndex == 0);
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
    log_trace("!%s(%d) ",yytext, cxMemory->i);
    s_lastReturnedLexem = lexem;
    return(lexem);

 endOfMacArg:
    assert(0);

 endOfFile:
    if ((!LANGUAGE(LANG_JAVA)) && inStacki != 0) {
        popInclude();
        poseCachePoint(1);
        goto nextYylex;
    }
    /* add the test whether in COMPLETION, communication string found */
    return(0);
}	/* end of yylex() */

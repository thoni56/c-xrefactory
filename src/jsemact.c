#include "jsemact.h"

#include "commons.h"
#include "globals.h"
#include "classcaster.h"
#include "misc.h"
#include "extract.h"
#include "yylex.h"
#include "semact.h"
#include "cxref.h"
#include "classfilereader.h"
#include "filedescriptor.h"
#include "html.h"
#include "editor.h"
#include "enumTxt.h"
#include "symbol.h"
#include "javafqttab.h"
#include "list.h"

#include "java_parser.x"

#include "parsers.h"
#include "recyacc.h"
#include "jslsemact.h"

#include "protocol.h"

#include "log.h"


S_javaStat *s_javaStat;
S_javaStat s_initJavaStat;


#define IsJavaReferenceType(m) (m==TypeStruct || m==TypeArray)

static int javaNotFqtUsageCorrection(Symbol *sym, int usage);


void fill_nestedSpec(S_nestedSpec *nestedSpec, struct symbol *cl,
                     char membFlag, short unsigned  accFlags) {
    nestedSpec->cl = cl;
    nestedSpec->membFlag = membFlag;
    nestedSpec->accFlags = accFlags;
}

void fillJavaStat(S_javaStat *javaStat, S_idList *className, S_typeModifier *thisType, Symbol *thisClass,
                  int currentNestedIndex, char *currentPackage, char *unnamedPackagePath,
                  char *namedPackagePath, S_symbolTable *locals, S_idList *lastParsedName,
                  unsigned methodModifiers, S_currentlyParsedCl parsingPositions, int classFileIndex,
                  S_javaStat *next) {

    javaStat->className = className;
    javaStat->thisType = thisType;
    javaStat->thisClass = thisClass;
    javaStat->currentNestedIndex = currentNestedIndex;
    javaStat->currentPackage = currentPackage;
    javaStat->unnamedPackagePath = unnamedPackagePath;
    javaStat->namedPackagePath = namedPackagePath;
    javaStat->locals = locals;
    javaStat->lastParsedName = lastParsedName;
    javaStat->methodModifiers = methodModifiers;
    javaStat->cp = parsingPositions;
    javaStat->classFileIndex = classFileIndex;
    javaStat->next = next;
}


char *javaCreateComposedName(char *prefix,
                             S_idList *className,
                             int classNameSeparator,
                             char *name,
                             char *resBuff,
                             int resBuffSize
                             ) {
    int len, ll, sss;
    char *ln;
    char separator;
    S_idList *ii;

    if (name == NULL) name = "";
    if (prefix == NULL) prefix = "";
    sss = 0;
    len = 0;
    ll = strlen(prefix);
    if (ll!=0) sss=1;
    len += ll;
    for (ii=className; ii!=NULL; ii=ii->next) {
        if (sss) len ++;
        len += strlen(ii->fname);
        sss = 1;
    }
    ll = strlen(name);
    if (sss && ll!=0) len++;
    len += ll;
    if (resBuff == NULL) {
        XX_ALLOCC(ln, len+1, char);
    } else {
        assert(len < resBuffSize);
        ln = resBuff;
    }
    ll = strlen(name);
    len -= ll;
    strcpy(ln+len,name);
    if (ll == 0) sss = 0;
    else sss = 1;
    separator = '.';
    for (ii=className; ii!=NULL; ii=ii->next) {
        if (sss) {
            len --;
            ln[len] = separator;
        }
        ll = strlen(ii->fname);
        len -= ll;
        strncpy(ln+len,ii->fname,ll);
        sss = 1;
        assert(ii->nameType==TypeStruct || ii->nameType==TypePackage
                || ii->nameType==TypeExpression);
        if (ii->nameType==TypeStruct &&
                ii->next!=NULL && ii->next->nameType==TypeStruct) {
            separator = '$';
        } else {
            separator = classNameSeparator;
        }
    }
    ll = strlen(prefix);
    if (sss && ll!=0) {len --; ln[len] = separator; }
    len -= ll;
    strncpy(ln+len, prefix, ll);
    assert(len == 0);
    return(ln);
}

void javaCheckForPrimaryStart(S_position *cpos, S_position *bpos) {
    if (s_opt.taskRegime != RegimeEditServer) return;
    if (POSITION_EQ(s_cxRefPos, *cpos)) {
        s_primaryStartPosition = *bpos;
    }
}

void javaCheckForPrimaryStartInNameList(S_idList *name, S_position *pp) {
    S_idList *ll;
    if (s_opt.taskRegime != RegimeEditServer) return;
    for(ll=name; ll!=NULL; ll=ll->next) {
        javaCheckForPrimaryStart(&ll->id.p, pp);
    }
}

void javaCheckForStaticPrefixStart(S_position *cpos, S_position *bpos) {
    if (s_opt.taskRegime != RegimeEditServer) return;
    if (POSITION_EQ(s_cxRefPos, *cpos)) {
        s_staticPrefixStartPosition = *bpos;
    }
}

void javaCheckForStaticPrefixInNameList(S_idList *name, S_position *pp) {
    S_idList *ll;
    if (s_opt.taskRegime != RegimeEditServer) return;
    for(ll=name; ll!=NULL; ll=ll->next) {
        javaCheckForStaticPrefixStart(&ll->id.p, pp);
    }
}

S_position *javaGetNameStartingPosition(S_idList *name) {
    S_idList *ll;
    S_position *res;
    res = &s_noPos;
    for(ll=name; ll!=NULL; ll=ll->next) {
        res = &ll->id.p;
    }
    return(res);
}

static S_reference *javaAddClassCxReference(Symbol *dd, S_position *pos, unsigned usage) {
    S_reference *res;
    res = addCxReference(dd, pos, usage, s_noneFileIndex, s_noneFileIndex);
    return(res);
}

static void javaAddNameCxReference(S_idList *id, unsigned usage) {
    char *cname;
    Symbol dd;

    assert(id != NULL);
    cname = javaCreateComposedName(NULL,id,'/',NULL,tmpMemory,SIZE_TMP_MEM);
    fillSymbol(&dd, id->id.name, cname, id->id.p);
    fillSymbolBits(&dd.bits, ACCESS_DEFAULT, id->nameType, StorageNone);

    /* if you do something else do attention on the union initialisation */
    addCxReference(&dd, &id->id.p, usage,s_noneFileIndex, s_noneFileIndex);
}

Symbol *javaAddType(S_idList *class, Access access, S_position *p) {
    Symbol *dd;
    dd = javaTypeSymbolDefinition(class, access, TYPE_ADD_YES);
    dd->bits.access = access;
    addCxReference(dd, p, UsageDefined,s_noneFileIndex, s_noneFileIndex);
    htmlAddJavaDocReference(dd, p, s_noneFileIndex, s_noneFileIndex);
    return(dd);
}

void javaAddNestedClassesAsTypeDefs(Symbol *cc, S_idList *oclassname,
                                    int accessFlags) {
    S_symStructSpec *ss;
    S_idList	ll;
    Symbol        *nn;
    int i;

    assert(cc && cc->bits.symType==TypeStruct);
    ss = cc->u.s;
    assert(ss);
    for(i=0; i<ss->nestedCount; i++) {
        if (ss->nest[i].membFlag) {
            nn = ss->nest[i].cl;
            assert(nn);
            //& XX_ALLOC(ll, S_idList);
            fillId(&ll.id, nn->name, cc, s_noPos);
            fillIdList(&ll, ll.id, nn->name,TypeStruct,oclassname);
            javaTypeSymbolDefinition(&ll, accessFlags, TYPE_ADD_YES);
        }
    }
}

// resName can be NULL!!!
static int javaFindFile0( char *classPath,char *slash,char *name,
                          char *suffix, char **resName, struct stat *stt) {
    char            fname[MAX_FILE_NAME_SIZE];
    char			*ffn,*ss;
    int				res;
    res = 0;
    ss = strmcpy(fname,classPath);
    ss = strmcpy(ss,slash);
    ss = strmcpy(ss,name);
    ss = strmcpy(ss, suffix);
    assert(ss-fname+1 < MAX_FILE_NAME_SIZE);
    ffn = normalizeFileName(fname,s_cwd);
//&	fprintf(dumpOut, "looking for file %s\n", ffn);
    if (statb(ffn,stt) == 0) {
//&	fprintf(dumpOut, "found in  buffer file %s\n", ffn);
        res = 1;
    }
    if (res && resName!=NULL) {
        XX_ALLOCC(*resName, strlen(ffn)+1, char);
        strcpy(*resName, ffn);
    }
    return(res);
}

static int specialFileNameCasesCheck(char *fname) {
#ifdef __WIN32__
    WIN32_FIND_DATA		fdata;
    HANDLE				han;
    int					dif;
    char				*ss;
    S_editorBuffer		*buff;
    // first check if the file is from editor
    buff = editorGetOpenedBuffer(fname);
    if (buff != NULL) return(1);
    // there is only drive name before the first slash, copy it.
    ss = lastOccurenceInString(fname, '/');
    if (ss==NULL) ss = lastOccurenceInString(fname, '\\');
    if (ss==NULL) return(1);
    log_trace("translating %s",ttt);
    han = FindFirstFile(fname, &fdata);
    if (han == INVALID_HANDLE_VALUE) return(1);
    dif = strcmp(ss+1, fdata.cFileName);
    FindClose(han);
    log_trace("result %s",ttt);
    return(dif==0);
#else
    return(1);
#endif
}

/* TODO this function strangely ressembles to javaFindFile, join them ????*/
bool javaTypeFileExist(S_idList *name) {
    char            *fname;
    struct stat		stt;
    S_stringList	*cp;
    int				i;
    S_idList	tname;

    if (name==NULL) return false;
    tname = *name;
    tname.nameType = TypeStruct;

    // first check if I have its class in file table
    // hmm this is causing problems in on-line editing when some misspelled
    // completion strings were added as types, then a package is resolved
    // as a type and a File from inside directory is not completed.
    // I try to solve it by requiring sourcefile index
    fname = javaCreateComposedName(":", &tname, '/', "class", tmpMemory, SIZE_TMP_MEM);
    fname[1] = ZIP_SEPARATOR_CHAR;

    if  (fileTabExists(&s_fileTab, fname+1)) {
        int fileIndex = fileTabLookup(&s_fileTab, fname+1);
        if (s_fileTab.tab[fileIndex]->b.sourceFile != s_noneFileIndex) {
            return true;
        }
    }

    if (s_javaStat->unnamedPackagePath != NULL) {		/* unnamed package */
        fname = javaCreateComposedName(NULL,&tname,SLASH,"java",tmpMemory,SIZE_TMP_MEM);
        log_trace("testing existence of file '%s'", fname);
        if (statb(fname,&stt)==0 && specialFileNameCasesCheck(fname))
            return true;
        //&fname = javaCreateComposedName(NULL,&tname,SLASH,"class",tmpMemory,SIZE_TMP_MEM);
        //&if (statb(fname,&stt)==0 && specialFileNameCasesCheck(fname)) return true;
    }
    JavaMapOnPaths(s_javaSourcePaths, {
        fname = javaCreateComposedName(currentPath,&tname,SLASH,"java",tmpMemory,SIZE_TMP_MEM);
        log_trace("testing existence of file '%s'", fname);
        if (statb(fname,&stt)==0 && specialFileNameCasesCheck(fname))
            return true;
    });
    for (cp=s_javaClassPaths; cp!=NULL; cp=cp->next) {
        fname = javaCreateComposedName(cp->d,&tname,SLASH,"class",tmpMemory,SIZE_TMP_MEM);
        // hmm. do not need to check statb for .class files
        if (statb(fname,&stt)==0 && specialFileNameCasesCheck(fname))
            return true;
    }
    // databazes
    fname=javaCreateComposedName(NULL,&tname,'/',"class",tmpMemory,SIZE_TMP_MEM);
    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchiveTable[i].fn[0]!=0; i++) {
        if (fsIsMember(&s_zipArchiveTable[i].dir,fname,0,ADD_NO,NULL))
            return true;
    }
    // auto-inferred source-path
    if (s_javaStat->namedPackagePath != NULL) {
        fname = javaCreateComposedName(s_javaStat->namedPackagePath,&tname,SLASH,"java",tmpMemory,SIZE_TMP_MEM);
        if (statb(fname,&stt)==0 && specialFileNameCasesCheck(fname))
            return true;
    }
    return false;
}

static int javaFindClassFile(char *name, char **resName, struct stat *stt) {
    S_stringList *cp;
    int i;

    if (s_javaStat->unnamedPackagePath != NULL) {		/* unnamed package */
        if (javaFindFile0( s_javaStat->unnamedPackagePath,"/",name, ".class",
                          resName, stt)) return(1);
    }
    // now other classpaths
    for (cp=s_javaClassPaths; cp!=NULL; cp=cp->next) {
        if (javaFindFile0( cp->d,"/",name, ".class", resName, stt)) return(1);
    }
    // finally look into databazes
    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && s_zipArchiveTable[i].fn[0]!=0; i++) {
//&fprintf(dumpOut,"looking in %s\n", s_zipArchiveTable[i].fn);fflush(dumpOut);
        if (zipFindFile(name,resName,&s_zipArchiveTable[i])) {
            *stt = s_zipArchiveTable[i].st;
            return(1);
        }
    }
    return(0);
}

static int javaFindSourceFile(char *name, char **resName, struct stat *stt) {
    S_stringList	*cp;

    if (s_javaStat->unnamedPackagePath != NULL) {		/* unnamed package */
/*fprintf(dumpOut,"searching for %s %s\n",s_javaStat->thisFileDir,name);fflush(dumpOut);*/
        if (javaFindFile0( s_javaStat->unnamedPackagePath,"/",name, ".java",
                          resName, stt)) return(1);
    }
    // sourcepaths
    JavaMapOnPaths(s_javaSourcePaths, {
        if (javaFindFile0(currentPath,"/",name,".java",resName,stt)) return(1);
    });
    // now other classpaths
    for (cp=s_javaClassPaths; cp!=NULL; cp=cp->next) {
        if (javaFindFile0( cp->d,"/",name, ".java", resName, stt)) return(1);
    }
    // auto-inferred source-path
    if (s_javaStat->namedPackagePath != NULL) {
        if (javaFindFile0(s_javaStat->namedPackagePath,"/",name, ".java", resName, stt)) return(1);
    }
    return(0);
}

// if file exists, then set its name to non NULL !!!!!!!!!!
static int javaFindFile(Symbol *clas,
                        char **resSourceFile,
                        char **resClassFile
    ) {
    char ttt[MAX_FILE_NAME_SIZE];
    int rc,rs,si;
    struct stat sourceStat;
    struct stat classStat;
    char *lname, *nn, *slname;
    lname = slname = clas->linkName;
    if (strchr(lname, '$')!=NULL) {
        // looking for an inner class, source must be included in outer file
        strcpy(ttt, clas->linkName);
        slname = ttt;
        nn = strchr(ttt, '$');
        assert(nn);
        *nn = 0;
    }
    *resClassFile = *resSourceFile = "";
//&fprintf(dumpOut,"!looking for %s.classf(in %s)== %d\n", lname, slname, clas->u.s->classFile); fflush(dumpOut);fprintf(dumpOut,"!looking for %s %s\n", lname, s_fileTab.tab[clas->u.s->classFile]->name); fflush(dumpOut);
    rs = javaFindSourceFile(slname, resSourceFile, &sourceStat);
    assert(clas->u.s && s_fileTab.tab[clas->u.s->classFile]);
    si = s_fileTab.tab[clas->u.s->classFile]->b.sourceFile;
//&fprintf(dumpOut,"source search %d %d\n", rs, si);
    if (rs==0 && si!= -1 && si!=s_noneFileIndex) {
        // try the source indicated by source field of filetab
        assert(s_fileTab.tab[si]);
//&fprintf(dumpOut,"checking %s\n", s_fileTab.tab[si]->name);
        rs = javaFindFile0( "","",s_fileTab.tab[si]->name, "", resSourceFile,
                            &sourceStat);
//&fprintf(dumpOut,"result %d %s\n", rs, *resSourceFile);
    }
    rc = javaFindClassFile(lname, resClassFile, &classStat);
    if (rc==0) *resClassFile = NULL;
//&fprintf(dumpOut,": checking to input name '%s' '%s'\n",*resSourceFile, s_fileTab.tab[s_olOriginalFileNumber]->name);
    if (rs==0) *resSourceFile = NULL;
//&fprintf(dumpOut,"O.K. here we are rc, rs == %d, %d\n", rc,rs);
    if (s_opt.javaSlAllowed == 0) {
        if (rc) return(RESULT_IS_CLASS_FILE);
        else return(RESULT_NO_FILE_FOUND);
    }
    if (rc==0 && rs==0) return(RESULT_NO_FILE_FOUND);
    if (rc==1 && rs==0) return(RESULT_IS_CLASS_FILE);
    if (rc==0 && rs==1) return(RESULT_IS_JAVA_FILE);
    assert(rs==1 && rc==1);
//&fprintf(dumpOut,"comparing src(%s)", ctime(&sourceStat.st_mtime));fprintf(dumpOut,"to class(%s)", ctime(&classStat.st_mtime));
    if (sourceStat.st_mtime > classStat.st_mtime) {
        return(RESULT_IS_JAVA_FILE);
    } else {
        return(RESULT_IS_CLASS_FILE);
    }
}

static int javaFqtNameIsFromThePackage(char *cpack, char *classFqName) {
    register char   *p1,*p2;
    for(p1=cpack, p2=classFqName; *p1 == *p2; p1++,p2++) ;
    if (*p1 != 0) return(0);
    if (*p2 == 0) return(0);
    //& if (*p2 != '/') return(0);
    for(p2++; *p2; p2++) if (*p2 == '/') return(0);
    return(1);
}

int javaFqtNamesAreFromTheSamePackage(char *nn1, char *nn2) {
    register char   *p1,*p2;
    if (nn1==NULL || nn2==NULL) return(0);
//&fprintf(dumpOut,"checking equal package %s %s\n", nn1, nn2);
    for(p1=nn1, p2=nn2; *p1 == *p2 && *p1 && *p2; p1++,p2++) ;
    for(; *p1; p1++) if (*p1 == '/') return(0);
    for(; *p2; p2++) if (*p2 == '/') return(0);
//*fprintf(dumpOut,"YES EQUALS\n");
    return(1);
}

int javaClassIsInCurrentPackage(Symbol *cl) {
    if (s_jsl!=NULL) {
        if (s_jsl->classStat->thisClass == NULL) {
            // probably import class.*; (nested subclasses)
            return(javaFqtNameIsFromThePackage(
                s_jsl->classStat->thisPackage,
                cl->linkName));
        } else {
            return(javaFqtNamesAreFromTheSamePackage(
                cl->linkName,
                s_jsl->classStat->thisClass->linkName));
        }
    } else {
        if (s_javaStat->thisClass == NULL) {
            // probably import class.*; (nested subclasses)
            return(javaFqtNameIsFromThePackage(
                s_javaStat->currentPackage,
                cl->linkName));
        } else {
            return(javaFqtNamesAreFromTheSamePackage(
                cl->linkName,
                s_javaStat->thisClass->linkName));
        }
    }
}

static Symbol *javaFQTypeSymbolDefinitionCreate(char *name,
                                                char *fqName, int ii) {
    Symbol *memb;
    SymbolList *pppl;
    char *lname1, *sname;

    CF_ALLOCC(sname, strlen(name)+1, char);
    strcpy(sname, name);

    CF_ALLOCC(lname1, strlen(fqName)+1, char);
    strcpy(lname1, fqName);

    CF_ALLOC(memb, Symbol);
    fillSymbol(memb, sname, lname1, s_noPos);
    fillSymbolBits(&memb->bits, ACCESS_DEFAULT, TypeStruct, StorageNone);

    CF_ALLOC(memb->u.s, S_symStructSpec);

    initSymStructSpec(memb->u.s, /*.records=*/NULL);
    S_typeModifier *stype = &memb->u.s->stype;
    /* Assumed to be Struct/Union/Enum? */
    initTypeModifierAsStructUnionOrEnum(stype, /*.kind=*/TypeStruct, /*.u.t=*/memb,
                                            /*.typedefSymbol=*/NULL, /*.next=*/NULL);
    S_typeModifier *sptrtype = &memb->u.s->sptrtype;
    initTypeModifierAsPointer(sptrtype, &memb->u.s->stype);

    CF_ALLOC(pppl, SymbolList);
    /* REPLACED: FILL_symbolList(pppl, memb, NULL); with compound literal */
    *pppl = (SymbolList){.d = memb, .next = NULL};

    javaFqtTabAdd(&s_javaFqtTab,pppl,&ii);

    // I think this can be there, as it is very used
    javaCreateClassFileItem(memb);
    // this would be too strong, javaLoadClassSymbolsFromFile(memb);
    /* so, this table is not freed by Trail */
    //&if (stringContainsSubstring(fqName, "ComboBoxTreeFilter")) {fprintf(dumpOut,"\nAAAAAAAAAAAAA : %s %s\n\n", name, fqName);} if (strcmp(fqName, "ComboBoxTreeFilter")==0) assert(0);
    return(memb);
}

Symbol *javaFQTypeSymbolDefinition(char *name, char *fqName) {
    Symbol symbol, *member;
    SymbolList ppl, *pppl;
    int position;

    /* This probably creates a SymbolList element so ..IsMember() can be used */
    /* TODO: create a function to check if a *Symbol* is member... */
    fillSymbol(&symbol, name, fqName, s_noPos);
    fillSymbolBits(&symbol.bits, ACCESS_DEFAULT, TypeStruct, StorageNone);

    /* REPLACED: FILL_symbolList(&ppl, &symbol, NULL); with compound literal */
    ppl = (SymbolList){.d = &symbol, .next = NULL};

    if (javaFqtTabIsMember(&s_javaFqtTab, &ppl, &position, &pppl)) {
        member = pppl->d;
    } else {
        assert(position >= 0);
        member = javaFQTypeSymbolDefinitionCreate(name, fqName, position);
    }
    return member;
}

Symbol *javaGetFieldClass(char *fieldLinkName, char **fieldAdr) {
    /* also .class suffix can be considered as field !!!!!!!!!!! */
    /* also ; is considered as the end of class fq name */
    /* so, this function is used to determine class file name also */
    /* if not, change '.' to appropriate char */
    /* But this function is very time expensive */
    char sbuf[MAX_FILE_NAME_SIZE];
    char fqbuf[MAX_FILE_NAME_SIZE];
    char *p,*lp,*lpp;
    Symbol        *memb;
    int fqlen,slen;
    lp = fieldLinkName;
    lpp = NULL;
    for(p=fieldLinkName; *p && *p!=';'; p++) {
        if (*p == '/' || *p == '$') lp = p+1;
        if (*p == '.') lpp = p;
    }
    if (lpp != NULL && lpp>lp) p = lpp;
    if (fieldAdr != NULL) *fieldAdr = p;
    fqlen = p-fieldLinkName;
    assert(fqlen+1 < MAX_FILE_NAME_SIZE);
    strncpy(fqbuf,fieldLinkName, fqlen);
    fqbuf[fqlen]=0;
    slen = p-lp;
    strncpy(sbuf,lp,slen);
    sbuf[slen]=0;
    memb = javaFQTypeSymbolDefinition(sbuf, fqbuf);
    return(memb);
}

// I think that s_symTab should contain symbollist, not symbols!
static Symbol *javaAddTypeToSymbolTable(Symbol *memb, int accessFlags, S_position *importPos, bool isExplicitlyImported) {
    Symbol *nmemb;
    //&Symbol *memb2;

    memb->bits.access = accessFlags;
    memb->bits.isExplicitlyImported = isExplicitlyImported;
    //&if (symbolTableIsMember(s_symbolTable, memb, &ii, &memb2)) {
    //&	while (memb2!=NULL && strcmp(memb->linkName, memb2->linkName)!=0) {
    //&		symbolTableNextMember(memb, &memb2);
    //&	}
    //&	if (memb2!=NULL) memb2->bits.access = accessFlags;
    //&}
    XX_ALLOC(nmemb, Symbol);
    *nmemb = *memb;
    nmemb->pos = *importPos;
    addSymbol(nmemb, s_symbolTable);
    //&memb = nmemb;

    return(memb);
}

Symbol *javaTypeSymbolDefinition(S_idList *tname,
                                   int accessFlags,
                                   int addTyp){
    Symbol                pp,*memb;
    char                    fqtName[MAX_FILE_NAME_SIZE];

    assert(tname);
    assert(tname->nameType == TypeStruct);

    fillSymbol(&pp, tname->id.name, tname->id.name, s_noPos);
    fillSymbolBits(&pp.bits, accessFlags, TypeStruct, StorageNone);

    javaCreateComposedName(NULL,tname,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    memb = javaFQTypeSymbolDefinition(tname->id.name, fqtName);
    if (addTyp == TYPE_ADD_YES) {
        memb = javaAddTypeToSymbolTable(memb, accessFlags, &s_noPos, false);
    }
    return(memb);
}

Symbol *javaTypeSymbolUsage(S_idList *tname,
                                   int accessFlags){
    Symbol                pp,*memb;
    int                     ii;
    char                    fqtName[MAX_FILE_NAME_SIZE];

    assert(tname);
    assert(tname->nameType == TypeStruct);

    fillSymbol(&pp, tname->id.name, tname->id.name, s_noPos);
    fillSymbolBits(&pp.bits, accessFlags, TypeStruct, StorageNone);

    if (tname->next==NULL && symbolTableIsMember(s_symbolTable, &pp, &ii, &memb)) {
        // get canonical copy
        memb = javaFQTypeSymbolDefinition(memb->name, memb->linkName);
        return(memb);
    }
    javaCreateComposedName(NULL,tname,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    memb = javaFQTypeSymbolDefinition(tname->id.name, fqtName);
    return(memb);
}

Symbol *javaTypeNameDefinition(S_idList *tname) {
    Symbol    *memb;
    Symbol		*dd;
    S_typeModifier		*td;

    memb = javaTypeSymbolUsage(tname, ACCESS_DEFAULT);
    td = newStructTypeModifier(memb);
    dd = newSymbolAsType(memb->name, memb->linkName, tname->id.p, td);

    return dd;
}

static void javaJslLoadSuperClasses(Symbol *cc, int currentParsedFile) {
    SymbolList *ss;
    static int nestingCount = 0;

    nestingCount ++;
    if (nestingCount > MAX_CLASSES) {
        fatalError(ERR_INTERNAL, "unexpected cycle in class hierarchy", XREF_EXIT_ERR);
    }
    for(ss=cc->u.s->super; ss!=NULL; ss=ss->next) {
        cfAddCastsToModule(cc, ss->d);
    }
    nestingCount --;
}

void javaReadSymbolFromSourceFileInit(int sourceFileNum,
                                      S_jslTypeTab *typeTab ) {
    S_jslStat           *njsl;
    char				*yyg;
    int					yygsize;
    XX_ALLOC(njsl, S_jslStat);
    // very space consuming !!!, it takes about 400kb of working memory
    // TODO!!!! to allocate and save only used parts of 'gyyvs - gyyvsp'
    // and 'gyyss - gyyssp' ??? And copying twice? definitely yes!
    //yygsize = sizeof(struct yyGlobalState);
    yygsize = ((char*)(s_yygstate->gyyvsp+1)) - ((char *)s_yygstate);
    XX_ALLOCC(yyg, yygsize, char);
    fillJslStat(njsl, 0, sourceFileNum, s_language, typeTab, NULL, NULL,
                 uniyylval, (S_yyGlobalState*)yyg, yygsize, s_jsl);
    memcpy(njsl->savedYYstate, s_yygstate, yygsize);
    memcpy(njsl->yyIdentBuf, s_yyIdentBuf,
           sizeof(S_id[YYBUFFERED_ID_INDEX]));
    s_jsl = njsl;
    s_language = LANG_JAVA;
}

void javaReadSymbolFromSourceFileEnd(void) {
    s_language = s_jsl->language;
    uniyylval = s_jsl->savedyylval;
    memcpy(s_yygstate, s_jsl->savedYYstate, s_jsl->yyStateSize);
    memcpy(s_yyIdentBuf, s_jsl->yyIdentBuf,
           sizeof(S_id[YYBUFFERED_ID_INDEX]));
    s_jsl = s_jsl->next;
}

void javaReadSymbolsFromSourceFileNoFreeing(char *fname, char *asfname) {
    FILE                    *ff;
    S_editorBuffer			*bb;
    SymbolList            *ll;
    int						cfilenum;
    static int              nestDeep = 0;
    nestDeep ++;

    log_debug("[jsl] FIRST PASS through %s level %d", fname, nestDeep);
    ff = NULL;
    //&bb = editorGetOpenedAndLoadedBuffer(fname);
    bb = editorFindFile(fname);
    if (bb==NULL) {
        ff = fopen(fname, "r");
        if (ff==NULL) {
            error(ERR_CANT_OPEN, fname);
            goto fini;
        }
    }
    // ?? is this really necessary?
    // memset(s_yygstate, sizeof(struct yyGlobalState), 0);
    uniyylval = & s_yygstate->gyylval;
    pushNewInclude(ff, bb, asfname, "\n");
    cfilenum = cFile.lexBuffer.buffer.fileNumber;
    s_jsl->pass = 1;
    javayyparse();
    popInclude();      // this will close the file
    log_debug("[jsl] CLOSE file %s level %d", fname, nestDeep);
    log_debug("[jsl] SECOND PASS through %s level %d", fname, nestDeep);
    ff = NULL;
    //&bb = editorGetOpenedAndLoadedBuffer(fname);
    bb = editorFindFile(fname);
    if (bb==NULL) {
        ff = fopen(fname, "r");
        if (ff==NULL) {
            error(ERR_CANT_OPEN, fname);
            goto fini;
        }
    }
    pushNewInclude(ff, bb, asfname, "\n");
    cfilenum = cFile.lexBuffer.buffer.fileNumber;
    s_jsl->pass = 2;
    javayyparse();
    popInclude();      // this will close the file
    log_debug("[jsl] CLOSE file %s level %d", fname, nestDeep);
    for(ll=s_jsl->waitList; ll!=NULL; ll=ll->next) {
        javaJslLoadSuperClasses(ll->d, cfilenum);
    }
 fini:
    nestDeep --;
}

void javaReadSymbolsFromSourceFile(char *fname) {
    S_jslTypeTab    *typeTab;
    int				fileIndex;
    int				memBalance;

    fileIndex = addFileTabItem(fname);
    memBalance = s_topBlock->firstFreeIndex;
    stackMemoryBlockStart();
    XX_ALLOC(typeTab, S_jslTypeTab);
    javaReadSymbolFromSourceFileInit(fileIndex, typeTab);
    jslTypeTabInit(typeTab, MAX_JSL_SYMBOLS);
    javaReadSymbolsFromSourceFileNoFreeing(fname, fname);
    // there may be several unbalanced blocks
    while (memBalance < s_topBlock->firstFreeIndex) stackMemoryBlockFree();
    javaReadSymbolFromSourceFileEnd();
}

static void addJavaFileDependency(int file, char *onfile) {
    int         fileIndex;
    S_position	pos;

    // do dependencies only when doing cross reference file
    if (s_opt.taskRegime != RegimeXref) return;
    // also do it only for source files
    if (! s_fileTab.tab[file]->b.commandLineEntered) return;
    fileIndex = addFileTabItem(onfile);
    fillPosition(&pos, file, 0, 0);
    addIncludeReference(fileIndex, &pos);
}


static void javaHackCopySourceLoadedCopyPars(Symbol *memb) {
    Symbol *cl;
    cl = javaFQTypeSymbolDefinition(memb->name, memb->linkName);
    if (cl->bits.javaSourceIsLoaded) {
        memb->bits.access = cl->bits.access;
        memb->bits.storage = cl->bits.storage;
        memb->bits.symType = cl->bits.symType;
        memb->bits.javaSourceIsLoaded = cl->bits.javaSourceIsLoaded;
    }
}

void javaLoadClassSymbolsFromFile(Symbol *memb) {
    char *sname, *cname;
    Symbol *cl;
    int ffound, cfi, cInd;
    if (memb == NULL) return;
//&fprintf(dumpOut,"!requesting class (%d)%s\n", memb, memb->linkName);
    sname = cname = "";
    if (memb->bits.javaFileIsLoaded == 0) {
        memb->bits.javaFileIsLoaded = 1;
        // following is a hack due to multiple items in symbol tab !!!
        cl = javaFQTypeSymbolDefinition(memb->name, memb->linkName);
        if (cl!=NULL && cl!=memb) cl->bits.javaFileIsLoaded = 1;
        cInd = javaCreateClassFileItem(memb);
        addCfClassTreeHierarchyRef(cInd, UsageClassFileDefinition);
        ffound = javaFindFile(memb, &sname, &cname);
//&fprintf(dumpOut,"![load] file containing %s, %s, %s %s\n", memb->linkName, miscellaneousName[ffound], sname, cname);
//&sprintf(tmpBuff,"![load] file containing %s, %s, %s %s\n", memb->linkName, miscellaneousName[ffound], sname, cname);ppcGenRecord(PPC_IGNORE, tmpBuff, "\n");
        if (ffound == RESULT_IS_JAVA_FILE) {
            assert(memb->u.s);
            cfi = memb->u.s->classFile;
            assert(s_fileTab.tab[cfi]);
            // set it to none, if class is inside jslparsing  will re-set it
            s_fileTab.tab[cfi]->b.sourceFile=s_noneFileIndex;
            javaReadSymbolsFromSourceFile(sname);
            if (s_fileTab.tab[cfi]->b.sourceFile == s_noneFileIndex) {
                // class definition not found in the source file,
                // (moved inner class) retry searching for class file
                ffound = javaFindFile(memb, &sname, &cname);
            }
            if (memb->bits.javaSourceIsLoaded == 0){
                // HACK, probably loaded into another possition of symboltab, make copy
                javaHackCopySourceLoadedCopyPars(memb);
            }
        }
        if (ffound == RESULT_IS_CLASS_FILE) {
            javaReadClassFile(cname,memb, LOAD_SUPER);
        } else if (ffound == RESULT_NO_FILE_FOUND) {
            if (displayingErrorMessages()) {
                sprintf(tmpBuff, "class %s not found", memb->name);
                error(ERR_ST, tmpBuff);
            }
        }
        // I need to get also accessflags for example
        if (cl!=NULL && cl!=memb) cl->bits = memb->bits;
        if (sname != NULL) {
            addJavaFileDependency(s_olOriginalFileNumber, sname);
        }
    }
//&fprintf(dumpOut,"finishing with  class %s\n", memb->linkName);
}


static int findTopLevelNameInternal(
                                char *name,
                                S_recFindStr *resRfs,
                                Symbol **resMemb,
                                int classif,
                                S_javaStat *startingScope,
                                int accCheck,
                                int visibCheck,
                                S_javaStat **rscope
                                ) {
    int				ii,res;
    Symbol        sd;
    S_javaStat		*cscope;

    assert((!LANGUAGE(LANG_JAVA)) ||
        (classif==CLASS_TO_EXPR || classif==CLASS_TO_METHOD));
    assert(accCheck==ACC_CHECK_YES || accCheck==ACC_CHECK_NO);
    assert(visibCheck==VISIB_CHECK_YES || visibCheck==VISIB_CHECK_NO);

    fillSymbol(&sd, name, name, s_noPos);
    fillSymbolBits(&sd.bits, 0, TypeDefault, StorageNone);

    res = RETURN_NOT_FOUND;
    for(cscope=startingScope;
        cscope!=NULL && cscope->thisClass!=NULL && res!=RETURN_OK;
        cscope=cscope->next
        ) {
        assert(cscope->thisClass);
        if (classif!=CLASS_TO_METHOD && symbolTableIsMember(cscope->locals, &sd, &ii, resMemb)) {
            /* it is an argument or local variable */
            /* this is tricky */
            /* I guess, you cannot have an overloaded function here, so ... */
            log_trace("%s is identified as local var or parameter", name);
            fillRecFindStr(resRfs, NULL, NULL, *resMemb,s_recFindCl++);
            *rscope = NULL;
        } else {
            /* if present, then as a structure record */
            log_trace("putting %s as base class", cscope->thisClass->name);
            fillRecFindStr(resRfs, cscope->thisClass, NULL, NULL,s_recFindCl++);
            recFindPush(cscope->thisClass, resRfs);
            *rscope = cscope;
        }
        log_trace("%s %s", miscellaneousEnumName[classif], miscellaneousEnumName[accCheck]);
        res = findStrRecordSym(resRfs, name, resMemb, classif, accCheck, visibCheck);
    }
    return(res);
}

int findTopLevelName(
                                char                *name,
                                S_recFindStr        *resRfs,
                                Symbol			**resMemb,
                                int                 classif
                                ) {
    int				res;
    S_javaStat		*scopeToSearch, *resultScope;
    res = findTopLevelNameInternal(name, resRfs, resMemb, classif,
                                   s_javaStat, ACC_CHECK_YES, VISIB_CHECK_YES,
                                   &scopeToSearch);
    // O.K. determine class to search
    if (res != RETURN_OK) {
        // no class to search find, anyway this is a compiler error,
        scopeToSearch = s_javaStat;
    }
    if (scopeToSearch!=NULL) {
//&fprintf(dumpOut,"relooking for %s in %s\n", name, scopeToSearch->thisClass->name);
        res = findTopLevelNameInternal(name, resRfs, resMemb, classif,
                                       scopeToSearch, ACC_CHECK_NO, VISIB_CHECK_NO,
                                       &resultScope);
    }
    return(res);
}

static int javaIsNestedClass(Symbol *tclas, char *name, Symbol **innmemb) {
    register int    i,n;
    S_nestedSpec	*inners;
    Symbol        *clas;
    // take just one super class, no interfaces, for speed reasons
    for(clas=tclas;
        clas!=NULL && clas->u.s->super!=NULL;
        clas=clas->u.s->super->d) {
        assert(clas->bits.symType == TypeStruct && clas->u.s);
        n = clas->u.s->nestedCount;
        inners = clas->u.s->nest;
        for(i=0; i<n; i++) {
//&fprintf(dumpOut,"checking %s<->%s\n",inners[i].cl->name, name);fflush(dumpOut);
            if (inners[i].membFlag && strcmp(inners[i].cl->name, name)==0) {
                // the following if makes problem, because when resolving
                // a name Outer.Inner I do not care whether it is static
                // or not. Why I have put this there????
//&				if ((inners[i].cl->bits.access&ACCESS_STATIC)==0) {
                    *innmemb = inners[i].cl;
                    return(1);
//&				}
            }
        }
    }
    return(0);
}

static int javaClassIsInnerNonStaticMemberClass(Symbol *tclas, Symbol *name) {
    register int    i,n;
    S_nestedSpec	*inners;
    Symbol        *clas;
    // take just one super class, no interfaces, for speed reasons
    for(clas=tclas;
        clas!=NULL && clas->u.s->super!=NULL;
        clas=clas->u.s->super->d) {
        assert(clas->bits.symType == TypeStruct && clas->u.s);
        n = clas->u.s->nestedCount;
        inners = clas->u.s->nest;
        for(i=0; i<n; i++) {
//&fprintf(dumpOut,"checking %s<->%s\n",inners[i].cl->name, name);fflush(dumpOut);
            if (inners[i].membFlag && strcmp(inners[i].cl->linkName, name->linkName)==0
                && (inners[i].cl->bits.access & ACCESS_STATIC) == 0) {
                return(1);
            }
        }
    }
    return(0);
}

#if ZERO
static int javaSimpleNameIsInnerMemberClass(char *name, S_symbol **innmemb) {
    S_javaStat		*cscope;
//&fprintf(dumpOut,"looking for inner class %s\n",name);
    for(	cscope=s_javaStat;
            cscope!=NULL && cscope->thisClass!=NULL;
            cscope=cscope->next
        ) {
        if (javaIsNestedClass(cscope->thisClass, name, innmemb)) {
//&fprintf(dumpOut,"inner class %s found %s %s\n",name,(*innmemb)->name, (*innmemb)->linkName);fflush(dumpOut);
            return(1);
        }
    }
    return(0);
}
#endif

int javaIsInnerAndCanGetUnnamedEnclosingInstance(Symbol *name, Symbol **outEi) {
    S_javaStat		*cscope;
//&fprintf(dumpOut,"looking for inner class %s\n",name);
    for(	cscope=s_javaStat;
            cscope!=NULL && cscope->thisClass!=NULL;
            cscope=cscope->next
        ) {
        if (javaClassIsInnerNonStaticMemberClass(cscope->thisClass, name)) {
//&fprintf(dumpOut,"inner class %s found %s %s\n",name,(*innmemb)->name, (*innmemb)->linkName);fflush(dumpOut);
            *outEi = cscope->thisClass;
            return(1);
        }
    }
    return(0);
}

#define JAVA_CLASS_SAN_HAVE_IT(name,str,outImportPos,mm,memb,haveit) {\
    haveit = 1;\
    *str = mm;\
    name->nameType = TypeStruct;\
    name->fname = mm->linkName;\
    if (cxrefFlag == ADD_CX_REFS) {\
        ipos = & memb->pos; /* here MUST be memb, not mm, as it contains the import line !!*/\
        if (ipos->file != s_noneFileIndex && ipos->file != -1) {\
            javaAddImportConstructionReference(ipos, ipos, UsageUsed);\
        }\
    }\
}

int javaClassifySingleAmbigNameToTypeOrPack(S_idList *name,
                                            Symbol **str,
                                            int cxrefFlag
    ){
    Symbol sd, *mm, *memb, *nextmemb;
    int unused;
    bool haveit;
    S_position *ipos;

    fillSymbol(&sd, name->id.name, name->id.name, s_noPos);
    fillSymbolBits(&sd.bits, ACCESS_DEFAULT, TypeStruct, StorageNone);

    haveit = false;
    if (symbolTableIsMember(s_symbolTable, &sd, &unused, &memb)) {
        /* a type */
        assert(memb);
        // O.K. I have to load the class in order to check its access flags
        for(; memb!=NULL; memb=nextmemb) {
            nextmemb = memb;
            symbolTableNextMember(&sd, &nextmemb);
            // take canonical copy (as there can be more than one class
            // item in symtab
            mm = javaFQTypeSymbolDefinition(memb->name, memb->linkName);
            if ((s_opt.ooChecksBits & OOC_ALL_CHECKS)) {
                // do carefully all checks
                if (! haveit) {
                    javaLoadClassSymbolsFromFile(mm);
                    if (javaOuterClassAccessible(mm)) {
                        JAVA_CLASS_SAN_HAVE_IT(name, str, outImportPos, mm, memb, haveit);
                    }
                } else {
                    if ((*str) != mm) {
                        javaLoadClassSymbolsFromFile(mm);
                        if (javaOuterClassAccessible(mm)) {
                            // multiple imports
                            if ((*str)->bits.isExplicitlyImported == mm->bits.isExplicitlyImported) {
                                assert(strcmp((*str)->linkName, mm->linkName)!=0);
                                haveit = false;
                                // this is tricky, mark the import as used
                                // it is "used" to disqualify some references, so
                                // during name reduction refactoring, it will not adding
                                // such import
                                ipos = &memb->pos;
                                if (ipos->file != s_noneFileIndex && ipos->file != -1) {
                                    javaAddImportConstructionReference(ipos, ipos, UsageUsed);
                                }
                                goto breakcycle;
                            }
                        }
                    }
                }
            } else {
                // just find the first accessible
                if (nextmemb == NULL) {
                    JAVA_CLASS_SAN_HAVE_IT(name, str, outImportPos, mm, memb, haveit);
                    goto breakcycle;
                } else {
                    // O.K. there may be an ambiguity resolved by accessibility
                    javaLoadClassSymbolsFromFile(mm);
                    if (javaOuterClassAccessible(mm)) {
                        JAVA_CLASS_SAN_HAVE_IT(name, str, outImportPos, mm, memb, haveit);
                        goto breakcycle;
                    }
                }
            }
        }
    }
 breakcycle:
    if (haveit)
        return TypeStruct;

    name->nameType = TypePackage;
    return TypePackage;
}

#define AddAmbCxRef(classif,sym,pos,usage, minacc,oref, rfs) {\
    S_usageBits ub;\
    if (classif!=CLASS_TO_METHOD) {\
        if (rfs != NULL && rfs->currClass!=NULL) {\
            assert(rfs && rfs->currClass && \
                rfs->currClass->bits.symType==TypeStruct && rfs->currClass->u.s);\
            assert(rfs && rfs->baseClass && \
                rfs->baseClass->bits.symType==TypeStruct && rfs->baseClass->u.s);\
            if (s_opt.server_operation!=OLO_ENCAPSULATE \
                || ! javaRecordAccessible(rfs, rfs->baseClass, rfs->currClass, sym, ACCESS_PRIVATE)) {\
                fill_usageBits(&ub, usage, minacc);\
                oref=addCxReferenceNew(sym,pos, &ub,\
                    rfs->currClass->u.s->classFile,\
                    rfs->baseClass->u.s->classFile);\
            }\
        } else {\
            oref=addCxReference(sym,pos,usage,s_noneFileIndex,s_noneFileIndex);\
        }\
    }\
}

char *javaImportSymbolName_st(int file, int line, int coll) {
    static char res[MAX_CX_SYMBOL_SIZE];
    sprintf(res, "%s:%x:%x:%x%cimport on line %3d", LINK_NAME_IMPORT_STATEMENT,
            file, line, coll,
            LINK_NAME_SEPARATOR,
            /*& simpleFileName(getRealFileNameStatic(s_fileTab.tab[file]->name)), &*/
            line);
    return(res);
}

void javaAddImportConstructionReference(S_position *importPos, S_position *pos, int usage) {
    char *isymName;
    isymName = javaImportSymbolName_st(importPos->file, importPos->line, importPos->col);
//&fprintf(dumpOut,"using import on %s:%d (%d)  at %s:%d\n", simpleFileName(s_fileTab.tab[importPos->file]->name), importPos->line, importPos->col, simpleFileName(s_fileTab.tab[pos->file]->name), pos->line);
    addSpecialFieldReference(isymName, StorageDefault, s_noneFileIndex, pos, usage);
}

static int javaClassifySingleAmbigName( S_idList *name,
                                        S_recFindStr *rfs,
                                        Symbol **str,
                                        S_typeModifier **expr,
                                        S_reference **oref,
                                        int classif, int uusage,
                                        int cxrefFlag
    ) {
    int             res, nfqtusage, minacc;
    S_recFindStr *nullRfs = NULL;

    if (classif==CLASS_TO_EXPR || classif==CLASS_TO_METHOD) {
        /* argument, local variable or class record */
        if (findTopLevelName(name->id.name,rfs,str,classif)==RETURN_OK) {
            *expr = (*str)->u.type;
            name->nameType = TypeExpression;
            if (cxrefFlag==ADD_CX_REFS) {
                minacc = javaGetMinimalAccessibility(rfs, *str);
                AddAmbCxRef(classif,*str, &name->id.p, uusage, minacc, *oref, rfs);
                if (rfs!=NULL && rfs->currClass != NULL) {
                    // the question is: is a reference to static field
                    // also reference to 'this'? If yes, it will
                    // prevent many methods from beeing turned static.
                    if ((*str)->bits.access & ACCESS_STATIC) {
                        nfqtusage = javaNotFqtUsageCorrection(rfs->currClass,UsageNotFQField);
                        addSpecialFieldReference(LINK_NAME_NOT_FQT_ITEM,StorageField,
                                rfs->currClass->u.s->classFile,
                                &name->id.p, nfqtusage);
                    } else {
                        addThisCxReferences(rfs->baseClass->u.s->classFile, &name->id.p);
                    }
                }
            }
            return(name->nameType);
        }
    }
    res = javaClassifySingleAmbigNameToTypeOrPack( name, str, cxrefFlag);
    if (res == TypeStruct) {
        if (cxrefFlag==ADD_CX_REFS) {
            AddAmbCxRef(classif, *str, &name->id.p, uusage, MIN_REQUIRED_ACCESS, *oref, nullRfs);
            // the problem is here when invoked as nested "new Name()"?
            nfqtusage = javaNotFqtUsageCorrection((*str), UsageNotFQType);
            addSpecialFieldReference(LINK_NAME_NOT_FQT_ITEM, StorageField,
                                     (*str)->u.s->classFile,
                                     &name->id.p, nfqtusage);
        }
    } else if (res == TypePackage) {
        if (cxrefFlag==ADD_CX_REFS) {
            javaAddNameCxReference(name, uusage);
        }
    } else {
        // Well, I do not know, putting the assert there just to learn
        assert(0);
    }
    return(name->nameType);
}

static int javaNotFqtUsageCorrection(Symbol *sym, int usage) {
    int             rr,pplen;
    S_recFindStr    localRfs;
    Symbol        *str;
    S_typeModifier *expr;
    S_reference     *loref;
    S_idList   sname;
    char            *pp, packname[TMP_STRING_SIZE];

    if (s_opt.taskRegime == RegimeHtmlGenerate) return(usage);

    pp = strchr(sym->linkName, '/');
    if (pp==NULL) pp = sym->linkName;
    pplen = pp - sym->linkName;
    assert(pplen < TMP_STRING_SIZE-1);
    strncpy(packname, sym->linkName, pplen);
    packname[pplen] = 0;

    fillfIdList(&sname, packname, NULL, s_noPos, packname, TypeExpression, NULL);
        rr = javaClassifySingleAmbigName(&sname,&localRfs,&str,&expr,&loref,
                                         CLASS_TO_EXPR, UsageNone, NO_CX_REFS);
    if (rr != TypePackage) {
        return(UsageNonExpandableNotFQTName);
    }
    return(usage);
}

static void javaResetUselessReference(S_reference *ref) {
    if (ref != NULL /*& && ref->usage.base == UsageLastUseless &*/) {
        ref->usage.base = UsageOtherUseless;
    }
}

/*
 check whether name as single name will result to str, if yes add
 UsageUselessFqt reference
*/
static void javaCheckForUselessFqt(S_idList *name, int classif, Symbol *rstr,
                                   S_reference **oref, S_reference *lref){
    int             rr, uselessFqt;
    S_recFindStr    localRfs;
    Symbol        *str;
    S_typeModifier *expr;
    S_reference     *loref;
    S_idList   sname;

    uselessFqt = 0;

    // no useless name reference for HTML as they overwrites normal
    // usage reference and makes problems
    if (s_opt.taskRegime == RegimeHtmlGenerate) return;

    //& for(nn=name; nn!=NULL && nn->nameType!=TypePackage; nn=nn->next) ;
    //& if (nn==NULL) return;
    // if it is not fqt (i.e. does not contain package), do nothing
    // TODO, this must be optional !!!!

    sname = *name;
    sname.next = NULL;
    //& rr = javaClassifySingleAmbigNameToTypeOrPack( &sname, &str, NO_CX_REFS); // wrong
    rr = javaClassifySingleAmbigName(&sname,&localRfs,&str,&expr,&loref,
                                      classif, UsageNone, NO_CX_REFS);
    if (rr==TypeStruct) {
        assert(str && rstr);
//&fprintf(dumpOut,"!checking %s == %s  (%d==%d)\n",str->linkName,rstr->linkName,str->u.s->classFile,rstr->u.s->classFile);
        // equality of pointers may be too strong ???
        // what about classfile index comparing
        //if (strcmp(str->linkName, rstr->linkName) == 0) {
        assert(str->u.s!=NULL && rstr->u.s!=NULL);
        if (str->u.s->classFile == rstr->u.s->classFile) {
            assert(name && rstr->u.s);
            *oref = addUselessFQTReference(rstr->u.s->classFile,&name->id.p);
//&fprintf(dumpOut,"!adding TYPE useless reference on %d,%d\n", name->id.p.line, name->id.p.col);
            javaResetUselessReference(lref);
            uselessFqt = 1;
        }
    }
    if (! uselessFqt) {
        assert(name->next != NULL);			// it is long name
        assert(name && rstr->u.s);
        addUnimportedTypeLongReference(rstr->u.s->classFile,&name->id.p);
    }
}

static S_reference *javaCheckForUselessTypeName(S_idList   *name,
                                                int				classif,
                                                S_recFindStr	*rfs,
                                                S_reference     **oref,
                                                S_reference     *lref
    ) {
    int             rr;
    S_recFindStr    localRfs;
    Symbol        *str;
    S_typeModifier *expr;
    S_reference     *loref;
    S_idList   sname;
    S_reference     *res;

    res = NULL;

    // no useless name reference for HTML as they overwrites normal
    // usage reference and makes problems
    if (s_opt.taskRegime == RegimeHtmlGenerate) return(res);

    //& for(nn=name; nn!=NULL && nn->nameType!=TypePackage; nn=nn->next) ;
    //& if (nn==NULL) return;
    // if it is not fqt (i.e. does not contain package), do nothing
    // TODO, this must be optional !!!!

    // not a static reference?
    //& if (name->next==NULL || name->next->nameType!=TypeStruct) return;

    sname = *name;
    sname.next = NULL;
    rr = javaClassifySingleAmbigName(&sname,&localRfs,&str,&expr,&loref,
                                      classif, UsageNone, NO_CX_REFS);
    assert(rfs && rfs->currClass && rfs->currClass->u.s && name);
//&fprintf(dumpOut,"!checking rr == %s, %x\n", typeName[rr], localRfs.currClass);
    if (rr==TypeExpression && localRfs.currClass!=NULL) {
//&fprintf(dumpOut,"!checking %d(%s) == %d(%s)\n",rfs->currClass->u.s->classFile,rfs->currClass->linkName,localRfs.currClass->u.s->classFile,localRfs.currClass->linkName);
        // equality of pointers may be too strong ???
        if(rfs->currClass->u.s->classFile==localRfs.currClass->u.s->classFile){
            *oref = addUselessFQTReference(rfs->currClass->u.s->classFile, &name->id.p);
//&fprintf(dumpOut,"!adding useless reference on %d,%d\n", name->id.p.line, name->id.p.col);
            javaResetUselessReference(lref);
        }
    }
    return(res);
}

static void javaClassifyNameToNestedType(S_idList *name, Symbol *outerc, int uusage, Symbol **str, S_reference **oref) {
    name->nameType = TypeStruct;
    *str = javaTypeSymbolUsage(name,ACCESS_DEFAULT);
    javaLoadClassSymbolsFromFile(*str);
    // you have to search the class, it may come from superclass

    if ((s_opt.ooChecksBits & OOC_ALL_CHECKS)==0
        || javaRecordAccessible(NULL,outerc, outerc, *str, (*str)->bits.access)) {
        *oref = javaAddClassCxReference(*str, &name->id.p, uusage);
    }
}

static void classifiedToNestedClass(S_idList *name, Symbol **str, S_reference **oref, S_reference **rdtoref, int classif, int uusage, Symbol *pstr, S_reference *prdtoref, int allowUselesFqtRefs) {
    name->nameType = TypeStruct;
    //&*str=javaTypeSymbolUsage(name,ACCESS_DEFAULT);
    javaLoadClassSymbolsFromFile(*str);
    if ((s_opt.ooChecksBits & OOC_ALL_CHECKS)==0
        || javaRecordAccessible(NULL,pstr, pstr, *str, (*str)->bits.access)) {
        *oref = javaAddClassCxReference(*str, &name->id.p, uusage);
        if (allowUselesFqtRefs == USELESS_FQT_REFS_ALLOWED) {
            javaCheckForUselessFqt(name, classif, *str, rdtoref, prdtoref);
        }
    }
}


int javaClassifyAmbiguousName(
        S_idList *name,
        S_recFindStr *rfs,	// can be NULL
        Symbol **str,
        S_typeModifier **expr,
        S_reference **oref,
        S_reference **rdtoref,  // output last useless reference, can be NULL
        int allowUselesFqtRefs,
        int classif,
        int usage) {
    int			pres,rf,classif2,rr;
    int			uusage, minacc;
    Symbol    *pstr;
    S_recFindStr localRfs;
    S_typeModifier     *pexpr;
    S_reference			*poref, *localrdref, *prdtoref;
    assert(classif==CLASS_TO_TYPE || classif==CLASS_TO_EXPR ||
           classif==CLASS_TO_METHOD);
    uusage = usage;
    if (uusage==UsageUsed && WORK_NEST_LEVEL0()) uusage = USAGE_TOP_LEVEL_USED;
    if (rfs == NULL) rfs = &localRfs;
    if (rdtoref == NULL) rdtoref = &localrdref;
    *oref = NULL; *rdtoref = NULL;
    /* returns TypeStruct, TypeExpression */
    assert(name);
    if (name->next == NULL) {
        /* a single name */
        javaClassifySingleAmbigName(name,rfs,str,expr,oref,classif,uusage, ADD_CX_REFS);
    } else {
        /* composed name */
        if (classif == CLASS_TO_METHOD) {
            classif2 = CLASS_TO_EXPR;
        } else {
            classif2 = classif;
        }
        pres = javaClassifyAmbiguousName(name->next, NULL, &pstr,
                                         &pexpr, &poref, &prdtoref, allowUselesFqtRefs,
                                         classif2,UsageUsed);
        *rdtoref = prdtoref;
        switch (pres) {
        case TypePackage:
            if (javaTypeFileExist(name)) {
                name->nameType = TypeStruct;
                *str = javaTypeSymbolUsage(name, ACCESS_DEFAULT);
                if (allowUselesFqtRefs == USELESS_FQT_REFS_ALLOWED) {
                    javaCheckForUselessFqt(name, classif, *str, rdtoref, prdtoref);
                }
                javaLoadClassSymbolsFromFile(*str);
                if ((s_opt.ooChecksBits & OOC_ALL_CHECKS)==0
                    || javaOuterClassAccessible(*str)) {
                    *oref = javaAddClassCxReference(*str, &name->id.p, uusage);
                }
            } else {
                name->nameType = TypePackage;
                javaAddNameCxReference(name, uusage);
            }
            break;
        case TypeStruct:
            if (classif==CLASS_TO_TYPE) {
                javaLoadClassSymbolsFromFile(pstr);
                name->nameType = TypeStruct;
                if (javaIsNestedClass(pstr,name->id.name,str)) {
                    classifiedToNestedClass(name, str, oref, rdtoref, classif, uusage, pstr, prdtoref, allowUselesFqtRefs);
                } else {
                    javaClassifyNameToNestedType(name, pstr, uusage, str, oref);
                    if (allowUselesFqtRefs == USELESS_FQT_REFS_ALLOWED) {
                        javaCheckForUselessFqt(name,classif,*str,rdtoref,prdtoref);
                    }
                }
            } else {
                javaLoadClassSymbolsFromFile(pstr);
                rf = findStrRecordSym(iniFind(pstr,rfs), name->id.name, str,
                                      classif, ACC_CHECK_NO, VISIB_CHECK_NO);
                *expr = (*str)->u.type;
                if (rf == RETURN_OK) {
                    name->nameType = TypeExpression;
                    if ((s_opt.ooChecksBits & OOC_ALL_CHECKS)==0
                        || javaRecordVisibleAndAccessible(rfs, rfs->baseClass, rfs->currClass, *str)) {
                        minacc = javaGetMinimalAccessibility(rfs, *str);
                        AddAmbCxRef(classif,*str,&name->id.p, uusage, minacc, *oref, rfs);
                        if (allowUselesFqtRefs == USELESS_FQT_REFS_ALLOWED) {
                            javaCheckForUselessTypeName(name, classif, rfs,
                                                        rdtoref, prdtoref);
                        }
                    }
                } else {
                    if (javaIsNestedClass(pstr,name->id.name,str)) {
                        classifiedToNestedClass(name, str, oref, rdtoref, classif, uusage, pstr, prdtoref, allowUselesFqtRefs);
                    } else {		/* error, no such record found */
                        name->nameType = TypeExpression;
                        noSuchRecordError(name->id.name);
                    }
                }
            }
            break;
        case TypeExpression:
            if (pexpr->kind == TypeArray) pexpr = &s_javaArrayObjectSymbol.u.s->stype;
//&			if (pexpr->kind == TypeError) {
//&				addTrivialCxReference(LINK_NAME_INDUCED_ERROR,TypeInducedError,StorageDefault,
//&                                   &name->id.p, UsageUsed);
//&			}
            if (pexpr->kind != TypeStruct) {
                *str = &s_errorSymbol;
            } else {
                javaLoadClassSymbolsFromFile(pexpr->u.t);
                rr = findStrRecordSym(iniFind(pexpr->u.t,rfs), name->id.name,
                                      str, classif, ACC_CHECK_NO, VISIB_CHECK_NO);
                if (rr == RESULT_OK) {
                    if ((s_opt.ooChecksBits & OOC_ALL_CHECKS)==0
                        || javaRecordVisibleAndAccessible(rfs, rfs->baseClass, rfs->currClass, *str)) {
                        minacc = javaGetMinimalAccessibility(rfs, *str);
                        AddAmbCxRef(classif,*str,&name->id.p,uusage, minacc, *oref, rfs);
                    }
                } else {
                    noSuchRecordError(name->id.name);
                }
            }
            *expr = (*str)->u.type;
            name->nameType = TypeExpression;
            break;
        default: assert(0);
        }
    }
    return(name->nameType);
}

#undef AddAmbCxRef

S_typeModifier *javaClassifyToExpressionName(S_idList *name,
                                              S_reference **oref) {
    Symbol    *str;
    S_typeModifier		*expr,*res;
    int atype;
    atype = javaClassifyAmbiguousName(name,NULL,&str,&expr,oref,NULL, USELESS_FQT_REFS_ALLOWED,CLASS_TO_EXPR,UsageUsed);
    if (atype == TypeExpression) res = expr;
    else if (atype == TypeStruct) {
        assert(str && str->u.s);
        res = &str->u.s->stype; /* because of casts & s_errorModifier;*/
        assert(res && res->kind == TypeStruct);
    } else res = & s_errorModifier;
    return(res);
}

// returns last useless reference (if any)
S_reference *javaClassifyToTypeOrPackageName(S_idList *tname, int usage, Symbol **str, int allowUselesFqtRefs) {
    S_typeModifier		*expr;
    S_reference			*rr, *lastUselessRef;
    lastUselessRef = NULL;
    javaClassifyAmbiguousName(tname, NULL, str, &expr, &rr, &lastUselessRef, allowUselesFqtRefs,
                              CLASS_TO_TYPE, usage);
    return(lastUselessRef);
}

S_reference *javaClassifyToTypeName(S_idList *tname, int usage, Symbol **str, int allowUselesFqtRefs) {
    S_reference *res;

    res = javaClassifyToTypeOrPackageName(tname, usage, str, allowUselesFqtRefs);
    if (tname->nameType != TypeStruct) {
        // there is probably a problem with class or source path
        tname->nameType = TypeStruct;
        // but following will create double reference, prefer to not add
        //ss = javaTypeSymbolUsage(tname, ACC_DEFAULT);
        //addCxReference(ss, &tname->idi.p, usage, s_noneFileIndex, s_noneFileIndex);
    }
    return(res);
}

// !!! this is called also for qualified super
Symbol * javaQualifiedThis(S_idList *tname, S_id *thisid) {
    Symbol			*str;
    S_typeModifier		*expr;
    S_reference			*rr, *lastUselessRef;
    int					ttype;
    lastUselessRef = NULL;
    ttype = javaClassifyAmbiguousName(tname, NULL,&str,&expr,&rr,
                                      &lastUselessRef, USELESS_FQT_REFS_ALLOWED,CLASS_TO_TYPE,UsageUsed);
    if (ttype == TypeStruct) {
        addThisCxReferences(str->u.s->classFile, &thisid->p);
/*&
        addSpecialFieldReference(LINK_NAME_MAYBE_THIS_ITEM,StorageField,
                                 str->u.s->classFile, &thisid->p,
                                 UsageMaybeThis);
&*/
        if (str->u.s->classFile == s_javaStat->classFileIndex) {
            // redundant qualified this prefix
            addUselessFQTReference(str->u.s->classFile, &thisid->p);
//&fprintf(dumpOut,"!adding useless reference on %d,%d\n", name->idi.p.line, name->idi.p.coll);
            javaResetUselessReference(lastUselessRef);
        }
    } else {
        str = &s_errorSymbol;
    }
    tname->nameType = TypeStruct;
    return(str);
}

void javaClassifyToPackageName( S_idList *id ) {
    S_idList *ii;
    for (ii=id; ii!=NULL; ii=ii->next) ii->nameType = TypePackage;
}

void javaClassifyToPackageNameAndAddRefs(S_idList *id, int usage) {
    S_idList *ii;
    javaClassifyToPackageName( id);
    for (ii=id; ii!=NULL; ii=ii->next) javaAddNameCxReference(ii, usage);
}

void javaAddPackageDefinition(S_idList *id) {
    javaClassifyToPackageNameAndAddRefs(id, UsageUsed); // UsageDefined);
}

void javaSetFieldLinkName(Symbol *field) {
    char *ln;
    assert(s_javaStat);
    ln = javaCreateComposedName(NULL,s_javaStat->className,'/', field->name, NULL, 0);
    field->linkName = ln;
}


Symbol *javaCreateNewMethod(char *nn, S_position *p, int mem) {
    S_typeModifier *m;
    Symbol		*res;
    char            *name;
    if (mem==MEMORY_CF) {
        CF_ALLOCC(name, strlen(nn)+1, char);
        strcpy(name, nn);
        CF_ALLOC(m, S_typeModifier);
        CF_ALLOC(res, Symbol);
    } else {
        name = nn;
        m = StackMemAlloc(S_typeModifier);
        res = StackMemAlloc(Symbol);
    }

    initTypeModifierAsFunction(m, NULL, NULL, NULL, NULL);
    fillSymbolWithType(res, name, name, *p, m);

    return(res);
}

int javaTypeToString(S_typeModifier *type, char *pp, int ppSize) {
    int ppi;
    S_typeModifier *tt;
    ppi=0;
    for (tt=type; tt!=NULL; tt=tt->next) {
        if (tt->kind == TypeArray) {
            sprintf(pp+ppi,"[");
            ppi += strlen(pp+ppi);
        } else if (tt->kind == TypeStruct) {
            assert(tt->u.t);
            sprintf(pp+ppi,"L%s;",tt->u.t->linkName);
            ppi += strlen(pp+ppi);
        } else {
            assert(s_javaBaseTypeCharCodes[tt->kind]!=0);
            pp[ppi++] = s_javaBaseTypeCharCodes[tt->kind];
        }
        assert(ppi < ppSize);
    }
    pp[ppi]=0;
    return(ppi);
}

int javaIsYetInTheClass(Symbol *clas, char *lname, Symbol **eq) {
    Symbol        *r;
    assert(clas && clas->u.s);
    for (r=clas->u.s->records; r!=NULL; r=r->next) {
    //&fprintf(dumpOut, "[javaIsYetInTheClass] checking %s <-> %s\n",r->linkName, lname);fflush(dumpOut);
        if (strcmp(r->linkName, lname)==0) {
            *eq = r;
            return(1);
        }
    }
    *eq = NULL;
    return(0);
}


static int javaNumberOfNativeMethodsWithThisName(Symbol *clas, char *name) {
    Symbol        *r;
    int				res;
    res = 0;
    assert(clas && clas->bits.symType==TypeStruct && clas->u.s);
    for (r=clas->u.s->records; r!=NULL; r=r->next) {
        if (strcmp(r->name, name)==0 && (r->bits.access&ACCESS_NATIVE)) {
            res++;
        }
    }
    return(res);
}


int javaSetFunctionLinkName(Symbol *clas, Symbol *decl,int mem) {
    static char pp[MAX_PROFILE_SIZE];
    char *ln;
    int ppi, res;
    Symbol *args;
    Symbol *memb;

    res = 0;
    if (decl == &s_errorSymbol || decl->bits.symType==TypeError) return(res);
    assert(decl->bits.symType == TypeDefault);
    assert(decl->u.type);
    if (decl->u.type->kind != TypeFunction) return(res);
    ppi=0;
//&	if (decl->bits.access & ACCESS_STATIC) {
//&		sprintf(pp+ppi,"%s.%s",clas->linkName, decl->name);
//&	} else {
        sprintf(pp+ppi,"%s", decl->name);
//&	}
    ppi += strlen(pp+ppi);
    sprintf(pp+ppi,"(");
    ppi += strlen(pp+ppi);
    for(args=decl->u.type->u.f.args; args!=NULL; args=args->next) {
        ppi += javaTypeToString(args->u.type, pp+ppi, MAX_PROFILE_SIZE-ppi);
    }
    sprintf(pp+ppi,")");
    ppi += strlen(pp+ppi);
    assert(ppi<MAX_PROFILE_SIZE);
    if (javaIsYetInTheClass(clas, pp, &memb)) {
        decl->linkName = memb->linkName;
    } else {
        if (mem == MEMORY_CF) {
            CF_ALLOCC(ln, ppi+1, char);
        } else {
            assert(mem==MEMORY_XX);
            XX_ALLOCC(ln, ppi+1, char);
        }
        strcpy(ln,pp);
        decl->linkName = ln;
        res = 1;
    }
    return(res);
}

static void addNativeMethodCxReference(Symbol *decl, Symbol *clas) {
    char nlname[MAX_CX_SYMBOL_SIZE];
    char *s, *d;

    sprintf(nlname, "Java_");
    s = clas->linkName;
    d = nlname + strlen(nlname);
    for(; *s; s++,d++) {
        if (*s=='/' || *s=='\\' || *s=='.') *d = '_';
        else if (*s=='_') {*d++ = '_'; *d = '1';}
        else if (*s=='$') {*d++ = '_'; *d++='0'; *d++='0'; *d++='0'; *d++='2'; *d='4';}
        else *d = *s;
    }
    *d++ = '_';
    if (javaNumberOfNativeMethodsWithThisName(clas, decl->name) > 1) {
        s = decl->linkName;
    } else {
        s = decl->name;
    }
    for(; *s; s++,d++) {
        if (*s=='/' || *s=='\\' || *s=='.') *d = '_';
        else if (*s=='_') {*d++ = '_'; *d = '1';}
        else if (*s==';') {*d++ = '_'; *d = '2';}
        else if (*s=='[') {*d++ = '_'; *d = '3';}
        else if (*s=='$') {*d++ = '_'; *d++='0'; *d++='0'; *d++='0'; *d++='2'; *d='4';}
        else if (*s=='(') {*d++ = '_'; *d = '_';}
        else if (*s==')') *d=0;
        else *d = *s;
    }
    *d = 0;
    assert(d-nlname < MAX_CX_SYMBOL_SIZE-1);
    addTrivialCxReference(nlname, TypeDefault,StorageExtern, &decl->pos,
                          UsageJavaNativeDeclared);
}

int javaLinkNameIsANestedClass(char *cname) {
    // this is a very hack!!   TODO do this seriously
    // an alternative can be 'javaSimpleNameIsInnerMemberClass', but it seems
    // to be even worst
    if (strchr(cname, '$') != NULL) return(1);
    return(0);
}

int isANestedClass(Symbol *ss) {
    return(javaLinkNameIsANestedClass(ss->linkName));
}

int javaLinkNameIsAnnonymousClass(char *linkname) {
    char *ss;
    // this is a very hack too!!   TODO do this seriously
    ss = linkname;
    while ((ss=strchr(ss, '$'))!=NULL) {
        ss++;
        if (isdigit(*ss)) return(1);
    }
    return(0);
}

void addThisCxReferences(int classIndex, S_position *pos) {
    int usage;
    if (classIndex == s_javaStat->classFileIndex) {
        usage = UsageMaybeThis;
    } else {
        usage = UsageMaybeQualifiedThis;
    }
    addSpecialFieldReference(LINK_NAME_MAYBE_THIS_ITEM,StorageField,
                             classIndex, pos, usage);
}

S_reference *addUselessFQTReference(int classIndex, S_position *pos) {
    S_reference *res;
    res = addSpecialFieldReference(LINK_NAME_IMPORTED_QUALIFIED_ITEM,StorageField,
                                   classIndex, pos, UsageLastUseless);
    return(res);
}

S_reference *addUnimportedTypeLongReference(int classIndex, S_position *pos) {
    S_reference *res;
    res = addSpecialFieldReference(LINK_NAME_UNIMPORTED_QUALIFIED_ITEM, StorageField,
                                   classIndex, pos, UsageUsed);
    return(res);
}

void addSuperMethodCxReferences(int classIndex, S_position *pos) {
    addSpecialFieldReference(LINK_NAME_SUPER_METHOD_ITEM,StorageField,
                             classIndex, pos,
                             UsageSuperMethod);
}

Symbol *javaPrependDirectEnclosingInstanceArgument(Symbol *args) {
    warning(ERR_ST,"[javaPrependDirectEnclosingInstanceArgument] not yet implemented");
    return(args);
}

void addMethodCxReferences(unsigned modif, Symbol *method, Symbol *clas) {
    int clasn;
    assert(clas && clas->u.s);
    clasn = clas->u.s->classFile;
    assert(clasn!=s_noneFileIndex);
    addCxReference(method, &method->pos, UsageDefined, clasn, clasn);
    if (modif & ACCESS_NATIVE) {
        addNativeMethodCxReference(method, clas);
    }
}

Symbol *javaMethodHeader(unsigned modif, Symbol *type,
                           Symbol *decl, int storage) {
    int newFun,vClass;
    completeDeclarator(type, decl);
    decl->bits.access = modif;
    decl->bits.storage = storage;
    if (s_javaStat->thisClass->bits.access & ACCESS_INTERFACE) {
        // set interface default access flags
        decl->bits.access |= (ACCESS_PUBLIC | ACCESS_ABSTRACT);
    }
    newFun = javaSetFunctionLinkName(s_javaStat->thisClass, decl,MEMORY_XX);
    //&assert(newFun==0); // This should be allways zero now with jsl.
    vClass = s_javaStat->classFileIndex;
    addMethodCxReferences(modif, decl, s_javaStat->thisClass);
    htmlAddJavaDocReference(decl, &decl->pos, vClass, vClass);
    if (newFun) {
        LIST_APPEND(Symbol, s_javaStat->thisClass->u.s->records, decl);
    }
    return(decl);
}

void javaAddMethodParametersToSymTable(Symbol *method) {
    Symbol *p;
    int i;
    for(p=method->u.type->u.f.args,i=1; p!=NULL; p=p->next,i++) {
        addFunctionParameterToSymTable(method, p, i, s_javaStat->locals);
    }
}

void javaMethodBodyBeginning(Symbol *method) {
    assert(method->u.type && method->u.type->kind == TypeFunction);
    s_cp.function = method;
    genInternalLabelReference(-1, UsageDefined);
    s_count.localVar = 0;
    javaAddMethodParametersToSymTable(method);
    method->u.type->u.m.signature = strchr(method->linkName, '(');
    s_javaStat->methodModifiers = method->bits.access;
}

// this should be merged with _bef_ token!
void javaMethodBodyEnding(S_position *endpos) {
    if (s_opt.taskRegime == RegimeHtmlGenerate) {
        htmlAddFunctionSeparatorReference();
    } else if (s_opt.taskRegime == RegimeEditServer) {
        if (s_cp.parserPassedMarker && !s_cp.thisMethodMemoriesStored){
            s_cps.methodCoordEndLine = cFile.lineNumber+1;
        }
    }
    // I rely that it is nil, for example in setmove target
    s_cp.function = NULL;
}

void javaAddMapedTypeName(
                            char *file,
                            char *path,
                            char *pack,
                            Completions *c,
                            void *vdirid,
                            int  *storage
                        ) {
    char				*p;
    char                ttt2[MAX_FILE_NAME_SIZE];
    int					len2;
    S_idList       dd2,*packid;
    Symbol			*memb;

    /*&fprintf(dumpOut,":import type %s %s %s\n", file, path, pack);&*/
    packid = (S_idList *) vdirid;
    for(p=file; *p && *p!='.' && *p!='$'; p++) ;
    if (*p != '.') return;
    if (strcmp(p,".class")!=0 && strcmp(p,".java")!=0) return;
    len2 = p - file;
    strncpy(ttt2, file, len2);
    assert(len2+1 < MAX_FILE_NAME_SIZE);
    ttt2[len2] = 0;
    fillfIdList(&dd2, ttt2, NULL, s_noPos, ttt2, TypeStruct, packid);
    memb = javaTypeSymbolDefinition(&dd2, ACCESS_DEFAULT, TYPE_ADD_YES);
    log_debug(":import type %s == %s", memb->name, memb->linkName);
    memb = memb;                /* If not DEBUG memb is "set but not used..." */
}

S_typeModifier *javaClassNameType(S_idList *typeName) {
    Symbol *st;

    assert(typeName);
    assert(typeName->nameType == TypeStruct);
    st = javaTypeSymbolUsage(typeName, ACCESS_DEFAULT);

    return newStructTypeModifier(st);
}

S_typeModifier *javaNestedNewType(Symbol *sym, S_id *thenew,
                                  S_idList *idl) {
    S_idList       d1,d2;
    S_typeModifier     *res;
    char                *id2;
    S_id			*id;
    Symbol			*str;
    S_reference			*rr;
    if (idl->next == NULL) {
        // standard nested new
        id = &idl->id;
        assert(sym && sym->linkName);
        id2 = sym->linkName;
        fillfIdList(&d2, id2, sym, s_noPos, id2, TypeStruct, NULL);
        fillIdList(&d1, *id, id->name, TypeStruct, &d2);
        javaClassifyNameToNestedType(&d1, sym, UsageUsed, &str, &rr);
        res = javaClassNameType(&d1);
    } else {
        // O.K. extended case, usually syntax error, but ...
        javaClassifyToTypeName(idl, UsageUsed, &str, USELESS_FQT_REFS_ALLOWED);
        res = javaClassNameType(idl);
        // you may also check that idl->next == sym
        if (res && res->kind == TypeStruct) {
            assert(res->u.t && res->u.t->u.s);
            if (res->u.t->bits.access & ACCESS_STATIC) {
                // add the prefix of new as redundant long name
                addUselessFQTReference(res->u.t->u.s->classFile, &thenew->p);
            }
        } else {
            res = &s_errorModifier;
        }
    }
    return(res);
}

S_typeModifier *javaNewAfterName(S_idList *name, S_id *thenew, S_idList *idl) {
    Symbol *str;
    S_typeModifier *expr, *res;
    int atype;
    S_reference *rr;

    atype = javaClassifyAmbiguousName(name,NULL,&str,&expr,&rr,NULL, USELESS_FQT_REFS_ALLOWED,CLASS_TO_EXPR,UsageUsed);
    if (atype == TypeExpression) {
        if (expr->kind != TypeStruct) res = & s_errorModifier;
        else res = javaNestedNewType(expr->u.t, thenew, idl);
    } else if (atype == TypeStruct) {
        assert(str);
        assert(str->bits.symType == TypeStruct);
        res = javaNestedNewType(str, thenew, idl);
    } else res = & s_errorModifier;
    return(res);
}

static int javaExistBaseTypeWideningConversion(int t1, int t2) {
    int i1,i2,res;
    assert(t1>=0 && t1<MAX_TYPE);
    assert(t2>=0 && t2<MAX_TYPE);
/*fprintf(dumpOut,"testing base convertibility of %s to %s\n",typeName[t1],typesName[t2]);fflush(dumpOut);*/
    if (t1 == t2) return(1);
    i1 = s_javaTypePCTIConvert[t1];
    i2 = s_javaTypePCTIConvert[t2];
    if (i1 == 0 || i2 == 0) return(0);
    assert(i1-1>=0 && i1-1<MAX_PCTIndex-1);
    assert(i2-1>=0 && i2-1<MAX_PCTIndex-1);
    res = s_javaPrimitiveWideningConversions[i1-1][i2-1];
    assert(res == 0 || res == 1);
/*fprintf(dumpOut,"the result is %d\n",res); fflush(dumpOut);*/
    return(res);
}

static int javaExistBaseWideningConversion(char c1, char c2) {
    int t1,t2;
/*fprintf(dumpOut,"testing base convertibility of %c to %c\n",c1,c2);fflush(dumpOut);*/
    assert(c1>=0 && c1<MAX_CHARS);
    assert(c2>=0 && c2<MAX_CHARS);
    if (c1 == c2) return(1);
    t1 = s_javaCharCodeBaseTypes[c1];
    t2 = s_javaCharCodeBaseTypes[c2];
    return(javaExistBaseTypeWideningConversion(t1, t2));
}

#define PassArrayTypeString(tstring) {\
    for(tstring++; *tstring && isdigit(*tstring); tstring++) ;\
}

static char * javaPassToNextParamInTypeString(char *ss) {
    assert(ss);
    while (*ss == '[') PassArrayTypeString(ss) ;
    assert(*ss);
    if (*ss == 'L') {
        for(ss++; *ss && *ss!=';'; ss++) ;
        ss++;
    } else {
        ss++;
    }
    return(ss);
}

static Symbol *javaStringTypeObject(char **ss) {
    char        *s;
    Symbol    *res;
    s = *ss;
    if (*s == 'L') {
        res = javaGetFieldClass(s+1,&s);
        s++;
    } else if (*s == '[') {
        res = &s_javaArrayObjectSymbol;
        s = javaPassToNextParamInTypeString(s);
    } else {
        res = NULL;
        s++;
    }
    *ss = s;
    return(res);
}

static int javaExistWideningConversion(char **t1, char **t2) {
    char ss1,*s1, *s2;
    char c1,c2;
    Symbol *sym1,*sym2;
    int res = 1;
    s1 = *t1; s2 = *t2;
    assert(s1 && s2);
/*fprintf(dumpOut,"testing convertibility of %s to %s\n",s1,s2);fflush(dumpOut);*/
    while (*s1 == '[' && *s2 == '[') {
        /* dereference array */
        PassArrayTypeString(s1);
        assert(*s1);
        PassArrayTypeString(s2);
        assert(*s2);
    }
    if (*s1 == JAVA_NULL_CODE && *s2 == '[') {
        s1++;
        while (*s2 == '[') PassArrayTypeString(s2);
        if (*s2 == 'L') javaStringTypeObject(&s2);
        else s2++;
        goto finish;
    }
    if (*s1 == 'L' || *s2 == 'L') {
        ss1 = *s1;
        sym1 = javaStringTypeObject(&s1);
        sym2 = javaStringTypeObject(&s2);
        if (ss1 == JAVA_NULL_CODE) {res &= 1; goto finish;}
        if (ss1 == '['
                &&  (sym2 == s_javaCloneableSymbol
                     || sym2 == s_javaIoSerializableSymbol
                     || sym2 == s_javaObjectSymbol)) {
            res &= 1; goto finish;
        }
        if (sym1==NULL || sym2 == NULL) {res &= 0; goto finish;}
        assert(sym1 && sym2 && sym1->u.s);
        if (sym1 == sym2) {res &= 1; goto finish;}
        javaLoadClassSymbolsFromFile(sym1);
/*fprintf(dumpOut,"test cctmembership of %s to %s\n",sym1->linkName, sym2->linkName);*/
        res &= cctIsMember(& sym1->u.s->casts, sym2, 1);
        goto finish;
    } else {
        c1 = *s1++; c2 = *s2++;
        res &= javaExistBaseWideningConversion(c1,c2);
        goto finish;
    }
    assert(0);
finish:
    *t1 = s1; *t2 = s2;
/*fprintf(dumpOut,"the result is %d\n",res); fflush(dumpOut);*/
    return(res);
}


static int javaSmallerProfile(Symbol *s1, Symbol *s2) {
    int r;
    char *p1,*p2;
    assert(s1 && s1->bits.symType==TypeDefault && s1->u.type);
    assert(s1->u.type->kind == TypeFunction && s1->u.type->u.m.signature);
    assert(s2 && s2->bits.symType==TypeDefault && s2->u.type);
    assert(s2->u.type->kind == TypeFunction && s2->u.type->u.m.signature);
    p1 = s1->u.type->u.m.signature;
    p2 = s2->u.type->u.m.signature;
/*fprintf(dumpOut,"comparing %s to %s\n",p1,p2); fflush(dumpOut);*/
    assert(*p1 == '(');
    assert(*p2 == '(');
    p1 ++; p2++;
    while (*p1 != ')' && *p2 != ')') {
        r = javaExistWideningConversion(&p1, &p2);
        if (r == 0) return(0);
    }
    if (*p1 != ')' || *p2 != ')') return(0);
/*fprintf(dumpOut,"the result is 1\n"); fflush(dumpOut);*/
    return(1);
}

int javaMethodApplicability(Symbol *memb, char *actArgs) {
    int r;
    char *fargs;
    assert(memb && memb->bits.symType==TypeDefault && memb->u.type);
    assert(memb->u.type->kind == TypeFunction && memb->u.type->u.m.signature);
    fargs = memb->u.type->u.m.signature;
//&sprintf(tmpBuff,"testing applicability of %s to %s\n",fargs,actArgs);ppcGenTmpBuff();
    assert(*fargs == '(');
    fargs ++;
    while (*fargs != ')' && *actArgs!=0) {
        r = javaExistWideningConversion(&actArgs, &fargs);
        if (r == 0) return(0);
    }
    if (*fargs == ')' && *actArgs == 0) return(PROFILE_APPLICABLE);
    if (*actArgs == 0) return(PROFILE_PARTIALLY_APPLICABLE);
    return(PROFILE_NOT_APPLICABLE);
}

Symbol *javaGetSuperClass(Symbol *cc) {
    SymbolList		*sups;
    assert(cc->bits.symType == TypeStruct && cc->u.s);
    sups = cc->u.s->super;
    if (sups == NULL) return(&s_errorSymbol);	/* class Object only */
    return(sups->d);
}

Symbol *javaCurrentSuperClass(void) {
    S_typeModifier     *tt;
    Symbol			*cc;

    assert(s_javaStat);
    tt = s_javaStat->thisType;
    assert(tt->kind == TypeStruct);
    cc = tt->u.t;
    return(javaGetSuperClass(cc));
}

/* ********************* method invocations ************************** */

static S_typeModifier *javaMethodInvocation(
                                        S_recFindStr *rfs,
                                        Symbol *memb,
                                        S_id *name,
                                        S_typeModifierList *args,
                                        int invocationType,
                                        S_position *superPos) {
    char				actArg[MAX_PROFILE_SIZE];
    Symbol            * appl[MAX_APPL_OVERLOAD_FUNS];
    int                 funCl[MAX_APPL_OVERLOAD_FUNS];
    unsigned			minacc[MAX_APPL_OVERLOAD_FUNS];
    SymbolList		*ee;
    S_typeModifierList *aaa;
    S_usageBits			ub;
    int					smallesti, baseCl, vApplCl, vFunCl, usedusage;
    int					i,appli,actArgi,rr;

    assert(rfs->baseClass);  // method must be inside a class
    assert(rfs->baseClass->bits.symType == TypeStruct);
    baseCl = rfs->baseClass->u.s->classFile;
    assert(baseCl != -1);

//&sprintf(tmpBuff,"java method invocation\n"); ppcGenTmpBuff();
//&sprintf(tmpBuff,"the method is %s == '%s'\n",memb->name,memb->linkName);ppcGenTmpBuff();
    assert(memb && memb->bits.symType==TypeDefault && memb->u.type->kind == TypeFunction);
    for(aaa=args; aaa!=NULL; aaa=aaa->next) {
        if (aaa->d->kind == TypeError) {
//&fprintf(dumpOut,"induced missinterpred at %d\n", name->p.line);
            addTrivialCxReference(LINK_NAME_INDUCED_ERROR,TypeInducedError,StorageDefault,
                                  &name->p, UsageUsed);
            return(&s_errorModifier);
        }
    }
    *actArg = 0; actArgi = 0;
    for(aaa=args; aaa!=NULL; aaa=aaa->next) {
        actArgi += javaTypeToString(aaa->d,actArg+actArgi,MAX_PROFILE_SIZE-actArgi);
    }
//&sprintf(tmpBuff,"arguments types == %s\n",actArg);ppcGenTmpBuff();
    appli = 0;
    do {
        assert(memb != NULL);
//&sprintf(tmpBuff,"testing: %s\n",memb->linkName);ppcGenTmpBuff();
        if (javaRecordVisibleAndAccessible(rfs, rfs->baseClass, rfs->currClass, memb)
            && javaMethodApplicability(memb,actArg) == PROFILE_APPLICABLE) {
            appl[appli] = memb;
            minacc[appli] = javaGetMinimalAccessibility(rfs, memb);
            assert(rfs && rfs->currClass && rfs->currClass->bits.symType==TypeStruct);
            funCl[appli] = rfs->currClass->u.s->classFile;
            assert(funCl[appli] != -1);
            appli++;
//&sprintf(tmpBuff,"applicable: %s of %s\n",memb->linkName,rfs->currClass->linkName);ppcGenTmpBuff();
        }
        rr = findStrRecordSym(rfs, name->name, &memb,
                              CLASS_TO_METHOD, ACC_CHECK_NO, VISIB_CHECK_NO);
        if(invocationType==CONSTRUCTOR_INVOCATION&&rfs->baseClass!=rfs->currClass){
            // constructors are not inherited
            rr = RETURN_NOT_FOUND;
        }
    } while (rr==RETURN_OK);
    if (appli == 0) return(&s_errorModifier);
//&sprintf(tmpBuff,"looking for smallest\n");ppcGenTmpBuff();
    smallesti = 0;
    for (i=1; i<appli; i++) {
        if (! javaSmallerProfile(appl[smallesti], appl[i])) smallesti = i;
/*&		if (strcmp(appl[smallesti]->u.type->u.m.signature, appl[i]->u.type->u.m.signature)==0) {&*/
            /* virtual application, take one from the super-class */
/*&			smallesti = i;&*/
/*&		}&*/
    }
    for (i=0; i<appli; i++) {
        if (! javaSmallerProfile(appl[smallesti], appl[i])) return(&s_errorModifier);
    }
//&sprintf(tmpBuff,"the invoked method is %s of %s\n\n",appl[smallesti]->linkName,s_fileTab.tab[funCl[smallesti]]->name);ppcGenTmpBuff();
    assert(appl[smallesti]->bits.symType == TypeDefault);
    assert(appl[smallesti]->u.type->kind == TypeFunction);
    assert(funCl[smallesti] != -1);
    vFunCl = funCl[smallesti];
    vApplCl = baseCl;
//&	if (appl[smallesti]->bits.access & ACCESS_STATIC) {
//&		vFunCl = vApplCl = s_noneFileIndex;
//&	}
    usedusage = UsageUsed;
    if (invocationType == CONSTRUCTOR_INVOCATION) {
        // this is just because java2html, so that constructors invocations
        // are linked to constructors rather than classes
        usedusage = UsageConstructorUsed;
    }
    if (invocationType == SUPER_METHOD_INVOCATION) {
        usedusage = UsageMethodInvokedViaSuper;
        addSuperMethodCxReferences(vFunCl, superPos);
    }
    fill_usageBits(&ub, usedusage, minacc[smallesti]);
    addCxReferenceNew(appl[smallesti], &name->p, &ub, vFunCl, vApplCl);
    if (s_opt.server_operation == OLO_EXTRACT) {
        for(ee=appl[smallesti]->u.type->u.m.exceptions; ee!=NULL; ee=ee->next) {
            addCxReference(ee->d, &name->p, UsageThrown, s_noneFileIndex, s_noneFileIndex);
        }
    }
    return(appl[smallesti]->u.type->next);
}


S_extRecFindStr *javaCrErfsForMethodInvocationN(S_idList *name) {
    S_extRecFindStr		*erfs;
    S_typeModifier		*expr;
    S_reference			*rr;
    int					nt;
    XX_ALLOC(erfs, S_extRecFindStr);
    erfs->params = NULL;
    nt = javaClassifyAmbiguousName(name, &erfs->s,&erfs->memb,&expr,&rr,NULL, USELESS_FQT_REFS_ALLOWED,CLASS_TO_METHOD,UsageUsed);
    if (nt != TypeExpression) {
        methodNameNotRecognized(name->id.name);
        return(NULL);
    }
    if (expr == &s_errorModifier) return(NULL);
    return(erfs);
}

S_typeModifier *javaMethodInvocationN(	S_idList *name,
                                        S_typeModifierList *args
                                    ) {
    S_extRecFindStr		*erfs;
    S_typeModifier		*res;
    erfs = javaCrErfsForMethodInvocationN(name);
    if (erfs == NULL) return(&s_errorModifier);
    res = javaMethodInvocation(&erfs->s, erfs->memb, &name->id, args,REGULAR_METHOD,&s_noPos);
    return(res);
}

S_extRecFindStr *javaCrErfsForMethodInvocationT(S_typeModifier *tt,
                                                S_id *name
    ) {
    S_extRecFindStr		*erfs;
    int					rr;
    log_trace("invocation of %s", name->name);
    if (tt->kind == TypeArray) tt = &s_javaArrayObjectSymbol.u.s->stype;
    if (tt->kind != TypeStruct) {
        methodAppliedOnNonClass(name->name);
        return(NULL);
    }
    XX_ALLOC(erfs, S_extRecFindStr);
    erfs->params = NULL;
    javaLoadClassSymbolsFromFile(tt->u.t);
    rr = findStrRecordSym(iniFind(tt->u.t,&erfs->s), name->name, &erfs->memb,
                        CLASS_TO_METHOD, ACC_CHECK_NO,VISIB_CHECK_NO);
    if (rr != RETURN_OK) {
        noSuchRecordError(name->name);
        return(NULL);
    }
    return(erfs);
}

S_typeModifier *javaMethodInvocationT(S_typeModifier *tt,
                                       S_id *name,
                                       S_typeModifierList *args
                                       ) {
    S_extRecFindStr		*erfs;
    S_typeModifier		*res;
    erfs = javaCrErfsForMethodInvocationT(tt, name);
    if (erfs == NULL) return(&s_errorModifier);
    res = javaMethodInvocation(&erfs->s, erfs->memb, name, args,REGULAR_METHOD,&s_noPos);
    return(res);
}

S_extRecFindStr *javaCrErfsForMethodInvocationS(S_id *super, S_id *name) {
    Symbol            *ss;
    S_extRecFindStr		*erfs;
    int					rr;
    ss = javaCurrentSuperClass();
    if (ss == &s_errorSymbol || ss->bits.symType==TypeError) return(NULL);
    assert(ss && ss->bits.symType == TypeStruct);
    assert(ss->bits.javaFileIsLoaded);
    XX_ALLOC(erfs, S_extRecFindStr);
    erfs->params = NULL;
/*	I do not know, once will come the day I will know
    if (s_javaStat->cpMethod != NULL) {
        erfs->s.accessed = s_javaStat->cpMethod->b.accessFlags;
    }
*/
    rr = findStrRecordSym(iniFind(ss, &erfs->s), name->name, &erfs->memb,
                        CLASS_TO_METHOD,ACC_CHECK_NO,VISIB_CHECK_NO);
    if (rr != RETURN_OK) return(NULL);
    return(erfs);
}

S_typeModifier *javaMethodInvocationS(S_id *super,
                                       S_id *name,
                                       S_typeModifierList *args
    ) {
    S_extRecFindStr		*erfs;
    S_typeModifier		*res;
    erfs = javaCrErfsForMethodInvocationS(super, name);
    if (erfs==NULL) return(&s_errorModifier);
    res = javaMethodInvocation(&erfs->s, erfs->memb, name, args, SUPER_METHOD_INVOCATION,&super->p);
    assert(erfs->s.currClass && erfs->s.currClass->u.s);
    return(res);
}

S_extRecFindStr *javaCrErfsForConstructorInvocation(Symbol *clas,
                                                    S_position *pos
    ) {
    S_extRecFindStr		*erfs;
    int					rr;
    if (clas == &s_errorSymbol || clas->bits.symType==TypeError) return(NULL);
    assert(clas && clas->bits.symType == TypeStruct);
    javaLoadClassSymbolsFromFile(clas);
    XX_ALLOC(erfs, S_extRecFindStr);
    erfs->params = NULL;
    assert(clas->bits.javaFileIsLoaded);
    rr = findStrRecordSym(iniFind(clas, &erfs->s), clas->name, &erfs->memb,
                        CLASS_TO_METHOD,ACC_CHECK_NO,VISIB_CHECK_NO);
    if (rr != RETURN_OK) return(NULL);
    return(erfs);
}

S_typeModifier *javaConstructorInvocation(Symbol *clas,
                                           S_position *pos,
                                           S_typeModifierList *args
    ) {
    S_extRecFindStr		*erfs;
    S_typeModifier		*res;
    S_id			name;
    erfs = javaCrErfsForConstructorInvocation(clas, pos);
    if (erfs == NULL) return(&s_errorModifier);
    if (erfs->s.baseClass != erfs->s.currClass) return(&s_errorModifier);
    fillId(&name, clas->name, NULL, *pos);
    res = javaMethodInvocation(&erfs->s, erfs->memb, &name, args,CONSTRUCTOR_INVOCATION,&s_noPos);
    return(res);
}


/* ************************ expression evaluations ********************* */


static int javaIsNumeric(S_typeModifier *tt) {
    assert(tt);
    switch (tt->kind) {
    case TypeByte: case TypeShort: case TypeChar:
    case TypeInt: case TypeLong:
    case TypeDouble: case TypeFloat:
        return(1);
    default:
        return(0);
    }
}

S_typeModifier *javaCheckNumeric(S_typeModifier *tt) {
    if (javaIsNumeric(tt)) return(tt);
    else return(&s_errorModifier);
}

S_typeModifier *javaNumericPromotion(S_typeModifier *tt) {
    assert(tt);
    switch (tt->kind) {
    case TypeByte:
    case TypeShort:
    case TypeChar:
        return newSimpleTypeModifier(TypeInt);
    case TypeInt:
    case TypeLong:
    case TypeDouble:
    case TypeFloat:
        return(tt);
    default:
        return(&s_errorModifier);
    }
}

S_typeModifier *javaBinaryNumericPromotion(S_typeModifier *t1, S_typeModifier *t2) {
    int m1,m2,resultingType;

    m1 = t1->kind;
    m2 = t2->kind;
    assert(t1 && t2);
    resultingType = TypeInt;
    if (m1 == TypeDouble || m2 == TypeDouble) resultingType = TypeDouble;
    else if (m1 == TypeFloat || m2 == TypeFloat) resultingType = TypeFloat;
    else if (m1 == TypeLong || m2 == TypeLong) resultingType = TypeLong;

    return newSimpleTypeModifier(resultingType);
}

S_typeModifier *javaBitwiseLogicalPromotion(	S_typeModifier *t1,
                                                S_typeModifier *t2
                                            ) {
    assert(t1 && t2);
    if (t1->kind == TypeBoolean && t2->kind == TypeBoolean) return(t1);
    return(javaBinaryNumericPromotion(t1,t2));
}

int javaIsStringType(S_typeModifier *tt) {
    if (tt->kind != TypeStruct) return(0);
    return(tt->u.t == s_javaStringSymbol);
}

static int javaEqualTypes(S_typeModifier *t1,S_typeModifier *t2) {
    int m;
lastRecursionLabel:
    if (t1->kind != t2->kind) return(0);
    m = t1->kind;
    if (m == TypeStruct || m == TypeUnion) return(t1->u.t == t2->u.t);
    if (m == TypeArray) {
        t1 = t1->next; t2 = t2->next;
        goto lastRecursionLabel;
    }
    assert(m != TypeFunction);
    return(1);
}

static int javaTypeConvertible(	S_typeModifier *t1,
                                S_typeModifier *t2
                            ) {
    Symbol    *s1,*s2;
    int         res;
lastRecLabel:
    if (javaIsNumeric(t1) && javaIsNumeric(t2)) {
        return(javaExistBaseTypeWideningConversion(t1->kind, t2->kind));
    }
    if (t1->kind != t2->kind) return(0);
    if (t1->kind == TypeArray) {
        t1 = t1->next; t2 = t2->next;
        goto lastRecLabel;
    }
    if (t1->kind == TypeStruct) {
        s1 = t1->u.t; s2 = t2->u.t;
        assert(s1 && s2);
        assert(s1->bits.symType == TypeStruct && s1->u.s);
        javaLoadClassSymbolsFromFile(s1);
        res = cctIsMember(&s1->u.s->casts, s2, 1);
//&fprintf(dumpOut,"!checking convertibility %s->%s, res==%d\n",s1->linkName, s2->linkName, res);fflush(dumpOut);
        return(res);
    }
    return(0);
}

S_typeModifier *javaConditionalPromotion(	S_typeModifier *t1,
                                            S_typeModifier *t2
                                        ) {
    if (javaEqualTypes(t1,t2)) return(t1);
    if (javaIsNumeric(t1) && javaIsNumeric(t2)) {
        if (t1->kind == TypeShort && t2->kind == TypeByte) return(t1);
        if (t1->kind == TypeByte && t2->kind == TypeShort) return(t2);
        /* TO FINISH FOR BYTE, SHORT, CHAR CONSTANT */
        return(javaBinaryNumericPromotion(t1,t2));
    }
    if (t1->kind == TypeNull && IsJavaReferenceType(t2->kind)) return(t2);
    if (t2->kind == TypeNull && IsJavaReferenceType(t1->kind)) return(t1);
    if (! IsJavaReferenceType(t1->kind) || ! IsJavaReferenceType(t2->kind)) {
        return(&s_errorModifier);
    }
    if (javaTypeConvertible(t1,t2)) return(t2);
    if (javaTypeConvertible(t2,t1)) return(t1);
    return(&s_errorModifier);
}

void javaTypeDump(S_typeModifier *tt) {
    assert(tt);
    if (tt->kind == TypeArray) {
        javaTypeDump(tt->next);
        fprintf(dumpOut,"[]");
    } else if (tt->kind == TypeStruct) {
        fprintf(dumpOut,"%s",tt->u.t->linkName);
    } else {
        fprintf(dumpOut,"%s",typeEnumName[tt->kind]);
    }
}

void javaAddJslReadedTopLevelClasses(S_jslTypeTab  *jslTypeTab) {
    int					i;
    JslSymbolList     *ss;
    for(i=0; i<jslTypeTab->size; i++) {
        if (jslTypeTab->tab[i]!=NULL) {
            LIST_REVERSE(JslSymbolList, jslTypeTab->tab[i]);
            for(ss=jslTypeTab->tab[i]; ss!=NULL; ss=ss->next) {
                javaAddTypeToSymbolTable(ss->d, ss->d->bits.access, &ss->pos, ss->isExplicitlyImported);
            }
            LIST_REVERSE(JslSymbolList, jslTypeTab->tab[i]);
        }
    }
}

static void javaAddNestedClassToSymbolTab( Symbol *str ) {
    S_symStructSpec *ss;
    int i;

    assert(str && str->bits.symType==TypeStruct);
    ss = str->u.s;
    assert(ss);
    for(i=0; i<ss->nestedCount; i++) {
        if (ss->nest[i].membFlag && javaRecordAccessible(NULL,str, str, ss->nest[i].cl, ss->nest[i].accFlags)) {
            javaAddTypeToSymbolTable(ss->nest[i].cl, ss->nest[i].cl->bits.access, &s_noPos, false);
        }
    }
}

void javaAddSuperNestedClassToSymbolTab( Symbol *cc ) {
    SymbolList *ss;
    for(ss=cc->u.s->super; ss != NULL; ss=ss->next) {
        javaAddSuperNestedClassToSymbolTab(ss->d);
    }
    javaAddNestedClassToSymbolTab(cc);
}


struct freeTrail *newClassDefinitionBegin(S_id *name,
                                          Access access,
                                          Symbol *anonymousInterface) {
    S_idList   *p;
    Symbol        *dd,*ddd;
    S_freeTrail     *res;
    S_javaStat      *oldStat;
    int             nnest,noff,classf;
    S_nestedSpec	*nst,*nn;
    S_symbolTable	*locals;
    S_id		idi;

    assert(s_javaStat);
    oldStat = s_javaStat;
    XX_ALLOC(s_javaStat, S_javaStat);
    *s_javaStat = *oldStat;
    s_javaStat->next = oldStat;
    XX_ALLOC(locals, S_symbolTable);
    symbolTableInit(locals, MAX_CL_SYMBOLS);
/*&fprintf(dumpOut,"adding new class %s\n",name->name);fflush(dumpOut);&*/
    if (oldStat->next!=NULL) {
        /* ** nested class ** */
        if (oldStat->thisClass->bits.access & ACCESS_INTERFACE) {
            access |= (ACCESS_PUBLIC | ACCESS_STATIC);
        }
        nnest = oldStat->thisClass->u.s->nestedCount;
        nst = oldStat->thisClass->u.s->nest;
        noff = oldStat->currentNestedIndex;
        oldStat->currentNestedIndex ++;
//&sprintf(tmpBuff,"checking %d of %d of %s(%d)\n", noff,nnest,oldStat->thisClass->linkName, oldStat->thisClass);ppcGenTmpBuff();
        assert(noff >=0 && noff<nnest);
        nn = & nst[noff];
        // nested class, it should be the same order as in first pass
        // but name can be different for anonymous classes?
//&fprintf(dumpOut,"comparing '%s' <-> '%s'\n", nn->cl->name, name->name);fflush(dumpOut);
//&		innerNamesCorrect = (strcmp(nn->cl->name, name->name)==0);
//&		assert(innerNamesCorrect);
        dd = nn->cl;
        fillId(&idi,dd->linkName, NULL, name->p);
        XX_ALLOC(p, S_idList);
        fillIdList(p, idi, dd->linkName, TypeStruct, NULL);
        ddd = javaAddType(p, access, & name->p);
        assert(dd==ddd);
        res = s_topBlock->trail;
        //&javaCreateClassFileItem(dd);
    } else {
        /* probably base class */
        XX_ALLOC(p,S_idList);
        fillIdList(p,*name,name->name,TypeStruct,s_javaStat->className);
        dd = javaAddType(p, access, & name->p);
        res = s_topBlock->trail;
        assert(dd->bits.symType == TypeStruct);
        s_spp[SPP_LAST_TOP_LEVEL_CLASS_POSITION] = name->p;
    }
    classf = dd->u.s->classFile;
    if (classf == -1) classf = s_noneFileIndex;
    fillJavaStat(s_javaStat,p,&dd->u.s->stype,dd,0, oldStat->currentPackage,
                  oldStat->unnamedPackagePath, oldStat->namedPackagePath,
                  locals, oldStat->lastParsedName,ACCESS_DEFAULT,s_cp,classf,oldStat);
    // added 8/8/2001 for clearing s_cp.function for SET_TARGET_POSITION check
    s_cp = s_cpInit;
//&fprintf(dumpOut,"clearing s_cp\n");
    return(res);
}

struct freeTrail *newAnonClassDefinitionBegin(S_id *interfName) {
    struct freeTrail * res;
    S_idList	*ll;
    Symbol		*interf, *str;
    XX_ALLOC(ll, S_idList);
    fillIdList(ll, *interfName, interfName->name, TypeDefault, NULL);
    javaClassifyToTypeName(ll,UsageUsed,&str, USELESS_FQT_REFS_ALLOWED);
    interf = javaTypeNameDefinition(ll);
    res = newClassDefinitionBegin(&s_javaAnonymousClassName, ACCESS_DEFAULT,
                                  interf);
    return(res);
}

void newClassDefinitionEnd(S_freeTrail *trail) {
    assert(s_javaStat && s_javaStat->next);
    removeFromTrailUntil(trail);
    /* TODO: WTF?!??! */
    // the following line makes that method extraction does not work,
    // make attention with it, ? really
    s_cp = s_javaStat->cp;
    s_javaStat = s_javaStat->next;
    log_trace("recovering s_cp to %d", s_cp.cxMemiAtClassBegin);
}

void javaInitArrayObject(void) {
    static Symbol s_lengthSymbol;
    static S_symStructSpec s_arraySpec;

    assert(s_javaObjectSymbol != NULL);
    javaLoadClassSymbolsFromFile(s_javaObjectSymbol);

    fillSymbolWithType(&s_lengthSymbol, "length", "java/lang/array.length",
                       s_noPos, &s_defaultIntModifier);

    initSymStructSpec(&s_arraySpec, /*.records=*/&s_lengthSymbol);
    /* Assumed to be Struct/Union/Enum? */
    initTypeModifierAsStructUnionOrEnum(&s_arraySpec.stype, /*.kind=*/TypeStruct,
                                        /*.u.t=*/&s_javaArrayObjectSymbol,
                                        /*.typedefSymbol=*/NULL, /*.next=*/NULL);
    initTypeModifierAsPointer(&s_arraySpec.sptrtype, &s_arraySpec.stype);

    fillSymbolWithStruct(&s_javaArrayObjectSymbol, "__arrayObject__", "__arrayObject__",
                         s_noPos, &s_arraySpec);
    fillSymbolBits(&s_javaArrayObjectSymbol.bits, ACCESS_PUBLIC, TypeStruct, StorageDefault);
    s_javaArrayObjectSymbol.u.s = &s_arraySpec;

    javaCreateClassFileItem(&s_javaArrayObjectSymbol);
    addSuperClassOrInterfaceByName(&s_javaArrayObjectSymbol,s_javaLangObjectLinkName,
                                   s_noneFileIndex, LOAD_SUPER);
}

S_typeModifier *javaArrayFieldAccess(S_id *id) {
    Symbol *rec=NULL;
    findStrRecordFromType(&s_javaArrayObjectSymbol.u.s->stype, id, &rec, CLASS_TO_EXPR);
    assert(rec);
    return(rec->u.type);
}

void javaParsedSuperClass(Symbol *symbol) {
    SymbolList *pp;
    assert(s_javaStat->thisClass && s_javaStat->thisClass->bits.symType==TypeStruct);
    assert(s_javaStat->thisClass->u.s);
    assert(symbol && symbol->bits.symType==TypeStruct && symbol->u.s);
    for(pp=s_javaStat->thisClass->u.s->super; pp!=NULL; pp=pp->next) {
        if (pp->d == symbol) break;
    }
    if (pp==NULL) {
//&fprintf(dumpOut,"manual super class %s of %s == %s\n",symbol->linkName,s_javaStat->thisClass->linkName, s_fileTab.tab[s_javaStat->thisClass->u.s->classFile]->name);fflush(dumpOut);
        //&assert(0); // this should never comed now
        javaLoadClassSymbolsFromFile(symbol);
        addSuperClassOrInterface(s_javaStat->thisClass, symbol,
                                 cFile.lexBuffer.buffer.fileNumber);
    }
}

void javaSetClassSourceInformation(char *package, S_id *classId) {
    char    fqt[MAX_FILE_NAME_SIZE];
    char    className[2*MAX_FILE_NAME_SIZE];
    int		fileIndex;

    assert(classId!=NULL);
    if (*package == 0) {
        sprintf(fqt, "%s", classId->name);
    } else {
        sprintf(fqt, "%s/%s", package, classId->name);
    }
    SPRINT_FILE_TAB_CLASS_NAME(className, fqt);
    fileIndex = addFileTabItem(className);
    s_fileTab.tab[fileIndex]->b.sourceFile = classId->p.file;
}


void javaCheckIfPackageDirectoryIsInClassOrSourcePath(char *dir) {
    S_stringList	*pp;
    if (s_opt.taskRegime == RegimeEditServer) return;
    for(pp=s_javaClassPaths; pp!=NULL; pp=pp->next) {
        if (fnCmp(dir, pp->d)==0) return;
    }
    JavaMapOnPaths(s_javaSourcePaths, {
        if (fnCmp(dir, currentPath)==0) return;
    });
    sprintf(tmpBuff, "Directory %s is not listed in paths", dir);
    warning(ERR_ST, tmpBuff);
}

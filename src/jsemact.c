#include "jsemact.h"

#include "commons.h"
#include "globals.h"
#include "counters.h"
#include "options.h"
#include "classcaster.h"
#include "misc.h"
#include "extract.h"
#include "proto.h"
#include "usage.h"
#include "yylex.h"
#include "semact.h"
#include "cxref.h"
#include "classfilereader.h"
#include "filedescriptor.h"
#include "editor.h"
#include "symbol.h"
#include "javafqttab.h"
#include "list.h"
#include "fileio.h"
#include "filetable.h"
#include "stackmemory.h"

#include "parsers.h"
#include "recyacc.h"
#include "jslsemact.h"

#include "protocol.h"

#include "log.h"


typedef enum {
    RESULT_IS_CLASS_FILE,
    RESULT_IS_JAVA_FILE,
    RESULT_NO_FILE_FOUND
} FindJavaFileResult;


JavaStat *javaStat;
JavaStat s_initJavaStat;


#define IsJavaReferenceType(m) (m==TypeStruct || m==TypeArray)

static int javaNotFqtUsageCorrection(Symbol *sym, int usage);


void fill_nestedSpec(S_nestedSpec *nestedSpec, struct symbol *cl,
                     char membFlag, short unsigned  accFlags) {
    nestedSpec->cl = cl;
    nestedSpec->membFlag = membFlag;
    nestedSpec->accFlags = accFlags;
}

void fillJavaStat(JavaStat *javaStat, IdList *className, TypeModifier *thisType, Symbol *thisClass,
                  int currentNestedIndex, char *currentPackage, char *unnamedPackagePath,
                  char *namedPackagePath, SymbolTable *locals, IdList *lastParsedName,
                  unsigned methodModifiers, CurrentlyParsedClassInfo parsingPositions, int classFileNumber,
                  JavaStat *next) {

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
    javaStat->classFileNumber = classFileNumber;
    javaStat->next = next;
}


char *javaCreateComposedName(char *prefix,
                             IdList *className,
                             int classNameSeparator,
                             char *name,
                             char *resultBuffer,
                             int resultBufferSize
) {
    int len, ll;
    bool sss;
    char *ln;
    char separator;
    IdList *ids;

    if (name == NULL)
        name = "";
    if (prefix == NULL)
        prefix = "";
    sss = false;
    len = 0;
    ll = strlen(prefix);
    if (ll!=0)
        sss = true;
    len += ll;
    for (ids=className; ids!=NULL; ids=ids->next) {
        if (sss)
            len ++;
        len += strlen(ids->fqtname);
        sss = true;
    }
    ll = strlen(name);
    if (sss && ll!=0)
        len++;
    len += ll;
    if (resultBuffer == NULL) {
        ln = stackMemoryAlloc(len+1);
    } else {
        assert(len < resultBufferSize);
        ln = resultBuffer;
    }
    ll = strlen(name);
    len -= ll;
    strcpy(ln+len,name);
    if (ll == 0)
        sss = false;
    else
        sss = true;
    separator = '.';
    for (ids=className; ids!=NULL; ids=ids->next) {
        if (sss) {
            len --;
            ln[len] = separator;
        }
        ll = strlen(ids->fqtname);
        len -= ll;
        strncpy(ln + len, ids->fqtname, ll);
        sss = true;
        assert(ids->nameType==TypeStruct || ids->nameType==TypePackage
                || ids->nameType==TypeExpression);
        if (ids->nameType==TypeStruct &&
                ids->next!=NULL && ids->next->nameType==TypeStruct) {
            separator = '$';
        } else {
            separator = classNameSeparator;
        }
    }
    ll = strlen(prefix);
    if (sss && ll!=0) {
        len --;
        ln[len] = separator;
    }
    len -= ll;
    strncpy(ln+len, prefix, ll);
    assert(len == 0);
    return ln;
}

void javaCheckForPrimaryStart(Position *cpos, Position *bpos) {
    if (options.mode != ServerMode) return;
    if (positionsAreEqual(cxRefPosition, *cpos)) {
        s_primaryStartPosition = *bpos;
    }
}

Position *javaGetNameStartingPosition(IdList *name) {
    IdList *ll;
    Position *res;
    res = &noPosition;
    for(ll=name; ll!=NULL; ll=ll->next) {
        res = &ll->id.position;
    }
    return res;
}

static Reference *javaAddClassCxReference(Symbol *dd, Position *pos, unsigned usage) {
    Reference *res;
    res = addCxReference(dd, pos, usage, NO_FILE_NUMBER, NO_FILE_NUMBER);
    return res;
}

static void javaAddNameCxReference(IdList *id, unsigned usage) {
    assert(id != NULL);
    char tmpString[MAX_SOURCE_PATH_SIZE];
    char *cname = javaCreateComposedName(NULL, id, '/', NULL, tmpString, sizeof(tmpString));

    Symbol dd = makeSymbol(id->id.name, cname, id->id.position);
    dd.type = id->nameType;
    dd.storage = StorageNone;

    /* if you do something else do attention on the union initialisation */
    addCxReference(&dd, &id->id.position, usage, NO_FILE_NUMBER, NO_FILE_NUMBER);
}

Symbol *javaAddType(IdList *class, Access access, Position *p) {
    Symbol *dd;
    dd = javaTypeSymbolDefinition(class, access, ADD_YES);
    dd->access = access;
    addCxReference(dd, p, UsageDefined, NO_FILE_NUMBER, NO_FILE_NUMBER);
    return dd;
}

// resName can be NULL!!!
static bool javaFindFile0(char *classPath, char *separator, char *name,
                          char *suffix, char **resultingName) {
    char fullName[MAX_FILE_NAME_SIZE];
    char *normalizedFileName;
    bool found = false;

    snprintf(fullName, MAX_FILE_NAME_SIZE, "%s%s%s%s", classPath, separator, name, suffix);
    assert(strlen(fullName)+1 < MAX_FILE_NAME_SIZE);

    normalizedFileName = normalizeFileName(fullName, cwd);
    log_trace("looking for file '%s'", normalizedFileName);
    if (editorFileExists(normalizedFileName)) {
        log_trace("found in buffer file '%s'", normalizedFileName);
        found = true;
    }
    if (found && resultingName!=NULL) {
        *resultingName = stackMemoryAlloc(strlen(normalizedFileName)+1);
        strcpy(*resultingName, normalizedFileName);
    }
    return found;
}

static int specialFileNameCasesCheck(char *fname) {
#ifdef __WIN32__
    WIN32_FIND_DATA		fdata;
    HANDLE				handle;
    int					dif;
    char				*ss;
    EditorBuffer		*buff;
    // first check if the file is from editor
    buff = getOpenedEditorBuffer(fname);
    if (buff != NULL)
        return true;
    // there is only drive name before the first slash, copy it.
    ss = lastOccurenceInString(fname, '/');
    if (ss==NULL)
        ss = lastOccurenceInString(fname, '\\');
    if (ss==NULL)
        return true;
    log_trace("translating %s", ttt);
    handle = FindFirstFile(fname, &fdata);
    if (handle == INVALID_HANDLE_VALUE)
        return true;
    dif = strcmp(ss+1, fdata.cFileName);
    FindClose(handle);
    log_trace("result %s", ttt);
    return dif==0;
#else
    return true;
#endif
}

/* TODO this function strangely ressembles to javaFindFile, join them ????*/
bool javaTypeFileExist(IdList *name) {
    char *fname;
    IdList tname;
    char tmpString[MAX_SOURCE_PATH_SIZE];

    if (name==NULL)
        return false;
    tname = *name;
    tname.nameType = TypeStruct;

    // first check if I have its class in file table
    // hmm this is causing problems in on-line editing when some misspelled
    // completion strings were added as types, then a package is resolved
    // as a type and a File from inside directory is not completed.
    // I try to solve it by requiring sourcefile index
    fname = javaCreateComposedName(":", &tname, '/', "class",
                                   tmpString, sizeof(tmpString));
    fname[1] = ZIP_SEPARATOR_CHAR;

    if (existsInFileTable(fname+1)) {
        int fileNumber = lookupFileTable(fname+1);
        if (getFileItem(fileNumber)->sourceFileNumber != NO_FILE_NUMBER) {
            return true;
        }
    }

    if (javaStat->unnamedPackagePath != NULL) {		/* unnamed package */
        fname = javaCreateComposedName(NULL, &tname, FILE_PATH_SEPARATOR, "java",
                                       tmpString, sizeof(tmpString));
        log_trace("testing existence of file '%s'", fname);
        if (editorFileExists(fname) && specialFileNameCasesCheck(fname))
            return true;
    }
    MapOverPaths(javaSourcePaths, {
        fname =
            javaCreateComposedName(currentPath, &tname, FILE_PATH_SEPARATOR, "java", tmpString, sizeof(tmpString));
        log_trace("testing existence of file '%s'", fname);
        if (editorFileExists(fname) && specialFileNameCasesCheck(fname))
            return true;
    });
    for (StringList *cp=javaClassPaths; cp!=NULL; cp=cp->next) {
        fname = javaCreateComposedName(cp->string, &tname, FILE_PATH_SEPARATOR, "class",
                                       tmpString, sizeof(tmpString));
        // hmm. do not need to check statb for .class files
        if (editorFileExists(fname) && specialFileNameCasesCheck(fname))
            return true;
    }
    // Archives...
    fname=javaCreateComposedName(NULL, &tname, '/', "class", tmpString, sizeof(tmpString));
    for (int i=0; i<MAX_JAVA_ZIP_ARCHIVES && zipArchiveTable[i].fn[0]!=0; i++) {
        if (fsIsMember(&zipArchiveTable[i].dir,fname,0,ADD_NO,NULL))
            return true;
    }
    // auto-inferred source-path
    if (javaStat->namedPackagePath != NULL) {
        fname = javaCreateComposedName(javaStat->namedPackagePath, &tname, FILE_PATH_SEPARATOR, "java",
                                       tmpString, sizeof(tmpString));
        if (editorFileExists(fname) && specialFileNameCasesCheck(fname))
            return true;
    }
    return false;
}

static bool javaFindClassFile(char *name, char **resultingName, time_t *modifiedTimeP) {
    struct stat stat;
    if (javaStat->unnamedPackagePath != NULL) {		/* unnamed package */
        if (javaFindFile0(javaStat->unnamedPackagePath, "/", name, ".class", resultingName)) {
            *modifiedTimeP = editorFileModificationTime(*resultingName);
            return true;
        }
    }
    // now other classpaths
    for (StringList *cp=javaClassPaths; cp!=NULL; cp=cp->next) {
        if (javaFindFile0(cp->string, "/", name, ".class", resultingName)) {
            *modifiedTimeP = editorFileModificationTime(*resultingName);
            return true;
        }
    }
    // finally look into java archives
    /* TODO: if it were not for this, we could skip the stat return value */
    for (int i=0; i<MAX_JAVA_ZIP_ARCHIVES && zipArchiveTable[i].fn[0]!=0; i++) {
        log_trace("Looking in '%s'", zipArchiveTable[i].fn);
        if (zipFindFile(name,resultingName,&zipArchiveTable[i])) {
            stat = zipArchiveTable[i].stat;
            *modifiedTimeP = stat.st_mtime;
            return true;
        }
    }
    return false;
}

static int javaFindSourceFile(char *name, char **resultingName) {
    StringList	*cp;

    if (javaStat->unnamedPackagePath != NULL) {		/* unnamed package */
        if (javaFindFile0(javaStat->unnamedPackagePath, "/", name, ".java", resultingName))
            return true;
    }
    // sourcepaths
    MapOverPaths(javaSourcePaths, {
        if (javaFindFile0(currentPath, "/", name, ".java", resultingName))
            return true;
    });
    // now other classpaths
    for (cp=javaClassPaths; cp!=NULL; cp=cp->next) {
        if (javaFindFile0(cp->string, "/", name, ".java", resultingName))
            return true;
    }
    // auto-inferred source-path
    if (javaStat->namedPackagePath != NULL) {
        if (javaFindFile0(javaStat->namedPackagePath, "/", name, ".java", resultingName))
            return true;
    }
    return false;
}

// if file exists, then set its name
static FindJavaFileResult javaFindFile(Symbol *classSymbol,
                                       char **sourceFileNameP,
                                       char **classFileNameP
) {
    int sourceIndex;
    bool classFound;
    bool sourceFound;
    char *linkName, *slname;

    log_trace("looking for Java file '%s'", classSymbol->linkName);
    linkName = slname = classSymbol->linkName;
    if (strchr(linkName, '$')!=NULL) {
        char innerName[MAX_FILE_NAME_SIZE];
        char *dollarPosition;
        // looking for an inner class, source must be included in outer file
        strcpy(innerName, classSymbol->linkName);
        slname = innerName;
        dollarPosition = strchr(innerName, '$');
        assert(dollarPosition);
        *dollarPosition = 0;
    }
    *classFileNameP = *sourceFileNameP = "";
    log_trace("!looking for %s.classf(in %s)== %d", linkName, slname, classSymbol->u.structSpec->classFileNumber);
    log_trace("!looking for %s %s", linkName, getFileItem(classSymbol->u.structSpec->classFileNumber)->name);
    sourceFound = javaFindSourceFile(slname, sourceFileNameP);
    assert(classSymbol->u.structSpec);
    FileItem *fileItem = getFileItem(classSymbol->u.structSpec->classFileNumber);
    sourceIndex = fileItem->sourceFileNumber;

    if (!sourceFound && sourceIndex!=-1 && sourceIndex!=NO_FILE_NUMBER) {
        // try the source indicated by source field of filetab
        FileItem *sourceFileItem = getFileItem(sourceIndex);
        log_trace("checking %s", sourceFileItem->name);
        sourceFound = javaFindFile0("", "", sourceFileItem->name, "", sourceFileNameP);
        log_trace("result %d %s", sourceFound, *sourceFileNameP);
    }

    /* We need to retain the modified time for class files since it can be inside an archive */
    time_t modifiedTime;
    classFound = javaFindClassFile(linkName, classFileNameP, &modifiedTime);
    if (!classFound)
        *classFileNameP = NULL;
    if (!sourceFound)
        *sourceFileNameP = NULL;

    log_trace("found source file: %s, found class file: %s)", sourceFound?"yes":"no", classFound?"yes":"no");
    if (!options.javaSlAllowed) {
        if (classFound)
            return RESULT_IS_CLASS_FILE;
        else
            return RESULT_NO_FILE_FOUND;
    }
    if (!classFound && !sourceFound)
        return RESULT_NO_FILE_FOUND;
    if (classFound && !sourceFound)
        return RESULT_IS_CLASS_FILE;
    if (!classFound && sourceFound)
        return RESULT_IS_JAVA_FILE;
    assert(sourceFound && classFound);

    if (fileModificationTime(*sourceFileNameP) > modifiedTime) {
        return RESULT_IS_JAVA_FILE;
    } else {
        return RESULT_IS_CLASS_FILE;
    }
}

static int javaFqtNameIsFromThePackage(char *cpack, char *classFqName) {
    char   *p1,*p2;

    for(p1=cpack, p2=classFqName; *p1 == *p2; p1++,p2++) ;
    if (*p1 != 0) return false;
    if (*p2 == 0) return false;
    //& if (*p2 != '/') return false;
    for(p2++; *p2; p2++) if (*p2 == '/') return false;
    return true;
}

int javaFqtNamesAreFromTheSamePackage(char *nn1, char *nn2) {
    char   *p1,*p2;

    if (nn1==NULL || nn2==NULL) return false;
//&fprintf(dumpOut,"checking equal package %s %s\n", nn1, nn2);
    for(p1=nn1, p2=nn2; *p1 == *p2 && *p1 && *p2; p1++,p2++) ;
    for(; *p1; p1++) if (*p1 == '/') return false;
    for(; *p2; p2++) if (*p2 == '/') return false;
//*fprintf(dumpOut,"YES EQUALS\n");
    return true;
}

int javaClassIsInCurrentPackage(Symbol *cl) {
    if (s_jsl!=NULL) {
        if (s_jsl->classStat->thisClass == NULL) {
            // probably import class.*; (nested subclasses)
            return javaFqtNameIsFromThePackage(s_jsl->classStat->thisPackage,
                                               cl->linkName);
        } else {
            return javaFqtNamesAreFromTheSamePackage(cl->linkName,
                                                     s_jsl->classStat->thisClass->linkName);
        }
    } else {
        if (javaStat->thisClass == NULL) {
            // probably import class.*; (nested subclasses)
            return javaFqtNameIsFromThePackage(javaStat->currentPackage,
                                               cl->linkName);
        } else {
            return javaFqtNamesAreFromTheSamePackage(cl->linkName,
                                                     javaStat->thisClass->linkName);
        }
    }
}

static Symbol *javaFQTypeSymbolDefinitionCreate(char *name, char *fqName) {
    Symbol *memb;
    SymbolList *pppl;
    char *lname1, *sname;

    sname = cfAllocc(strlen(name)+1, char);
    strcpy(sname, name);

    lname1 = cfAllocc(strlen(fqName)+1, char);
    strcpy(lname1, fqName);

    memb = cfAlloc(Symbol);
    fillSymbol(memb, sname, lname1, noPosition);

    memb->type = TypeStruct;
    memb->storage = StorageNone;

    memb->u.structSpec = cfAlloc(S_symStructSpec);

    initSymStructSpec(memb->u.structSpec, /*.records=*/NULL);
    TypeModifier *type = &memb->u.structSpec->type;
    /* Assumed to be Struct/Union/Enum? */
    initTypeModifierAsStructUnionOrEnum(type, /*.kind=*/TypeStruct, /*.u.t=*/memb,
                                            /*.typedefSymbol=*/NULL, /*.next=*/NULL);
    TypeModifier *ptrtype = &memb->u.structSpec->ptrtype;
    initTypeModifierAsPointer(ptrtype, &memb->u.structSpec->type);

    pppl = cfAlloc(SymbolList);
    /* REPLACED: FILL_symbolList(pppl, memb, NULL); with compound literal */
    *pppl = (SymbolList){.element = memb, .next = NULL};

    javaFqtTableAdd(&javaFqtTable, pppl);

    // I think this can be there, as it is very used
    javaCreateClassFileItem(memb);
    // this would be too strong, javaLoadClassSymbolsFromFile(memb);
    /* so, this table is not freed when freeing FrameAllocations */
    //&if (stringContainsSubstring(fqName, "ComboBoxTreeFilter")) {fprintf(dumpOut,"\nAAAAAAAAAAAAA : %s %s\n\n", name, fqName);} if (strcmp(fqName, "ComboBoxTreeFilter")==0) assert(0);
    return memb;
}

Symbol *javaFQTypeSymbolDefinition(char *name, char *fqName) {
    Symbol symbol, *member;
    SymbolList ppl, *pppl;

    /* This probably creates a SymbolList element so ..IsMember() can be used */
    /* TODO: SymbolList is here used as a FQT, create that type... */
    fillSymbol(&symbol, name, fqName, noPosition);
    symbol.type = TypeStruct;
    symbol.storage = StorageNone;

    /* REPLACED: FILL_symbolList(&ppl, &symbol, NULL); with compound literal */
    ppl = (SymbolList){.element = &symbol, .next = NULL};

    if (javaFqtTableIsMember(&javaFqtTable, &ppl, NULL, &pppl)) {
        member = pppl->element;
    } else {
        member = javaFQTypeSymbolDefinitionCreate(name, fqName);
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
    return memb;
}


static Symbol *javaAddTypeToSymbolTable(Symbol *original, int accessFlags, Position *importPos,
                                        bool isExplicitlyImported
) {
    Symbol *added;

    original->access = accessFlags;
    original->isExplicitlyImported = isExplicitlyImported;

    added = newSymbolAsCopyOf(original);
    added->pos = *importPos;
    addSymbolToFrame(symbolTable, added);

    return original;
}

Symbol *javaTypeSymbolDefinition(IdList *tname,
                                 int accessFlags,
                                 int addType
){
    Symbol symbol, *typeSymbol;
    char fqtName[MAX_FILE_NAME_SIZE];

    assert(tname);
    assert(tname->nameType == TypeStruct);

    fillSymbol(&symbol, tname->id.name, tname->id.name, noPosition);
    symbol.access = accessFlags;
    symbol.type = TypeStruct;
    symbol.storage = StorageNone;

    javaCreateComposedName(NULL, tname, '/', NULL, fqtName, MAX_FILE_NAME_SIZE);
    typeSymbol = javaFQTypeSymbolDefinition(tname->id.name, fqtName);
    if (addType == ADD_YES) {
        typeSymbol = javaAddTypeToSymbolTable(typeSymbol, accessFlags, &noPosition, false);
    }
    return typeSymbol;
}

Symbol *javaTypeSymbolUsage(IdList *tname, int accessFlags) {
    Symbol symbol, *member;
    char fqtName[MAX_FILE_NAME_SIZE];

    assert(tname);
    assert(tname->nameType == TypeStruct);

    fillSymbol(&symbol, tname->id.name, tname->id.name, noPosition);
    symbol.access = accessFlags;
    symbol.type = TypeStruct;
    symbol.storage = StorageNone;

    if (tname->next==NULL && symbolTableIsMember(symbolTable, &symbol, NULL, &member)) {
        // get canonical copy
        member = javaFQTypeSymbolDefinition(member->name, member->linkName);
        return member;
    }
    javaCreateComposedName(NULL,tname,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    member = javaFQTypeSymbolDefinition(tname->id.name, fqtName);
    return member;
}

Symbol *javaTypeNameDefinition(IdList *tname) {
    Symbol    *memb;
    Symbol		*dd;
    TypeModifier		*td;

    memb = javaTypeSymbolUsage(tname, AccessDefault);
    td = newStructTypeModifier(memb);
    dd = newSymbolAsType(memb->name, memb->linkName, tname->id.position, td);

    return dd;
}

static void javaJslLoadSuperClasses(Symbol *cc, int currentParsedFile) {
    SymbolList *ss;
    static int nestingCount = 0;

    nestingCount ++;
    if (nestingCount > MAX_CLASSES) {
        FATAL_ERROR(ERR_INTERNAL, "unexpected cycle in class hierarchy", XREF_EXIT_ERR);
    }
    for(ss=cc->u.structSpec->super; ss!=NULL; ss=ss->next) {
        cfAddCastsToModule(cc, ss->element);
    }
    nestingCount --;
}

void javaReadSymbolFromSourceFileInit(int sourceFileNum,
                                      JslTypeTab *typeTab ) {
    S_jslStat           *njsl;
    char				*yyg;
    int					yygsize;
    njsl = stackMemoryAlloc(sizeof(S_jslStat));
    // very space consuming !!!, it takes about 400kb of working memory
    // TODO!!!! to allocate and save only used parts of 'gyyvs - gyyvsp'
    // and 'gyyss - gyyssp' ??? And copying twice? definitely yes!
    //yygsize = sizeof(struct yyGlobalState);
    yygsize = ((char*)(s_yygstate->gyyvsp+1)) - ((char *)s_yygstate);
    yyg = stackMemoryAlloc(yygsize);
    fillJslStat(njsl, 0, sourceFileNum, currentLanguage, typeTab, NULL, NULL,
                 uniyylval, (S_yyGlobalState*)yyg, yygsize, s_jsl);
    memcpy(njsl->savedYYstate, s_yygstate, yygsize);
    memcpy(njsl->yyIdentBuf, yyIdBuffer,
           sizeof(Id[YYIDBUFFER_SIZE]));
    s_jsl = njsl;
    currentLanguage = LANG_JAVA;
}

void javaReadSymbolFromSourceFileEnd(void) {
    currentLanguage = s_jsl->language;
    uniyylval = s_jsl->savedyylval;
    memcpy(s_yygstate, s_jsl->savedYYstate, s_jsl->yyStateSize);
    memcpy(yyIdBuffer, s_jsl->yyIdentBuf,
           sizeof(Id[YYIDBUFFER_SIZE]));
    s_jsl = s_jsl->next;
}

void javaReadSymbolsFromSourceFileNoFreeing(char *fname, char *asfname) {
    FILE *file;
    EditorBuffer *buffer;
    int cfilenum;
    static int nestingDepth = 0;

    nestingDepth++;

    // ?? is this really necessary?
    // memset(s_yygstate, sizeof(struct yyGlobalState), 0);
    uniyylval = & s_yygstate->gyylval;

    for (int pass=1; pass<=2; pass++) {
        log_debug("[jsl] PASS %d through %s level %d", pass, fname, nestingDepth);
        file = NULL;
        buffer = findEditorBufferForFile(fname);
        if (buffer==NULL) {
            file = openFile(fname, "r");
            if (file==NULL) {
                errorMessage(ERR_CANT_OPEN, fname);
                goto fini;
            }
        }
        pushInclude(file, buffer, asfname, "\n");
        cfilenum    = currentFile.characterBuffer.fileNumber;
        s_jsl->pass = pass;
        //java_yyparse();
        popInclude();      // this will close the file
        log_debug("[jsl] CLOSE file %s level %d", fname, nestingDepth);
    }

    for (SymbolList *ll=s_jsl->waitList; ll!=NULL; ll=ll->next) {
        javaJslLoadSuperClasses(ll->element, cfilenum);
    }
 fini:
    nestingDepth--;
}

void javaReadSymbolsFromSourceFile(char *fname) {
    JslTypeTab    *typeTab;
    int				fileNumber;
    int				memBalance;

    fileNumber = addFileNameToFileTable(fname);
    memBalance = currentBlock->firstFreeIndex;
    beginBlock();
    typeTab = stackMemoryAlloc(sizeof(JslTypeTab));
    javaReadSymbolFromSourceFileInit(fileNumber, typeTab);
    jslTypeTabInit(typeTab, MAX_JSL_SYMBOLS);
    javaReadSymbolsFromSourceFileNoFreeing(fname, fname);
    // there may be several unbalanced blocks
    while (memBalance < currentBlock->firstFreeIndex)
        endBlock();
    javaReadSymbolFromSourceFileEnd();
}

static void addJavaFileDependency(int file, char *onfile) {
    int         fileNumber;
    Position	pos;

    // do dependencies only when doing cross reference file
    if (options.mode != XrefMode)
        return;
    // also do it only for source files
    if (!getFileItem(file)->isArgument)
        return;
    fileNumber = addFileNameToFileTable(onfile);
    pos = makePosition(file, 0, 0);
    addIncludeReference(fileNumber, &pos);
}


static void javaHackCopySourceLoadedCopyPars(Symbol *memb) {
    Symbol *cl;
    cl = javaFQTypeSymbolDefinition(memb->name, memb->linkName);
    if (cl->javaSourceIsLoaded) {
        memb->access = cl->access;
        memb->storage = cl->storage;
        memb->type = cl->type;
        memb->javaSourceIsLoaded = cl->javaSourceIsLoaded;
    }
}

void javaLoadClassSymbolsFromFile(Symbol *memb) {
    char *sourceName, *className;
    Symbol *cl;
    int cfi, cInd;
    FindJavaFileResult findResult;

    if (memb == NULL)
        return;

    log_trace("!requesting class (%d)%s", memb, memb->linkName);
    sourceName = className = "";
    if (!memb->javaClassIsLoaded) {
        memb->javaClassIsLoaded = true;
        // following is a hack due to multiple items in symbol tab !!!
        cl = javaFQTypeSymbolDefinition(memb->name, memb->linkName);
        if (cl!=NULL && cl!=memb)
            cl->javaClassIsLoaded = true;
        cInd = javaCreateClassFileItem(memb);
        addCfClassTreeHierarchyRef(cInd, UsageClassFileDefinition);
        findResult = javaFindFile(memb, &sourceName, &className);
        if (findResult == RESULT_IS_JAVA_FILE) {
            assert(memb->u.structSpec);
            cfi = memb->u.structSpec->classFileNumber;
            FileItem *cfiFileItem = getFileItem(cfi);
            // set it to none, if class is inside jslparsing  will re-set it
            cfiFileItem->sourceFileNumber=NO_FILE_NUMBER;
            javaReadSymbolsFromSourceFile(sourceName);
            if (cfiFileItem->sourceFileNumber == NO_FILE_NUMBER) {
                // class definition not found in the source file,
                // (moved inner class) retry searching for class file
                findResult = javaFindFile(memb, &sourceName, &className);
            }
            if (!memb->javaSourceIsLoaded) {
                // HACK, probably loaded into another possition of symboltab, make copy
                javaHackCopySourceLoadedCopyPars(memb);
            }
        }
        if (findResult == RESULT_IS_CLASS_FILE) {
            javaReadClassFile(className,memb, LOAD_SUPER);
        } else if (findResult == RESULT_NO_FILE_FOUND) {
            if (displayingErrorMessages()) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "class %s not found", memb->name);
                errorMessage(ERR_ST, tmpBuff);
            }
        }
        // I need to get also accessflags for example
        if (cl!=NULL && cl!=memb) {
            cl->type = memb->type;
            cl->access = memb->access;
            cl->storage = memb->storage;
        }
        if (sourceName != NULL) {
            addJavaFileDependency(olOriginalFileNumber, sourceName);
        }
    }
}


static Result findTopLevelNameInternal(char *name,
                                    S_recFindStr *resRfs,
                                    Symbol **resultingMemberP,
                                    int classif,
                                    JavaStat *startingScope,
                                    int accessibilityCheck,
                                    int visibilityCheck,
                                    JavaStat **rscope
) {
    Result result;
    Symbol symbol;
    JavaStat *cscope;

    assert((!LANGUAGE(LANG_JAVA)) ||
        (classif==CLASS_TO_EXPR || classif==CLASS_TO_METHOD));
    assert(accessibilityCheck==ACCESSIBILITY_CHECK_YES || accessibilityCheck==ACCESSIBILITY_CHECK_NO);
    assert(visibilityCheck==VISIBILITY_CHECK_YES || visibilityCheck==VISIBILITY_CHECK_NO);

    fillSymbol(&symbol, name, name, noPosition);
    symbol.storage = StorageNone;

    result = RESULT_NOT_FOUND;
    for(cscope=startingScope;
        cscope!=NULL && cscope->thisClass!=NULL && result!=RESULT_OK;
        cscope=cscope->next
    ) {
        assert(cscope->thisClass);
        if (classif!=CLASS_TO_METHOD && symbolTableIsMember(cscope->locals, &symbol, NULL, resultingMemberP)) {
            /* it is an argument or local variable */
            /* this is tricky */
            /* I guess, you cannot have an overloaded function here, so ... */
            log_trace("%s is identified as local var or parameter", name);
            fillRecFindStr(resRfs, NULL, NULL, *resultingMemberP, s_recFindCl++);
            *rscope = NULL;
        } else {
            /* if present, then as a structure record */
            log_trace("putting %s as base class", cscope->thisClass->name);
            fillRecFindStr(resRfs, cscope->thisClass, NULL, NULL,s_recFindCl++);
            recFindPush(cscope->thisClass, resRfs);
            *rscope = cscope;
        }
        result = findStrRecordSym(resultingMemberP, resRfs, name, classif, accessibilityCheck, visibilityCheck);
    }
    return result;
}

Result findTopLevelName(char *name, S_recFindStr *resRfs,
                     Symbol **resMemb, int classif) {
    Result result;
    JavaStat *scopeToSearch, *resultScope;

    result = findTopLevelNameInternal(name, resRfs, resMemb, classif,
                                   javaStat, ACCESSIBILITY_CHECK_YES, VISIBILITY_CHECK_YES,
                                   &scopeToSearch);
    // O.K. determine class to search
    if (result != RESULT_OK) {
        // no class to search find, anyway this is a compiler error,
        scopeToSearch = javaStat;
    }
    if (scopeToSearch!=NULL) {
        log_trace("relooking for %s in %s", name, scopeToSearch->thisClass->name);
        result = findTopLevelNameInternal(name, resRfs, resMemb, classif,
                                       scopeToSearch, ACCESSIBILITY_CHECK_NO, VISIBILITY_CHECK_NO,
                                       &resultScope);
    }
    return result;
}

static int javaIsNestedClass(Symbol *tclas, char *name, Symbol **innmemb) {
    int    i,n;
    S_nestedSpec	*inners;
    Symbol        *clas;

    // take just one super class, no interfaces, for speed reasons
    for(clas=tclas;
        clas!=NULL && clas->u.structSpec->super!=NULL;
        clas=clas->u.structSpec->super->element) {
        assert(clas->type == TypeStruct && clas->u.structSpec);
        n = clas->u.structSpec->nestedCount;
        inners = clas->u.structSpec->nestedClasses;
        for(i=0; i<n; i++) {
//& fprintf(dumpOut,"checking %s<->%s\n",inners[i].cl->name, name);fflush(dumpOut);
            if (inners[i].membFlag && strcmp(inners[i].cl->name, name)==0) {
                // the following if makes problem, because when resolving
                // a name Outer.Inner I do not care whether it is static
                // or not. Why I have put this there????
//&				if ((inners[i].cl->access&AccessStatic)==0) {
                    *innmemb = inners[i].cl;
                    return true;
//&				}
            }
        }
    }
    return false;
}

static int javaClassIsInnerNonStaticMemberClass(Symbol *tclas, Symbol *name) {
    int    i,n;
    S_nestedSpec	*inners;
    Symbol        *clas;

    // take just one super class, no interfaces, for speed reasons
    for(clas=tclas;
        clas!=NULL && clas->u.structSpec->super!=NULL;
        clas=clas->u.structSpec->super->element) {
        assert(clas->type == TypeStruct && clas->u.structSpec);
        n = clas->u.structSpec->nestedCount;
        inners = clas->u.structSpec->nestedClasses;
        for(i=0; i<n; i++) {
//&fprintf(dumpOut,"checking %s<->%s\n",inners[i].cl->name, name);fflush(dumpOut);
            if (inners[i].membFlag && strcmp(inners[i].cl->linkName, name->linkName)==0
                && (inners[i].cl->access & AccessStatic) == 0) {
                return true;
            }
        }
    }
    return false;
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
            return true;
        }
    }
    return false;
}
#endif

int javaIsInnerAndCanGetUnnamedEnclosingInstance(Symbol *name, Symbol **outEi) {
    JavaStat		*cscope;
//&fprintf(dumpOut,"looking for inner class %s\n",name);
    for(	cscope=javaStat;
            cscope!=NULL && cscope->thisClass!=NULL;
            cscope=cscope->next
        ) {
        if (javaClassIsInnerNonStaticMemberClass(cscope->thisClass, name)) {
//&fprintf(dumpOut,"inner class %s found %s %s\n",name,(*innmemb)->name, (*innmemb)->linkName);fflush(dumpOut);
            *outEi = cscope->thisClass;
            return true;
        }
    }
    return false;
}


int javaClassifySingleAmbigNameToTypeOrPack(IdList *name,
                                            Symbol **str,
                                            IncludeCxrefs cxrefFlag
){
    Symbol symbol, *mm, *member, *nextmemb;
    bool haveit;
    Position *ipos;

    fillSymbol(&symbol, name->id.name, name->id.name, noPosition);
    symbol.type = TypeStruct;
    symbol.storage = StorageNone;

    haveit = false;
    if (symbolTableIsMember(symbolTable, &symbol, NULL, &member)) {
        /* a type */
        assert(member);
        // O.K. I have to load the class in order to check its access flags
        for(; member!=NULL; member=nextmemb) {
            nextmemb = member;
            symbolTableNextMember(&symbol, &nextmemb);
            // take canonical copy (as there can be more than one class
            // item in symtab
            mm = javaFQTypeSymbolDefinition(member->name, member->linkName);
            if ((options.ooChecksBits & OOC_ALL_CHECKS)) {
                // do carefully all checks
                if (! haveit) {
                    javaLoadClassSymbolsFromFile(mm);
                    if (javaOuterClassAccessible(mm)) {
                        //JAVA_CLASS_CAN_HAVE_IT(name, str, mm, member, haveit);
                        haveit         = true;
                        *str           = mm;
                        name->nameType = TypeStruct;
                        name->fqtname  = mm->linkName;
                        if (cxrefFlag == ADD_CX_REFS) {
                            ipos = &member->pos; /* here MUST be memb, not mm, as it contains the import line !!*/
                            if (ipos->file != NO_FILE_NUMBER && ipos->file != -1) {
                                javaAddImportConstructionReference(ipos, ipos, UsageUsed);
                            }
                        }
                    }
                } else {
                    if ((*str) != mm) {
                        javaLoadClassSymbolsFromFile(mm);
                        if (javaOuterClassAccessible(mm)) {
                            // multiple imports
                            if ((*str)->isExplicitlyImported == mm->isExplicitlyImported) {
                                assert(strcmp((*str)->linkName, mm->linkName)!=0);
                                haveit = false;
                                // this is tricky, mark the import as used
                                // it is "used" to disqualify some references, so
                                // during name reduction refactoring, it will not adding
                                // such import
                                ipos = &member->pos;
                                if (ipos->file != NO_FILE_NUMBER && ipos->file != -1) {
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
                    // JAVA_CLASS_CAN_HAVE_IT(name, str, mm, member, haveit);
                    haveit         = true;
                    *str           = mm;
                    name->nameType = TypeStruct;
                    name->fqtname  = mm->linkName;
                    if (cxrefFlag == ADD_CX_REFS) {
                        ipos = &member->pos; /* here MUST be memb, not mm, as it contains the import line !!*/
                        if (ipos->file != NO_FILE_NUMBER && ipos->file != -1) {
                            javaAddImportConstructionReference(ipos, ipos, UsageUsed);
                        }
                    }
                    goto breakcycle;
                } else {
                    // O.K. there may be an ambiguity resolved by accessibility
                    javaLoadClassSymbolsFromFile(mm);
                    if (javaOuterClassAccessible(mm)) {
                        //JAVA_CLASS_CAN_HAVE_IT(name, str, mm, member, haveit);
                        haveit         = true;
                        *str           = mm;
                        name->nameType = TypeStruct;
                        name->fqtname  = mm->linkName;
                        if (cxrefFlag == ADD_CX_REFS) {
                            ipos = &member->pos; /* here MUST be memb, not mm, as it contains the import line !!*/
                            if (ipos->file != NO_FILE_NUMBER && ipos->file != -1) {
                                javaAddImportConstructionReference(ipos, ipos, UsageUsed);
                            }
                        }
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

static void addAmbCxRef(int classif, Symbol *sym, Position *pos, UsageKind usageKind, int minacc, Reference **oref, S_recFindStr *rfs) {
    Usage usage;
    if (classif!=CLASS_TO_METHOD) {
        if (rfs != NULL && rfs->currentClass != NULL) {
            assert(rfs && rfs->currentClass && rfs->currentClass->type == TypeStruct &&
                   rfs->currentClass->u.structSpec);
            assert(rfs && rfs->baseClass &&
                   rfs->baseClass->type==TypeStruct && rfs->baseClass->u.structSpec);
            if (options.serverOperation != OLO_ENCAPSULATE ||
                !javaRecordAccessible(rfs, rfs->baseClass, rfs->currentClass, sym, AccessPrivate)) {
                fillUsage(&usage, usageKind, minacc);
                *oref = addNewCxReference(sym, pos, usage, rfs->currentClass->u.structSpec->classFileNumber,
                                          rfs->baseClass->u.structSpec->classFileNumber);
            }
        } else {
            *oref=addCxReference(sym, pos, usageKind, NO_FILE_NUMBER, NO_FILE_NUMBER);
        }
    }
}

char *javaImportSymbolName_st(int file, int line, int coll) {
    static char res[MAX_CX_SYMBOL_SIZE];
    sprintf(res, "%s:%x:%x:%x%cimport on line %3d", LINK_NAME_IMPORT_STATEMENT,
            file, line, coll,
            LINK_NAME_SEPARATOR,
            /*& simpleFileName(getRealFileName_static(getFileItem(file)->name)), &*/
            line);
    return res;
}

void javaAddImportConstructionReference(Position *importPos, Position *pos, int usage) {
    char *isymName;

    isymName = javaImportSymbolName_st(importPos->file, importPos->line, importPos->col);
    log_trace("using import on %s:%d (%d) at %s:%d\n", simpleFileName(getFileItem(importPos->file)->name),
              importPos->line, importPos->col, simpleFileName(getFileItem(pos->file)->name), pos->line);
    addSpecialFieldReference(isymName, StorageDefault, NO_FILE_NUMBER, pos, usage);
}

static int javaClassifySingleAmbigName(IdList *name,
                                       S_recFindStr *rfs,
                                       Symbol **str,
                                       TypeModifier **expr,
                                       Reference **oref,
                                       int classif, int uusage,
                                       IncludeCxrefs cxrefFlag
) {
    int res, nfqtusage, minacc;
    S_recFindStr *nullRfs = NULL;

    if (classif==CLASS_TO_EXPR || classif==CLASS_TO_METHOD) {
        /* argument, local variable or class record */
        if (findTopLevelName(name->id.name,rfs,str,classif)==RESULT_OK) {
            *expr = (*str)->u.typeModifier;
            name->nameType = TypeExpression;
            if (cxrefFlag==ADD_CX_REFS) {
                minacc = javaGetMinimalAccessibility(rfs, *str);
                addAmbCxRef(classif,*str, &name->id.position, uusage, minacc, oref, rfs);
                if (rfs != NULL && rfs->currentClass != NULL) {
                    // the question is: is a reference to static field
                    // also reference to 'this'? If yes, it will
                    // prevent many methods from beeing turned static.
                    if ((*str)->access & AccessStatic) {
                        nfqtusage = javaNotFqtUsageCorrection(rfs->currentClass, UsageNotFQField);
                        addSpecialFieldReference(LINK_NAME_NOT_FQT_ITEM, StorageField,
                                                 rfs->currentClass->u.structSpec->classFileNumber,
                                                 &name->id.position, nfqtusage);
                    } else {
                        addThisCxReferences(rfs->baseClass->u.structSpec->classFileNumber, &name->id.position);
                    }
                }
            }
            return name->nameType;
        }
    }
    res = javaClassifySingleAmbigNameToTypeOrPack(name, str, cxrefFlag);
    if (res == TypeStruct) {
        if (cxrefFlag==ADD_CX_REFS) {
            addAmbCxRef(classif, *str, &name->id.position, uusage, MIN_REQUIRED_ACCESS, oref, nullRfs);
            // the problem is here when invoked as nested "new Name()"?
            nfqtusage = javaNotFqtUsageCorrection((*str), UsageNotFQType);
            addSpecialFieldReference(LINK_NAME_NOT_FQT_ITEM, StorageField,
                                     (*str)->u.structSpec->classFileNumber,
                                     &name->id.position, nfqtusage);
        }
    } else if (res == TypePackage) {
        if (cxrefFlag==ADD_CX_REFS) {
            javaAddNameCxReference(name, uusage);
        }
    } else {
        // Well, I do not know, putting the assert there just to learn
        assert(0);
    }
    return name->nameType;
}

static int javaNotFqtUsageCorrection(Symbol *sym, int usage) {
    int             rr,pplen;
    S_recFindStr    localRfs;
    Symbol        *str;
    TypeModifier *expr;
    Reference     *loref;
    IdList   sname;
    char            *pp, packname[TMP_STRING_SIZE];

    pp = strchr(sym->linkName, '/');
    if (pp==NULL) pp = sym->linkName;
    pplen = pp - sym->linkName;
    assert(pplen < TMP_STRING_SIZE-1);
    strncpy(packname, sym->linkName, pplen);
    packname[pplen] = 0;

    fillfIdList(&sname, packname, NULL, noPosition, packname, TypeExpression, NULL);
        rr = javaClassifySingleAmbigName(&sname,&localRfs,&str,&expr,&loref,
                                         CLASS_TO_EXPR, UsageNone, NO_CX_REFS);
    if (rr != TypePackage) {
        return UsageNonExpandableNotFQTName;
    }
    return usage;
}

static void javaResetUselessReference(Reference *ref) {
    if (ref != NULL /*& && ref->usage.kind == UsageLastUseless &*/) {
        ref->usage.kind = UsageOtherUseless;
    }
}

/*
 check whether name as single name will result to str, if yes add
 UsageUselessFqt reference
*/
static void javaCheckForUselessFqt(IdList *name, int classif, Symbol *rstr,
                                   Reference **oref, Reference *lref){
    int             rr;
    bool uselessFqt = false;
    S_recFindStr    localRfs;
    Symbol        *str;
    TypeModifier *expr;
    Reference     *loref;
    IdList   sname;

    //& for(nn=name; nn!=NULL && nn->nameType!=TypePackage; nn=nn->next) ;
    //& if (nn==NULL) return;
    // if it is not fqt (i.e. does not contain package), do nothing
    // TODO, this must be optional !!!!

    sname = *name;
    sname.next = NULL;
    rr = javaClassifySingleAmbigName(&sname,&localRfs,&str,&expr,&loref,
                                      classif, UsageNone, NO_CX_REFS);
    if (rr==TypeStruct) {
        assert(str && rstr);
        log_trace("!checking %s == %s  (%d==%d)", str->linkName, rstr->linkName,
                  str->u.structSpec->classFileNumber, rstr->u.structSpec->classFileNumber);
        // equality of pointers may be too strong ???
        // what about classfile index comparing
        //if (strcmp(str->linkName, rstr->linkName) == 0) {
        assert(str->u.structSpec!=NULL && rstr->u.structSpec!=NULL);
        if (str->u.structSpec->classFileNumber == rstr->u.structSpec->classFileNumber) {
            assert(name && rstr->u.structSpec);
            *oref = addUselessFQTReference(rstr->u.structSpec->classFileNumber,&name->id.position);
            log_trace("!adding TYPE useless reference on %d,%d", name->id.position.line, name->id.position.col);
            javaResetUselessReference(lref);
            uselessFqt = true;
        }
    }
    if (!uselessFqt) {
        assert(name->next != NULL);			// it is long name
        assert(name && rstr->u.structSpec);
        addUnimportedTypeLongReference(rstr->u.structSpec->classFileNumber,&name->id.position);
    }
}

static Reference *javaCheckForUselessTypeName(IdList   *name,
                                                int				classif,
                                                S_recFindStr	*rfs,
                                                Reference     **oref,
                                                Reference     *lref
    ) {
    int             rr;
    S_recFindStr    localRfs;
    Symbol        *str;
    TypeModifier *expr;
    Reference     *loref;
    IdList   sname;
    Reference     *res;

    res = NULL;

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
    assert(rfs && rfs->currentClass && rfs->currentClass->u.structSpec && name);
    //& fprintf(dumpOut,"!checking rr == %s, %x\n", typeName[rr], localRfs.currentClass);
    if (rr == TypeExpression && localRfs.currentClass != NULL) {
        //&fprintf(dumpOut,"!checking %d(%s) ==
        //%d(%s)\n",rfs->currentClass->u.structSpec->classFileNumber,rfs->currentClass->linkName,localRfs.currentClass->u.structSpec->classFileNumber,localRfs.currentClass->linkName);
        // equality of pointers may be too strong ???
        if (rfs->currentClass->u.structSpec->classFileNumber ==
            localRfs.currentClass->u.structSpec->classFileNumber) {
            *oref = addUselessFQTReference(rfs->currentClass->u.structSpec->classFileNumber, &name->id.position);
            //&fprintf(dumpOut,"!adding useless reference on %d,%d\n", name->id.position.line,
            //name->id.position.col);
            javaResetUselessReference(lref);
        }
    }
    return res;
}

static void javaClassifyNameToNestedType(IdList *name, Symbol *outerc, int uusage, Symbol **str, Reference **oref) {
    name->nameType = TypeStruct;
    *str = javaTypeSymbolUsage(name,AccessDefault);
    javaLoadClassSymbolsFromFile(*str);
    // you have to search the class, it may come from superclass

    if ((options.ooChecksBits & OOC_ALL_CHECKS)==0
        || javaRecordAccessible(NULL,outerc, outerc, *str, (*str)->access)) {
        *oref = javaAddClassCxReference(*str, &name->id.position, uusage);
    }
}

static void classifiedToNestedClass(IdList *name, Symbol **str, Reference **oref, Reference **rdtoref, int classif, int uusage, Symbol *pstr, Reference *prdtoref, int allowUselesFqtRefs) {
    name->nameType = TypeStruct;
    //&*str=javaTypeSymbolUsage(name,AccessDefault);
    javaLoadClassSymbolsFromFile(*str);
    if ((options.ooChecksBits & OOC_ALL_CHECKS)==0
        || javaRecordAccessible(NULL,pstr, pstr, *str, (*str)->access)) {
        *oref = javaAddClassCxReference(*str, &name->id.position, uusage);
        if (allowUselesFqtRefs == USELESS_FQT_REFS_ALLOWED) {
            javaCheckForUselessFqt(name, classif, *str, rdtoref, prdtoref);
        }
    }
}


Type javaClassifyAmbiguousName(
        IdList *name,
        S_recFindStr *rfs,	// can be NULL
        Symbol **str,
        TypeModifier **expr,
        Reference **oref,
        Reference **rdtoref,  // output last useless reference, can be NULL
        int allowUselesFqtRefs,
        int classif,
        int usage) {
    int			pres,classif2;
    int			uusage, minacc;
    Symbol    *pstr;
    S_recFindStr localRfs;
    TypeModifier     *pexpr;
    Reference			*poref, *localrdref, *prdtoref;

    assert(classif==CLASS_TO_TYPE || classif==CLASS_TO_EXPR ||
           classif==CLASS_TO_METHOD);
    uusage = usage;
    if (uusage==UsageUsed && nestingLevel() == 0)
        uusage = USAGE_TOP_LEVEL_USED;
    if (rfs == NULL)
        rfs = &localRfs;
    if (rdtoref == NULL)
        rdtoref = &localrdref;
    *oref = NULL;
    *rdtoref = NULL;
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
                *str = javaTypeSymbolUsage(name, AccessDefault);
                if (allowUselesFqtRefs == USELESS_FQT_REFS_ALLOWED) {
                    javaCheckForUselessFqt(name, classif, *str, rdtoref, prdtoref);
                }
                javaLoadClassSymbolsFromFile(*str);
                if ((options.ooChecksBits & OOC_ALL_CHECKS)==0
                    || javaOuterClassAccessible(*str)) {
                    *oref = javaAddClassCxReference(*str, &name->id.position, uusage);
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
                Result rf = findStrRecordSym(str, iniFind(pstr,rfs), name->id.name,
                                      classif, ACCESSIBILITY_CHECK_NO, VISIBILITY_CHECK_NO);
                *expr = (*str)->u.typeModifier;
                if (rf == RESULT_OK) {
                    name->nameType = TypeExpression;
                    if ((options.ooChecksBits & OOC_ALL_CHECKS) == 0 ||
                        javaRecordVisibleAndAccessible(rfs, rfs->baseClass, rfs->currentClass, *str)) {
                        minacc = javaGetMinimalAccessibility(rfs, *str);
                        addAmbCxRef(classif,*str,&name->id.position, uusage, minacc, oref, rfs);
                        if (allowUselesFqtRefs == USELESS_FQT_REFS_ALLOWED) {
                            javaCheckForUselessTypeName(name, classif, rfs,
                                                        rdtoref, prdtoref);
                        }
                    }
                } else {
                    if (javaIsNestedClass(pstr,name->id.name,str)) {
                        classifiedToNestedClass(name, str, oref, rdtoref, classif, uusage, pstr, prdtoref, allowUselesFqtRefs);
                    } else {		/* error, no such field found */
                        name->nameType = TypeExpression;
                        noSuchFieldError(name->id.name);
                    }
                }
            }
            break;
        case TypeExpression:
            if (pexpr->type == TypeArray) pexpr = &s_javaArrayObjectSymbol.u.structSpec->type;
//&			if (pexpr->type == TypeError) {
//&				addTrivialCxReference(LINK_NAME_INDUCED_ERROR,TypeInducedError,StorageDefault,
//&                                   name->id.position, UsageUsed);
//&			}
            if (pexpr->type != TypeStruct) {
                *str = &errorSymbol;
            } else {
                javaLoadClassSymbolsFromFile(pexpr->u.t);
                Result result = findStrRecordSym(str, iniFind(pexpr->u.t,rfs), name->id.name, classif, ACCESSIBILITY_CHECK_NO, VISIBILITY_CHECK_NO);
                if (result == RESULT_OK) {
                    if ((options.ooChecksBits & OOC_ALL_CHECKS) == 0 ||
                        javaRecordVisibleAndAccessible(rfs, rfs->baseClass, rfs->currentClass, *str)) {
                        minacc = javaGetMinimalAccessibility(rfs, *str);
                        addAmbCxRef(classif,*str,&name->id.position,uusage, minacc, oref, rfs);
                    }
                } else {
                    noSuchFieldError(name->id.name);
                }
            }
            *expr = (*str)->u.typeModifier;
            name->nameType = TypeExpression;
            break;
        default: assert(0);
        }
    }
    return name->nameType;
}

TypeModifier *javaClassifyToExpressionName(IdList *name,
                                           Reference **oref) {
    Symbol    *str;
    TypeModifier		*expr,*res;
    int atype;
    atype = javaClassifyAmbiguousName(name,NULL,&str,&expr,oref,NULL, USELESS_FQT_REFS_ALLOWED,CLASS_TO_EXPR,UsageUsed);
    if (atype == TypeExpression) res = expr;
    else if (atype == TypeStruct) {
        assert(str && str->u.structSpec);
        res = &str->u.structSpec->type; /* because of casts & s_errorModifier;*/
        assert(res && res->type == TypeStruct);
    } else res = & errorModifier;
    return res;
}

// returns last useless reference (if any)
Reference *javaClassifyToTypeOrPackageName(IdList *tname, int usage, Symbol **str, int allowUselesFqtRefs) {
    TypeModifier		*expr;
    Reference			*rr, *lastUselessRef;
    lastUselessRef = NULL;
    javaClassifyAmbiguousName(tname, NULL, str, &expr, &rr, &lastUselessRef, allowUselesFqtRefs,
                              CLASS_TO_TYPE, usage);
    return lastUselessRef;
}

Reference *javaClassifyToTypeName(IdList *tname, int usage, Symbol **str, int allowUselesFqtRefs) {
    Reference *res;

    res = javaClassifyToTypeOrPackageName(tname, usage, str, allowUselesFqtRefs);
    if (tname->nameType != TypeStruct) {
        // there is probably a problem with class or source path
        tname->nameType = TypeStruct;
        // but following will create double reference, prefer to not add
        //ss = javaTypeSymbolUsage(tname, ACC_DEFAULT);
        //addCxReference(ss, &tname->idi.p, usage, s_noneFileIndex, s_noneFileIndex);
    }
    return res;
}

// !!! this is called also for qualified super
Symbol * javaQualifiedThis(IdList *tname, Id *thisid) {
    Symbol			*str;
    TypeModifier		*expr;
    Reference			*rr, *lastUselessRef;
    int					ttype;
    lastUselessRef = NULL;
    ttype = javaClassifyAmbiguousName(tname, NULL,&str,&expr,&rr,
                                      &lastUselessRef, USELESS_FQT_REFS_ALLOWED,CLASS_TO_TYPE,UsageUsed);
    if (ttype == TypeStruct) {
        addThisCxReferences(str->u.structSpec->classFileNumber, &thisid->position);
/*&
        addSpecialFieldReference(LINK_NAME_MAYBE_THIS_ITEM,StorageField,
                                 str->u.structSpec->classFileNumber, &thisid->position,
                                 UsageMaybeThis);
&*/
        if (str->u.structSpec->classFileNumber == javaStat->classFileNumber) {
            // redundant qualified this prefix
            addUselessFQTReference(str->u.structSpec->classFileNumber, &thisid->position);
//&fprintf(dumpOut,"!adding useless reference on %d,%d\n", name->idi.p.line, name->idi.p.coll);
            javaResetUselessReference(lastUselessRef);
        }
    } else {
        str = &errorSymbol;
    }
    tname->nameType = TypeStruct;
    return str;
}

void javaClassifyToPackageName( IdList *id ) {
    IdList *ii;
    for (ii=id; ii!=NULL; ii=ii->next) ii->nameType = TypePackage;
}

void javaClassifyToPackageNameAndAddRefs(IdList *id, int usage) {
    IdList *ii;
    javaClassifyToPackageName( id);
    for (ii=id; ii!=NULL; ii=ii->next) javaAddNameCxReference(ii, usage);
}

void javaAddPackageDefinition(IdList *id) {
    javaClassifyToPackageNameAndAddRefs(id, UsageUsed); // UsageDefined);
}

void javaSetFieldLinkName(Symbol *field) {
    char *ln;
    assert(javaStat);
    ln = javaCreateComposedName(NULL,javaStat->className,'/', field->name, NULL, 0);
    field->linkName = ln;
}


Symbol *javaCreateNewMethod(char *nn, Position *p, int mem) {
    TypeModifier *m;
    Symbol *symbol;
    char *name;

    if (mem==MEMORY_CF) {
        name = cfAllocc(strlen(nn)+1, char);
        strcpy(name, nn);
        m = cfAlloc(TypeModifier);
        symbol = cfAlloc(Symbol);
    } else {
        name = nn;
        m = stackMemoryAlloc(sizeof(TypeModifier));
        symbol = stackMemoryAlloc(sizeof(Symbol));
    }

    initTypeModifierAsFunction(m, NULL, NULL, NULL, NULL);
    fillSymbolWithTypeModifier(symbol, name, name, *p, m);

    return symbol;
}

int javaTypeToString(TypeModifier *type, char *pp, int ppSize) {
    int ppi;
    TypeModifier *tt;
    ppi=0;
    for (tt=type; tt!=NULL; tt=tt->next) {
        if (tt->type == TypeArray) {
            sprintf(pp+ppi,"[");
            ppi += strlen(pp+ppi);
        } else if (tt->type == TypeStruct) {
            assert(tt->u.t);
            sprintf(pp+ppi,"L%s;",tt->u.t->linkName);
            ppi += strlen(pp+ppi);
        } else {
            assert(javaBaseTypeCharCodes[tt->type]!=0);
            pp[ppi++] = javaBaseTypeCharCodes[tt->type];
        }
        assert(ppi < ppSize);
    }
    pp[ppi]=0;
    return ppi;
}

int javaIsYetInTheClass(Symbol *clas, char *lname, Symbol **eq) {
    Symbol        *r;
    assert(clas && clas->u.structSpec);
    for (r=clas->u.structSpec->records; r!=NULL; r=r->next) {
    //&fprintf(dumpOut, "[javaIsYetInTheClass] checking %s <-> %s\n",r->linkName, lname);fflush(dumpOut);
        if (strcmp(r->linkName, lname)==0) {
            *eq = r;
            return true;
        }
    }
    *eq = NULL;
    return false;
}


static int javaNumberOfNativeMethodsWithThisName(Symbol *clas, char *name) {
    Symbol        *r;
    int				res;
    res = 0;
    assert(clas && clas->type==TypeStruct && clas->u.structSpec);
    for (r=clas->u.structSpec->records; r!=NULL; r=r->next) {
        if (strcmp(r->name, name)==0 && (r->access&AccessNative)) {
            res++;
        }
    }
    return res;
}


int javaSetFunctionLinkName(Symbol *clas, Symbol *decl, enum memoryClass mem) {
    static char pp[MAX_PROFILE_SIZE];
    char *ln;
    int ppi, res;
    Symbol *args;
    Symbol *memb;

    res = 0;
    if (decl == &errorSymbol || decl->type==TypeError)
        return res;
    assert(decl->type == TypeDefault);
    assert(decl->u.typeModifier);
    if (decl->u.typeModifier->type != TypeFunction)
        return res;
    ppi=0;
    sprintf(pp+ppi,"%s", decl->name);
    ppi += strlen(pp+ppi);
    sprintf(pp+ppi,"(");
    ppi += strlen(pp+ppi);
    for(args=decl->u.typeModifier->u.f.args; args!=NULL; args=args->next) {
        ppi += javaTypeToString(args->u.typeModifier, pp+ppi, MAX_PROFILE_SIZE-ppi);
    }
    sprintf(pp+ppi,")");
    ppi += strlen(pp+ppi);
    assert(ppi<MAX_PROFILE_SIZE);
    if (javaIsYetInTheClass(clas, pp, &memb)) {
        decl->linkName = memb->linkName;
    } else {
        if (mem == MEMORY_CF) {
            ln = cfAllocc(ppi+1, char);
        } else {
            assert(mem==MEMORY_XX);
            ln = stackMemoryAlloc(ppi+1);
        }
        strcpy(ln,pp);
        decl->linkName = ln;
        res = 1;
    }
    return res;
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
    addTrivialCxReference(nlname, TypeDefault,StorageExtern, decl->pos,
                          UsageJavaNativeDeclared);
}

int javaLinkNameIsANestedClass(char *cname) {
    // this is a very hack!!   TODO do this seriously
    // an alternative can be 'javaSimpleNameIsInnerMemberClass', but it seems
    // to be even worst
    if (strchr(cname, '$') != NULL) return true;
    return false;
}

int isANestedClass(Symbol *ss) {
    return javaLinkNameIsANestedClass(ss->linkName);
}

int javaLinkNameIsAnnonymousClass(char *linkname) {
    char *ss;
    // this is a very hack too!!   TODO do this seriously
    ss = linkname;
    while ((ss=strchr(ss, '$'))!=NULL) {
        ss++;
        if (isdigit(*ss)) return true;
    }
    return false;
}

void addThisCxReferences(int classIndex, Position *pos) {
    int usage;
    if (classIndex == javaStat->classFileNumber) {
        usage = UsageMaybeThis;
    } else {
        usage = UsageMaybeQualifiedThis;
    }
    addSpecialFieldReference(LINK_NAME_MAYBE_THIS_ITEM,StorageField,
                             classIndex, pos, usage);
}

Reference *addUselessFQTReference(int classIndex, Position *pos) {
    Reference *res;
    res = addSpecialFieldReference(LINK_NAME_IMPORTED_QUALIFIED_ITEM,StorageField,
                                   classIndex, pos, UsageLastUseless);
    return res;
}

Reference *addUnimportedTypeLongReference(int classIndex, Position *pos) {
    Reference *res;
    res = addSpecialFieldReference(LINK_NAME_UNIMPORTED_QUALIFIED_ITEM, StorageField,
                                   classIndex, pos, UsageUsed);
    return res;
}

void addSuperMethodCxReferences(int classIndex, Position *pos) {
    addSpecialFieldReference(LINK_NAME_SUPER_METHOD_ITEM,StorageField,
                             classIndex, pos,
                             UsageSuperMethod);
}

Symbol *javaPrependDirectEnclosingInstanceArgument(Symbol *args) {
    warningMessage(ERR_ST,"[javaPrependDirectEnclosingInstanceArgument] not yet implemented");
    return args;
}

void addMethodCxReferences(unsigned modif, Symbol *method, Symbol *clas) {
    int clasn;
    assert(clas && clas->u.structSpec);
    clasn = clas->u.structSpec->classFileNumber;
    assert(clasn!=NO_FILE_NUMBER);
    addCxReference(method, &method->pos, UsageDefined, clasn, clasn);
    if (modif & AccessNative) {
        addNativeMethodCxReference(method, clas);
    }
}

Symbol *javaMethodHeader(unsigned modif, Symbol *type,
                           Symbol *decl, int storage) {
    int newFun;

    completeDeclarator(type, decl);
    decl->access = modif;
    decl->storage = storage;
    if (javaStat->thisClass->access & AccessInterface) {
        // set interface default access flags
        decl->access |= (AccessPublic | AccessAbstract);
    }
    newFun = javaSetFunctionLinkName(javaStat->thisClass, decl, MEMORY_XX);
    addMethodCxReferences(modif, decl, javaStat->thisClass);
    if (newFun) {
        LIST_APPEND(Symbol, javaStat->thisClass->u.structSpec->records, decl);
    }
    return decl;
}

void javaAddMethodParametersToSymTable(Symbol *method) {
    Symbol *p;
    int i;
    for(p=method->u.typeModifier->u.f.args,i=1; p!=NULL; p=p->next,i++) {
        addFunctionParameterToSymTable(javaStat->locals, method, p, i);
    }
}

void javaMethodBodyBeginning(Symbol *method) {
    assert(method->u.typeModifier && method->u.typeModifier->type == TypeFunction);
    parsedClassInfo.function = method;
    generateInternalLabelReference(-1, UsageDefined);
    counters.localVar = 0;
    javaAddMethodParametersToSymTable(method);
    method->u.typeModifier->u.m.signature = strchr(method->linkName, '(');
    javaStat->methodModifiers = method->access;
}

// this should be merged with _bef_ token!
void javaMethodBodyEnding(Position *endpos) {
    if (options.mode == ServerMode) {
        if (parsedClassInfo.parserPassedMarker && !parsedClassInfo.thisMethodMemoriesStored){
            parsedInfo.methodCoordEndLine = currentFile.lineNumber+1;
        }
    }
    // I rely that it is nil, for example in setmove target
    parsedClassInfo.function = NULL;
}

TypeModifier *javaClassNameType(IdList *typeName) {
    Symbol *st;

    assert(typeName);
    assert(typeName->nameType == TypeStruct);
    st = javaTypeSymbolUsage(typeName, AccessDefault);

    return newStructTypeModifier(st);
}


/* ********************* method invocations ************************** */

static void methodAppliedOnNonClass(char *rec) {
    char message[TMP_BUFF_SIZE];
    if (options.debug || options.errors) {
        sprintf(message, "'%s' not applied on a class", rec);
        errorMessage(ERR_ST, message);
    }
}

static void methodNameNotRecognized(char *rec) {
    char message[TMP_BUFF_SIZE];
    if (options.debug || options.errors) {
        sprintf(message, "'%s' not recognized as method name", rec);
        errorMessage(ERR_ST, message);
    }
}

S_extRecFindStr *javaCrErfsForMethodInvocationN(IdList *name) {
    S_extRecFindStr		*erfs;
    TypeModifier		*expr;
    Reference			*rr;
    int					nt;
    erfs = stackMemoryAlloc(sizeof(S_extRecFindStr));
    erfs->params = NULL;
    nt = javaClassifyAmbiguousName(name, &erfs->s,&erfs->memb,&expr,&rr,NULL, USELESS_FQT_REFS_ALLOWED,CLASS_TO_METHOD,UsageUsed);
    if (nt != TypeExpression) {
        methodNameNotRecognized(name->id.name);
        return NULL;
    }
    if (expr == &errorModifier)
        return NULL;
    return erfs;
}

S_extRecFindStr *javaCrErfsForMethodInvocationT(TypeModifier *tt,
                                                Id *name
    ) {
    S_extRecFindStr		*erfs;

    log_trace("invocation of %s", name->name);
    if (tt->type == TypeArray) tt = &s_javaArrayObjectSymbol.u.structSpec->type;
    if (tt->type != TypeStruct) {
        methodAppliedOnNonClass(name->name);
        return NULL;
    }
    erfs = stackMemoryAlloc(sizeof(S_extRecFindStr));
    erfs->params = NULL;
    javaLoadClassSymbolsFromFile(tt->u.t);
    Result rr = findStrRecordSym(&erfs->memb, iniFind(tt->u.t,&erfs->s), name->name,
                        CLASS_TO_METHOD, ACCESSIBILITY_CHECK_NO,VISIBILITY_CHECK_NO);
    if (rr != RESULT_OK) {
        noSuchFieldError(name->name);
        return NULL;
    }
    return erfs;
}

S_extRecFindStr *javaCrErfsForConstructorInvocation(Symbol *clas,
                                                    Position *pos
    ) {
    S_extRecFindStr		*erfs;

    if (clas == &errorSymbol || clas->type==TypeError)
        return NULL;
    assert(clas && clas->type == TypeStruct);
    javaLoadClassSymbolsFromFile(clas);
    erfs = stackMemoryAlloc(sizeof(S_extRecFindStr));
    erfs->params = NULL;
    assert(clas->javaClassIsLoaded);
    Result rr = findStrRecordSym(&erfs->memb, iniFind(clas, &erfs->s), clas->name,
                        CLASS_TO_METHOD,ACCESSIBILITY_CHECK_NO,VISIBILITY_CHECK_NO);
    if (rr != RESULT_OK)
        return NULL;
    return erfs;
}

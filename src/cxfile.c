#include "cxfile.h"

#include "yylex.h"              /* addFileTabItem() ? */
#include "cxref.h"
#include "options.h"
#include "reftab.h"
#include "classhierarchy.h"
#include "fileio.h"
#include "list.h"

#include "protocol.h"           /* C_XREF_FILE_VERSION_NUMBER */
#include "globals.h"
#include "commons.h"
#include "misc.h"
#include "hash.h"
#include "log.h"
#include "utils.h"              /* recursivelyCreateFileDirIfNotExists() */


/* *********************** INPUT/OUTPUT FIELD MARKERS ************************** */

#define CXFI_FILE_FUMTIME   'm'     /* last full update mtime for file item */
#define CXFI_FILE_UMTIME    'p'     /* last update mtime for file item */
#define CXFI_SOURCE_INDEX   'o'     /* source index for java classes */
#define CXFI_INPUT_FROM_COMMAND_LINE  'i'     /* file was introduced from command line */

#define CXFI_USAGE          'u'
#define CXFI_REQUIRED_ACCESS     'A'     /* java reference required accessibility index */
#define CXFI_SYMBOL_INDEX   's'
#define CXFI_FILE_INDEX     'f'
#define CXFI_LINE_INDEX     'l'
#define CXFI_COLUMN_INDEX   'c'
#define CXFI_REFERENCE      'r'     /* using 'fsulc' */

#define CXFI_SYMBOL_TYPE    't'

#define CXFI_SUBCLASS       'd'     /* dole = down in slovac */
#define CXFI_SUPERCLASS     'h'     /* hore = up in slovac */
#define CXFI_CLASS_EXT      'e'     /* using 'fhd' */

#define CXFI_ACCESS_BITS    'a'     /* java access bit */
#define CXFI_STORAGE        'g'     /* storaGe field */

#define CXFI_MACRO_BASE_FILE 'b'    /* ref to a file invoking macro */

#define CXFI_SYMBOL_NAME    '/'     /* using 'atdhg' -> 's'             */
#define CXFI_CLASS_NAME     '+'     /*               -> 'h' info    */
#define CXFI_FILE_NAME      ':'     /*               -> 'ifm' info  */

#define CXFI_CHECK_NUMBER   'k'
#define CXFI_REFNUM         'n'
#define CXFI_VERSION        'v'
#define CXFI_MARKER_LIST    '@'
#define CXFI_REMARK         '#'

static int generatedFieldMarkersList[] = {
    CXFI_FILE_FUMTIME,
    CXFI_FILE_UMTIME,
    CXFI_FILE_INDEX,
    CXFI_SOURCE_INDEX,
    CXFI_SYMBOL_TYPE,
    CXFI_USAGE,
    CXFI_LINE_INDEX,
    CXFI_COLUMN_INDEX,
    CXFI_SYMBOL_INDEX,
    CXFI_REFERENCE,
    CXFI_SUPERCLASS,
    CXFI_SUBCLASS,
    CXFI_CLASS_EXT,
    CXFI_INPUT_FROM_COMMAND_LINE,
    CXFI_MACRO_BASE_FILE,
    CXFI_REFNUM,
    CXFI_ACCESS_BITS,
    CXFI_REQUIRED_ACCESS,
    CXFI_STORAGE,
    CXFI_CHECK_NUMBER,
    -1
};

#define MAX_CX_SYMBOL_TAB 2

typedef struct lastCxFileInfo {
    int                 onLineReferencedSym;
    SymbolsMenu     *onLineRefMenuItem;
    int                 onLineRefIsBestMatchFlag; // vyhodit ?
    SymbolReferenceItem *symbolTab[MAX_CX_SYMBOL_TAB];
    bool                symbolIsWritten[MAX_CX_SYMBOL_TAB];
    int                 macroBaseFileGeneratedForSym[MAX_CX_SYMBOL_TAB];
    char                markers[MAX_CHARS];
    int                 values[MAX_CHARS];
    void                (*fun[MAX_CHARS])(int size, int marker, CharacterBuffer *cb, int additional);
    int                 additional[MAX_CHARS];

    // dead code detection vars
    int                 symbolToCheckForDeadness;
    char                deadSymbolIsDefined;

    // following item can be used only via symbolTab,
    // it is just to simplify memory handling !!!!!!!!!!!!!!!!
    SymbolReferenceItem     cachedSymbolReferenceItem[MAX_CX_SYMBOL_TAB];
    char                cachedSymbolName[MAX_CX_SYMBOL_TAB][MAX_CX_SYMBOL_SIZE];
} LastCxFileInfo;

static LastCxFileInfo lastIncomingInfo;
static LastCxFileInfo lastOutgoingInfo;


static CharacterBuffer cxfCharacterBuffer;

static unsigned s_decodeFilesNum[MAX_FILES];

static char tmpFileName[MAX_FILE_NAME_SIZE];


static void fPutDecimal(FILE *ff, int num) {
    static char ttt[TMP_STRING_SIZE]= {0,};
    char *d;
    int n;

    /* TODO: why re-implement fprintf("%d",num)? */
    n = num;
    assert(n>=0);
    d = ttt+TMP_STRING_SIZE-1;
    while (n>=10) {
        *(--d) = n%10 + '0';
        n = n/10;
    }
    *(--d) = n + '0';
    assert(d>=ttt);             /* TODO: WTF? */
    fputs(d, ff);
}


/* *********************** INPUT/OUTPUT ************************** */

int cxFileHashNumber(char *sym) {
    unsigned   res,r;
    char       *ss,*bb;
    int        c;

    if (options.referenceFileCount <= 1) return(0);
    if (options.xfileHashingMethod == XFILE_HASH_DEFAULT) {
        res = 0;
        ss = sym;
        while ((c = *ss)) {
            if (c == '(') break;
            SYMTAB_HASH_FUN_INC(res, c);
            if (LINK_NAME_MAYBE_START(c)) res = 0;
            ss++;
        }
        SYMTAB_HASH_FUN_FINAL(res);
        res %= options.referenceFileCount;
        return(res);
    } else if (options.xfileHashingMethod == XFILE_HASH_ALPHA1) {
        assert(options.referenceFileCount == XFILE_HASH_ALPHA1_REFNUM);
        for(ss = bb = sym; *ss && *ss!='('; ss++) {
            c = *ss;
            if (LINK_NAME_MAYBE_START(c)) bb = ss+1;
        }
        c = *bb;
        c = tolower(c);
        if (c>='a' && c<='z') res = c-'a';
        else res = ('z'-'a')+1;
        assert(res>=0 && res<options.referenceFileCount);
        return(res);
    } else if (options.xfileHashingMethod == XFILE_HASH_ALPHA2) {
        for(ss = bb = sym; *ss && *ss!='('; ss++) {
            c = *ss;
            if (LINK_NAME_MAYBE_START(c)) bb = ss+1;
        }
        c = *bb;
        c = tolower(c);
        if (c>='a' && c<='z') r = c-'a';
        else r = ('z'-'a')+1;
        if (c==0) res=0;
        else {
            c = *(bb+1);
            c = tolower(c);
            if (c>='a' && c<='z') res = c-'a';
            else res = ('z'-'a')+1;
        }
        res = r*XFILE_HASH_ALPHA1_REFNUM + res;
        assert(res>=0 && res<options.referenceFileCount);
        return(res);
    } else {
        assert(0);
        return(0);
    }
}

static int searchSingleStringEqual(char *s, char *c) {
    while (*s!=0 && *s!=' ' && *s!='\t' && tolower(*s)==tolower(*c)) {
        c++; s++;
    }
    if (*s==0 || *s==' ' || *s=='\t') return(1);
    return(0);
}

static int searchSingleStringFitness(char *cxtag, char *searchedStr, int len) {
    char *cc;
    int i, pilotc;

    assert(searchedStr);
    pilotc = tolower(*searchedStr);
    if (pilotc == '^') {
        // check for exact prefix
        return(searchSingleStringEqual(searchedStr+1, cxtag));
    } else {
        cc = cxtag;
        for(cc=cxtag, i=0; *cc && i<len; cc++,i++) {
            if (searchSingleStringEqual(searchedStr, cc)) return(1);
        }
    }
    return(0);
}

static int searchStringNonWildcardFitness(char *cxtag, int len) {
    char    *ss;
    int     r;
    ss = options.olcxSearchString;
    while (*ss) {
        while (*ss==' ' || *ss=='\t') ss++;
        if (*ss == 0) goto fini1;
        r = searchSingleStringFitness(cxtag, ss, len);
        if (r==0) return(0);
        while (*ss!=0 && *ss!=' ' && *ss!='\t') ss++;
    }
 fini1:
    return(1);
}

int searchStringFitness(char *cxtag, int len) {
    if (s_wildcardSearch) return(shellMatch(cxtag, len, options.olcxSearchString, false));
    else return(searchStringNonWildcardFitness(cxtag, len));
}


#define maxOf(a, b) (((a) > (b)) ? (a) : (b))

char *crTagSearchLineStatic(char *name, Position *p,
                            int *len1, int *len2, int *len3) {
    static char res[2*COMPLETION_STRING_SIZE];
    char file[TMP_STRING_SIZE];
    char dir[TMP_STRING_SIZE];
    char *ffname;
    int l1,l2,l3,fl, dl;

    l1 = l2 = l3 = 0;
    l1 = strlen(name);

    /*&
      char type[TMP_STRING_SIZE];
      type[0]=0;
      if (symType != TypeDefault) {
      l2 = strmcpy(type,typeName[symType]+4) - type;
      }
      &*/

    ffname = fileTable.tab[p->file]->name;
    assert(ffname);
    ffname = getRealFileNameStatic(ffname);
    fl = strlen(ffname);
    l3 = strmcpy(file,simpleFileName(ffname)) - file;

    dl = fl /*& - l3 &*/ ;
    strncpy(dir, ffname, dl);
    dir[dl]=0;

    *len1 = maxOf(*len1, l1);
    *len2 = maxOf(*len2, l2);
    *len3 = maxOf(*len3, l3);

    if (options.tagSearchSpecif == TSS_SEARCH_DEFS_ONLY_SHORT
        || options.tagSearchSpecif==TSS_FULL_SEARCH_SHORT) {
        sprintf(res, "%s", name);
    } else {
        sprintf(res, "%-*s :%-*s :%s", *len1, name, *len3, file, dir);
    }
    return res;
}

// Filter out symbols which pollute search reports
bool symbolNameShouldBeHiddenFromReports(char *name) {
    char *s;

    // internal xref symbol?
    if (name[0] == ' ')
        return true;

    // Only Java specific pollution
    if (! LANGUAGE(LANG_JAVA))
        return false;

    // Hide class$ fields
    if (strncmp(name, "class$", 6)==0)
        return true;

    // Hide anonymous classes
    if (isdigit(name[0]))
        return true;

    // And what is this?
    s = name;
    while ((s=strchr(s, '$'))!=NULL) {
        s++;
        while (isdigit(*s))
            s++;
        if (*s == '.' || *s=='(' || *s=='$' || *s==0)
            return true;
    }

    return false;
}

void searchSymbolCheckReference(SymbolReferenceItem  *ss, Reference *rr) {
    char ssname[MAX_CX_SYMBOL_SIZE];
    char *s, *sname;
    int slen;

    if (ss->b.symType == TypeCppInclude)
        return;   // no %%i symbols
    if (symbolNameShouldBeHiddenFromReports(ss->name))
        return;

    linkNamePrettyPrint(ssname, ss->name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
    sname = ssname;
    slen = strlen(sname);
    // if completing without profile, cut profile
    if (options.tagSearchSpecif==TSS_SEARCH_DEFS_ONLY_SHORT
        || options.tagSearchSpecif==TSS_FULL_SEARCH_SHORT) {
        s = strchr(sname, '(');
        if (s!=NULL) *s = 0;
    }
    // cut package name for checking
    do {
        s = strchr(sname, '.');
        if (s!=NULL) sname = s+1;
    } while (s!=NULL);
    slen = strlen(sname);
    if (searchStringFitness(sname, slen)) {
        static int count = 0;
        //& olCompletionListPrepend(sname, NULL, NULL, 0, NULL, NULL, rr, ss->vFunClass, s_olcxCurrentUser->retrieverStack.top);
        //&sprintf(tmpBuff,"adding %s of %s(%d) matched %s %d", sname, fileTable.tab[rr->p.file]->name, rr->p.file, options.olcxSearchString, s_wildcardSearch);ppcBottomInformation(tmpBuff);
        olCompletionListPrepend(sname, NULL, NULL, 0, NULL, ss, rr, ss->b.symType, ss->vFunClass, s_olcxCurrentUser->retrieverStack.top);
        // this is a hack for memory reduction
        // compact completions from time to time
        count ++;
        if (count > COMPACT_TAG_SEARCH_AFTER) {
            tagSearchCompactShortResults();
            count = 0;
        }
    }
}

/* ************************* WRITE **************************** */

static void get_version_string(char *ttt) {                             \
    sprintf(ttt," file format: C-xrefactory %s ", C_XREF_FILE_VERSION_NUMBER); \
}


static void writeCompactRecord(char marker, int info, char *blankPrefix) {
    assert(marker >= 0 && marker < MAX_CHARS);
    assert(info >= 0);
    if (*blankPrefix!=0) fputs(blankPrefix, cxOut);
    if (info != 0) fPutDecimal(cxOut, info);
    fputc(marker, cxOut);
    lastOutgoingInfo.values[marker] = info;
}

static void writeOptionalCompactRecord(char marker, int info, char *blankPrefix) {
    assert(marker >= 0 && marker < MAX_CHARS);
    if (*blankPrefix!=0) fputs(blankPrefix, cxOut);

    /* If value for marker is same as last, don't write anything */
    if (lastOutgoingInfo.values[marker] != info) {
        /* If the info to write is not 0 then write it, else just write the marker */
        if (info != 0)
            fPutDecimal(cxOut, info);
        fputc(marker, cxOut);
        lastOutgoingInfo.values[marker] = info;
    }
}


static void writeStringRecord(int marker, char *s, char *blankPrefix) {
    int rsize;
    rsize = strlen(s)+1;
    if (*blankPrefix!=0) fputs(blankPrefix, cxOut);
    fPutDecimal(cxOut, rsize);
    fputc(marker, cxOut);
    fputs(s, cxOut);
}

/* Here we do the actual writing of the symbol */
static void writeSymbolItem(int symbolIndex) {
    SymbolReferenceItem *d;

    /* First the symbol info, if not done already */
    writeOptionalCompactRecord(CXFI_SYMBOL_INDEX, symbolIndex, "");

    /* Then the reference info */
    d = lastOutgoingInfo.symbolTab[symbolIndex];
    writeOptionalCompactRecord(CXFI_SYMBOL_TYPE, d->b.symType, "\n"); /* Why newline in the middle of all this? */
    writeOptionalCompactRecord(CXFI_SUBCLASS, d->vApplClass, "");
    writeOptionalCompactRecord(CXFI_SUPERCLASS, d->vFunClass, "");
    writeOptionalCompactRecord(CXFI_ACCESS_BITS, d->b.accessFlags, "");
    writeOptionalCompactRecord(CXFI_STORAGE, d->b.storage, "");
    lastOutgoingInfo.macroBaseFileGeneratedForSym[symbolIndex] = 0;
    lastOutgoingInfo.symbolIsWritten[symbolIndex] = true;
    writeStringRecord(CXFI_SYMBOL_NAME, d->name, "\t");
    fputc('\t', cxOut);
}

static void writeSymbolItemIfNotWritten(int symbolIndex) {
    if (! lastOutgoingInfo.symbolIsWritten[symbolIndex]) {
        writeSymbolItem(symbolIndex);
    }
}

static void writeCxReferenceBase(int symbolIndex, Usage usage, int requiredAccess, int file, int line, int col) {
    writeSymbolItemIfNotWritten(symbolIndex);
    if (usage == UsageMacroBaseFileUsage) {
        /* optimize the number of those references to 1 */
        assert(symbolIndex>=0 && symbolIndex<MAX_CX_SYMBOL_TAB);
        if (lastOutgoingInfo.macroBaseFileGeneratedForSym[symbolIndex]) return;
        lastOutgoingInfo.macroBaseFileGeneratedForSym[symbolIndex] = 1;
    }
    writeOptionalCompactRecord(CXFI_USAGE, usage, "");
    writeOptionalCompactRecord(CXFI_REQUIRED_ACCESS, requiredAccess, "");
    writeOptionalCompactRecord(CXFI_SYMBOL_INDEX, symbolIndex, "");
    writeOptionalCompactRecord(CXFI_FILE_INDEX, file, "");
    writeOptionalCompactRecord(CXFI_LINE_INDEX, line, "");
    writeOptionalCompactRecord(CXFI_COLUMN_INDEX, col, "");
    writeCompactRecord(CXFI_REFERENCE, 0, "");
}

static void writeCxReference(Reference *reference, int symbolNum) {
    writeCxReferenceBase(symbolNum, reference->usage.base, reference->usage.requiredAccess,
                         reference->p.file, reference->p.line, reference->p.col);
}

static void writeSubClassInfo(int sup, int inf, int origin) {
    writeOptionalCompactRecord(CXFI_FILE_INDEX, origin, "\n");
    writeOptionalCompactRecord(CXFI_SUPERCLASS, sup, "");
    writeOptionalCompactRecord(CXFI_SUBCLASS, inf, "");
    writeCompactRecord(CXFI_CLASS_EXT, 0, "");
}

static void writeFileIndexItem(struct fileItem *fi, int ii) {
    writeOptionalCompactRecord(CXFI_FILE_INDEX, ii, "\n");
    writeOptionalCompactRecord(CXFI_FILE_UMTIME, fi->lastUpdateMtime, " ");
    writeOptionalCompactRecord(CXFI_FILE_FUMTIME, fi->lastFullUpdateMtime, " ");
    writeOptionalCompactRecord(CXFI_INPUT_FROM_COMMAND_LINE, fi->b.commandLineEntered, "");
    if (fi->b.isInterface) {
        writeOptionalCompactRecord(CXFI_ACCESS_BITS, AccessInterface, "");
    } else {
        writeOptionalCompactRecord(CXFI_ACCESS_BITS, AccessDefault, "");
    }
    writeStringRecord(CXFI_FILE_NAME, fi->name, " ");
}

static void writeFileSourceIndexItem(struct fileItem *fileItem, int ii) {
    if (fileItem->b.sourceFileNumber != noFileIndex) {
        writeOptionalCompactRecord(CXFI_FILE_INDEX, ii, "\n");
        writeCompactRecord(CXFI_SOURCE_INDEX, fileItem->b.sourceFileNumber, " ");
    }
}

static void genClassHierarchyItems(struct fileItem *fi, int ii) {
    ClassHierarchyReference *p;
    for(p=fi->superClasses; p!=NULL; p=p->next) {
        writeSubClassInfo(p->superClass, ii, p->ofile);
    }
}


static ClassHierarchyReference *findSuperiorInSuperClasses(int superior, FileItem *iFile) {
    ClassHierarchyReference *p;
    for(p=iFile->superClasses; p!=NULL && p->superClass!=superior; p=p->next)
        ;

    return p;
}


static void createSubClassInfo(int superior, int inferior, int origin, int genfl) {
    FileItem      *iFile, *sFile;
    ClassHierarchyReference   *p, *pp;

    iFile = fileTable.tab[inferior];
    sFile = fileTable.tab[superior];
    assert(iFile && sFile);
    p = findSuperiorInSuperClasses(superior, iFile);
    if (p==NULL) {
        p = newClassHierarchyReference(origin, superior, iFile->superClasses);
        iFile->superClasses = p;
        assert(options.taskRegime);
        if (options.taskRegime == RegimeXref) {
            if (genfl == CX_FILE_ITEM_GEN)
                writeSubClassInfo(superior, inferior, origin);
        }
        pp = newClassHierarchyReference(origin, inferior, sFile->superClasses);
        sFile->inferiorClasses = pp;
    }
}

void addSubClassItemToFileTab( int sup, int inf, int origin) {
    if (sup >= 0 && inf >= 0) {
        createSubClassInfo(sup, inf, origin, NO_CX_FILE_ITEM_GEN);
    }
}


void addSubClassesItemsToFileTab(Symbol *ss, int origin) {
    int cf1;
    SymbolList *sups;

    if (ss->bits.symbolType != TypeStruct) return;
    /*fprintf(dumpOut,"testing %s\n",ss->name);*/
    assert(ss->bits.javaFileIsLoaded);
    if (!ss->bits.javaFileIsLoaded)
        return;
    cf1 = ss->u.s->classFile;
    assert(cf1 >= 0 &&  cf1 < MAX_FILES);
    /*fprintf(dumpOut,"loaded: #sups == %d\n",ns);*/
    for(sups=ss->u.s->super; sups!=NULL; sups=sups->next) {
        assert(sups->d && sups->d->bits.symbolType == TypeStruct);
        addSubClassItemToFileTab( sups->d->u.s->classFile, cf1, origin);
    }
}

/* *************************************************************** */

static void genRefItem0(SymbolReferenceItem *d, bool force) {
    Reference *reference;
    int symbolIndex;

    log_trace("generate cxref for symbol '%s'", d->name);
    symbolIndex = 0;
    assert(strlen(d->name)+1 < MAX_CX_SYMBOL_SIZE);

    strcpy(lastOutgoingInfo.cachedSymbolName[symbolIndex], d->name);
    fillSymbolRefItem(&lastOutgoingInfo.cachedSymbolReferenceItem[symbolIndex],
                      lastOutgoingInfo.cachedSymbolName[symbolIndex],
                      d->fileHash, // useless put 0
                      d->vApplClass, d->vFunClass);
    fillSymbolRefItemBits(&lastOutgoingInfo.cachedSymbolReferenceItem[symbolIndex].b,
                           d->b.symType, d->b.storage,
                           d->b.scope, d->b.accessFlags, d->b.category);
    lastOutgoingInfo.symbolTab[symbolIndex] = &lastOutgoingInfo.cachedSymbolReferenceItem[symbolIndex];
    lastOutgoingInfo.symbolIsWritten[symbolIndex] = false;

    if (d->b.category == CategoryLocal) return;
    if (d->refs == NULL && !force) return;

    for(reference = d->refs; reference!=NULL; reference=reference->next) {
        log_trace("checking ref: loading=%d --< %s:%d", fileTable.tab[reference->p.file]->b.cxLoading,
                  fileTable.tab[reference->p.file]->name, reference->p.line);
        if (options.update==UPDATE_CREATE || fileTable.tab[reference->p.file]->b.cxLoading) {
            writeCxReference(reference, symbolIndex);
        } else {
            log_trace("Some kind of update (%d) or not loading (%d), so don't writeCxReference()",
                      options.update, fileTable.tab[reference->p.file]->b.cxLoading);
            assert(0);
        }
    }
    //&fflush(cxOut);
}

static void genRefItem(SymbolReferenceItem *dd) {
    genRefItem0(dd, false);
}

#define COMPOSE_CXFI_CHECK_NUM(filen,hashMethod,exactPositionLinkFlag) ( \
        ((filen)*XFILE_HASH_MAX+hashMethod)*2+exactPositionLinkFlag     \
    )

#define DECOMPOSE_CXFI_CHECK_NUM(num,filen,hashMethod,exactPositionLinkFlag){ \
        unsigned tmp;                                                   \
        tmp = num;                                                      \
        exactPositionLinkFlag = tmp % 2;                                \
        tmp = tmp / 2;                                                  \
        hashMethod = tmp % XFILE_HASH_MAX;                              \
        tmp = tmp / XFILE_HASH_MAX;                                     \
        filen = tmp;                                                    \
    }

static void genCxFileHead(void) {
    char sr[MAX_CHARS];
    char ttt[TMP_STRING_SIZE];
    int i;
    memset(&lastOutgoingInfo, 0, sizeof(lastOutgoingInfo));
    for(i=0; i<MAX_CHARS; i++) {
        lastOutgoingInfo.values[i] = -1;
    }
    get_version_string(ttt);
    writeStringRecord(CXFI_VERSION, ttt, "\n\n");
    fprintf(cxOut,"\n\n\n");
    for(i=0; i<MAX_CHARS && generatedFieldMarkersList[i] != -1; i++) {
        sr[i] = generatedFieldMarkersList[i];
    }
    assert(i < MAX_CHARS);
    sr[i]=0;
    writeStringRecord(CXFI_MARKER_LIST, sr, "");
    writeCompactRecord(CXFI_REFNUM, options.referenceFileCount, " ");
    writeCompactRecord(CXFI_CHECK_NUMBER, COMPOSE_CXFI_CHECK_NUM(
                                                               MAX_FILES,
                                                               options.xfileHashingMethod,
                                                               options.exactPositionResolve
                                                               ), " ");
}

static void openInOutReferenceFiles(int updateFlag, char *filename) {
    if (updateFlag) {
        char *tempname = create_temporary_filename();
        strcpy(tmpFileName, tempname);
        copyFileFromTo(filename, tmpFileName);
    }

    assert(filename);
    cxOut = openFile(filename,"w");
    if (cxOut == NULL)
        fatalError(ERR_CANT_OPEN, filename, XREF_EXIT_ERR);

    if (updateFlag) {
        inputFile = openFile(tmpFileName, "r");
        if (inputFile==NULL)
            warningMessage(ERR_CANT_OPEN_FOR_READ, tmpFileName);
    } else {
        inputFile = NULL;
    }
}

static void closeReferenceFile(char *fname) {
    if (inputFile != NULL) {
        closeFile(inputFile);
        inputFile = NULL;
        removeFile(tmpFileName);
    }
    closeFile(cxOut);
    cxOut = stdout;
}

/* suffix contains '/' at the beginning */
static void genPartialFileTabRefFile(int updateFlag,
                                     char *dirname,
                                     char *suffix,
                                     void mapfun(FileItem *, int),
                                     void mapfun2(FileItem *, int)) {
    char filename[MAX_FILE_NAME_SIZE];

    sprintf(filename, "%s%s", dirname, suffix);
    assert(strlen(filename) < MAX_FILE_NAME_SIZE-1);
    openInOutReferenceFiles(updateFlag, filename);
    genCxFileHead();
    fileTableMapWithIndex(&fileTable, mapfun);
    if (mapfun2!=NULL)
        fileTableMapWithIndex(&fileTable, mapfun2);
    scanCxFile(fullScanFunctionSequence);
    closeReferenceFile(filename);
}

static void generateRefsFromMemory(int fileOrder) {
    int                 tsize;
    SymbolReferenceItem *pp;

    tsize = referenceTable.size;
    for (int i=0; i<tsize; i++) {
        for (pp=referenceTable.tab[i]; pp!=NULL; pp=pp->next) {
            if (pp->b.category == CategoryLocal)
                continue;
            if (pp->refs == NULL)
                continue;
            if (pp->fileHash == fileOrder)
                genRefItem0(pp, false);
        }
    }
}

void genReferenceFile(bool updating, char *filename) {
    if (!updating)
        removeFile(filename);

    recursivelyCreateFileDirIfNotExists(filename);

    if (options.referenceFileCount <= 1) {
        /* single reference file */
        openInOutReferenceFiles(updating, filename);
        /*&     fileTabMap(&fileTable, javaInitSubClassInfo); &*/
        genCxFileHead();
        fileTableMapWithIndex(&fileTable, writeFileIndexItem);
        fileTableMapWithIndex(&fileTable, writeFileSourceIndexItem);
        fileTableMapWithIndex(&fileTable, genClassHierarchyItems);
        scanCxFile(fullScanFunctionSequence);
        refTabMap(&referenceTable, genRefItem);
        closeReferenceFile(filename);
    } else {
        /* several reference files */
        char referenceFileName[MAX_FILE_NAME_SIZE];
        char *dirname = filename;
        int i;

        createDir(dirname);
        genPartialFileTabRefFile(updating,dirname,REFERENCE_FILENAME_FILES,
                                 writeFileIndexItem, writeFileSourceIndexItem);
        genPartialFileTabRefFile(updating,dirname,REFERENCE_FILENAME_CLASSES,
                                 genClassHierarchyItems, NULL);
        for (i=0; i<options.referenceFileCount; i++) {
            sprintf(referenceFileName, "%s%s%04d", dirname, REFERENCE_FILENAME_PREFIX, i);
            assert(strlen(referenceFileName) < MAX_FILE_NAME_SIZE-1);
            openInOutReferenceFiles(updating, referenceFileName);
            genCxFileHead();
            scanCxFile(fullScanFunctionSequence);
            //&refTabMap4(&referenceTable, genPartialRefItem, i);
            generateRefsFromMemory(i);
            closeReferenceFile(referenceFileName);
        }
    }
}

static void writeCxFileCompatibilityError(char *message) {
    static time_t lastMessageTime;
    if (options.taskRegime == RegimeEditServer) {
        if (lastMessageTime < s_fileProcessStartTime) {
            errorMessage(ERR_ST, message);
            lastMessageTime = time(NULL);
        }
    } else {
        fatalError(ERR_ST, message, XREF_EXIT_ERR);
    }
}


/* ************************* READ **************************** */

static void cxrfReadRecordMarkers(int size,
                                  int marker,
                                  CharacterBuffer *cb,
                                  int additionalArg
) {
    int i, ch;

    assert(marker == CXFI_MARKER_LIST);
    for(i=0; i<size-1; i++) {
        ch = getChar(cb);
        lastIncomingInfo.markers[ch] = 1;
    }
}


static void cxrfVersionCheck(int size,
                             int marker,
                             CharacterBuffer *cb,
                             int additionalArg
) {
    char versionString[TMP_STRING_SIZE];
    char thisVersionString[TMP_STRING_SIZE];
    int i, ch;

    assert(marker == CXFI_VERSION);
    for(i=0; i<size-1; i++) {
        ch = getChar(cb);
        versionString[i]=ch;
    }
    versionString[i]=0;
    get_version_string(thisVersionString);
    if (strcmp(versionString, thisVersionString)) {
        writeCxFileCompatibilityError("The Tag file was not generated by the current version of C-xrefactory, recreate it");
    }
}

static void cxrfCheckNumber(int size,
                            int marker,
                            CharacterBuffer *cb,
                            int additionalArg
                            ) {
    int magicn, filen, hashMethod, exactPositionLinkFlag;
    char tmpBuff[TMP_BUFF_SIZE];

    assert(marker == CXFI_CHECK_NUMBER);
    if (options.create)
        return; // no check when creating new file

    magicn = lastIncomingInfo.values[CXFI_CHECK_NUMBER];
    DECOMPOSE_CXFI_CHECK_NUM(magicn,filen,hashMethod,exactPositionLinkFlag);
    if (filen != MAX_FILES) {
        sprintf(tmpBuff,"The Tag file was generated with different MAX_FILES, recreate it");
        writeCxFileCompatibilityError(tmpBuff);
    }
    if (hashMethod != options.xfileHashingMethod) {
        sprintf(tmpBuff,"The Tag file was generated with different hash method, recreate it");
        writeCxFileCompatibilityError(tmpBuff);
    }
    log_trace("checking %d <-> %d", exactPositionLinkFlag, options.exactPositionResolve);
    if (exactPositionLinkFlag != options.exactPositionResolve) {
        if (exactPositionLinkFlag) {
            sprintf(tmpBuff,"The Tag file was generated with '-exactpositionresolve' flag, recreate it");
        } else {
            sprintf(tmpBuff,"The Tag file was generated without '-exactpositionresolve' flag, recreate it");
        }
        writeCxFileCompatibilityError(tmpBuff);
    }
}

static int cxrfFileItemShouldBeUpdatedFromCxFile(FileItem *ffi) {
    bool updateFromCxFile = true;

    log_trace("re-read info from '%s' for '%s'?", options.cxrefFileName, ffi->name);
    if (options.taskRegime == RegimeXref) {
        if (ffi->b.cxLoading && ! ffi->b.cxSaved) {
            updateFromCxFile = false;
        } else {
            updateFromCxFile = true;
        }
    }
    if (options.taskRegime == RegimeEditServer) {
        log_trace("last inspected == %d, start at %d\n", ffi->lastInspected, s_fileProcessStartTime);
        if (ffi->lastInspected < s_fileProcessStartTime) {
            updateFromCxFile = true;
        } else {
            updateFromCxFile = false;
        }
    }
    log_trace("%s re-read info from '%s' for '%s'", updateFromCxFile?"yes,":"no, not necessary to",
              options.cxrefFileName, ffi->name);

    return updateFromCxFile;
}

static void cxReadFileName(int size,
                           int marker,
                           CharacterBuffer *cb,
                           int genFl
                           ) {
    char id[MAX_FILE_NAME_SIZE];
    struct fileItem *fileItem;
    int i, ii, fileIndex, len, commandLineFlag, isInterface;
    time_t fumtime, umtime;
    char ch;

    assert(marker == CXFI_FILE_NAME);
    fumtime = (time_t) lastIncomingInfo.values[CXFI_FILE_FUMTIME];
    umtime = (time_t) lastIncomingInfo.values[CXFI_FILE_UMTIME];
    commandLineFlag = lastIncomingInfo.values[CXFI_INPUT_FROM_COMMAND_LINE];
    isInterface=((lastIncomingInfo.values[CXFI_ACCESS_BITS] & AccessInterface)!=0);
    ii = lastIncomingInfo.values[CXFI_FILE_INDEX];
    for (i=0; i<size-1; i++) {
        ch = getChar(cb);
        id[i] = ch;
    }
    id[i] = 0;
    len = i;
    assert(len+1 < MAX_FILE_NAME_SIZE);
    assert(ii>=0 && ii<MAX_FILES);
    if (!fileTableExists(&fileTable, id)) {
        fileIndex = addFileTabItem(id);
        fileItem = fileTable.tab[fileIndex];
        fileItem->b.commandLineEntered = commandLineFlag;
        fileItem->b.isInterface = isInterface;
        if (fileItem->lastFullUpdateMtime == 0) fileItem->lastFullUpdateMtime=fumtime;
        if (fileItem->lastUpdateMtime == 0) fileItem->lastUpdateMtime=umtime;
        assert(options.taskRegime);
        if (options.taskRegime == RegimeXref) {
            if (genFl == CX_GENERATE_OUTPUT) {
                writeFileIndexItem(fileItem, fileIndex);
            }
        }
    } else {
        fileIndex = fileTableLookup(&fileTable, id);
        fileItem = fileTable.tab[fileIndex];
        if (cxrfFileItemShouldBeUpdatedFromCxFile(fileItem)) {
            fileItem->b.isInterface = isInterface;
            // Set it to none, it will be updated by source item
            fileItem->b.sourceFileNumber = noFileIndex;
        }
        if (options.taskRegime == RegimeEditServer) {
            fileItem->b.commandLineEntered = commandLineFlag;
        }
        if (fileItem->lastFullUpdateMtime == 0) fileItem->lastFullUpdateMtime=fumtime;
        if (fileItem->lastUpdateMtime == 0) fileItem->lastUpdateMtime=umtime;
        //&if (fumtime>fileItem->lastFullUpdateMtime) fileItem->lastFullUpdateMtime=fumtime;
        //&if (umtime>fileItem->lastUpdateMtime) fileItem->lastUpdateMtime=umtime;
    }
    fileItem->b.isFromCxfile = true;
    s_decodeFilesNum[ii]=fileIndex;
    log_trace("%d: '%s' scanned: added as %d",ii,id,fileIndex);
}

static void cxrfSourceIndex(int size,
                            int marker,
                            CharacterBuffer *cb,
                            int genFl
                            ) {
    int file, sfile;

    assert(marker == CXFI_SOURCE_INDEX);
    file = lastIncomingInfo.values[CXFI_FILE_INDEX];
    file = s_decodeFilesNum[file];
    sfile = lastIncomingInfo.values[CXFI_SOURCE_INDEX];
    sfile = s_decodeFilesNum[sfile];
    assert(file>=0 && file<MAX_FILES && fileTable.tab[file]);
    // hmmm. here be more generous in getting corrct source info
    if (fileTable.tab[file]->b.sourceFileNumber == noFileIndex) {
        //&fprintf(dumpOut,"setting %d source to %d\n", file, sfile);fflush(dumpOut);
        //&fprintf(dumpOut,"setting %s source to %s\n", fileTable.tab[file]->name, fileTable.tab[sfile]->name);fflush(dumpOut);
        // first check that it is not set directly from source
        if (! fileTable.tab[file]->b.cxLoading) {
            fileTable.tab[file]->b.sourceFileNumber = sfile;
        }
    }
}

static int scanSymNameString(int size,
                             CharacterBuffer *cb,
                             char *id) {
    int i, len;
    char ch;

    for (i=0; i<size-1; i++) {
        ch = getChar(cb);
        id[i] = ch;
    }
    id[i] = 0;
    len = i;
    assert(len+1 < MAX_CX_SYMBOL_SIZE);

    return len;
}


static void getSymTypeAndClasses(int *_symType, int *_vApplClass,
                                 int *_vFunClass) {
    int symType, vApplClass, vFunClass;
    symType = lastIncomingInfo.values[CXFI_SYMBOL_TYPE];
    vApplClass = lastIncomingInfo.values[CXFI_SUBCLASS];
    vApplClass = s_decodeFilesNum[vApplClass];
    assert(fileTable.tab[vApplClass] != NULL);
    vFunClass = lastIncomingInfo.values[CXFI_SUPERCLASS];
    vFunClass = s_decodeFilesNum[vFunClass];
    assert(fileTable.tab[vFunClass] != NULL);
    *_symType = symType;
    *_vApplClass = vApplClass;
    *_vFunClass = vFunClass;
}



static void cxrfSymbolNameForFullUpdateSchedule(int size,
                                                int marker,
                                                CharacterBuffer *cb,
                                                int additionalArg
                                                ) {
    SymbolReferenceItem *ddd, *memb;
    int si, symType, len, rr, vApplClass, vFunClass, accessFlags;
    int storage;
    char *id;
    char *ss;

    assert(marker == CXFI_SYMBOL_NAME);
    accessFlags = lastIncomingInfo.values[CXFI_ACCESS_BITS];
    storage = lastIncomingInfo.values[CXFI_STORAGE];
    si = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    assert(si>=0 && si<MAX_CX_SYMBOL_TAB);
    id = lastIncomingInfo.cachedSymbolName[si];
    len = scanSymNameString(size, cb, id);
    getSymTypeAndClasses( &symType, &vApplClass, &vFunClass);
    //&fprintf(dumpOut,":scanning ref of %s %d %d: \n",id,symType,vFunClass);fflush(dumpOut);
    if (symType!=TypeCppInclude || strcmp(id, LINK_NAME_INCLUDE_REFS)!=0) {
        lastIncomingInfo.onLineReferencedSym = -1;
        return;
    }
    ddd = &lastIncomingInfo.cachedSymbolReferenceItem[si];
    lastIncomingInfo.symbolTab[si] = ddd;
    fillSymbolRefItem(ddd, id, cxFileHashNumber(id), //useless, put 0
                      vApplClass, vFunClass);
    fillSymbolRefItemBits(&ddd->b, symType, storage, ScopeGlobal, accessFlags, CategoryGlobal);
    rr = refTabIsMember(&referenceTable, ddd, NULL, &memb);
    if (rr == 0) {
        CX_ALLOCC(ss, len+1, char);
        strcpy(ss,id);
        CX_ALLOC(memb, SymbolReferenceItem);
        fillSymbolRefItem(memb,ss, cxFileHashNumber(ss),
                                    vApplClass, vFunClass);
        fillSymbolRefItemBits(&memb->b, symType, storage,
                               ScopeGlobal, accessFlags, CategoryGlobal);
        refTabAdd(&referenceTable, memb);
    }
    lastIncomingInfo.symbolTab[si] = memb;
    lastIncomingInfo.onLineReferencedSym = si;
}

static void cxfileCheckLastSymbolDeadness(void) {
    if (lastIncomingInfo.symbolToCheckForDeadness != -1
        && lastIncomingInfo.deadSymbolIsDefined) {
        //&sprintf(tmpBuff,"adding %s storage==%s", lastIncomingInfo.symbolTab[lastIncomingInfo.symbolToCheckForDeadness]->name, storagesName[lastIncomingInfo.symbolTab[lastIncomingInfo.symbolToCheckForDeadness]->b.storage]);ppcGenRecord(PPC_INFORMATION, tmpBuff);
        olAddBrowsedSymbol(lastIncomingInfo.symbolTab[lastIncomingInfo.symbolToCheckForDeadness],
                           &s_olcxCurrentUser->browserStack.top->hkSelectedSym,
                           1,1,0,UsageDefined,0, &s_noPos, UsageDefined);
    }
}


static bool symbolIsReportableAsDead(SymbolReferenceItem *ss) {
    if (ss==NULL || ss->name[0]==' ')
        return false;

    // you need to be strong here, in fact struct record can be used
    // without using struct explicitly
    if (ss->b.symType == TypeStruct)
        return false;

    // maybe I should collect also all toString() references?
    if (ss->b.storage==StorageMethod && strcmp(ss->name,"toString()")==0)
        return false;

    // in this first approach restrict this to variables and functions
    if (ss->b.symType == TypeMacro)
        return false;
    return true;
}

static bool canBypassAcceptableSymbol(SymbolReferenceItem *symbol) {
    int nlen,len;
    char *nn, *nnn;

    GET_BARE_NAME(symbol->name, nn, len);
    GET_BARE_NAME(options.browsedSymName, nnn, nlen);
    if (len != nlen)
        return false;
    if (strncmp(nn, nnn, len))
        return false;
    return true;
}

static void cxrfSymbolName(int size,
                           int marker,
                           CharacterBuffer *cb,
                           int additionalArg
                           ) {
    SymbolReferenceItem *ddd, *memb;
    SymbolsMenu *cms;
    int si, symType, rr, vApplClass, vFunClass, ols, accessFlags, storage;
    char *id;

    assert(marker == CXFI_SYMBOL_NAME);
    if (options.taskRegime==RegimeEditServer && additionalArg==DEAD_CODE_DETECTION) {
        // check if previous symbol was dead
        cxfileCheckLastSymbolDeadness();
    }
    accessFlags = lastIncomingInfo.values[CXFI_ACCESS_BITS];
    storage = lastIncomingInfo.values[CXFI_STORAGE];
    si = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    assert(si>=0 && si<MAX_CX_SYMBOL_TAB);
    id = lastIncomingInfo.cachedSymbolName[si];
    scanSymNameString( size, cb, id);
    getSymTypeAndClasses(&symType, &vApplClass, &vFunClass);

    ddd = &lastIncomingInfo.cachedSymbolReferenceItem[si];
    lastIncomingInfo.symbolTab[si] = ddd;
    fillSymbolRefItem(ddd,id,
                                cxFileHashNumber(id), // useless put 0
                                vApplClass, vFunClass);
    fillSymbolRefItemBits(&ddd->b,symType, storage, ScopeGlobal, accessFlags,
                           CategoryGlobal);
    rr = refTabIsMember(&referenceTable, ddd, NULL, &memb);
    while (rr && memb->b.category!=CategoryGlobal) rr=refTabNextMember(ddd, &memb);
    assert(options.taskRegime);
    if (options.taskRegime == RegimeXref) {
        if (memb==NULL) memb=ddd;
        genRefItem0(memb, true);
        ddd->refs = memb->refs; // note references to not generate multiple
        memb->refs = NULL;      // HACK, remove them, to not be regenerated
    }
    if (options.taskRegime == RegimeEditServer) {
        if (additionalArg == DEAD_CODE_DETECTION) {
            if (symbolIsReportableAsDead(lastIncomingInfo.symbolTab[si])) {
                lastIncomingInfo.symbolToCheckForDeadness = si;
                lastIncomingInfo.deadSymbolIsDefined = 0;
            } else {
                lastIncomingInfo.symbolToCheckForDeadness = -1;
            }
        } else if (options.server_operation!=OLO_TAG_SEARCH) {
            cms = NULL; ols = 0;
            if (additionalArg == CX_MENU_CREATION) {
                cms = createSelectionMenu(ddd);
                if (cms == NULL) {
                    ols = 0;
                } else {
                    if (IS_BEST_FIT_MATCH(cms)) ols = 2;
                    else ols = 1;
                }
            } else if (additionalArg!=CX_BY_PASS) {
                ols=itIsSymbolToPushOlReferences(ddd,s_olcxCurrentUser->browserStack.top,&cms,DEFAULT_VALUE);
            }
            lastIncomingInfo.onLineRefMenuItem = cms;
            if (ols || (additionalArg==CX_BY_PASS && canBypassAcceptableSymbol(ddd))
                ) {
                lastIncomingInfo.onLineReferencedSym = si;
                lastIncomingInfo.onLineRefIsBestMatchFlag = (ols == 2);
                log_trace("symbol %s is O.K. for %s (ols==%d)", ddd->name, options.browsedSymName, ols);
            } else {
                if (lastIncomingInfo.onLineReferencedSym == si) {
                    lastIncomingInfo.onLineReferencedSym = -1;
                }
            }
        }
    }
}

static void cxrfReferenceForFullUpdateSchedule(int size,
                                               int marker,
                                               CharacterBuffer *cb,
                                               int additionalArg
                                               ) {
    Position pos;
    UsageBits usageBits;
    int file, line, col, usage, sym, vApplClass, vFunClass;
    int symType,reqAcc;

    assert(marker == CXFI_REFERENCE);
    usage = lastIncomingInfo.values[CXFI_USAGE];
    reqAcc = lastIncomingInfo.values[CXFI_REQUIRED_ACCESS];
    fillUsageBits(&usageBits, usage, reqAcc);
    sym = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    file = lastIncomingInfo.values[CXFI_FILE_INDEX];
    file = s_decodeFilesNum[file];
    assert(fileTable.tab[file]!=NULL);
    line = lastIncomingInfo.values[CXFI_LINE_INDEX];
    col = lastIncomingInfo.values[CXFI_COLUMN_INDEX];
    getSymTypeAndClasses( &symType, &vApplClass, &vFunClass);
    log_trace("%d %d->%d %d", usage, file, s_decodeFilesNum[file], line);
    pos = makePosition(file, line, col);
    if (lastIncomingInfo.onLineReferencedSym ==
        lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
        addToRefList(&lastIncomingInfo.symbolTab[sym]->refs,
                     &usageBits,&pos);
    }
}

static bool isInRefList(Reference *list,
                        UsageBits *pusage,
                        Position *pos) {
    Reference *rr;
    Reference ppp;
    fillReference(&ppp, *pusage, *pos, NULL);
    SORTED_LIST_FIND2(rr, Reference, ppp, list);
    if (rr==NULL || SORTED_LIST_NEQ(rr,ppp))
        return false;
    return true;
}


static void cxrfReference(int size,
                          int marker,
                          CharacterBuffer *cb,
                          int additionalArg
                          ) {
    Position pos;
    Reference rr;
    UsageBits usageBits;
    int file, line, col, usage, sym, reqAcc;
    int copyrefFl;

    assert(marker == CXFI_REFERENCE);
    usage = lastIncomingInfo.values[CXFI_USAGE];
    reqAcc = lastIncomingInfo.values[CXFI_REQUIRED_ACCESS];
    sym = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    file = lastIncomingInfo.values[CXFI_FILE_INDEX];
    file = s_decodeFilesNum[file];
    assert(fileTable.tab[file]!=NULL);
    line = lastIncomingInfo.values[CXFI_LINE_INDEX];
    col = lastIncomingInfo.values[CXFI_COLUMN_INDEX];
    /*&fprintf(dumpOut,"%d %d %d  ", usage,file,line);fflush(dumpOut);&*/
    assert(options.taskRegime);
    if (options.taskRegime == RegimeXref) {
        if (fileTable.tab[file]->b.cxLoading&&fileTable.tab[file]->b.cxSaved) {
            /* if we repass refs after overflow */
            pos = makePosition(file, line, col);
            fillUsageBits(&usageBits, usage, reqAcc);
            copyrefFl = ! isInRefList(lastIncomingInfo.symbolTab[sym]->refs,
                                      &usageBits, &pos);
        } else {
            copyrefFl = ! fileTable.tab[file]->b.cxLoading;
        }
        if (copyrefFl)
            writeCxReferenceBase(sym, usage, reqAcc, file, line, col);
    } else if (options.taskRegime == RegimeEditServer) {
        pos = makePosition(file, line, col);
        fillUsageBits(&usageBits, usage, reqAcc);
        fillReference(&rr, usageBits, pos, NULL);
        if (additionalArg == DEAD_CODE_DETECTION) {
            if (OL_VIEWABLE_REFS(&rr)) {
                // restrict reported symbols to those defined in project
                // input file
                if (IS_DEFINITION_USAGE(rr.usage.base)
                    && fileTable.tab[rr.p.file]->b.commandLineEntered
                    ) {
                    lastIncomingInfo.deadSymbolIsDefined = 1;
                } else if (! IS_DEFINITION_OR_DECL_USAGE(rr.usage.base)) {
                    lastIncomingInfo.symbolToCheckForDeadness = -1;
                }
            }
        } else if (additionalArg == OL_LOOKING_2_PASS_MACRO_USAGE) {
            if (    lastIncomingInfo.onLineReferencedSym ==
                    lastIncomingInfo.values[CXFI_SYMBOL_INDEX]
                    &&  rr.usage.base == UsageMacroBaseFileUsage) {
                s_olMacro2PassFile = rr.p.file;
            }
        } else {
            if (options.server_operation == OLO_TAG_SEARCH) {
                if (rr.usage.base==UsageDefined
                    || ((options.tagSearchSpecif==TSS_FULL_SEARCH
                         || options.tagSearchSpecif==TSS_FULL_SEARCH_SHORT)
                        &&  (rr.usage.base==UsageDeclared
                             || rr.usage.base==UsageClassFileDefinition))) {
                    searchSymbolCheckReference(lastIncomingInfo.symbolTab[sym],&rr);
                }
            } else if (options.server_operation == OLO_SAFETY_CHECK1) {
                if (    lastIncomingInfo.onLineReferencedSym !=
                        lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
                    olcxCheck1CxFileReference(lastIncomingInfo.symbolTab[sym],
                                              &rr);
                }
            } else {
                if (    lastIncomingInfo.onLineReferencedSym ==
                        lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
                    if (additionalArg == CX_MENU_CREATION) {
                        assert(lastIncomingInfo.onLineRefMenuItem);
                        if (file!=s_olOriginalFileNumber
                            || !fileTable.tab[file]->b.commandLineEntered
                            || options.server_operation==OLO_GOTO
                            || options.server_operation==OLO_CGOTO
                            || options.server_operation==OLO_PUSH_NAME
                            || options.server_operation==OLO_PUSH_SPECIAL_NAME
                            ) {
                            //&fprintf(dumpOut,":adding reference %s:%d\n", fileTable.tab[rr.p.file]->name, rr.p.line);
                            olcxAddReferenceToSymbolsMenu(lastIncomingInfo.onLineRefMenuItem, &rr, lastIncomingInfo.onLineRefIsBestMatchFlag);
                        }
                    } else if (additionalArg == CX_BY_PASS) {
                        if (positionsAreEqual(s_olcxByPassPos,rr.p)) {
                            // got the bypass reference
                            //&fprintf(dumpOut,":adding bypass selected symbol %s\n", lastIncomingInfo.symbolTab[sym]->name);
                            olAddBrowsedSymbol(lastIncomingInfo.symbolTab[sym],
                                               &s_olcxCurrentUser->browserStack.top->hkSelectedSym,
                                               1, 1, 0, usage,0,&s_noPos, UsageNone);
                        }
                    } else {
                        olcxAddReference(&s_olcxCurrentUser->browserStack.top->references, &rr,
                                         lastIncomingInfo.onLineRefIsBestMatchFlag);
                    }
                }
            }
        }
    }
}


static void cxrfRefNum(int fileRefNum,
                       int marker,
                       CharacterBuffer *cb,
                       int additionalArg
                       ) {
    int check;

    check = checkReferenceFileCountOption(fileRefNum);
    if (check == 0) {
        assert(options.taskRegime);
        fatalError(ERR_ST,"Tag file was generated with different '-refnum' options, recreate it!", XREF_EXIT_ERR);
    }
}

static void cxrfSubClass(int size,
                         int marker,
                         CharacterBuffer *cb,
                         int additionalArg
                         ) {
    int fileIndex;
    int super_class, sub_class;

    assert(marker == CXFI_CLASS_EXT);
    fileIndex = lastIncomingInfo.values[CXFI_FILE_INDEX];
    super_class = lastIncomingInfo.values[CXFI_SUPERCLASS];
    sub_class = lastIncomingInfo.values[CXFI_SUBCLASS];
    /*fprintf(dumpOut,"%d %d->%d %d  ", usage,file,s_decodeFilesNum[file],line);*/
    /*fflush(dumpOut);*/

    fileIndex = s_decodeFilesNum[fileIndex];
    assert(fileTable.tab[fileIndex]!=NULL);

    super_class = s_decodeFilesNum[super_class];
    assert(fileTable.tab[super_class]!=NULL);
    sub_class = s_decodeFilesNum[sub_class];
    assert(fileTable.tab[sub_class]!=NULL);
    assert(options.taskRegime);

    switch (options.taskRegime) {
    case RegimeXref:
        if (!fileTable.tab[fileIndex]->b.cxLoading &&
            additionalArg==CX_GENERATE_OUTPUT) {
            writeSubClassInfo(super_class, sub_class, fileIndex);  // updating refs
        }
        break;
    case RegimeEditServer:
        if (fileIndex!=s_input_file_number) {
            log_trace("reading %s < %s", simpleFileName(fileTable.tab[sub_class]->name),
                      simpleFileName(fileTable.tab[super_class]->name));
            createSubClassInfo(super_class, sub_class, fileIndex, NO_CX_FILE_ITEM_GEN);
        }
        break;
    default:
        assert(0);              /* Undefined & Refactory should not happen? */
    }
}


static int scanInteger(CharacterBuffer *cb, int *_ch) {
    int scannedInt, ch = *_ch;
    ch = skipWhiteSpace(cb, ch);
    scannedInt = 0;
    while (isdigit(ch)) {
        scannedInt = scannedInt*10 + ch-'0';
        ch = getChar(&cxfCharacterBuffer);
    }
    *_ch = ch;

    return scannedInt;
}



void scanCxFile(ScanFileFunctionStep *scanningFunctions) {
    int scannedInt = 0;
    int ch,i;

    ENTER();
    if (inputFile == NULL) {
        log_trace("No input file");
        LEAVE();
        return;
    }

    memset(&lastIncomingInfo, 0, sizeof(lastIncomingInfo));
    lastIncomingInfo.onLineReferencedSym = -1;
    lastIncomingInfo.symbolToCheckForDeadness = -1;
    lastIncomingInfo.onLineRefMenuItem = NULL;
    lastIncomingInfo.markers[CXFI_SUBCLASS] = noFileIndex;
    lastIncomingInfo.markers[CXFI_SUPERCLASS] = noFileIndex;
    s_decodeFilesNum[noFileIndex] = noFileIndex;

    for(i=0; scanningFunctions[i].recordCode>0; i++) {
        assert(scanningFunctions[i].recordCode < MAX_CHARS);
        ch = scanningFunctions[i].recordCode;
        lastIncomingInfo.fun[ch] = scanningFunctions[i].handleFun;
        lastIncomingInfo.additional[ch] = scanningFunctions[i].additionalArg;
    }

    initCharacterBuffer(&cxfCharacterBuffer, inputFile);
    ch = ' ';
    while(! cxfCharacterBuffer.isAtEOF) {
        CharacterBuffer *cb = &cxfCharacterBuffer;
        scannedInt = scanInteger(cb, &ch);

        if (cxfCharacterBuffer.isAtEOF)
            break;

        assert(ch >= 0 && ch<MAX_CHARS);
        if (lastIncomingInfo.markers[ch]) {
            lastIncomingInfo.values[ch] = scannedInt;
        }
        if (lastIncomingInfo.fun[ch] != NULL) {
            (*lastIncomingInfo.fun[ch])(scannedInt, ch, &cxfCharacterBuffer,
                                     lastIncomingInfo.additional[ch]);
        } else if (! lastIncomingInfo.markers[ch]) {
            assert(scannedInt>0);
            skipCharacters(cb, scannedInt-1);
            //& CxSkipNChars(scannedInt-1, next, end, cb);
            /* { */
            /*     int ccount = scannedInt-1; */
            /*     while (cb->next + ccount > cb->end) { */
            /*         ccount -= cb->end - cb->next; */
            /*         cb->next = cb->end; */
            /*         ch = getChar(cb); */
            /*         ccount --; */
            /*     } */
            /*     cb->next += ccount; */
            /* } */
        }
        ch = getChar(cb);
    }

    if (options.taskRegime==RegimeEditServer
        && (options.server_operation==OLO_LOCAL_UNUSED
            || options.server_operation==OLO_GLOBAL_UNUSED)) {
        // check if last symbol was dead
        cxfileCheckLastSymbolDeadness();
    }

    LEAVE();
}


/* suffix contains '/' at the beginning !!! */
bool scanReferenceFile(char *fileName, char *suffix1, char *suffix2,
                      ScanFileFunctionStep *scanFunTab) {
    char fn[MAX_FILE_NAME_SIZE];

    sprintf(fn, "%s%s%s", fileName, suffix1, suffix2);
    assert(strlen(fn) < MAX_FILE_NAME_SIZE-1);
    log_trace(":scanning file %s", fn);
    inputFile = openFile(fn, "r");
    if (inputFile==NULL) {
        return false;
    } else {
        scanCxFile(scanFunTab);
        closeFile(inputFile);
        inputFile = NULL;
        return true;
    }
}

void scanReferenceFiles(char *fname, ScanFileFunctionStep *scanFunTab) {
    char nn[MAX_FILE_NAME_SIZE];
    int i;

    if (options.referenceFileCount <= 1) {
        scanReferenceFile(fname,"","",scanFunTab);
    } else {
        scanReferenceFile(fname,REFERENCE_FILENAME_FILES,"",scanFunTab);
        scanReferenceFile(fname,REFERENCE_FILENAME_CLASSES,"",scanFunTab);
        for (i=0; i<options.referenceFileCount; i++) {
            sprintf(nn,"%04d",i);
            scanReferenceFile(fname,REFERENCE_FILENAME_PREFIX,nn,scanFunTab);
        }
    }
}

bool smartReadFileTabFile(void) {
    static time_t savedModificationTime = 0; /* Cache previously read file data... */
    static off_t savedFileSize = 0;
    static char previouslyReadFileName[MAX_FILE_NAME_SIZE] = ""; /* ... and name */
    char fileName[MAX_FILE_NAME_SIZE];

    if (options.referenceFileCount <= 1) {
        sprintf(fileName, "%s", options.cxrefFileName);
    } else {
        sprintf(fileName, "%s%s", options.cxrefFileName, REFERENCE_FILENAME_FILES);
    }
    if (editorFileExists(fileName)) {
        size_t currentSize = editorFileSize(fileName);
        time_t currentModificationTime = editorFileModificationTime(fileName);
        if (strcmp(previouslyReadFileName, fileName) != 0
            || savedModificationTime != currentModificationTime
            || savedFileSize != currentSize)
        {
            log_trace(":(re)reading file tab");
            if (scanReferenceFile(fileName, "", "", normalScanFunctionSequence)) {
                strcpy(previouslyReadFileName, fileName);
                savedModificationTime = currentModificationTime;
                savedFileSize = currentSize;
            }
        } else {
            log_trace(":saving the (re)reading of file tab");
        }
        return true;
    }
    return false;
}

// symbolName can be NULL !!!!!!
void readOneAppropReferenceFile(char *symbolName,
                                ScanFileFunctionStep  *scanFileFunctionTable
) {
    static char fns[MAX_FILE_NAME_SIZE];
    int i;

    if (options.cxrefFileName == NULL)
        return;
    cxOut = stdout;
    if (options.referenceFileCount <= 1) {
        scanReferenceFile(options.cxrefFileName,"","",scanFileFunctionTable);
    } else {
        if (!smartReadFileTabFile())
            return;
        if (!scanReferenceFile(options.cxrefFileName, REFERENCE_FILENAME_CLASSES, "",
                               scanFileFunctionTable))
            return;
        if (symbolName == NULL)
            return;

        /* following must be after reading XFiles*/
        i = cxFileHashNumber(symbolName);

        sprintf(fns, "%04d", i);
        assert(strlen(fns) < MAX_FILE_NAME_SIZE-1);
        scanReferenceFile(options.cxrefFileName, REFERENCE_FILENAME_PREFIX, fns,
                          scanFileFunctionTable);
    }
}

/* ************************************************************ */


ScanFileFunctionStep normalScanFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
    {CXFI_REFNUM, cxrfRefNum, 0},
    {-1,NULL, 0},
};

ScanFileFunctionStep fullScanFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_VERSION, cxrfVersionCheck, 0},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
    {CXFI_FILE_NAME, cxReadFileName, CX_GENERATE_OUTPUT},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_GENERATE_OUTPUT},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, DEFAULT_VALUE},
    {CXFI_REFERENCE, cxrfReference, CX_FIRST_PASS},
    {CXFI_CLASS_EXT, cxrfSubClass, CX_GENERATE_OUTPUT},
    {CXFI_REFNUM, cxrfRefNum, 0},
    {-1,NULL, 0},
};

ScanFileFunctionStep byPassFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_VERSION, cxrfVersionCheck, 0},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, CX_BY_PASS},
    {CXFI_REFERENCE, cxrfReference, CX_BY_PASS},
    {CXFI_CLASS_EXT, cxrfSubClass, CX_JUST_READ},
    {CXFI_REFNUM, cxrfRefNum, 0},
    {-1,NULL, 0},
};

ScanFileFunctionStep symbolLoadMenuRefsFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_VERSION, cxrfVersionCheck, 0},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, DEFAULT_VALUE},
    {CXFI_REFERENCE, cxrfReference, CX_MENU_CREATION},
    {CXFI_CLASS_EXT, cxrfSubClass, CX_JUST_READ},
    {CXFI_REFNUM, cxrfRefNum, 0},
    {-1,NULL, 0},
};

ScanFileFunctionStep symbolMenuCreationFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_VERSION, cxrfVersionCheck, 0},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, CX_MENU_CREATION},
    {CXFI_REFERENCE, cxrfReference, CX_MENU_CREATION},
    {CXFI_CLASS_EXT, cxrfSubClass, CX_JUST_READ},
    {CXFI_REFNUM, cxrfRefNum, 0},
    {-1,NULL, 0},
};

ScanFileFunctionStep fullUpdateFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_VERSION, cxrfVersionCheck, 0},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, 0},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolNameForFullUpdateSchedule, DEFAULT_VALUE},
    {CXFI_REFERENCE, cxrfReferenceForFullUpdateSchedule, DEFAULT_VALUE},
    {CXFI_REFNUM, cxrfRefNum, 0},
    {-1,NULL, 0},
};

ScanFileFunctionStep secondPassMacroUsageFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, DEFAULT_VALUE},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, OL_LOOKING_2_PASS_MACRO_USAGE},
    {CXFI_REFERENCE, cxrfReference, OL_LOOKING_2_PASS_MACRO_USAGE},
    {CXFI_CLASS_EXT, cxrfSubClass, CX_JUST_READ},
    {CXFI_REFNUM, cxrfRefNum, DEFAULT_VALUE},
    {-1,NULL, 0},
};

ScanFileFunctionStep classHierarchyFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, DEFAULT_VALUE},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_CLASS_EXT, cxrfSubClass, CX_JUST_READ},
    {CXFI_REFNUM, cxrfRefNum, DEFAULT_VALUE},
    {-1,NULL, 0},
};

ScanFileFunctionStep symbolSearchFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_REFNUM, cxrfRefNum, 0},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, SEARCH_SYMBOL},
    {CXFI_REFERENCE, cxrfReference, CX_FIRST_PASS},
    {-1,NULL, 0},
};

ScanFileFunctionStep deadCodeDetectionFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_REFNUM, cxrfRefNum, 0},
    {CXFI_FILE_NAME, cxReadFileName, CX_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CX_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, DEAD_CODE_DETECTION},
    {CXFI_REFERENCE, cxrfReference, DEAD_CODE_DETECTION},
    {-1,NULL, 0},
};

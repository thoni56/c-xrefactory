#include "cxfile.h"

#include "globals.h"
#include "commons.h"
#include "options.h"
#include "protocol.h"           /* C_XREF_FILE_VERSION_NUMBER */

#include "cxref.h"
#include "reftab.h"
#include "olcxtab.h"
#include "classhierarchy.h"
#include "fileio.h"
#include "list.h"
#include "filetable.h"

#include "misc.h"
#include "hash.h"
#include "usage.h"
#include "log.h"


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


typedef enum {
    CXSF_UNUSED = DO_NOT_LOAD_SUPER + 1,
    CXSF_DEFAULT,
    CXSF_JUST_READ,
    CXSF_GENERATE_OUTPUT,
    CXSF_BY_PASS,
    CXSF_FIRST_PASS,
    CXSF_MENU_CREATION,
    CXSF_DEAD_CODE_DETECTION,
    CXSF_PASS_MACRO_USAGE,
} CxScanFileOperation;


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
    SymbolsMenu         *onLineRefMenuItem;
    int                 onLineRefIsBestMatchFlag; // vyhodit ?
    ReferencesItem *symbolTab[MAX_CX_SYMBOL_TAB];
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
    ReferencesItem     cachedSymbolReferenceItem[MAX_CX_SYMBOL_TAB];
    char                cachedSymbolName[MAX_CX_SYMBOL_TAB][MAX_CX_SYMBOL_SIZE];
} LastCxFileInfo;

static LastCxFileInfo lastIncomingInfo;
static LastCxFileInfo lastOutgoingInfo;


static CharacterBuffer cxFileCharacterBuffer;

static unsigned decodeFileNumbers[MAX_FILES];

static char tmpFileName[MAX_FILE_NAME_SIZE];


typedef struct scanFileFunctionStep {
    int		recordCode;
    void    (*handleFun)(int size, int ri, CharacterBuffer *cb, int additionalArg); /* TODO: Break out a type */
    int		additionalArg;
} ScanFileFunctionStep;


static ScanFileFunctionStep normalScanFunctionSequence[];
static ScanFileFunctionStep fullScanFunctionSequence[];
static ScanFileFunctionStep fullUpdateFunctionSequence[];
static ScanFileFunctionStep byPassFunctionSequence[];
static ScanFileFunctionStep symbolMenuCreationFunctionSequence[];
static ScanFileFunctionStep secondPassMacroUsageFunctionSequence[];
static ScanFileFunctionStep classHierarchyFunctionSequence[];
static ScanFileFunctionStep globalUnusedDetectionFunctionSequence[];
static ScanFileFunctionStep symbolSearchFunctionSequence[];

static void scanCxFile(ScanFileFunctionStep *scanFuns);


static void fPutDecimal(FILE *file, int num) {
    fprintf(file, "%d", num);
}


/* *********************** INPUT/OUTPUT ************************** */

int cxFileHashNumber(char *sym) {
    unsigned   hash;
    char       *ss;
    int        c;

    if (options.referenceFileCount <= 1)
        return 0;

    hash = 0;
    ss = sym;
    while ((c = *ss)) {
        if (c == '(') break;
        SYMTAB_HASH_FUN_INC(hash, c);
        if (LINK_NAME_MAYBE_START(c))
            hash = 0;
        ss++;
    }
    SYMTAB_HASH_FUN_FINAL(hash);
    hash %= options.referenceFileCount;
    return hash;
}

static bool searchSingleStringEqual(char *s, char *c) {
    while (*s!=0 && *s!=' ' && *s!='\t' && tolower(*s)==tolower(*c)) {
        c++; s++;
    }
    if (*s==0 || *s==' ' || *s=='\t')
        return true;
    return false;
}

static bool searchSingleStringFitness(char *cxtag, char *searchedStr, int len) {
    char *cc;
    int i, pilotc;

    assert(searchedStr);
    pilotc = tolower(*searchedStr);
    if (pilotc == '^') {
        // check for exact prefix
        return searchSingleStringEqual(searchedStr+1, cxtag);
    } else {
        cc = cxtag;
        for (cc=cxtag, i=0; *cc && i<len; cc++,i++) {
            if (searchSingleStringEqual(searchedStr, cc))
                return true;
        }
    }
    return false;
}

static bool searchStringNonWildcardFitness(char *cxtag, int len) {
    char    *ss;
    int     r;

    ss = options.olcxSearchString;
    while (*ss) {
        while (*ss==' ' || *ss=='\t') ss++;
        if (*ss == 0) goto fini1;
        r = searchSingleStringFitness(cxtag, ss, len);
        if (r==0)
            return false;
        while (*ss!=0 && *ss!=' ' && *ss!='\t')
            ss++;
    }
 fini1:
    return true;
}

bool searchStringFitness(char *cxtag, int len) {
    if (containsWildcard(options.olcxSearchString))
        return shellMatch(cxtag, len, options.olcxSearchString, false);
    else
        return searchStringNonWildcardFitness(cxtag, len);
}


#define maxOf(a, b) (((a) > (b)) ? (a) : (b))

char *createTagSearchLineStatic(char *name, Position *position,
                                int *len1, int *len2, int *len3) {
    static char line[2*COMPLETION_STRING_SIZE];
    char file[TMP_STRING_SIZE];
    char dir[TMP_STRING_SIZE];
    char *ffname;
    int l1,l2,l3,fl, dl;

    l1 = l2 = l3 = 0;
    l1 = strlen(name);

    FileItem *fileItem = getFileItem(position->file);
    ffname = fileItem->name;
    assert(ffname);
    ffname = getRealFileName_static(ffname);
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
        sprintf(line, "%s", name);
    } else {
        sprintf(line, "%-*s :%-*s :%s", *len1, name, *len3, file, dir);
    }
    return line;                /* static! */
}

// Filter out symbols which pollute search reports
bool symbolNameShouldBeHiddenFromReports(char *name) {
    char *s;

    // internal xref symbol?
    if (name[0] == ' ')
        return true;

    // Only Java specific pollution below
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

void searchSymbolCheckReference(ReferencesItem  *referenceItem, Reference *reference) {
    char ssname[MAX_CX_SYMBOL_SIZE];
    char *s, *sname;
    int slen;

    if (referenceItem->bits.symType == TypeCppInclude)
        return;   // no %%i symbols
    if (symbolNameShouldBeHiddenFromReports(referenceItem->name))
        return;

    linkNamePrettyPrint(ssname, referenceItem->name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
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
        olCompletionListPrepend(sname, NULL, NULL, 0, NULL, referenceItem, reference, referenceItem->bits.symType,
                                referenceItem->vFunClass, currentUserData->retrieverStack.top);
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

static void get_version_string(char *string) {
    sprintf(string," file format: C-xrefactory %s ", C_XREF_FILE_VERSION_NUMBER);
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
    ReferencesItem *d;

    /* First the symbol info, if not done already */
    writeOptionalCompactRecord(CXFI_SYMBOL_INDEX, symbolIndex, "");

    /* Then the reference info */
    d = lastOutgoingInfo.symbolTab[symbolIndex];
    writeOptionalCompactRecord(CXFI_SYMBOL_TYPE, d->bits.symType, "\n"); /* Why newline in the middle of all this? */
    writeOptionalCompactRecord(CXFI_SUBCLASS, d->vApplClass, "");
    writeOptionalCompactRecord(CXFI_SUPERCLASS, d->vFunClass, "");
    writeOptionalCompactRecord(CXFI_ACCESS_BITS, d->bits.accessFlags, "");
    writeOptionalCompactRecord(CXFI_STORAGE, d->bits.storage, "");
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

static void writeCxReferenceBase(int symbolIndex, UsageKind usage, int requiredAccess, int file, int line, int col) {
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
    writeCxReferenceBase(symbolNum, reference->usage.kind, reference->usage.requiredAccess,
                         reference->position.file, reference->position.line, reference->position.col);
}

static void writeSubClassInfo(int superior, int inferior, int origin) {
    writeOptionalCompactRecord(CXFI_FILE_INDEX, origin, "\n");
    writeOptionalCompactRecord(CXFI_SUPERCLASS, superior, "");
    writeOptionalCompactRecord(CXFI_SUBCLASS, inferior, "");
    writeCompactRecord(CXFI_CLASS_EXT, 0, "");
}

static void writeFileIndexItem(FileItem *fileItem, int index) {
    writeOptionalCompactRecord(CXFI_FILE_INDEX, index, "\n");
    writeOptionalCompactRecord(CXFI_FILE_UMTIME, fileItem->lastUpdateMtime, " ");
    writeOptionalCompactRecord(CXFI_FILE_FUMTIME, fileItem->lastFullUpdateMtime, " ");
    writeOptionalCompactRecord(CXFI_INPUT_FROM_COMMAND_LINE, fileItem->bits.commandLineEntered, "");
    if (fileItem->bits.isInterface) {
        writeOptionalCompactRecord(CXFI_ACCESS_BITS, AccessInterface, "");
    } else {
        writeOptionalCompactRecord(CXFI_ACCESS_BITS, AccessDefault, "");
    }
    writeStringRecord(CXFI_FILE_NAME, fileItem->name, " ");
}

static void writeFileSourceIndexItem(FileItem *fileItem, int index) {
    if (fileItem->bits.sourceFileNumber != noFileIndex) {
        writeOptionalCompactRecord(CXFI_FILE_INDEX, index, "\n");
        writeCompactRecord(CXFI_SOURCE_INDEX, fileItem->bits.sourceFileNumber, " ");
    }
}

static void genClassHierarchyItems(FileItem *fileItem, int index) {
    ClassHierarchyReference *p;
    for (p=fileItem->superClasses; p!=NULL; p=p->next) {
        writeSubClassInfo(p->superClass, index, p->ofile);
    }
}


static ClassHierarchyReference *findSuperiorInSuperClasses(int superior, FileItem *iFile) {
    ClassHierarchyReference *p;
    for (p=iFile->superClasses; p!=NULL && p->superClass!=superior; p=p->next)
        ;

    return p;
}


static void createSubClassInfo(int superior, int inferior, int originFileIndex, int genfl) {
    ClassHierarchyReference   *p, *pp;

    FileItem *inferiorFile = getFileItem(inferior);
    FileItem *superiorFile = getFileItem(superior);

    p = findSuperiorInSuperClasses(superior, inferiorFile);
    if (p==NULL) {
        p = newClassHierarchyReference(originFileIndex, superior, inferiorFile->superClasses);
        inferiorFile->superClasses = p;
        assert(options.taskRegime);
        if (options.taskRegime == RegimeXref) {
            if (genfl == CX_FILE_ITEM_GEN)
                writeSubClassInfo(superior, inferior, originFileIndex);
        }
        pp = newClassHierarchyReference(originFileIndex, inferior, superiorFile->superClasses);
        superiorFile->inferiorClasses = pp;
    }
}

void addSubClassItemToFileTab( int sup, int inf, int originFileIndex) {
    if (sup >= 0 && inf >= 0) {
        createSubClassInfo(sup, inf, originFileIndex, NO_CX_FILE_ITEM_GEN);
    }
}


void addSubClassesItemsToFileTab(Symbol *symbol, int origin) {
    int cf1;
    SymbolList *sups;

    if (symbol->bits.symbolType != TypeStruct) return;
    /*fprintf(dumpOut,"testing %s\n",ss->name);*/
    assert(symbol->bits.javaFileIsLoaded);
    if (!symbol->bits.javaFileIsLoaded)
        return;
    cf1 = symbol->u.structSpec->classFileIndex;
    assert(cf1 >= 0 &&  cf1 < MAX_FILES);
    /*fprintf(dumpOut,"loaded: #sups == %d\n",ns);*/
    for (sups=symbol->u.structSpec->super; sups!=NULL; sups=sups->next) {
        assert(sups->d && sups->d->bits.symbolType == TypeStruct);
        addSubClassItemToFileTab(sups->d->u.structSpec->classFileIndex, cf1, origin);
    }
}

/* *************************************************************** */

static void genRefItem0(ReferencesItem *referenceItem, bool force) {
    Reference *reference;
    int symbolIndex;

    log_trace("generate cxref for symbol '%s'", referenceItem->name);
    symbolIndex = 0;
    assert(strlen(referenceItem->name)+1 < MAX_CX_SYMBOL_SIZE);

    strcpy(lastOutgoingInfo.cachedSymbolName[symbolIndex], referenceItem->name);
    fillSymbolRefItem(&lastOutgoingInfo.cachedSymbolReferenceItem[symbolIndex],
                      lastOutgoingInfo.cachedSymbolName[symbolIndex],
                      referenceItem->fileHash, // useless put 0
                      referenceItem->vApplClass, referenceItem->vFunClass);
    fillSymbolRefItemBits(&lastOutgoingInfo.cachedSymbolReferenceItem[symbolIndex].bits,
                          referenceItem->bits.symType, referenceItem->bits.storage,
                          referenceItem->bits.scope, referenceItem->bits.accessFlags, referenceItem->bits.category);
    lastOutgoingInfo.symbolTab[symbolIndex] = &lastOutgoingInfo.cachedSymbolReferenceItem[symbolIndex];
    lastOutgoingInfo.symbolIsWritten[symbolIndex] = false;

    if (referenceItem->bits.category == CategoryLocal) return;
    if (referenceItem->references == NULL && !force) return;

    for (reference = referenceItem->references; reference != NULL; reference = reference->next) {
        FileItem *fileItem = getFileItem(reference->position.file);
        log_trace("checking ref: loading=%d --< %s:%d", fileItem->bits.cxLoading,
                  fileItem->name, reference->position.line);
        if (options.update==UPDATE_CREATE || fileItem->bits.cxLoading) {
            writeCxReference(reference, symbolIndex);
        } else {
            log_trace("Some kind of update (%d) or not loading (%d), so don't writeCxReference()",
                      options.update, fileItem->bits.cxLoading);
            assert(0);
        }
    }
    //&fflush(cxOut);
}

static void genRefItem(ReferencesItem *referenceItem) {
    genRefItem0(referenceItem, false);
}

#define COMPOSE_CXFI_CHECK_NUM(filen, exactPositionLinkFlag) (  \
        ((filen)*XFILE_HASH_MAX)*2+exactPositionLinkFlag        \
    )

#define DECOMPOSE_CXFI_CHECK_NUM(num, filen, exactPositionLinkFlag){    \
        unsigned tmp;                                                   \
        tmp = num;                                                      \
        exactPositionLinkFlag = tmp % 2;                                \
        tmp = tmp / 2;                                                  \
        tmp = tmp / XFILE_HASH_MAX;                                     \
        filen = tmp;                                                    \
    }

static void genCxFileHead(void) {
    char sr[MAX_CHARS];
    char ttt[TMP_STRING_SIZE];
    int i;
    memset(&lastOutgoingInfo, 0, sizeof(lastOutgoingInfo));
    for (i=0; i<MAX_CHARS; i++) {
        lastOutgoingInfo.values[i] = -1;
    }
    get_version_string(ttt);
    writeStringRecord(CXFI_VERSION, ttt, "\n\n");
    fprintf(cxOut,"\n\n\n");
    for (i=0; i<MAX_CHARS && generatedFieldMarkersList[i] != -1; i++) {
        sr[i] = generatedFieldMarkersList[i];
    }
    assert(i < MAX_CHARS);
    sr[i]=0;
    writeStringRecord(CXFI_MARKER_LIST, sr, "");
    writeCompactRecord(CXFI_REFNUM, options.referenceFileCount, " ");
    writeCompactRecord(CXFI_CHECK_NUMBER, COMPOSE_CXFI_CHECK_NUM(MAX_FILES,
                                                                 options.exactPositionResolve),
                       " ");
}

static void openInOutReferenceFile(int updateFlag, char *filename) {
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
    openInOutReferenceFile(updateFlag, filename);
    genCxFileHead();
    mapOverFileTableWithIndex(mapfun);
    if (mapfun2!=NULL)
        mapOverFileTableWithIndex(mapfun2);
    scanCxFile(fullScanFunctionSequence);
    closeReferenceFile(filename);
}

static void generateRefsFromMemory(int fileOrder) {
    for (int i=0; i<referenceTable.size; i++) {
        for (ReferencesItem *r=referenceTable.tab[i]; r!=NULL; r=r->next) {
            if (r->bits.category == CategoryLocal)
                continue;
            if (r->references == NULL)
                continue;
            if (r->fileHash == fileOrder)
                genRefItem0(r, false);
        }
    }
}

void genReferenceFile(bool updating, char *filename) {
    if (!updating)
        removeFile(filename);

    recursivelyCreateFileDirIfNotExists(filename);

    if (options.referenceFileCount <= 1) {
        /* single reference file */
        openInOutReferenceFile(updating, filename);
        genCxFileHead();
        mapOverFileTableWithIndex(writeFileIndexItem);
        mapOverFileTableWithIndex(writeFileSourceIndexItem);
        mapOverFileTableWithIndex(genClassHierarchyItems);
        scanCxFile(fullScanFunctionSequence);
        refTabMap(&referenceTable, genRefItem);
        closeReferenceFile(filename);
    } else {
        /* several reference files */
        char referenceFileName[MAX_FILE_NAME_SIZE];
        char *dirname = filename;
        int i;

        createDirectory(dirname);
        genPartialFileTabRefFile(updating,dirname,REFERENCE_FILENAME_FILES,
                                 writeFileIndexItem, writeFileSourceIndexItem);
        genPartialFileTabRefFile(updating,dirname,REFERENCE_FILENAME_CLASSES,
                                 genClassHierarchyItems, NULL);
        for (i=0; i<options.referenceFileCount; i++) {
            sprintf(referenceFileName, "%s%s%04d", dirname, REFERENCE_FILENAME_PREFIX, i);
            assert(strlen(referenceFileName) < MAX_FILE_NAME_SIZE-1);
            openInOutReferenceFile(updating, referenceFileName);
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
        if (lastMessageTime < fileProcessingStartTime) {
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
    for (i=0; i<size-1; i++) {
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
    for (i=0; i<size-1; i++) {
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
    int magicn, filen, exactPositionLinkFlag;
    char tmpBuff[TMP_BUFF_SIZE];

    assert(marker == CXFI_CHECK_NUMBER);
    if (options.create)
        return; // no check when creating new file

    magicn = lastIncomingInfo.values[CXFI_CHECK_NUMBER];
    DECOMPOSE_CXFI_CHECK_NUM(magicn, filen, exactPositionLinkFlag);
    if (filen != MAX_FILES) {
        sprintf(tmpBuff,"The Tag file was generated with different MAX_FILES, recreate it");
        writeCxFileCompatibilityError(tmpBuff);
    }
    log_trace("checking exactPositionResolve: %d <-> %d", exactPositionLinkFlag, options.exactPositionResolve);
    if (exactPositionLinkFlag != options.exactPositionResolve) {
        if (exactPositionLinkFlag) {
            sprintf(tmpBuff,"The Tag file was generated with '-exactpositionresolve' flag, recreate it");
        } else {
            sprintf(tmpBuff,"The Tag file was generated without '-exactpositionresolve' flag, recreate it");
        }
        writeCxFileCompatibilityError(tmpBuff);
    }
}

static int cxrfFileItemShouldBeUpdatedFromCxFile(FileItem *fileItem) {
    bool updateFromCxFile = true;

    log_trace("re-read info from '%s' for '%s'?", options.cxrefsLocation, fileItem->name);
    if (options.taskRegime == RegimeXref) {
        if (fileItem->bits.cxLoading && !fileItem->bits.cxSaved) {
            updateFromCxFile = false;
        } else {
            updateFromCxFile = true;
        }
    }
    if (options.taskRegime == RegimeEditServer) {
        log_trace("last inspected == %d, start at %d\n", fileItem->lastInspected, fileProcessingStartTime);
        if (fileItem->lastInspected < fileProcessingStartTime) {
            updateFromCxFile = true;
        } else {
            updateFromCxFile = false;
        }
    }
    log_trace("%s re-read info from '%s' for '%s'", updateFromCxFile?"yes,":"no, not necessary to",
              options.cxrefsLocation, fileItem->name);

    return updateFromCxFile;
}

static void cxReadFileName(int size,
                           int marker,
                           CharacterBuffer *cb,
                           int additionalArg
) {
    char id[MAX_FILE_NAME_SIZE];
    FileItem *fileItem;
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
    if (!existsInFileTable(id)) {
        fileIndex = addFileNameToFileTable(id);
        fileItem = getFileItem(fileIndex);
        fileItem->bits.commandLineEntered = commandLineFlag;
        fileItem->bits.isInterface = isInterface;
        if (fileItem->lastFullUpdateMtime == 0)
            fileItem->lastFullUpdateMtime=fumtime;
        if (fileItem->lastUpdateMtime == 0)
            fileItem->lastUpdateMtime=umtime;
        assert(options.taskRegime);
        if (options.taskRegime == RegimeXref) {
            if (additionalArg == CXSF_GENERATE_OUTPUT) {
                writeFileIndexItem(fileItem, fileIndex);
            }
        }
    } else {
        fileIndex = lookupFileTable(id);
        fileItem = getFileItem(fileIndex);
        if (cxrfFileItemShouldBeUpdatedFromCxFile(fileItem)) {
            fileItem->bits.isInterface = isInterface;
            // Set it to none, it will be updated by source item
            fileItem->bits.sourceFileNumber = noFileIndex;
        }
        if (options.taskRegime == RegimeEditServer) {
            fileItem->bits.commandLineEntered = commandLineFlag;
        }
        if (fileItem->lastFullUpdateMtime == 0)
            fileItem->lastFullUpdateMtime=fumtime;
        if (fileItem->lastUpdateMtime == 0)
            fileItem->lastUpdateMtime=umtime;
        //&if (fumtime>fileItem->lastFullUpdateMtime) fileItem->lastFullUpdateMtime=fumtime;
        //&if (umtime>fileItem->lastUpdateMtime) fileItem->lastUpdateMtime=umtime;
    }
    fileItem->bits.isFromCxfile = true;
    decodeFileNumbers[ii]=fileIndex;
    log_trace("%d: '%s' scanned: added as %d", ii, id, fileIndex);
}

static void cxrfSourceIndex(int size,
                            int marker,
                            CharacterBuffer *cb,
                            int additionalArg
) {
    int file, sfile;

    assert(marker == CXFI_SOURCE_INDEX);
    file = lastIncomingInfo.values[CXFI_FILE_INDEX];
    file = decodeFileNumbers[file];
    sfile = lastIncomingInfo.values[CXFI_SOURCE_INDEX];
    sfile = decodeFileNumbers[sfile];

    FileItem *fileItem = getFileItem(file);
    assert(file>=0 && file<MAX_FILES && fileItem);
    // hmmm. here be more generous in getting correct source info
    if (fileItem->bits.sourceFileNumber == noFileIndex) {
        // first check that it is not set directly from source
        if (!fileItem->bits.cxLoading) {
            fileItem->bits.sourceFileNumber = sfile;
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


static void getSymTypeAndClasses(int *symType, int *vApplClass, int *vFunClass) {
    *symType = lastIncomingInfo.values[CXFI_SYMBOL_TYPE];

    *vApplClass = decodeFileNumbers[lastIncomingInfo.values[CXFI_SUBCLASS]];
    assert(getFileItem(*vApplClass) != NULL);

    *vFunClass = decodeFileNumbers[lastIncomingInfo.values[CXFI_SUPERCLASS]];
    assert(getFileItem(*vFunClass) != NULL);
}



static void cxrfSymbolNameForFullUpdateSchedule(int size,
                                                int marker,
                                                CharacterBuffer *cb,
                                                int additionalArg
) {
    ReferencesItem *memb;
    int si, symType, len, vApplClass, vFunClass, accessFlags;
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

    ReferencesItem *referenceItem = &lastIncomingInfo.cachedSymbolReferenceItem[si];
    lastIncomingInfo.symbolTab[si] = referenceItem;
    fillSymbolRefItem(referenceItem, id, cxFileHashNumber(id), //useless, put 0
                      vApplClass, vFunClass);
    fillSymbolRefItemBits(&referenceItem->bits, symType, storage, ScopeGlobal, accessFlags, CategoryGlobal);

    if (!refTabIsMember(&referenceTable, referenceItem, NULL, &memb)) {
        CX_ALLOCC(ss, len+1, char);
        strcpy(ss,id);
        CX_ALLOC(memb, ReferencesItem);
        fillSymbolRefItem(memb, ss, cxFileHashNumber(ss),
                          vApplClass, vFunClass);
        fillSymbolRefItemBits(&memb->bits, symType, storage,
                              ScopeGlobal, accessFlags, CategoryGlobal);
        refTabAdd(&referenceTable, memb);
    }
    lastIncomingInfo.symbolTab[si] = memb;
    lastIncomingInfo.onLineReferencedSym = si;
}

static void cxfileCheckLastSymbolDeadness(void) {
    if (lastIncomingInfo.symbolToCheckForDeadness != -1
        && lastIncomingInfo.deadSymbolIsDefined
    ) {
        olAddBrowsedSymbol(lastIncomingInfo.symbolTab[lastIncomingInfo.symbolToCheckForDeadness],
                           &currentUserData->browserStack.top->hkSelectedSym,
                           1,1,0,UsageDefined,0, &noPosition, UsageDefined);
    }
}


static bool symbolIsReportableAsUnused(ReferencesItem *referenceItem) {
    if (referenceItem==NULL || referenceItem->name[0]==' ')
        return false;

    // you need to be strong here, in fact struct record can be used
    // without using struct explicitly
    if (referenceItem->bits.symType == TypeStruct)
        return false;

    // maybe I should collect also all toString() references?
    if (referenceItem->bits.storage==StorageMethod && strcmp(referenceItem->name,"toString()")==0)
        return false;

    // in this first approach restrict this to variables and functions
    if (referenceItem->bits.symType == TypeMacro)
        return false;
    return true;
}

static bool canBypassAcceptableSymbol(ReferencesItem *symbol) {
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
    ReferencesItem *ddd, *memb;
    SymbolsMenu *cms;
    int si, symType, rr, vApplClass, vFunClass, ols, accessFlags, storage;
    char *id;

    assert(marker == CXFI_SYMBOL_NAME);
    if (options.taskRegime==RegimeEditServer && additionalArg==CXSF_DEAD_CODE_DETECTION) {
        // check if previous symbol was dead
        cxfileCheckLastSymbolDeadness();
    }
    accessFlags = lastIncomingInfo.values[CXFI_ACCESS_BITS];
    storage = lastIncomingInfo.values[CXFI_STORAGE];
    si = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    assert(si>=0 && si<MAX_CX_SYMBOL_TAB);
    id = lastIncomingInfo.cachedSymbolName[si];
    scanSymNameString(size, cb, id);
    getSymTypeAndClasses(&symType, &vApplClass, &vFunClass);

    ddd = &lastIncomingInfo.cachedSymbolReferenceItem[si];
    lastIncomingInfo.symbolTab[si] = ddd;
    fillSymbolRefItem(ddd, id,
                      cxFileHashNumber(id), // useless put 0
                      vApplClass, vFunClass);
    fillSymbolRefItemBits(&ddd->bits,symType, storage, ScopeGlobal, accessFlags,
                          CategoryGlobal);
    rr = refTabIsMember(&referenceTable, ddd, NULL, &memb);
    while (rr && memb->bits.category!=CategoryGlobal) rr=refTabNextMember(ddd, &memb);
    assert(options.taskRegime);
    if (options.taskRegime == RegimeXref) {
        if (memb==NULL) memb=ddd;
        genRefItem0(memb, true);
        ddd->references = memb->references; // note references to not generate multiple
        memb->references = NULL;      // HACK, remove them, to not be regenerated
    }
    if (options.taskRegime == RegimeEditServer) {
        if (additionalArg == CXSF_DEAD_CODE_DETECTION) {
            if (symbolIsReportableAsUnused(lastIncomingInfo.symbolTab[si])) {
                lastIncomingInfo.symbolToCheckForDeadness = si;
                lastIncomingInfo.deadSymbolIsDefined = 0;
            } else {
                lastIncomingInfo.symbolToCheckForDeadness = -1;
            }
        } else if (options.server_operation!=OLO_TAG_SEARCH) {
            cms = NULL; ols = 0;
            if (additionalArg == CXSF_MENU_CREATION) {
                cms = createSelectionMenu(ddd);
                if (cms == NULL) {
                    ols = 0;
                } else {
                    if (IS_BEST_FIT_MATCH(cms)) ols = 2;
                    else ols = 1;
                }
            } else if (additionalArg!=CXSF_BY_PASS) {
                ols=itIsSymbolToPushOlReferences(ddd,currentUserData->browserStack.top,&cms,DEFAULT_VALUE);
            }
            lastIncomingInfo.onLineRefMenuItem = cms;
            if (ols || (additionalArg==CXSF_BY_PASS && canBypassAcceptableSymbol(ddd))) {
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
    int      file, line, col, sym, vApplClass, vFunClass;
    UsageKind usageKind;
    Usage usage;
    int symType,reqAcc;

    assert(marker == CXFI_REFERENCE);

    usageKind = lastIncomingInfo.values[CXFI_USAGE];
    reqAcc = lastIncomingInfo.values[CXFI_REQUIRED_ACCESS];
    fillUsage(&usage, usageKind, reqAcc);

    sym = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];

    file = lastIncomingInfo.values[CXFI_FILE_INDEX];
    file = decodeFileNumbers[file];

    line = lastIncomingInfo.values[CXFI_LINE_INDEX];
    col = lastIncomingInfo.values[CXFI_COLUMN_INDEX];
    getSymTypeAndClasses( &symType, &vApplClass, &vFunClass);
    log_trace("%d %d->%d %d", usageKind, file, decodeFileNumbers[file], line);

    pos = makePosition(file, line, col);
    if (lastIncomingInfo.onLineReferencedSym == lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
        addToRefList(&lastIncomingInfo.symbolTab[sym]->references, usage, pos);
    }
}

static bool isInRefList(Reference *list,
                        Usage usage,
                        Position position) {
    Reference *foundReference;
    Reference reference;

    fillReference(&reference, usage, position, NULL);
    SORTED_LIST_FIND2(foundReference, Reference, reference, list);
    if (foundReference==NULL || SORTED_LIST_NEQ(foundReference,reference))
        return false;
    return true;
}


static void cxrfReference(int size,
                          int marker,
                          CharacterBuffer *cb,
                          int additionalArg
) {
    Position pos;
    Reference reference;
    Usage usage;
    int       file, line, col, sym, reqAcc;
    UsageKind usageKind;
    int copyrefFl;

    assert(marker == CXFI_REFERENCE);
    usageKind = lastIncomingInfo.values[CXFI_USAGE];
    reqAcc = lastIncomingInfo.values[CXFI_REQUIRED_ACCESS];

    sym = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];

    file = lastIncomingInfo.values[CXFI_FILE_INDEX];
    file = decodeFileNumbers[file];
    FileItem *fileItem = getFileItem(file);

    line = lastIncomingInfo.values[CXFI_LINE_INDEX];
    col = lastIncomingInfo.values[CXFI_COLUMN_INDEX];

    assert(options.taskRegime);
    if (options.taskRegime == RegimeXref) {
        if (fileItem->bits.cxLoading && fileItem->bits.cxSaved) {
            /* if we repass refs after overflow */
            pos = makePosition(file, line, col);
            fillUsage(&usage, usageKind, reqAcc);
            copyrefFl = !isInRefList(lastIncomingInfo.symbolTab[sym]->references,
                                     usage, pos);
        } else {
            copyrefFl = !fileItem->bits.cxLoading;
        }
        if (copyrefFl)
            writeCxReferenceBase(sym, usageKind, reqAcc, file, line, col);
    } else if (options.taskRegime == RegimeEditServer) {
        pos = makePosition(file, line, col);
        fillUsage(&usage, usageKind, reqAcc);
        fillReference(&reference, usage, pos, NULL);
        FileItem *referenceFileItem = getFileItem(reference.position.file);
        if (additionalArg == CXSF_DEAD_CODE_DETECTION) {
            if (OL_VIEWABLE_REFS(&reference)) {
                // restrict reported symbols to those defined in project input file
                if (IS_DEFINITION_USAGE(reference.usage.kind)
                    && referenceFileItem->bits.commandLineEntered
                ) {
                    lastIncomingInfo.deadSymbolIsDefined = 1;
                } else if (! IS_DEFINITION_OR_DECL_USAGE(reference.usage.kind)) {
                    lastIncomingInfo.symbolToCheckForDeadness = -1;
                }
            }
        } else if (additionalArg == CXSF_PASS_MACRO_USAGE) {
            if (lastIncomingInfo.onLineReferencedSym ==
                lastIncomingInfo.values[CXFI_SYMBOL_INDEX]
                && reference.usage.kind == UsageMacroBaseFileUsage
            ) {
                s_olMacro2PassFile = reference.position.file;
            }
        } else {
            if (options.server_operation == OLO_TAG_SEARCH) {
                if (reference.usage.kind==UsageDefined
                    || ((options.tagSearchSpecif==TSS_FULL_SEARCH
                         || options.tagSearchSpecif==TSS_FULL_SEARCH_SHORT)
                        &&  (reference.usage.kind==UsageDeclared
                             || reference.usage.kind==UsageClassFileDefinition))) {
                    searchSymbolCheckReference(lastIncomingInfo.symbolTab[sym],&reference);
                }
            } else if (options.server_operation == OLO_SAFETY_CHECK1) {
                if (lastIncomingInfo.onLineReferencedSym != lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
                    olcxCheck1CxFileReference(lastIncomingInfo.symbolTab[sym],
                                              &reference);
                }
            } else {
                if (lastIncomingInfo.onLineReferencedSym == lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
                    if (additionalArg == CXSF_MENU_CREATION) {
                        assert(lastIncomingInfo.onLineRefMenuItem);
                        if (file != olOriginalFileNumber || !fileItem->bits.commandLineEntered ||
                            options.server_operation == OLO_GOTO || options.server_operation == OLO_CGOTO ||
                            options.server_operation == OLO_PUSH_NAME ||
                            options.server_operation == OLO_PUSH_SPECIAL_NAME) {
                            log_trace (":adding reference %s:%d", referenceFileItem->name, reference.position.line);
                            olcxAddReferenceToSymbolsMenu(lastIncomingInfo.onLineRefMenuItem, &reference,
                                                          lastIncomingInfo.onLineRefIsBestMatchFlag);
                        }
                    } else if (additionalArg == CXSF_BY_PASS) {
                        if (positionsAreEqual(s_olcxByPassPos,reference.position)) {
                            // got the bypass reference
                            log_trace(":adding bypass selected symbol %s", lastIncomingInfo.symbolTab[sym]->name);
                            olAddBrowsedSymbol(lastIncomingInfo.symbolTab[sym],
                                               &currentUserData->browserStack.top->hkSelectedSym,
                                               1, 1, 0, usageKind,0,&noPosition, UsageNone);
                        }
                    } else {
                        olcxAddReference(&currentUserData->browserStack.top->references, &reference,
                                         lastIncomingInfo.onLineRefIsBestMatchFlag);
                    }
                }
            }
        }
    }
}


static void cxrfReferenceFileCountCheck(int referenceFileCount,
                                        int marker,
                                        CharacterBuffer *cb,
                                        int additionalArg
) {
    if (checkReferenceFileCountOption(referenceFileCount) == 0) {
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

    fileIndex = decodeFileNumbers[fileIndex];
    FileItem *fileItem = getFileItem(fileIndex);

    super_class = decodeFileNumbers[super_class];
    FileItem *superFileItem = getFileItem(super_class);

    sub_class = decodeFileNumbers[sub_class];
    FileItem *subFileItem = getFileItem(sub_class);

    assert(options.taskRegime);
    switch (options.taskRegime) {
    case RegimeXref:
        if (!fileItem->bits.cxLoading &&
            additionalArg==CXSF_GENERATE_OUTPUT) {
            writeSubClassInfo(super_class, sub_class, fileIndex);  // updating refs
        }
        break;
    case RegimeEditServer:
        if (fileIndex != inputFileNumber) {
            log_trace("reading %s < %s", simpleFileName(subFileItem->name),
                      simpleFileName(superFileItem->name));
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
        ch = getChar(&cxFileCharacterBuffer);
    }
    *_ch = ch;

    return scannedInt;
}


static void scanCxFile(ScanFileFunctionStep *scanFunctionTable) {
    int scannedInt = 0;
    int ch;

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
    decodeFileNumbers[noFileIndex] = noFileIndex;

    for (int i=0; scanFunctionTable[i].recordCode>0; i++) {
        assert(scanFunctionTable[i].recordCode < MAX_CHARS);
        ch = scanFunctionTable[i].recordCode;
        lastIncomingInfo.fun[ch] = scanFunctionTable[i].handleFun;
        lastIncomingInfo.additional[ch] = scanFunctionTable[i].additionalArg;
    }

    initCharacterBuffer(&cxFileCharacterBuffer, inputFile);
    ch = ' ';
    while (!cxFileCharacterBuffer.isAtEOF) {
        scannedInt = scanInteger(&cxFileCharacterBuffer, &ch);

        if (cxFileCharacterBuffer.isAtEOF)
            break;

        assert(ch >= 0 && ch<MAX_CHARS);
        if (lastIncomingInfo.markers[ch]) {
            lastIncomingInfo.values[ch] = scannedInt;
        }
        if (lastIncomingInfo.fun[ch] != NULL) {
            (*lastIncomingInfo.fun[ch])(scannedInt, ch, &cxFileCharacterBuffer,
                                        lastIncomingInfo.additional[ch]);
        } else if (! lastIncomingInfo.markers[ch]) {
            assert(scannedInt>0);
            skipCharacters(&cxFileCharacterBuffer, scannedInt-1);
        }
        ch = getChar(&cxFileCharacterBuffer);
    }

    /* TODO: This should be done outside this function... */
    if (options.taskRegime==RegimeEditServer
        && (options.server_operation==OLO_LOCAL_UNUSED
            || options.server_operation==OLO_GLOBAL_UNUSED)) {
        // check if last symbol was dead
        cxfileCheckLastSymbolDeadness();
    }

    LEAVE();
}


/* suffix contains '/' at the beginning !!! */
static bool scanReferenceFile(char *cxrefLocation, char *element1, char *element2,
                              ScanFileFunctionStep *scanFunctionTable) {
    char fn[MAX_FILE_NAME_SIZE];

    sprintf(fn, "%s%s%s", cxrefLocation, element1, element2);
    assert(strlen(fn) < MAX_FILE_NAME_SIZE-1);
    log_trace(":scanning file %s", fn);
    inputFile = openFile(fn, "r");
    if (inputFile==NULL) {
        return false;
    } else {
        scanCxFile(scanFunctionTable);
        closeFile(inputFile);
        inputFile = NULL;
        return true;
    }
}

static void scanReferenceFiles(char *cxrefLocation, ScanFileFunctionStep *scanFunctionTable) {
    char nn[MAX_FILE_NAME_SIZE];
    int i;

    if (options.referenceFileCount <= 1) {
        scanReferenceFile(cxrefLocation,"","",scanFunctionTable);
    } else {
        scanReferenceFile(cxrefLocation,REFERENCE_FILENAME_FILES,"",scanFunctionTable);
        scanReferenceFile(cxrefLocation,REFERENCE_FILENAME_CLASSES,"",scanFunctionTable);
        for (i=0; i<options.referenceFileCount; i++) {
            sprintf(nn,"%04d",i);
            scanReferenceFile(cxrefLocation,REFERENCE_FILENAME_PREFIX,nn,scanFunctionTable);
        }
    }
}

bool smartReadReferences(void) {
    static time_t savedModificationTime = 0; /* Cache previously read file data... */
    static off_t savedFileSize = 0;
    static char previouslyReadFileName[MAX_FILE_NAME_SIZE] = ""; /* ... and name */
    char cxrefFileName[MAX_FILE_NAME_SIZE];

    if (options.referenceFileCount <= 1) {
        sprintf(cxrefFileName, "%s", options.cxrefsLocation);
    } else {
        sprintf(cxrefFileName, "%s%s", options.cxrefsLocation, REFERENCE_FILENAME_FILES);
    }
    if (editorFileExists(cxrefFileName)) {
        size_t currentSize = editorFileSize(cxrefFileName);
        time_t currentModificationTime = editorFileModificationTime(cxrefFileName);
        if (strcmp(previouslyReadFileName, cxrefFileName) != 0
            || savedModificationTime != currentModificationTime
            || savedFileSize != currentSize)
        {
            log_trace(":(re)reading reference file '%s'", cxrefFileName);
            if (scanReferenceFile(cxrefFileName, "", "", normalScanFunctionSequence)) {
                strcpy(previouslyReadFileName, cxrefFileName);
                savedModificationTime = currentModificationTime;
                savedFileSize = currentSize;
            }
        } else {
            log_trace(":skipping (re)reading reference file '%s'", cxrefFileName);
        }
        return true;
    }
    return false;
}

// symbolName can be NULL !!!!!!
static void readOneAppropReferenceFile(char *symbolName,
                                       ScanFileFunctionStep  scanFileFunctionTable[]
) {
    if (options.cxrefsLocation == NULL)
        return;
    cxOut = stdout;
    if (options.referenceFileCount <= 1) {
        scanReferenceFile(options.cxrefsLocation, "", "", scanFileFunctionTable);
    } else {
        if (!smartReadReferences())
            return;
        if (!scanReferenceFile(options.cxrefsLocation, REFERENCE_FILENAME_CLASSES, "",
                               scanFileFunctionTable))
            return;
        if (symbolName == NULL)
            return;

        /* following must be after reading XFiles*/
        char fns[MAX_FILE_NAME_SIZE];
        int i = cxFileHashNumber(symbolName);

        sprintf(fns, "%04d", i);
        assert(strlen(fns) < MAX_FILE_NAME_SIZE-1);
        scanReferenceFile(options.cxrefsLocation, REFERENCE_FILENAME_PREFIX, fns,
                          scanFileFunctionTable);
    }
}


void normalScanReferenceFile(char *name) {
    scanReferenceFile(options.cxrefsLocation, name, "", normalScanFunctionSequence);
}

void scanForClassHierarchy(void) {
    readOneAppropReferenceFile(NULL, classHierarchyFunctionSequence);
}

void fullScanFor(char *symbolName) {
    readOneAppropReferenceFile(symbolName, fullUpdateFunctionSequence);
}

void scanForBypass(char *symbolName) {
    readOneAppropReferenceFile(symbolName, byPassFunctionSequence);
}

void scanReferencesToCreateMenu(char *symbolName){
    readOneAppropReferenceFile(symbolName, symbolMenuCreationFunctionSequence);
}

void scanForMacroUsage(char *symbolName) {
    readOneAppropReferenceFile(symbolName, secondPassMacroUsageFunctionSequence);
}

void scanForGlobalUnused(char *cxrefLocation) {
    scanReferenceFiles(cxrefLocation, globalUnusedDetectionFunctionSequence);
}

void scanForSearch(char *cxrefLocation) {
    scanReferenceFiles(cxrefLocation, symbolSearchFunctionSequence);
}


/* ************************************************************ */

static ScanFileFunctionStep normalScanFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, CXSF_UNUSED},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_JUST_READ},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep fullScanFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, cxrfVersionCheck, CXSF_UNUSED},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_GENERATE_OUTPUT},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_GENERATE_OUTPUT},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, CXSF_DEFAULT},
    {CXFI_REFERENCE, cxrfReference, CXSF_FIRST_PASS},
    {CXFI_CLASS_EXT, cxrfSubClass, CXSF_GENERATE_OUTPUT},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep byPassFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, cxrfVersionCheck, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, CXSF_BY_PASS},
    {CXFI_REFERENCE, cxrfReference, CXSF_BY_PASS},
    {CXFI_CLASS_EXT, cxrfSubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolLoadMenuRefsFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, cxrfVersionCheck, CXSF_UNUSED},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, CXSF_DEFAULT},
    {CXFI_REFERENCE, cxrfReference, CXSF_MENU_CREATION},
    {CXFI_CLASS_EXT, cxrfSubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolMenuCreationFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, cxrfVersionCheck, CXSF_UNUSED},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, CXSF_MENU_CREATION},
    {CXFI_REFERENCE, cxrfReference, CXSF_MENU_CREATION},
    {CXFI_CLASS_EXT, cxrfSubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep fullUpdateFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, cxrfVersionCheck, CXSF_UNUSED},
    {CXFI_CHECK_NUMBER, cxrfCheckNumber, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolNameForFullUpdateSchedule, CXSF_UNUSED},
    {CXFI_REFERENCE, cxrfReferenceForFullUpdateSchedule, CXSF_UNUSED},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep secondPassMacroUsageFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, CXSF_PASS_MACRO_USAGE},
    {CXFI_REFERENCE, cxrfReference, CXSF_PASS_MACRO_USAGE},
    {CXFI_CLASS_EXT, cxrfSubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep classHierarchyFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_CLASS_EXT, cxrfSubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolSearchFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, CXSF_UNUSED},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, SEARCH_SYMBOL},
    {CXFI_REFERENCE, cxrfReference, CXSF_FIRST_PASS},
    {-1,NULL, 0},
};

static ScanFileFunctionStep globalUnusedDetectionFunctionSequence[]={
    {CXFI_MARKER_LIST, cxrfReadRecordMarkers, 0},
    {CXFI_REFNUM, cxrfReferenceFileCountCheck, CXSF_UNUSED},
    {CXFI_FILE_NAME, cxReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, cxrfSourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, cxrfSymbolName, CXSF_DEAD_CODE_DETECTION},
    {CXFI_REFERENCE, cxrfReference, CXSF_DEAD_CODE_DETECTION},
    {-1,NULL, 0},
};

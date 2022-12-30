#include "cxfile.h"

#include "globals.h"
#include "commons.h"
#include "options.h"
#include "protocol.h"           /* C_XREF_FILE_VERSION_NUMBER */

#include "cxref.h"
#include "reftab.h"
#include "session.h"
#include "characterreader.h"
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
    ReferencesItem     cachedReferencesItem[MAX_CX_SYMBOL_TAB];
    char               cachedSymbolName[MAX_CX_SYMBOL_TAB][MAX_CX_SYMBOL_SIZE];
} LastCxFileInfo;

static LastCxFileInfo lastIncomingInfo;
static LastCxFileInfo lastOutgoingInfo;


static CharacterBuffer cxFileCharacterBuffer;

static unsigned decodeFileNumbers[MAX_FILES];

static char tmpFileName[MAX_FILE_NAME_SIZE];

static FILE *cxFile = NULL;

static FILE *inputFile;

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

static bool searchSingleStringMatch(char *cxtag, char *searchedStr, int len) {
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
        r = searchSingleStringMatch(cxtag, ss, len);
        if (r==0)
            return false;
        while (*ss!=0 && *ss!=' ' && *ss!='\t')
            ss++;
    }
 fini1:
    return true;
}

bool searchStringMatch(char *cxtag, int len) {
    if (containsWildcard(options.olcxSearchString))
        return shellMatch(cxtag, len, options.olcxSearchString, false);
    else
        return searchStringNonWildcardFitness(cxtag, len);
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

    if (referenceItem->type == TypeCppInclude)
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
    if (searchStringMatch(sname, slen)) {
        static int count = 0;
        olCompletionListPrepend(sname, NULL, NULL, 0, NULL, referenceItem, reference, referenceItem->type,
                                referenceItem->vFunClass, sessionData.retrieverStack.top);
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
    if (*blankPrefix!=0)
        fputs(blankPrefix, cxFile);
    if (info != 0)
        fPutDecimal(cxFile, info);
    fputc(marker, cxFile);
    lastOutgoingInfo.values[marker] = info;
}

static void writeOptionalCompactRecord(char marker, int info, char *blankPrefix) {
    assert(marker >= 0 && marker < MAX_CHARS);
    if (*blankPrefix!=0)
        fputs(blankPrefix, cxFile);

    /* If value for marker is same as last, don't write anything */
    if (lastOutgoingInfo.values[marker] != info) {
        /* If the info to write is not 0 then write it, else just write the marker */
        if (info != 0)
            fPutDecimal(cxFile, info);
        fputc(marker, cxFile);
        lastOutgoingInfo.values[marker] = info;
    }
}


static void writeStringRecord(int marker, char *s, char *blankPrefix) {
    int rsize;
    rsize = strlen(s)+1;
    if (*blankPrefix!=0)
        fputs(blankPrefix, cxFile);
    fPutDecimal(cxFile, rsize);
    fputc(marker, cxFile);
    fputs(s, cxFile);
}

/* Here we do the actual writing of the symbol */
static void writeSymbolItem(int symbolIndex) {
    ReferencesItem *d;

    /* First the symbol info, if not done already */
    writeOptionalCompactRecord(CXFI_SYMBOL_INDEX, symbolIndex, "");

    /* Then the reference info */
    d = lastOutgoingInfo.symbolTab[symbolIndex];
    writeOptionalCompactRecord(CXFI_SYMBOL_TYPE, d->type, "\n"); /* Why newline in the middle of all this? */
    writeOptionalCompactRecord(CXFI_SUBCLASS, d->vApplClass, "");
    writeOptionalCompactRecord(CXFI_SUPERCLASS, d->vFunClass, "");
    writeOptionalCompactRecord(CXFI_ACCESS_BITS, d->access, "");
    writeOptionalCompactRecord(CXFI_STORAGE, d->storage, "");
    lastOutgoingInfo.macroBaseFileGeneratedForSym[symbolIndex] = 0;
    lastOutgoingInfo.symbolIsWritten[symbolIndex] = true;
    writeStringRecord(CXFI_SYMBOL_NAME, d->name, "\t");
    fputc('\t', cxFile);
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
    writeOptionalCompactRecord(CXFI_INPUT_FROM_COMMAND_LINE, fileItem->isArgument, "");
    if (fileItem->isInterface) {
        writeOptionalCompactRecord(CXFI_ACCESS_BITS, AccessInterface, "");
    } else {
        writeOptionalCompactRecord(CXFI_ACCESS_BITS, AccessDefault, "");
    }
    writeStringRecord(CXFI_FILE_NAME, fileItem->name, " ");
}

static void writeFileSourceIndexItem(FileItem *fileItem, int index) {
    if (fileItem->sourceFileNumber != noFileIndex) {
        writeOptionalCompactRecord(CXFI_FILE_INDEX, index, "\n");
        writeCompactRecord(CXFI_SOURCE_INDEX, fileItem->sourceFileNumber, " ");
    }
}

static void writeClassHierarchyItems(FileItem *fileItem, int index) {
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
        assert(options.mode);
        if (options.mode == XrefMode) {
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
    if (symbol->type != TypeStruct)
        return;

    log_trace("testing %s", symbol->name);
    assert(symbol->javaClassIsLoaded);
    if (!symbol->javaClassIsLoaded)
        return;
    int cf1 = symbol->u.structSpec->classFileIndex;
    assert(cf1 >= 0 &&  cf1 < MAX_FILES);

    for (SymbolList *sups=symbol->u.structSpec->super; sups!=NULL; sups=sups->next) {
        assert(sups->element && sups->element->type == TypeStruct);
        addSubClassItemToFileTab(sups->element->u.structSpec->classFileIndex, cf1, origin);
    }
}

/* *************************************************************** */

static void writeReferenceItem(ReferencesItem *referenceItem) {
    int symbolIndex = 0;

    log_trace("generate cxref for symbol '%s'", referenceItem->name);
    assert(strlen(referenceItem->name)+1 < MAX_CX_SYMBOL_SIZE);

    strcpy(lastOutgoingInfo.cachedSymbolName[symbolIndex], referenceItem->name);
    fillReferencesItem(&lastOutgoingInfo.cachedReferencesItem[symbolIndex],
                       lastOutgoingInfo.cachedSymbolName[symbolIndex],
                       referenceItem->fileHash, // useless put 0
                       referenceItem->vApplClass, referenceItem->vFunClass, referenceItem->type,
                       referenceItem->storage, referenceItem->scope, referenceItem->access,
                       referenceItem->category);
    lastOutgoingInfo.symbolTab[symbolIndex]       = &lastOutgoingInfo.cachedReferencesItem[symbolIndex];
    lastOutgoingInfo.symbolIsWritten[symbolIndex] = false;

    if (referenceItem->category == CategoryLocal)
        return;

    for (Reference *reference = referenceItem->references; reference != NULL; reference = reference->next) {
        FileItem *fileItem = getFileItem(reference->position.file);
        log_trace("checking ref: loading=%d --< %s:%d", fileItem->cxLoading,
                  fileItem->name, reference->position.line);
        if (options.update==UPDATE_DEFAULT || fileItem->cxLoading) {
            writeCxReference(reference, symbolIndex);
        } else {
            log_trace("Some kind of update (%d) or loading (%d), so don't writeCxReference()",
                      options.update, fileItem->cxLoading);
            assert(0);
        }
    }
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

static void writeCxFileHead(void) {
    char stringRecord[MAX_CHARS];
    char tempString[TMP_STRING_SIZE];
    int i;

    memset(&lastOutgoingInfo, 0, sizeof(lastOutgoingInfo));
    for (i=0; i<MAX_CHARS; i++) {
        lastOutgoingInfo.values[i] = -1;
    }

    get_version_string(tempString);
    writeStringRecord(CXFI_VERSION, tempString, "\n\n");
    fprintf(cxFile,"\n\n\n");

    for (i=0; i<MAX_CHARS && generatedFieldMarkersList[i] != -1; i++) {
        stringRecord[i] = generatedFieldMarkersList[i];
    }

    assert(i < MAX_CHARS);
    stringRecord[i]=0;
    writeStringRecord(CXFI_MARKER_LIST, stringRecord, "");
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
    cxFile = openFile(filename,"w");
    if (cxFile == NULL)
        FATAL_ERROR(ERR_CANT_OPEN, filename, XREF_EXIT_ERR);

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
    closeFile(cxFile);
    cxFile = NULL;
}

/* suffix contains '/' at the beginning */
static void writePartialReferenceFile(int updateFlag,
                                     char *dirname,
                                     char *suffix,
                                     void mapfun(FileItem *, int),
                                     void mapfun2(FileItem *, int)) {
    char filename[MAX_FILE_NAME_SIZE];

    sprintf(filename, "%s%s", dirname, suffix);
    assert(strlen(filename) < MAX_FILE_NAME_SIZE-1);
    openInOutReferenceFile(updateFlag, filename);
    writeCxFileHead();
    mapOverFileTableWithIndex(mapfun);
    if (mapfun2!=NULL)
        mapOverFileTableWithIndex(mapfun2);
    scanCxFile(fullScanFunctionSequence);
    closeReferenceFile(filename);
}

static void generateRefsFromMemory(int fileOrder) {
    for (int i=getNextExistingReferencesItem(0); i != -1; i = getNextExistingReferencesItem(i+1)) {
        for (ReferencesItem *r=getReferencesItem(i); r!=NULL; r=r->next) {
            if (r->category == CategoryLocal)
                continue;
            if (r->references == NULL)
                continue;
            if (r->fileHash == fileOrder)
                writeReferenceItem(r);
        }
    }
}

void writeReferenceFile(bool updating, char *filename) {
    if (!updating)
        removeFile(filename);

    recursivelyCreateFileDirIfNotExists(filename);

    if (options.referenceFileCount <= 1) {
        /* single reference file */
        openInOutReferenceFile(updating, filename);
        writeCxFileHead();
        mapOverFileTableWithIndex(writeFileIndexItem);
        mapOverFileTableWithIndex(writeFileSourceIndexItem);
        mapOverFileTableWithIndex(writeClassHierarchyItems);
        scanCxFile(fullScanFunctionSequence);
        mapOverReferenceTable(writeReferenceItem);
        closeReferenceFile(filename);
    } else {
        /* several reference files */
        char referenceFileName[MAX_FILE_NAME_SIZE];
        char *dirname = filename;

        createDirectory(dirname);
        writePartialReferenceFile(updating,dirname,REFERENCE_FILENAME_FILES,
                                 writeFileIndexItem, writeFileSourceIndexItem);
        writePartialReferenceFile(updating,dirname,REFERENCE_FILENAME_CLASSES,
                                 writeClassHierarchyItems, NULL);
        for (int i=0; i<options.referenceFileCount; i++) {
            sprintf(referenceFileName, "%s%s%04d", dirname, REFERENCE_FILENAME_PREFIX, i);
            assert(strlen(referenceFileName) < MAX_FILE_NAME_SIZE-1);
            openInOutReferenceFile(updating, referenceFileName);
            writeCxFileHead();
            scanCxFile(fullScanFunctionSequence);
            generateRefsFromMemory(i);
            closeReferenceFile(referenceFileName);
        }
    }
}

static void writeCxFileCompatibilityError(char *message) {
    static time_t lastMessageTime;
    if (options.mode == ServerMode) {
        if (lastMessageTime < fileProcessingStartTime) {
            errorMessage(ERR_ST, message);
            lastMessageTime = time(NULL);
        }
    } else {
        FATAL_ERROR(ERR_ST, message, XREF_EXIT_ERR);
    }
}


/* ************************* READ **************************** */

static void scanFunction_ReadRecordMarkers(int size,
                                  int marker,
                                  CharacterBuffer *cb,
                                  int additionalArg
) {
    assert(marker == CXFI_MARKER_LIST);
    for (int i=0; i<size-1; i++) {
        int ch = getChar(cb);
        lastIncomingInfo.markers[ch] = 1;
    }
}


static void scanFunction_VersionCheck(int size,
                                      int marker,
                                      CharacterBuffer *cb,
                                      int additionalArg
) {
    char versionString[TMP_STRING_SIZE];
    char thisVersionString[TMP_STRING_SIZE];

    assert(marker == CXFI_VERSION);
    getString(versionString, size-1, cb);
    get_version_string(thisVersionString);
    if (strcmp(versionString, thisVersionString) != 0) {
        writeCxFileCompatibilityError("The Tag file was not generated by the current version of C-xrefactory, recreate it");
    }
}

static void scanFunction_CheckNumber(int size,
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

static int fileItemShouldBeUpdatedFromCxFile(FileItem *fileItem) {
    bool updateFromCxFile = true;

    log_trace("re-read info from '%s' for '%s'?", options.cxrefsLocation, fileItem->name);
    if (options.mode == XrefMode) {
        if (fileItem->cxLoading && !fileItem->cxSaved) {
            updateFromCxFile = false;
        } else {
            updateFromCxFile = true;
        }
    }
    if (options.mode == ServerMode) {
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

static void scanFunction_ReadFileName(int size,
                                      int marker,
                                      CharacterBuffer *cb,
                                      int additionalArg
) {
    char id[MAX_FILE_NAME_SIZE];
    FileItem *fileItem;
    int fileIndex;
    bool isArgument, isInterface;
    time_t fumtime, umtime;

    assert(marker == CXFI_FILE_NAME);
    fumtime = (time_t) lastIncomingInfo.values[CXFI_FILE_FUMTIME];
    umtime = (time_t) lastIncomingInfo.values[CXFI_FILE_UMTIME];
    isArgument = lastIncomingInfo.values[CXFI_INPUT_FROM_COMMAND_LINE];
    isInterface=((lastIncomingInfo.values[CXFI_ACCESS_BITS] & AccessInterface)!=0);

    assert(size < MAX_FILE_NAME_SIZE);
    getString(id, size-1, cb);

    int lastIncomingFileIndex = lastIncomingInfo.values[CXFI_FILE_INDEX];
    assert(lastIncomingFileIndex>=0 && lastIncomingFileIndex<MAX_FILES);

    if (!existsInFileTable(id)) {
        fileIndex = addFileNameToFileTable(id);
        fileItem = getFileItem(fileIndex);
        fileItem->isArgument = isArgument;
        fileItem->isInterface = isInterface;
        if (fileItem->lastFullUpdateMtime == 0)
            fileItem->lastFullUpdateMtime=fumtime;
        if (fileItem->lastUpdateMtime == 0)
            fileItem->lastUpdateMtime=umtime;
        assert(options.mode);
        if (options.mode == XrefMode) {
            if (additionalArg == CXSF_GENERATE_OUTPUT) {
                writeFileIndexItem(fileItem, fileIndex);
            }
        }
    } else {
        fileIndex = lookupFileTable(id);
        fileItem = getFileItem(fileIndex);
        if (fileItemShouldBeUpdatedFromCxFile(fileItem)) {
            fileItem->isInterface = isInterface;
            // Set it to none, it will be updated by source item
            fileItem->sourceFileNumber = noFileIndex;
        }
        if (options.mode == ServerMode) {
            fileItem->isArgument = isArgument;
        }
        if (fileItem->lastFullUpdateMtime == 0)
            fileItem->lastFullUpdateMtime=fumtime;
        if (fileItem->lastUpdateMtime == 0)
            fileItem->lastUpdateMtime=umtime;
        //&if (fumtime>fileItem->lastFullUpdateMtime) fileItem->lastFullUpdateMtime=fumtime;
        //&if (umtime>fileItem->lastUpdateMtime) fileItem->lastUpdateMtime=umtime;
    }
    fileItem->isFromCxfile = true;
    decodeFileNumbers[lastIncomingFileIndex]=fileIndex;
    log_trace("%d: '%s' scanned: added as %d", lastIncomingFileIndex, id, fileIndex);
}

static void scanFunction_SourceIndex(int size,
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
    if (fileItem->sourceFileNumber == noFileIndex) {
        // first check that it is not set directly from source
        if (!fileItem->cxLoading) {
            fileItem->sourceFileNumber = sfile;
        }
    }
}

static int scanSymNameString(int size, CharacterBuffer *cb, char *id) {
    assert(size < MAX_CX_SYMBOL_SIZE);
    getString(id, size-1, cb);

    return size-1;
}


static void getSymTypeAndClasses(int *symType, int *vApplClass, int *vFunClass) {
    *symType = lastIncomingInfo.values[CXFI_SYMBOL_TYPE];

    *vApplClass = decodeFileNumbers[lastIncomingInfo.values[CXFI_SUBCLASS]];
    assert(getFileItem(*vApplClass) != NULL);

    *vFunClass = decodeFileNumbers[lastIncomingInfo.values[CXFI_SUPERCLASS]];
    assert(getFileItem(*vFunClass) != NULL);
}



static void scanFunction_SymbolNameForFullUpdateSchedule(int size,
                                                int marker,
                                                CharacterBuffer *cb,
                                                int additionalArg
) {
    ReferencesItem *memb;
    int symbolIndex, symType, len, vApplClass, vFunClass, accessFlags;
    int storage;
    char *id;

    assert(marker == CXFI_SYMBOL_NAME);
    accessFlags = lastIncomingInfo.values[CXFI_ACCESS_BITS];
    storage = lastIncomingInfo.values[CXFI_STORAGE];
    symbolIndex = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    assert(symbolIndex>=0 && symbolIndex<MAX_CX_SYMBOL_TAB);
    id = lastIncomingInfo.cachedSymbolName[symbolIndex];
    len = scanSymNameString(size, cb, id);
    getSymTypeAndClasses( &symType, &vApplClass, &vFunClass);
    //&fprintf(dumpOut,":scanning ref of %s %d %d: \n",id,symType,vFunClass);fflush(dumpOut);
    if (symType!=TypeCppInclude || strcmp(id, LINK_NAME_INCLUDE_REFS)!=0) {
        lastIncomingInfo.onLineReferencedSym = -1;
        return;
    }

    ReferencesItem *referenceItem = &lastIncomingInfo.cachedReferencesItem[symbolIndex];
    lastIncomingInfo.symbolTab[symbolIndex] = referenceItem;
    fillReferencesItem(referenceItem, id, cxFileHashNumber(id), //useless, put 0
                       vApplClass, vFunClass, symType, storage, ScopeGlobal, accessFlags, CategoryGlobal);

    if (!isMemberInReferenceTable(referenceItem, NULL, &memb)) {
        // TODO: This is more or less the body of a newReferencesItem()
        char *ss = cxAlloc(len+1);
        strcpy(ss,id);
        memb = cxAlloc(sizeof(ReferencesItem));
        fillReferencesItem(memb, ss, cxFileHashNumber(ss),
                           vApplClass, vFunClass, symType, storage,
                           ScopeGlobal, accessFlags, CategoryGlobal);
        addToReferencesTable(memb);
    }
    lastIncomingInfo.symbolTab[symbolIndex] = memb;
    lastIncomingInfo.onLineReferencedSym = symbolIndex;
}

static void cxfileCheckLastSymbolDeadness(void) {
    if (lastIncomingInfo.symbolToCheckForDeadness != -1
        && lastIncomingInfo.deadSymbolIsDefined
    ) {
        olAddBrowsedSymbol(lastIncomingInfo.symbolTab[lastIncomingInfo.symbolToCheckForDeadness],
                           &sessionData.browserStack.top->hkSelectedSym,
                           1,1,0,UsageDefined,0, &noPosition, UsageDefined);
    }
}


static bool symbolIsReportableAsUnused(ReferencesItem *referenceItem) {
    if (referenceItem==NULL || referenceItem->name[0]==' ')
        return false;

    // you need to be strong here, in fact struct record can be used
    // without using struct explicitly
    if (referenceItem->type == TypeStruct)
        return false;

    // maybe I should collect also all toString() references?
    if (referenceItem->storage==StorageMethod && strcmp(referenceItem->name,"toString()")==0)
        return false;

    // in this first approach restrict this to variables and functions
    if (referenceItem->type == TypeMacro)
        return false;
    return true;
}

static bool canBypassAcceptableSymbol(ReferencesItem *symbol) {
    int nlen,len;
    char *nn, *nnn;

    get_bare_name(symbol->name, &nn, &len);
    get_bare_name(options.browsedSymName, &nnn, &nlen);
    if (len != nlen)
        return false;
    if (strncmp(nn, nnn, len))
        return false;
    return true;
}

static void scanFunction_SymbolName(int size,
                           int marker,
                           CharacterBuffer *cb,
                           int additionalArg
) {
    ReferencesItem *referencesItem, *member;
    SymbolsMenu *cms;
    int symbolIndex, symType, vApplClass, vFunClass, ols, accessFlags, storage;
    char *id;

    assert(marker == CXFI_SYMBOL_NAME);
    if (options.mode==ServerMode && additionalArg==CXSF_DEAD_CODE_DETECTION) {
        // check if previous symbol was dead
        cxfileCheckLastSymbolDeadness();
    }
    accessFlags = lastIncomingInfo.values[CXFI_ACCESS_BITS];
    storage = lastIncomingInfo.values[CXFI_STORAGE];
    symbolIndex = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    assert(symbolIndex>=0 && symbolIndex<MAX_CX_SYMBOL_TAB);
    id = lastIncomingInfo.cachedSymbolName[symbolIndex];
    scanSymNameString(size, cb, id);
    getSymTypeAndClasses(&symType, &vApplClass, &vFunClass);

    referencesItem = &lastIncomingInfo.cachedReferencesItem[symbolIndex];
    lastIncomingInfo.symbolTab[symbolIndex] = referencesItem;
    fillReferencesItem(referencesItem, id,
                       cxFileHashNumber(id), // useless put 0
                       vApplClass, vFunClass, symType, storage, ScopeGlobal, accessFlags,
                       CategoryGlobal);

    bool isMember = isMemberInReferenceTable(referencesItem, NULL, &member);
    while (isMember && member->category!=CategoryGlobal)
        isMember = refTabNextMember(referencesItem, &member);

    assert(options.mode);
    if (options.mode == XrefMode) {
        if (member==NULL)
            member=referencesItem;
        writeReferenceItem(member);
        referencesItem->references = member->references; // note references to not generate multiple
        member->references = NULL;      // HACK, remove them, to not be regenerated
    }
    if (options.mode == ServerMode) {
        if (additionalArg == CXSF_DEAD_CODE_DETECTION) {
            if (symbolIsReportableAsUnused(lastIncomingInfo.symbolTab[symbolIndex])) {
                lastIncomingInfo.symbolToCheckForDeadness = symbolIndex;
                lastIncomingInfo.deadSymbolIsDefined = 0;
            } else {
                lastIncomingInfo.symbolToCheckForDeadness = -1;
            }
        } else if (options.serverOperation!=OLO_TAG_SEARCH) {
            cms = NULL; ols = 0;
            if (additionalArg == CXSF_MENU_CREATION) {
                cms = createSelectionMenu(referencesItem);
                if (cms == NULL) {
                    ols = 0;
                } else {
                    if (IS_BEST_FIT_MATCH(cms)) ols = 2;
                    else ols = 1;
                }
            } else if (additionalArg!=CXSF_BY_PASS) {
                ols=itIsSymbolToPushOlReferences(referencesItem, sessionData.browserStack.top, &cms,
                                                 DEFAULT_VALUE);
            }
            lastIncomingInfo.onLineRefMenuItem = cms;
            if (ols || (additionalArg==CXSF_BY_PASS && canBypassAcceptableSymbol(referencesItem))) {
                lastIncomingInfo.onLineReferencedSym = symbolIndex;
                lastIncomingInfo.onLineRefIsBestMatchFlag = (ols == 2);
                log_trace("symbol %s is O.K. for %s (ols==%d)", referencesItem->name, options.browsedSymName, ols);
            } else {
                if (lastIncomingInfo.onLineReferencedSym == symbolIndex) {
                    lastIncomingInfo.onLineReferencedSym = -1;
                }
            }
        }
    }
}

static void scanFunction_ReferenceForFullUpdateSchedule(int size,
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


static void scanFunction_Reference(int size,
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

    assert(options.mode);
    if (options.mode == XrefMode) {
        if (fileItem->cxLoading && fileItem->cxSaved) {
            /* if we repass refs after overflow */
            pos = makePosition(file, line, col);
            fillUsage(&usage, usageKind, reqAcc);
            copyrefFl = !isInRefList(lastIncomingInfo.symbolTab[sym]->references,
                                     usage, pos);
        } else {
            copyrefFl = !fileItem->cxLoading;
        }
        if (copyrefFl)
            writeCxReferenceBase(sym, usageKind, reqAcc, file, line, col);
    } else if (options.mode == ServerMode) {
        pos = makePosition(file, line, col);
        fillUsage(&usage, usageKind, reqAcc);
        fillReference(&reference, usage, pos, NULL);
        FileItem *referenceFileItem = getFileItem(reference.position.file);
        if (additionalArg == CXSF_DEAD_CODE_DETECTION) {
            if (OL_VIEWABLE_REFS(&reference)) {
                // restrict reported symbols to those defined in project input file
                if (IS_DEFINITION_USAGE(reference.usage.kind)
                    && referenceFileItem->isArgument
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
            if (options.serverOperation == OLO_TAG_SEARCH) {
                if (reference.usage.kind==UsageDefined
                    || ((options.tagSearchSpecif==TSS_FULL_SEARCH
                         || options.tagSearchSpecif==TSS_FULL_SEARCH_SHORT)
                        &&  (reference.usage.kind==UsageDeclared
                             || reference.usage.kind==UsageClassFileDefinition))) {
                    searchSymbolCheckReference(lastIncomingInfo.symbolTab[sym],&reference);
                }
            } else if (options.serverOperation == OLO_SAFETY_CHECK1) {
                if (lastIncomingInfo.onLineReferencedSym != lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
                    olcxCheck1CxFileReference(lastIncomingInfo.symbolTab[sym],
                                              &reference);
                }
            } else {
                if (lastIncomingInfo.onLineReferencedSym == lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
                    if (additionalArg == CXSF_MENU_CREATION) {
                        assert(lastIncomingInfo.onLineRefMenuItem);
                        if (file != olOriginalFileIndex || !fileItem->isArgument ||
                            options.serverOperation == OLO_GOTO || options.serverOperation == OLO_CGOTO ||
                            options.serverOperation == OLO_PUSH_NAME ||
                            options.serverOperation == OLO_PUSH_SPECIAL_NAME) {
                            log_trace (":adding reference %s:%d", referenceFileItem->name, reference.position.line);
                            olcxAddReferenceToSymbolsMenu(lastIncomingInfo.onLineRefMenuItem, &reference,
                                                          lastIncomingInfo.onLineRefIsBestMatchFlag);
                        }
                    } else if (additionalArg == CXSF_BY_PASS) {
                        if (positionsAreEqual(s_olcxByPassPos,reference.position)) {
                            // got the bypass reference
                            log_trace(":adding bypass selected symbol %s", lastIncomingInfo.symbolTab[sym]->name);
                            olAddBrowsedSymbol(lastIncomingInfo.symbolTab[sym],
                                               &sessionData.browserStack.top->hkSelectedSym,
                                               1, 1, 0, usageKind,0,&noPosition, UsageNone);
                        }
                    } else {
                        olcxAddReference(&sessionData.browserStack.top->references, &reference,
                                         lastIncomingInfo.onLineRefIsBestMatchFlag);
                    }
                }
            }
        }
    }
}


static void scanFunction_ReferenceFileCountCheck(int referenceFileCount,
                                                 int marker,
                                                 CharacterBuffer *cb,
                                                 int additionalArg
) {
    if (!referenceFileCountMatches(referenceFileCount)) {
        assert(options.mode);
        FATAL_ERROR(ERR_ST,"Tag file was generated with different '-refnum' options, recreate it!", XREF_EXIT_ERR);
    }
}

static void scanFunction_SubClass(int size,
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

    assert(options.mode);
    switch (options.mode) {
    case XrefMode:
        if (!fileItem->cxLoading &&
            additionalArg==CXSF_GENERATE_OUTPUT) {
            writeSubClassInfo(super_class, sub_class, fileIndex);  // updating refs
        }
        break;
    case ServerMode:
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
        } else if (!lastIncomingInfo.markers[ch]) {
            assert(scannedInt>0);
            skipCharacters(&cxFileCharacterBuffer, scannedInt-1);
        }
        ch = getChar(&cxFileCharacterBuffer);
    }

    /* TODO: This should be done outside this function... */
    if (options.mode==ServerMode
        && (options.serverOperation==OLO_LOCAL_UNUSED
            || options.serverOperation==OLO_GLOBAL_UNUSED)) {
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
    cxFile = stdout;
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
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_UNUSED},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep fullScanFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_UNUSED},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_GENERATE_OUTPUT},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_GENERATE_OUTPUT},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_DEFAULT},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_FIRST_PASS},
    {CXFI_CLASS_EXT, scanFunction_SubClass, CXSF_GENERATE_OUTPUT},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep byPassFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_BY_PASS},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_BY_PASS},
    {CXFI_CLASS_EXT, scanFunction_SubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolMenuCreationFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_UNUSED},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_MENU_CREATION},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_MENU_CREATION},
    {CXFI_CLASS_EXT, scanFunction_SubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep fullUpdateFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_UNUSED},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_UNUSED},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolNameForFullUpdateSchedule, CXSF_UNUSED},
    {CXFI_REFERENCE, scanFunction_ReferenceForFullUpdateSchedule, CXSF_UNUSED},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep secondPassMacroUsageFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_PASS_MACRO_USAGE},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_PASS_MACRO_USAGE},
    {CXFI_CLASS_EXT, scanFunction_SubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep classHierarchyFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_CLASS_EXT, scanFunction_SubClass, CXSF_JUST_READ},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolSearchFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_UNUSED},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, SEARCH_SYMBOL},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_FIRST_PASS},
    {-1,NULL, 0},
};

static ScanFileFunctionStep globalUnusedDetectionFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, 0},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_UNUSED},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_DEAD_CODE_DETECTION},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_DEAD_CODE_DETECTION},
    {-1,NULL, 0},
};

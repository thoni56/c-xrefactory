#include "cxfile.h"

#include "globals.h"
#include "commons.h"
#include "options.h"
#include "protocol.h"           /* C_XREF_FILE_VERSION_NUMBER */


#include "characterreader.h"
#include "completion.h"
#include "cxref.h"
#include "fileio.h"
#include "filetable.h"
#include "hash.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "misc.h"
#include "reftab.h"
#include "session.h"
#include "usage.h"


/* *********************** INPUT/OUTPUT FIELD MARKERS ************************** */

#define CXFI_FILE_FUMTIME   'm'     /* last full update kmtime for file item */
#define CXFI_FILE_UMTIME    'p'     /* last update mtime for file item */
#define CXFI_SOURCE_INDEX   'o'     /* source index for java classes */
#define CXFI_INPUT_FROM_COMMAND_LINE  'i'     /* file was introduced from command line */

#define CXFI_USAGE          'u'
#define CXFI_REQUIRED_ACCESS     'A'     /* java reference required accessibility index */
#define CXFI_SYMBOL_INDEX   's'
#define CXFI_FILE_NUMBER     'f'
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
    CXSF_NOP,
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
    CXFI_FILE_NUMBER,
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
    ReferenceItem *symbolTab[MAX_CX_SYMBOL_TAB];
    bool                symbolIsWritten[MAX_CX_SYMBOL_TAB];
    int                 macroBaseFileGeneratedForSym[MAX_CX_SYMBOL_TAB];
    char                markers[MAX_CHARS];
    int                 values[MAX_CHARS];
    void                (*fun[MAX_CHARS])(int size, int marker, CharacterBuffer *cb, CxScanFileOperation operation);
    int                 additional[MAX_CHARS];

    // dead code detection vars
    int                 symbolToCheckForDeadness;
    char                deadSymbolIsDefined;

    // following item can be used only via symbolTab,
    // it is just to simplify memory handling !!!!!!!!!!!!!!!!
    ReferenceItem      cachedReferenceItem[MAX_CX_SYMBOL_TAB];
    char               cachedSymbolName[MAX_CX_SYMBOL_TAB][MAX_CX_SYMBOL_SIZE];
} LastCxFileInfo;

static LastCxFileInfo lastIncomingInfo;
static LastCxFileInfo lastOutgoingInfo;


static CharacterBuffer cxFileCharacterBuffer;

static unsigned decodeFileNumbers[MAX_FILES];

static FILE *cxFile = NULL;

static FILE *inputFile;

typedef struct scanFileFunctionStep {
    int		recordCode;
    void    (*handleFun)(int size, int ri, CharacterBuffer *cb, CxScanFileOperation operation); /* TODO: Break out a type */
    int		additionalArg;
} ScanFileFunctionStep;


static ScanFileFunctionStep normalScanFunctionSequence[];
static ScanFileFunctionStep fullScanFunctionSequence[];
static ScanFileFunctionStep fullUpdateFunctionSequence[];
static ScanFileFunctionStep byPassFunctionSequence[];
static ScanFileFunctionStep symbolMenuCreationFunctionSequence[];
static ScanFileFunctionStep secondPassMacroUsageFunctionSequence[];
static ScanFileFunctionStep globalUnusedDetectionFunctionSequence[];
static ScanFileFunctionStep symbolSearchFunctionSequence[];

static void scanCxFile(ScanFileFunctionStep *scanFunctionTable);


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


// Filter out symbols which pollute search results
bool symbolShouldBeHiddenFromSearchResults(char *name) {
    // internal xref symbol?
    if (name[0] == ' ')
        return true;
    return false;
}

void searchSymbolCheckReference(ReferenceItem  *referenceItem, Reference *reference) {
    char ssname[MAX_CX_SYMBOL_SIZE];
    char *s, *sname;
    int slen;

    if (referenceItem->type == TypeCppInclude)
        return;   // no %%i symbols
    if (symbolShouldBeHiddenFromSearchResults(referenceItem->linkName))
        return;

    linkNamePrettyPrint(ssname, referenceItem->linkName, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
    sname = ssname;
    slen = strlen(sname);
    // if completing without profile, cut profile
    if (options.searchKind==SEARCH_DEFINITIONS_SHORT
        || options.searchKind==SEARCH_FULL_SHORT) {
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
        sessionData.retrieverStack.top->completions = completionListPrepend(
            sessionData.retrieverStack.top->completions, sname, NULL, NULL, 0, NULL, referenceItem,
            reference, referenceItem->type, referenceItem->vFunClass);
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
    ReferenceItem *d;

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
    writeStringRecord(CXFI_SYMBOL_NAME, d->linkName, "\t");
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
    writeOptionalCompactRecord(CXFI_FILE_NUMBER, file, "");
    writeOptionalCompactRecord(CXFI_LINE_INDEX, line, "");
    writeOptionalCompactRecord(CXFI_COLUMN_INDEX, col, "");
    writeCompactRecord(CXFI_REFERENCE, 0, "");
}

static void writeCxReference(Reference *reference, int symbolNum) {
    writeCxReferenceBase(symbolNum, reference->usage.kind, reference->usage.requiredAccess,
                         reference->position.file, reference->position.line, reference->position.col);
}

static void writeFileNumberItem(FileItem *fileItem, int number) {
    writeOptionalCompactRecord(CXFI_FILE_NUMBER, number, "\n");
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
    if (fileItem->sourceFileNumber != NO_FILE_NUMBER) {
        writeOptionalCompactRecord(CXFI_FILE_NUMBER, index, "\n");
        writeCompactRecord(CXFI_SOURCE_INDEX, fileItem->sourceFileNumber, " ");
    }
}

/* *************************************************************** */

static void writeReferenceItem(ReferenceItem *referenceItem) {
    int symbolIndex = 0;

    log_trace("generate cxref for symbol '%s'", referenceItem->linkName);
    assert(strlen(referenceItem->linkName)+1 < MAX_CX_SYMBOL_SIZE);

    strcpy(lastOutgoingInfo.cachedSymbolName[symbolIndex], referenceItem->linkName);
    fillReferenceItem(&lastOutgoingInfo.cachedReferenceItem[symbolIndex],
                       lastOutgoingInfo.cachedSymbolName[symbolIndex],
                       referenceItem->fileHash, // useless put 0
                       referenceItem->vApplClass, referenceItem->vFunClass, referenceItem->type,
                       referenceItem->storage, referenceItem->scope, referenceItem->access,
                       referenceItem->category);
    lastOutgoingInfo.symbolTab[symbolIndex]       = &lastOutgoingInfo.cachedReferenceItem[symbolIndex];
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

static char tmpFileName[MAX_FILE_NAME_SIZE];

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
    for (int i=getNextExistingReferenceItem(0); i != -1; i = getNextExistingReferenceItem(i+1)) {
        for (ReferenceItem *r=getReferenceItem(i); r!=NULL; r=r->next) {
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
        mapOverFileTableWithIndex(writeFileNumberItem);
        mapOverFileTableWithIndex(writeFileSourceIndexItem);
        scanCxFile(fullScanFunctionSequence);
        mapOverReferenceTable(writeReferenceItem);
        closeReferenceFile(filename);
    } else {
        /* several reference files */
        char referenceFileName[MAX_FILE_NAME_SIZE];
        char *dirname = filename;

        createDirectory(dirname);
        writePartialReferenceFile(updating,dirname,REFERENCE_FILENAME_FILES,
                                 writeFileNumberItem, writeFileSourceIndexItem);
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
                                           CxScanFileOperation operation
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
                                      CxScanFileOperation operation
) {
    char versionString[TMP_STRING_SIZE];
    char thisVersionString[TMP_STRING_SIZE];

    assert(marker == CXFI_VERSION);
    getString(cb, versionString, size-1);
    get_version_string(thisVersionString);
    if (strcmp(versionString, thisVersionString) != 0) {
        writeCxFileCompatibilityError("The Tag file was not generated by the current version of C-xrefactory, recreate it");
    }
}

static void scanFunction_CheckNumber(int size,
                            int marker,
                            CharacterBuffer *cb,
                            CxScanFileOperation operation
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

static void scanFunction_ReadFileName(int fileNameLength,
                                      int marker,
                                      CharacterBuffer *cb,
                                      CxScanFileOperation operation
) {
    char id[MAX_FILE_NAME_SIZE];
    FileItem *fileItem;
    int fileNumber;
    bool isArgument, isInterface;
    time_t fumtime, umtime;

    assert(marker == CXFI_FILE_NAME);
    fumtime = (time_t) lastIncomingInfo.values[CXFI_FILE_FUMTIME];
    umtime = (time_t) lastIncomingInfo.values[CXFI_FILE_UMTIME];
    isArgument = lastIncomingInfo.values[CXFI_INPUT_FROM_COMMAND_LINE];
    isInterface=((lastIncomingInfo.values[CXFI_ACCESS_BITS] & AccessInterface)!=0);

    assert(fileNameLength < MAX_FILE_NAME_SIZE);
    getString(cb, id, fileNameLength-1);

    int lastIncomingFileNumber = lastIncomingInfo.values[CXFI_FILE_NUMBER];
    assert(lastIncomingFileNumber>=0 && lastIncomingFileNumber<MAX_FILES);

    if (!existsInFileTable(id)) {
        fileNumber = addFileNameToFileTable(id);
        fileItem = getFileItem(fileNumber);
        fileItem->isArgument = isArgument;
        fileItem->isInterface = isInterface;
        if (fileItem->lastFullUpdateMtime == 0)
            fileItem->lastFullUpdateMtime=fumtime;
        if (fileItem->lastUpdateMtime == 0)
            fileItem->lastUpdateMtime=umtime;
        assert(options.mode);
        if (options.mode == XrefMode) {
            if (operation == CXSF_GENERATE_OUTPUT) {
                writeFileNumberItem(fileItem, fileNumber);
            }
        }
    } else {
        fileNumber = lookupFileTable(id);
        fileItem = getFileItem(fileNumber);
        if (fileItemShouldBeUpdatedFromCxFile(fileItem)) {
            fileItem->isInterface = isInterface;
            // Set it to none, it will be updated by source item
            fileItem->sourceFileNumber = NO_FILE_NUMBER;
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
    decodeFileNumbers[lastIncomingFileNumber]=fileNumber;
    log_trace("%d: '%s' scanned: added as %d", lastIncomingFileNumber, id, fileNumber);
}

static void scanFunction_SourceIndex(int size,
                            int marker,
                            CharacterBuffer *cb,
                            CxScanFileOperation operation
) {
    int file, sfile;

    assert(marker == CXFI_SOURCE_INDEX);
    file = lastIncomingInfo.values[CXFI_FILE_NUMBER];
    file = decodeFileNumbers[file];
    sfile = lastIncomingInfo.values[CXFI_SOURCE_INDEX];
    sfile = decodeFileNumbers[sfile];

    FileItem *fileItem = getFileItem(file);
    assert(file>=0 && file<MAX_FILES && fileItem);
    // hmmm. here be more generous in getting correct source info
    if (fileItem->sourceFileNumber == NO_FILE_NUMBER) {
        // first check that it is not set directly from source
        if (!fileItem->cxLoading) {
            fileItem->sourceFileNumber = sfile;
        }
    }
}

static int scanSymNameString(CharacterBuffer *cb, char *id, int size) {
    assert(size < MAX_CX_SYMBOL_SIZE);
    getString(cb, id, size-1);

    return size-1;
}


static void getSymbolTypeAndClasses(Type *symbolType, int *vApplClass, int *vFunClass) {
    *symbolType = lastIncomingInfo.values[CXFI_SYMBOL_TYPE];

    *vApplClass = decodeFileNumbers[lastIncomingInfo.values[CXFI_SUBCLASS]];
    assert(getFileItem(*vApplClass) != NULL);

    *vFunClass = decodeFileNumbers[lastIncomingInfo.values[CXFI_SUPERCLASS]];
    assert(getFileItem(*vFunClass) != NULL);
}



static void scanFunction_SymbolNameForFullUpdateSchedule(int size,
                                                int marker,
                                                CharacterBuffer *cb,
                                                CxScanFileOperation operation
) {
    ReferenceItem *memb;
    int symbolIndex, len, vApplClass, vFunClass, accessFlags;
    Type symbolType;
    int storage;
    char *id;

    assert(marker == CXFI_SYMBOL_NAME);
    accessFlags = lastIncomingInfo.values[CXFI_ACCESS_BITS];
    storage = lastIncomingInfo.values[CXFI_STORAGE];
    symbolIndex = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    assert(symbolIndex>=0 && symbolIndex<MAX_CX_SYMBOL_TAB);
    id = lastIncomingInfo.cachedSymbolName[symbolIndex];
    len = scanSymNameString(cb, id, size);
    getSymbolTypeAndClasses(&symbolType, &vApplClass, &vFunClass);
    //&fprintf(dumpOut,":scanning ref of %s %d %d: \n",id,symbolType,vFunClass);fflush(dumpOut);
    if (symbolType!=TypeCppInclude || strcmp(id, LINK_NAME_INCLUDE_REFS)!=0) {
        lastIncomingInfo.onLineReferencedSym = -1;
        return;
    }

    ReferenceItem *referenceItem = &lastIncomingInfo.cachedReferenceItem[symbolIndex];
    lastIncomingInfo.symbolTab[symbolIndex] = referenceItem;
    fillReferenceItem(referenceItem, id, cxFileHashNumber(id), //useless, put 0
                       vApplClass, vFunClass, symbolType, storage, ScopeGlobal, accessFlags, CategoryGlobal);

    if (!isMemberInReferenceTable(referenceItem, NULL, &memb)) {
        // TODO: This is more or less the body of a newReferenceItem()
        char *ss = cxAlloc(len+1);
        strcpy(ss,id);
        memb = cxAlloc(sizeof(ReferenceItem));
        fillReferenceItem(memb, ss, cxFileHashNumber(ss),
                           vApplClass, vFunClass, symbolType, storage,
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
        olAddBrowsedSymbolToMenu(&sessionData.browserStack.top->hkSelectedSym, lastIncomingInfo.symbolTab[lastIncomingInfo.symbolToCheckForDeadness],
                           true, true, 0, UsageDefined, 0, &noPosition, UsageDefined);
    }
}


static bool symbolIsReportableAsUnused(ReferenceItem *referenceItem) {
    if (referenceItem==NULL || referenceItem->linkName[0]==' ')
        return false;

    // you need to be strong here, in fact struct record can be used
    // without using struct explicitly
    if (referenceItem->type == TypeStruct)
        return false;

    // in this first approach restrict this to variables and functions
    if (referenceItem->type == TypeMacro)
        return false;
    return true;
}

static bool canBypassAcceptableSymbol(ReferenceItem *symbol) {
    int nlen,len;
    char *nn, *nnn;

    getBareName(symbol->linkName, &nn, &len);
    getBareName(options.browsedSymName, &nnn, &nlen);
    if (len != nlen)
        return false;
    if (strncmp(nn, nnn, len))
        return false;
    return true;
}

static void scanFunction_SymbolName(int size,
                                    int marker,
                                    CharacterBuffer *cb,
                                    CxScanFileOperation operation
) {
    ReferenceItem *referencesItem, *member;
    int symbolIndex, vApplClass, vFunClass, accessFlags, storage;
    Type symbolType;
    char *id;

    assert(marker == CXFI_SYMBOL_NAME);
    if (options.mode==ServerMode && operation==CXSF_DEAD_CODE_DETECTION) {
        // check if previous symbol was dead
        cxfileCheckLastSymbolDeadness();
    }
    accessFlags = lastIncomingInfo.values[CXFI_ACCESS_BITS];
    storage = lastIncomingInfo.values[CXFI_STORAGE];
    symbolIndex = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];
    assert(symbolIndex>=0 && symbolIndex<MAX_CX_SYMBOL_TAB);
    id = lastIncomingInfo.cachedSymbolName[symbolIndex];
    scanSymNameString(cb, id, size);
    getSymbolTypeAndClasses(&symbolType, &vApplClass, &vFunClass);

    referencesItem = &lastIncomingInfo.cachedReferenceItem[symbolIndex];
    lastIncomingInfo.symbolTab[symbolIndex] = referencesItem;
    fillReferenceItem(referencesItem, id,
                       cxFileHashNumber(id), // useless put 0
                       vApplClass, vFunClass, symbolType, storage, ScopeGlobal, accessFlags,
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
        if (operation == CXSF_DEAD_CODE_DETECTION) {
            if (symbolIsReportableAsUnused(lastIncomingInfo.symbolTab[symbolIndex])) {
                lastIncomingInfo.symbolToCheckForDeadness = symbolIndex;
                lastIncomingInfo.deadSymbolIsDefined = 0;
            } else {
                lastIncomingInfo.symbolToCheckForDeadness = -1;
            }
        } else if (options.serverOperation!=OLO_TAG_SEARCH) {
            int ols = 0;
            SymbolsMenu *cms = NULL;
            if (operation == CXSF_MENU_CREATION) {
                cms = createSelectionMenu(referencesItem);
                if (cms == NULL) {
                    ols = 0;
                } else {
                    if (IS_BEST_FIT_MATCH(cms))
                        ols = 2;
                    else
                        ols = 1;
                }
            } else if (operation!=CXSF_BY_PASS) {
                ols=itIsSymbolToPushOlReferences(referencesItem, sessionData.browserStack.top, &cms,
                                                 DEFAULT_VALUE);
            }
            lastIncomingInfo.onLineRefMenuItem = cms;
            if (ols || (operation==CXSF_BY_PASS && canBypassAcceptableSymbol(referencesItem))) {
                lastIncomingInfo.onLineReferencedSym = symbolIndex;
                lastIncomingInfo.onLineRefIsBestMatchFlag = (ols == 2);
                log_trace("symbol %s is O.K. for %s (ols==%d)", referencesItem->linkName, options.browsedSymName, ols);
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
                                               CxScanFileOperation operation
) {
    Position pos;
    int      file, line, col, symbolIndex, vApplClass, vFunClass;
    UsageKind usageKind;
    Usage usage;
    int requiredAccess;
    Type symbolType;

    assert(marker == CXFI_REFERENCE);

    usageKind = lastIncomingInfo.values[CXFI_USAGE];
    requiredAccess = lastIncomingInfo.values[CXFI_REQUIRED_ACCESS];
    fillUsage(&usage, usageKind, requiredAccess);

    symbolIndex = lastIncomingInfo.values[CXFI_SYMBOL_INDEX];

    file = lastIncomingInfo.values[CXFI_FILE_NUMBER];
    file = decodeFileNumbers[file];

    line = lastIncomingInfo.values[CXFI_LINE_INDEX];
    col = lastIncomingInfo.values[CXFI_COLUMN_INDEX];
    getSymbolTypeAndClasses(&symbolType, &vApplClass, &vFunClass);
    log_trace("%d %d->%d %d", usageKind, file, decodeFileNumbers[file], line);

    pos = makePosition(file, line, col);
    if (lastIncomingInfo.onLineReferencedSym == lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
        addToReferenceList(&lastIncomingInfo.symbolTab[symbolIndex]->references, usage, pos);
    }
}

static bool isInReferenceList(Reference *list, Usage usage, Position position) {
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
                          CxScanFileOperation operation
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

    file = lastIncomingInfo.values[CXFI_FILE_NUMBER];
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
            copyrefFl = !isInReferenceList(lastIncomingInfo.symbolTab[sym]->references,
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
        if (operation == CXSF_DEAD_CODE_DETECTION) {
            if (OL_VIEWABLE_REFS(&reference)) {
                // restrict reported symbols to those defined in project input file
                if (isDefinitionUsage(reference.usage.kind)
                    && referenceFileItem->isArgument
                ) {
                    lastIncomingInfo.deadSymbolIsDefined = 1;
                } else if (! isDefinitionOrDeclarationUsage(reference.usage.kind)) {
                    lastIncomingInfo.symbolToCheckForDeadness = -1;
                }
            }
        } else if (operation == CXSF_PASS_MACRO_USAGE) {
            if (lastIncomingInfo.onLineReferencedSym ==
                lastIncomingInfo.values[CXFI_SYMBOL_INDEX]
                && reference.usage.kind == UsageMacroBaseFileUsage
            ) {
                s_olMacro2PassFile = reference.position.file;
            }
        } else {
            if (options.serverOperation == OLO_TAG_SEARCH) {
                if (reference.usage.kind==UsageDefined
                    || ((options.searchKind==SEARCH_FULL
                         || options.searchKind==SEARCH_FULL_SHORT)
                        &&  (reference.usage.kind==UsageDeclared
                             || reference.usage.kind==UsageClassFileDefinition))) {
                    searchSymbolCheckReference(lastIncomingInfo.symbolTab[sym],&reference);
                }
            } else {
                if (lastIncomingInfo.onLineReferencedSym == lastIncomingInfo.values[CXFI_SYMBOL_INDEX]) {
                    if (operation == CXSF_MENU_CREATION) {
                        assert(lastIncomingInfo.onLineRefMenuItem);
                        if (file != olOriginalFileNumber || !fileItem->isArgument ||
                            options.serverOperation == OLO_GOTO || options.serverOperation == OLO_CGOTO ||
                            options.serverOperation == OLO_PUSH_NAME) {
                            log_trace (":adding reference %s:%d", referenceFileItem->name, reference.position.line);
                            olcxAddReferenceToSymbolsMenu(lastIncomingInfo.onLineRefMenuItem, &reference,
                                                          lastIncomingInfo.onLineRefIsBestMatchFlag);
                        }
                    } else if (operation == CXSF_BY_PASS) {
                        if (positionsAreEqual(s_olcxByPassPos,reference.position)) {
                            // got the bypass reference
                            log_trace(":adding bypass selected symbol %s", lastIncomingInfo.symbolTab[sym]->linkName);
                            olAddBrowsedSymbolToMenu(&sessionData.browserStack.top->hkSelectedSym, lastIncomingInfo.symbolTab[sym],
                                               true, true, 0, usageKind,0,&noPosition, UsageNone);
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


static void scanFunction_ReferenceFileCountCheck(int fileCountInReferenceFile,
                                                 int marker,
                                                 CharacterBuffer *cb,
                                                 CxScanFileOperation operation
) {
    if (!currentReferenceFileCountMatches(fileCountInReferenceFile)) {
        assert(options.mode);
        FATAL_ERROR(ERR_ST,"Tag file was generated with different '-refnum' options, recreate it!", XREF_EXIT_ERR);
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
    lastIncomingInfo.markers[CXFI_SUBCLASS] = NO_FILE_NUMBER;
    lastIncomingInfo.markers[CXFI_SUPERCLASS] = NO_FILE_NUMBER;
    decodeFileNumbers[NO_FILE_NUMBER] = NO_FILE_NUMBER;

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
static void readOneAppropiateReferenceFile(char *symbolName,
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

void fullScanFor(char *symbolName) {
    readOneAppropiateReferenceFile(symbolName, fullUpdateFunctionSequence);
}

void scanForBypass(char *symbolName) {
    readOneAppropiateReferenceFile(symbolName, byPassFunctionSequence);
}

void scanReferencesToCreateMenu(char *symbolName){
    readOneAppropiateReferenceFile(symbolName, symbolMenuCreationFunctionSequence);
}

void scanForMacroUsage(char *symbolName) {
    readOneAppropiateReferenceFile(symbolName, secondPassMacroUsageFunctionSequence);
}

void scanForGlobalUnused(char *cxrefLocation) {
    scanReferenceFiles(cxrefLocation, globalUnusedDetectionFunctionSequence);
}

void scanForSearch(char *cxrefLocation) {
    scanReferenceFiles(cxrefLocation, symbolSearchFunctionSequence);
}


/* ************************************************************ */

static ScanFileFunctionStep normalScanFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep fullScanFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_GENERATE_OUTPUT},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_GENERATE_OUTPUT},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_DEFAULT},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_FIRST_PASS},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep byPassFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_BY_PASS},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_BY_PASS},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolMenuCreationFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_MENU_CREATION},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_MENU_CREATION},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep fullUpdateFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolNameForFullUpdateSchedule, CXSF_NOP},
    {CXFI_REFERENCE, scanFunction_ReferenceForFullUpdateSchedule, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep secondPassMacroUsageFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_PASS_MACRO_USAGE},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_PASS_MACRO_USAGE},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolSearchFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, SEARCH_SYMBOL},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_FIRST_PASS},
    {-1,NULL, 0},
};

static ScanFileFunctionStep globalUnusedDetectionFunctionSequence[]={
    {CXFI_MARKER_LIST, scanFunction_ReadRecordMarkers, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SOURCE_INDEX, scanFunction_SourceIndex, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_DEAD_CODE_DETECTION},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_DEAD_CODE_DETECTION},
    {-1,NULL, 0},
};

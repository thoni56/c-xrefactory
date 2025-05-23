#include "cxfile.h"

#include <ctype.h>
#include <string.h>

#include "characterreader.h"
#include "commons.h"
#include "completion.h"
#include "cxref.h"
#include "editor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "reference.h"
#include "reftab.h"
#include "session.h"
#include "usage.h"


/* *********************** INPUT/OUTPUT FIELD MARKERS ************************** */

#define C_XREF_FILE_FORMAT_VERSION "1.8.0"

typedef enum {
    CXFI_FILE_FUMTIME          = 'm',     /* last full update mtime for file item */
    CXFI_FILE_UMTIME           = 'p',     /* last update mtime for file item */
    CXFI_COMMAND_LINE_ARGUMENT = 'i',     /* file was introduced from command line */

    CXFI_FILE_NUMBER           = 'f',
    CXFI_SYMBOL_INDEX          = 's',
    CXFI_USAGE                 = 'u',
    CXFI_LINE_INDEX            = 'l',
    CXFI_COLUMN_INDEX          = 'c',
    CXFI_REFERENCE             = 'r',     /* using 'fsulc', records */

    CXFI_SYMBOL_TYPE           = 't',

    CXFI_STORAGE               = 'g',     /* storaGe field */

    CXFI_SYMBOL_NAME           = '/',     /* using 'atdhg' -> 's',             */
    CXFI_FILE_NAME             = ':',     /*               -> 'ifm' info  */

    CXFI_CHECK_NUMBER          = 'k',
    CXFI_REFNUM                = 'n',
    CXFI_VERSION               = 'v',
    CXFI_KEY_LIST              = '@',
    CXFI_REMARK                = '#',
    CXFI_INCLUDEFILENUMBER     = 'd',     /* was dole = down in slovac, for subclass,
                                           * now file number for included file */
} CxFieldTag;

typedef enum {
    CXSF_NOP,
    CXSF_DEFAULT,
    CXSF_JUST_READ,
    CXSF_GENERATE_OUTPUT,
    CXSF_BYPASS,
    CXSF_FIRST_PASS,
    CXSF_MENU_CREATION,
    CXSF_DEAD_CODE_DETECTION,
    CXSF_PASS_MACRO_USAGE,
} CxScanFileOperation;


static int generatedFieldKeyList[] = {
    CXFI_FILE_FUMTIME,
    CXFI_FILE_UMTIME,
    CXFI_FILE_NUMBER,
    CXFI_SYMBOL_TYPE,
    CXFI_USAGE,
    CXFI_LINE_INDEX,
    CXFI_COLUMN_INDEX,
    CXFI_SYMBOL_INDEX,
    CXFI_REFERENCE,
    CXFI_INCLUDEFILENUMBER,
    CXFI_COMMAND_LINE_ARGUMENT,
    CXFI_REFNUM,
    CXFI_STORAGE,
    CXFI_CHECK_NUMBER,
    -1
};


typedef struct lastCxFileData {
    int                 onLineReferencedSym;
    SymbolsMenu        *onLineRefMenuItem;
    ReferenceItem      *referenceItem;
    bool                symbolIsWritten;
    bool                macroBaseFileGeneratedForSymbol;
    bool                keyUsed[MAX_CHARS];
    int                 data[MAX_CHARS];
    void                (*handlerFunction[MAX_CHARS])(int size, int key, CharacterBuffer *cb,
                                                      CxScanFileOperation operation);
    int                 argument[MAX_CHARS];

    // dead code detection vars
    int                 symbolToCheckForDeadness;
    char                deadSymbolIsDefined;

    // following item can be used only via symbolTab,
    // it is just to simplify memory handling !!!!!!!!!!!!!!!!
    ReferenceItem      cachedReferenceItem;
    char               cachedSymbolName[MAX_CX_SYMBOL_SIZE];
} LastCxFileData;

static LastCxFileData lastIncomingData;
static LastCxFileData lastOutgoingData;


static CharacterBuffer cxFileCharacterBuffer;

static unsigned fileNumberMapping[MAX_FILES];

static FILE *cxFile = NULL;

static FILE *currentReferenceFile;

typedef struct scanFileFunctionStep {
    int		recordCode;
    void    (*handlerFunction)(int size, int ri, CharacterBuffer *cb, CxScanFileOperation operation); /* TODO: Break out a type */
    int		argument;
} ScanFileFunctionStep;


static ScanFileFunctionStep normalScanFunctionSequence[];
static ScanFileFunctionStep fullScanFunctionSequence[];
static ScanFileFunctionStep fullUpdateFunctionSequence[];
static ScanFileFunctionStep byPassFunctionSequence[];
static ScanFileFunctionStep symbolMenuCreationFunctionSequence[];
static ScanFileFunctionStep secondPassMacroUsageFunctionSequence[];
static ScanFileFunctionStep globalUnusedDetectionFunctionSequence[];
static ScanFileFunctionStep symbolSearchFunctionSequence[];

static void scanCxFileUsing(ScanFileFunctionStep *scanFunctionTable);


static void fPutDecimal(int num, FILE *file) {
    fprintf(file, "%d", num);
}


/* *********************** INPUT/OUTPUT ************************** */

static unsigned increment_symbol_hash(unsigned old_hash, int charcode) {
    unsigned new_hash = old_hash;
    new_hash+=charcode;
    new_hash+=(new_hash<<10);
    new_hash^=(new_hash>>6);
    return new_hash;
}

static unsigned finalize_symbol_hash(unsigned old_hash) {
    unsigned new_hash = old_hash;
    new_hash+=(new_hash<<3); new_hash^=(new_hash>>11); new_hash+=(new_hash<<15);
    return new_hash;
}

int cxFileHashNumberForSymbol(char *symbolName) {
    unsigned   hash;
    char       *ch;
    int        c;

    if (options.referenceFileCount <= 1)
        return 0;

    hash = 0;
    ch = symbolName;
    while ((c = *ch) != '\0') {
        if (c == '(')
            break;
        hash = increment_symbol_hash(hash, c);
        if (LINK_NAME_MAYBE_START(c))
            hash = 0;
        ch++;
    }
    hash = finalize_symbol_hash(hash);
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

    prettyPrintLinkName(ssname, referenceItem->linkName, MAX_CX_SYMBOL_SIZE);
    sname = ssname;
    slen = strlen(sname);
    // if completing without profile, cut profile
    if (options.searchKind==SEARCH_DEFINITIONS_SHORT
        || options.searchKind==SEARCH_FULL_SHORT) {
        s = strchr(sname, '(');
        if (s!=NULL) *s = 0;
    }
    // cut package name(?) for checking
    do {
        s = strchr(sname, '.');
        if (s!=NULL)
            sname = s+1;
    } while (s!=NULL);
    slen = strlen(sname);
    if (searchStringMatch(sname, slen)) {
        static int count = 0;
        sessionData.retrieverStack.top->completions = completionListPrepend(
            sessionData.retrieverStack.top->completions, sname, NULL, NULL, referenceItem,
            reference, referenceItem->type, referenceItem->includedFileNumber);
        // compact completions from time to time
        count ++;
        if (count > COMPACT_TAGS_AFTER_SEARCH_COUNT) {
            tagSearchCompactShortResults();
            count = 0;
        }
    }
}

/* ************************* WRITE **************************** */

static void get_version_string(char *string) {
    sprintf(string," file format: C-xrefactory %s ", C_XREF_FILE_FORMAT_VERSION);
}


static void writeCompactRecord(char tag, int data, char *blankPrefix) {
    assert(tag >= 0 && tag < MAX_CHARS);
    assert(data >= 0);
    if (*blankPrefix!=0)
        fputs(blankPrefix, cxFile);
    if (data != 0)
        fPutDecimal(data, cxFile);
    fputc(tag, cxFile);
    lastOutgoingData.data[tag] = data;
}

static void writeOptionalCompactRecord(char tag, int data, char *blankPrefix) {
    assert(tag >= 0 && tag < MAX_CHARS);
    if (*blankPrefix!=0)
        fputs(blankPrefix, cxFile);

    /* If data for this tag is same as last, don't write anything */
    if (lastOutgoingData.data[tag] != data) {
        /* If the data to write is not 0 then write it, else just write the tag */
        if (data != 0)
            fPutDecimal(data, cxFile);
        fputc(tag, cxFile);
        lastOutgoingData.data[tag] = data;
    }
}


static void writeStringRecord(int tag, char *string, char *prefix) {
    if (*prefix!=0)
        fputs(prefix, cxFile);
    fPutDecimal(strlen(string)+1, cxFile);
    fputc(tag, cxFile);
    fputs(string, cxFile);
}

/* Here we do the actual writing of the symbol */
static void writeSymbolItem(void) {
    /* First the symbol info, if not done already */
    writeOptionalCompactRecord(CXFI_SYMBOL_INDEX, 0, "");

    /* Then the reference info */
    ReferenceItem *r = lastOutgoingData.referenceItem;
    writeOptionalCompactRecord(CXFI_SYMBOL_TYPE, r->type, "\n"); /* Why newline in the middle of all this? */
    writeOptionalCompactRecord(CXFI_INCLUDEFILENUMBER, r->includedFileNumber, ""); /* TODO - not used, but are actually include file refence */
    writeOptionalCompactRecord(CXFI_STORAGE, r->storage, "");
    lastOutgoingData.macroBaseFileGeneratedForSymbol = false;
    lastOutgoingData.symbolIsWritten = true;
    writeStringRecord(CXFI_SYMBOL_NAME, r->linkName, "\t");
    fputc('\t', cxFile);
}

static void writeSymbolItemIfNotWritten(void) {
    if (! lastOutgoingData.symbolIsWritten) {
        writeSymbolItem();
    } else {
        log_trace("Not writing already written symbol");
    }
}

static void writeCxReferenceBase(Usage usage, int file, int line, int col) {
    writeSymbolItemIfNotWritten();
    if (usage == UsageMacroBaseFileUsage) {
        /* optimize the number of those references to 1 */
        if (lastOutgoingData.macroBaseFileGeneratedForSymbol)
            return;
        lastOutgoingData.macroBaseFileGeneratedForSymbol = true;
    }
    writeOptionalCompactRecord(CXFI_USAGE, usage, "");
    writeOptionalCompactRecord(CXFI_SYMBOL_INDEX, 0, "");
    writeOptionalCompactRecord(CXFI_FILE_NUMBER, file, "");
    writeOptionalCompactRecord(CXFI_LINE_INDEX, line, "");
    writeOptionalCompactRecord(CXFI_COLUMN_INDEX, col, "");
    writeCompactRecord(CXFI_REFERENCE, 0, "");
}

static void writeCxReference(Reference *reference) {
    writeCxReferenceBase(reference->usage,
                         reference->position.file, reference->position.line, reference->position.col);
}

static void writeFileNumberItem(FileItem *fileItem, int number) {
    // keys = fpmia:
    writeOptionalCompactRecord(CXFI_FILE_NUMBER, number, "\n");
    writeOptionalCompactRecord(CXFI_FILE_UMTIME, fileItem->lastUpdateMtime, " ");
    writeOptionalCompactRecord(CXFI_FILE_FUMTIME, fileItem->lastFullUpdateMtime, " ");
    writeOptionalCompactRecord(CXFI_COMMAND_LINE_ARGUMENT, fileItem->isArgument, "");
    writeStringRecord(CXFI_FILE_NAME, fileItem->name, " ");
}

/* *************************************************************** */

static void writeReferenceItem(ReferenceItem *referenceItem) {
    log_trace("generate cxref for symbol '%s'", referenceItem->linkName);
    assert(strlen(referenceItem->linkName)+1 < MAX_CX_SYMBOL_SIZE);

    strcpy(lastOutgoingData.cachedSymbolName, referenceItem->linkName);
    lastOutgoingData.cachedReferenceItem = makeReferenceItem(
                       lastOutgoingData.cachedSymbolName, referenceItem->type,
                       referenceItem->storage, referenceItem->scope, referenceItem->visibility, referenceItem->includedFileNumber);
    lastOutgoingData.referenceItem   = &lastOutgoingData.cachedReferenceItem;
    lastOutgoingData.referenceItem   = &lastOutgoingData.cachedReferenceItem;
    lastOutgoingData.symbolIsWritten = false;

    if (referenceItem->visibility == LocalVisibility)
        return;

    for (Reference *reference = referenceItem->references; reference != NULL; reference = reference->next) {
        FileItem *fileItem = getFileItemWithFileNumber(reference->position.file);
        log_trace("checking ref: loading=%d --< %s:%d", fileItem->cxLoading,
                  fileItem->name, reference->position.line);
        if (options.update==UPDATE_DEFAULT || fileItem->cxLoading) {
            writeCxReference(reference);
        } else {
            log_trace("Some kind of update (%d) or loading (%d), so don't writeCxReference()",
                      options.update, fileItem->cxLoading);
            assert(0);
        }
    }
}

static int composeCxfiCheckNum(int fileCount, bool exactPositionLinkFlag) {
    return fileCount*XFILE_HASH_MAX*2 + exactPositionLinkFlag;
}

static void decomposeCxfiCheckNum(int magicNumber, int *fileCount, bool *exactPositionLinkFlag) {
    unsigned tmp;
    tmp = magicNumber;
    *exactPositionLinkFlag = tmp % 2;
    tmp = tmp / 2;
    tmp = tmp / XFILE_HASH_MAX;
    *fileCount = tmp;
}

static void writeCxFileHead(void) {
    char stringRecord[MAX_CHARS];
    char tempString[TMP_STRING_SIZE];
    int i;

    memset(&lastOutgoingData, 0, sizeof(lastOutgoingData));
    for (i=0; i<MAX_CHARS; i++) {
        lastOutgoingData.data[i] = -1;
    }

    get_version_string(tempString);
    writeStringRecord(CXFI_VERSION, tempString, "\n\n");
    fprintf(cxFile,"\n\n\n");

    for (i=0; i<MAX_CHARS && generatedFieldKeyList[i] != -1; i++) {
        stringRecord[i] = generatedFieldKeyList[i];
    }

    assert(i < MAX_CHARS);
    stringRecord[i]=0;
    writeStringRecord(CXFI_KEY_LIST, stringRecord, "");
    writeCompactRecord(CXFI_REFNUM, options.referenceFileCount, " ");
    writeCompactRecord(CXFI_CHECK_NUMBER, composeCxfiCheckNum(MAX_FILES, options.exactPositionResolve), " ");
}

static char tmpFileName[MAX_FILE_NAME_SIZE];

static void openInOutReferenceFile(bool updating, char *filename) {
    if (updating) {
        char *tempname = create_temporary_filename();
        strcpy(tmpFileName, tempname);
        copyFileFromTo(filename, tmpFileName);
    }

    assert(filename);
    cxFile = openFile(filename,"w");
    if (cxFile == NULL)
        FATAL_ERROR(ERR_CANT_OPEN, filename, XREF_EXIT_ERR);

    if (updating) {
        currentReferenceFile = openFile(tmpFileName, "r");
        if (currentReferenceFile==NULL)
            warningMessage(ERR_CANT_OPEN_FOR_READ, tmpFileName);
    } else {
        currentReferenceFile = NULL;
    }
}

static void closeCurrentReferenceFile(void) {
    if (currentReferenceFile != NULL) {
        closeFile(currentReferenceFile);
        currentReferenceFile = NULL;
        removeFile(tmpFileName);
    }
    closeFile(cxFile);
    cxFile = NULL;
}

/* suffix contains '/' at the beginning */
static void writePartialReferenceFile(bool updateFlag,
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
    scanCxFileUsing(fullScanFunctionSequence);
    closeCurrentReferenceFile();
}

static void writeReferencesFromMemoryIntoRefFileNo(int fileOrder) {
    for (int i=getNextExistingReferenceItem(0); i != -1; i = getNextExistingReferenceItem(i+1)) {
        for (ReferenceItem *r=getReferenceItem(i); r!=NULL; r=r->next) {
            if (r->visibility == LocalVisibility)
                continue;
            if (r->references == NULL)
                continue;
            if (cxFileHashNumberForSymbol(r->linkName) == fileOrder)
                writeReferenceItem(r);
            else
                log_trace("Skipping reference with linkname \"%s\"", r->linkName);
        }
    }
}

static void writeSingleReferenceFile(bool updating, char *filename) {
    openInOutReferenceFile(updating, filename);
    writeCxFileHead();
    mapOverFileTableWithIndex(writeFileNumberItem);
    scanCxFileUsing(fullScanFunctionSequence);
    mapOverReferenceTable(writeReferenceItem);
    closeCurrentReferenceFile();
}

static void writeMultipeReferenceFiles(bool updating, char *dirname) {
    char  referenceFileName[MAX_FILE_NAME_SIZE];

    createDirectory(dirname);
    writePartialReferenceFile(updating, dirname, REFERENCE_FILENAME_FILES, writeFileNumberItem, NULL);
    for (int i = 0; i < options.referenceFileCount; i++) {
        sprintf(referenceFileName, "%s%s%04d", dirname, REFERENCE_FILENAME_PREFIX, i);
        assert(strlen(referenceFileName) < MAX_FILE_NAME_SIZE - 1);
        openInOutReferenceFile(updating, referenceFileName);
        writeCxFileHead();
        scanCxFileUsing(fullScanFunctionSequence);
        writeReferencesFromMemoryIntoRefFileNo(i);
        closeCurrentReferenceFile();
    }
}

void writeReferenceFile(bool updating, char *filename) {
    if (!updating)
        removeFile(filename);

    recursivelyCreateFileDirIfNotExists(filename);

    if (options.referenceFileCount <= 1) {
        writeSingleReferenceFile(updating, filename);
    } else {
        writeMultipeReferenceFiles(updating, filename);
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

static void scanFunction_ReadKeys(int size,
                                  int key,
                                  CharacterBuffer *cb,
                                  CxScanFileOperation operation
) {
    assert(key == CXFI_KEY_LIST);
    for (int i=0; i<size-1; i++) {
        int ch = getChar(cb);
        lastIncomingData.keyUsed[ch] = true;
    }
}


static void scanFunction_VersionCheck(int size,
                                      int key,
                                      CharacterBuffer *cb,
                                      CxScanFileOperation operation
) {
    char versionString[TMP_STRING_SIZE];
    char thisVersionString[TMP_STRING_SIZE];

    assert(key == CXFI_VERSION);
    getString(cb, versionString, size-1);
    get_version_string(thisVersionString);
    if (strcmp(versionString, thisVersionString) != 0) {
        writeCxFileCompatibilityError("The reference database was not generated by the current version of C-xrefactory, recreate it");
    }
}

static void scanFunction_CheckNumber(int size,
                                     int key,
                                     CharacterBuffer *cb,
                                     CxScanFileOperation operation
) {
    int magicNumber, fileCount;
    bool exactPositionLinkFlag;
    char tmpBuff[TMP_BUFF_SIZE];

    assert(key == CXFI_CHECK_NUMBER);
    if (options.create)
        return; // no check when creating new file

    magicNumber = lastIncomingData.data[CXFI_CHECK_NUMBER];

    decomposeCxfiCheckNum(magicNumber, &fileCount, &exactPositionLinkFlag);
    if (fileCount != MAX_FILES) {
        sprintf(tmpBuff,"The reference database was generated with different MAX_FILES, recreate it");
        writeCxFileCompatibilityError(tmpBuff);
    }
    log_trace("checking exactPositionResolve: %d <-> %d", exactPositionLinkFlag, options.exactPositionResolve);
    if (exactPositionLinkFlag != options.exactPositionResolve) {
        if (exactPositionLinkFlag) {
            sprintf(tmpBuff,"The reference database was generated with '-exactpositionresolve' flag, recreate it");
        } else {
            sprintf(tmpBuff,"The reference database was generated without '-exactpositionresolve' flag, recreate it");
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
                                      int key,
                                      CharacterBuffer *cb,
                                      CxScanFileOperation operation
) {
    char fileName[MAX_FILE_NAME_SIZE];
    FileItem *fileItem;
    int fileNumber;
    bool isArgument;
    time_t fumtime, umtime;

    assert(key == CXFI_FILE_NAME);
    fumtime = (time_t) lastIncomingData.data[CXFI_FILE_FUMTIME];
    umtime = (time_t) lastIncomingData.data[CXFI_FILE_UMTIME];
    isArgument = lastIncomingData.data[CXFI_COMMAND_LINE_ARGUMENT];

    assert(fileNameLength < MAX_FILE_NAME_SIZE);
    getString(cb, fileName, fileNameLength-1);

    int lastIncomingFileNumber = lastIncomingData.data[CXFI_FILE_NUMBER];
    assert(lastIncomingFileNumber>=0 && lastIncomingFileNumber<MAX_FILES);

    if (!existsInFileTable(fileName)) {
        fileNumber = addFileNameToFileTable(fileName);
        fileItem = getFileItemWithFileNumber(fileNumber);
        fileItem->isArgument = isArgument;
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
        fileNumber = getFileNumberFromFileName(fileName);
        fileItem = getFileItemWithFileNumber(fileNumber);
        if (fileItemShouldBeUpdatedFromCxFile(fileItem)) {
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
    fileNumberMapping[lastIncomingFileNumber]=fileNumber;
    log_trace("%d: '%s' scanned: added as %d", lastIncomingFileNumber, fileName, fileNumber);
}

static int scanSymbolName(CharacterBuffer *cb, char *id, int size) {
    assert(size < MAX_CX_SYMBOL_SIZE);
    getString(cb, id, size-1);

    return size-1;
}


static Type getSymbolType(void) {
    return lastIncomingData.data[CXFI_SYMBOL_TYPE];
}


static void getIncludedFileNumber(int *includedFileNumber) {
    *includedFileNumber = fileNumberMapping[lastIncomingData.data[CXFI_INCLUDEFILENUMBER]];
    assert(getFileItemWithFileNumber(*includedFileNumber) != NULL);
}


static void scanFunction_SymbolNameForFullUpdateSchedule(int size,
                                                         int key,
                                                         CharacterBuffer *cb,
                                                         CxScanFileOperation operation
) {
    assert(key == CXFI_SYMBOL_NAME);
    Storage storage = lastIncomingData.data[CXFI_STORAGE];

    char *id = lastIncomingData.cachedSymbolName;
    int len = scanSymbolName(cb, id, size);

    Type symbolType = getSymbolType();
    if (symbolType!=TypeCppInclude || strcmp(id, LINK_NAME_INCLUDE_REFS)!=0) {
        lastIncomingData.onLineReferencedSym = -1;
        return;
    }

    ReferenceItem *referenceItem = &lastIncomingData.cachedReferenceItem;
    lastIncomingData.referenceItem = referenceItem;

    int includedFileNumber;
    getIncludedFileNumber(&includedFileNumber);
    *referenceItem = makeReferenceItem(id, symbolType, storage, GlobalScope, GlobalVisibility, includedFileNumber);

    ReferenceItem *foundReferenceItem;
    if (!isMemberInReferenceTable(referenceItem, NULL, &foundReferenceItem)) {
        // TODO: This is more or less the body of a newReferenceItem()
        char *ss = cxAlloc(len+1);
        strcpy(ss,id);
        foundReferenceItem = cxAlloc(sizeof(ReferenceItem));
        *foundReferenceItem = makeReferenceItem(ss, symbolType, storage,
                                                GlobalScope, GlobalVisibility, includedFileNumber);
        addToReferencesTable(foundReferenceItem);
    }
    lastIncomingData.referenceItem = foundReferenceItem;
    lastIncomingData.onLineReferencedSym = 0;
}

static void cxfileCheckLastSymbolDeadness(void) {
    if (lastIncomingData.symbolToCheckForDeadness != -1
        && lastIncomingData.deadSymbolIsDefined
    ) {
        addBrowsedSymbolToMenu(&sessionData.browserStack.top->hkSelectedSym, lastIncomingData.referenceItem,
                               true, true, 0, (SymbolRelation){.sameFile = false}, UsageDefined, 0, noPosition,
                               UsageDefined);
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
                                    int key,
                                    CharacterBuffer *cb,
                                    CxScanFileOperation operation
) {
    assert(key == CXFI_SYMBOL_NAME);
    if (options.mode==ServerMode && operation==CXSF_DEAD_CODE_DETECTION) {
        // check if previous symbol was dead
        cxfileCheckLastSymbolDeadness();
    }
    Storage storage = lastIncomingData.data[CXFI_STORAGE];

    char *id = lastIncomingData.cachedSymbolName;
    scanSymbolName(cb, id, size);

    Type symbolType = getSymbolType();

    ReferenceItem *referenceItem = &lastIncomingData.cachedReferenceItem;
    lastIncomingData.referenceItem = referenceItem;

    int includedFileNumber;
    getIncludedFileNumber(&includedFileNumber);
    *referenceItem = makeReferenceItem(id, symbolType, storage, GlobalScope, GlobalVisibility, includedFileNumber);

    ReferenceItem *foundMemberP;
    bool isMember = isMemberInReferenceTable(referenceItem, NULL, &foundMemberP);
    while (isMember && foundMemberP->visibility!=GlobalVisibility)
        isMember = refTabNextMember(referenceItem, &foundMemberP);

    assert(options.mode);
    if (options.mode == XrefMode) {
        if (foundMemberP==NULL)
            foundMemberP=referenceItem;
        writeReferenceItem(foundMemberP);
        referenceItem->references = foundMemberP->references; // note references to not generate multiple
        foundMemberP->references = NULL;      // HACK, remove them, to not be regenerated
    }
    if (options.mode == ServerMode) {
        if (operation == CXSF_DEAD_CODE_DETECTION) {
            if (symbolIsReportableAsUnused(lastIncomingData.referenceItem)) {
                lastIncomingData.symbolToCheckForDeadness = 0;
                lastIncomingData.deadSymbolIsDefined = 0;
            } else {
                lastIncomingData.symbolToCheckForDeadness = -1;
            }
        } else if (options.serverOperation!=OLO_TAG_SEARCH) {
            int ols = 0;
            SymbolsMenu *cms = NULL;
            if (operation == CXSF_MENU_CREATION) {
                cms = createSelectionMenu(referenceItem);
                if (cms == NULL) {
                    ols = 0;
                } else {
                    if (isBestFitMatch(cms))
                        ols = 2;
                    else
                        ols = 1;
                }
            } else if (operation!=CXSF_BYPASS) {
                ols=itIsSymbolToPushOlReferences(referenceItem, sessionData.browserStack.top, &cms,
                                                 DEFAULT_VALUE);
            }
            lastIncomingData.onLineRefMenuItem = cms;
            if (ols || (operation==CXSF_BYPASS && canBypassAcceptableSymbol(referenceItem))) {
                lastIncomingData.onLineReferencedSym = 0;
                log_trace("symbol %s is O.K. for %s (ols==%d)", referenceItem->linkName, options.browsedSymName, ols);
            } else {
                if (lastIncomingData.onLineReferencedSym == 0) {
                    lastIncomingData.onLineReferencedSym = -1;
                }
            }
        }
    }
}

static void scanFunction_ReferenceForFullUpdateSchedule(int size,
                                                        int key,
                                                        CharacterBuffer *cb,
                                                        CxScanFileOperation operation
) {

    assert(key == CXFI_REFERENCE);

    Usage usage = lastIncomingData.data[CXFI_USAGE];

    int unmapped_file = lastIncomingData.data[CXFI_FILE_NUMBER];
    int file = fileNumberMapping[unmapped_file];

    int line = lastIncomingData.data[CXFI_LINE_INDEX];
    int col = lastIncomingData.data[CXFI_COLUMN_INDEX];

    log_trace("Read reference with %s in file %d->%d at %d,%d", usageKindEnumName[usage],
              unmapped_file, file, line, col);

    Position pos = makePosition(file, line, col);
    if (lastIncomingData.onLineReferencedSym == lastIncomingData.data[CXFI_SYMBOL_INDEX]) {
        addToReferenceList(&lastIncomingData.referenceItem->references, pos, usage);
    }
}

static bool isInReferenceList(Reference *list, Usage usage, Position position) {
    Reference *foundReference;
    Reference reference = makeReference(position, usage, NULL);

    SORTED_LIST_FIND2(foundReference, Reference, reference, list);
    if (foundReference==NULL || SORTED_LIST_NEQ(foundReference,reference))
        return false;
    return true;
}


static void scanFunction_Reference(int size,
                                   int key,
                                   CharacterBuffer *cb,
                                   CxScanFileOperation operation
) {
    assert(key == CXFI_REFERENCE);
    Usage usage = lastIncomingData.data[CXFI_USAGE];

    int file = lastIncomingData.data[CXFI_FILE_NUMBER];
    file = fileNumberMapping[file];
    FileItem *fileItem = getFileItemWithFileNumber(file);

    int line = lastIncomingData.data[CXFI_LINE_INDEX];
    int col = lastIncomingData.data[CXFI_COLUMN_INDEX];

    assert(options.mode);
    if (options.mode == XrefMode) {
        int copyrefFl;
        if (fileItem->cxLoading && fileItem->cxSaved) {
            /* if we repass refs after overflow */
            copyrefFl = !isInReferenceList(lastIncomingData.referenceItem->references,
                                     usage, makePosition(file, line, col));
        } else {
            copyrefFl = !fileItem->cxLoading;
        }
        if (copyrefFl)
            writeCxReferenceBase(usage, file, line, col);
    } else if (options.mode == ServerMode) {
        Reference reference = makeReference(makePosition(file, line, col), usage, NULL);
        FileItem *referenceFileItem = getFileItemWithFileNumber(reference.position.file);
        if (operation == CXSF_DEAD_CODE_DETECTION) {
            if (isVisibleUsage(reference.usage)) {
                // restrict reported symbols to those defined in project input file
                if (isDefinitionUsage(reference.usage)
                    && referenceFileItem->isArgument
                ) {
                    lastIncomingData.deadSymbolIsDefined = 1;
                } else if (! isDefinitionOrDeclarationUsage(reference.usage)) {
                    lastIncomingData.symbolToCheckForDeadness = -1;
                }
            }
        } else if (operation == CXSF_PASS_MACRO_USAGE) {
            if (lastIncomingData.onLineReferencedSym == lastIncomingData.data[CXFI_SYMBOL_INDEX]
                && reference.usage == UsageMacroBaseFileUsage
            ) {
                olMacro2PassFile = reference.position.file;
            }
        } else {
            if (options.serverOperation == OLO_TAG_SEARCH) {
                if (reference.usage==UsageDefined
                    || ((options.searchKind==SEARCH_FULL
                         || options.searchKind==SEARCH_FULL_SHORT)
                        && reference.usage==UsageDeclared)
                    ) {
                    searchSymbolCheckReference(lastIncomingData.referenceItem, &reference);
                }
            } else {
                if (lastIncomingData.onLineReferencedSym == lastIncomingData.data[CXFI_SYMBOL_INDEX]) {
                    if (operation == CXSF_MENU_CREATION) {
                        assert(lastIncomingData.onLineRefMenuItem);
                        if (file != originalFileNumber || !fileItem->isArgument ||
                            options.serverOperation == OLO_GOTO || options.serverOperation == OLO_COMPLETION_GOTO ||
                            options.serverOperation == OLO_PUSH_NAME) {
                            log_trace (":adding reference %s:%d", referenceFileItem->name, reference.position.line);
                            olcxAddReferenceToSymbolsMenu(lastIncomingData.onLineRefMenuItem, &reference);
                        }
                    } else if (operation == CXSF_BYPASS) {
                        if (positionsAreEqual(olcxByPassPos,reference.position)) {
                            // got the bypass reference
                            log_trace(":adding bypass selected symbol %s", lastIncomingData.referenceItem->linkName);
                            addBrowsedSymbolToMenu(&sessionData.browserStack.top->hkSelectedSym,
                                                   lastIncomingData.referenceItem, true, true,
                                                   0, (SymbolRelation){.sameFile = false}, usage, 0, noPosition,
                                                   UsageNone);
                        }
                    } else {
                        addReferenceToList(&sessionData.browserStack.top->references, &reference);
                    }
                }
            }
        }
    }
}


static void scanFunction_ReferenceFileCountCheck(int fileCountInReferenceFile,
                                                 int key,
                                                 CharacterBuffer *cb,
                                                 CxScanFileOperation operation
) {
    if (!currentReferenceFileCountMatches(fileCountInReferenceFile)) {
        assert(options.mode);
        FATAL_ERROR(ERR_ST,"The reference database was generated with different '-refnum' options, recreate it!", XREF_EXIT_ERR);
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

static void resetIncomingData() {
    memset(&lastIncomingData, 0, sizeof(lastIncomingData));
    lastIncomingData.onLineReferencedSym             = -1;
    lastIncomingData.symbolToCheckForDeadness        = -1;
    lastIncomingData.onLineRefMenuItem               = NULL;
    lastIncomingData.keyUsed[CXFI_INCLUDEFILENUMBER] = NO_FILE_NUMBER;
    fileNumberMapping[NO_FILE_NUMBER]                = NO_FILE_NUMBER;
}

static void setupRecordKeyHandlersFromTable(ScanFileFunctionStep scanFunctionTable[]) {
    /* Set up the keys and handlers from the provided table */
    for (int i = 0; scanFunctionTable[i].recordCode > 0; i++) {
        assert(scanFunctionTable[i].recordCode < MAX_CHARS);
        int ch = scanFunctionTable[i].recordCode;
        lastIncomingData.handlerFunction[ch] = scanFunctionTable[i].handlerFunction;
        lastIncomingData.argument[ch] = scanFunctionTable[i].argument;
    }
}

static void scanCxFileUsing(ScanFileFunctionStep scanFunctionTable[]) {
    ENTER();
    if (currentReferenceFile == NULL) {
        log_trace("No reference file opened");
        LEAVE();
        return;
    }

    resetIncomingData();

    setupRecordKeyHandlersFromTable(scanFunctionTable);

    initCharacterBufferFromFile(&cxFileCharacterBuffer, currentReferenceFile);
    int ch = ' ';
    while (!cxFileCharacterBuffer.isAtEOF) {
        int scannedInt = scanInteger(&cxFileCharacterBuffer, &ch);

        if (cxFileCharacterBuffer.isAtEOF)
            break;

        assert(ch >= 0 && ch<MAX_CHARS);
        if (lastIncomingData.keyUsed[ch]) {
            lastIncomingData.data[ch] = scannedInt;
        }
        if (lastIncomingData.handlerFunction[ch] != NULL) {
            (*lastIncomingData.handlerFunction[ch])(scannedInt, ch, &cxFileCharacterBuffer,
                                                    lastIncomingData.argument[ch]);
        } else if (!lastIncomingData.keyUsed[ch]) {
            /* We are not interested in this value so just skip over */
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
    currentReferenceFile = openFile(fn, "r");
    if (currentReferenceFile==NULL) {
        return false;
    } else {
        scanCxFileUsing(scanFunctionTable);
        closeFile(currentReferenceFile);
        currentReferenceFile = NULL;
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
        int i = cxFileHashNumberForSymbol(symbolName);

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
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep fullScanFunctionSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_GENERATE_OUTPUT},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_DEFAULT},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_FIRST_PASS},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep byPassFunctionSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_BYPASS},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_BYPASS},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolMenuCreationFunctionSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_MENU_CREATION},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_MENU_CREATION},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep fullUpdateFunctionSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolNameForFullUpdateSchedule, CXSF_NOP},
    {CXFI_REFERENCE, scanFunction_ReferenceForFullUpdateSchedule, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep secondPassMacroUsageFunctionSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_PASS_MACRO_USAGE},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_PASS_MACRO_USAGE},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static ScanFileFunctionStep symbolSearchFunctionSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, SEARCH_SYMBOL},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_FIRST_PASS},
    {-1,NULL, 0},
};

static ScanFileFunctionStep globalUnusedDetectionFunctionSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_ReferenceFileCountCheck, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_DEAD_CODE_DETECTION},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_DEAD_CODE_DETECTION},
    {-1,NULL, 0},
};

#include "cxfile.h"

#include <ctype.h>
#include <string.h>

#include "browsermenu.h"
#include "characterreader.h"
#include "commons.h"
#include "cxref.h"
#include "editor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "list.h"
#include "log.h"
#include "match.h"
#include "misc.h"
#include "options.h"
#include "parsing.h"
#include "referenceableitem.h"
#include "referenceableitemtable.h"
#include "search.h"
#include "session.h"
#include "usage.h"


#define COMPACT_TAGS_AFTER_SEARCH_COUNT 10000	/* compact tag search results after n items*/


/* ***************************  cxref filenames ********************* */

#if defined(__WIN32__)

#define CXFILENAME_FILES "\\XFiles"
#define CXFILENAME_PREFIX "\\X"

#else

#define CXFILENAME_FILES "/XFiles"
#define CXFILENAME_PREFIX "/X"

#endif

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
    CXSF_FIRST_PASS,
    CXSF_MENU_CREATION,
    CXSF_DEAD_CODE_DETECTION,
    CXSF_PASS_MACRO_USAGE,
} CxFileScanOperation;


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
    BrowserMenu        *onLineRefMenuItem;
    ReferenceableItem      *referenceableItem;
    bool                symbolIsWritten;
    bool                macroBaseFileGeneratedForSymbol;
    bool                keyUsed[MAX_CHARS];
    int                 data[MAX_CHARS];
    void                (*handlerFunction[MAX_CHARS])(int size, int key, CharacterBuffer *cb,
                                                      CxFileScanOperation operation);
    int                 argument[MAX_CHARS];

    // dead code detection vars
    int                 symbolToCheckForDeadness;
    char                deadSymbolIsDefined;

    // following item can be used only via symbolTab,
    // it is just to simplify memory handling !!!!!!!!!!!!!!!!
    ReferenceableItem      cachedReferenceableItem;
    char               cachedSymbolName[MAX_CX_SYMBOL_SIZE];
} LastCxFileData;

static LastCxFileData lastIncomingData;
static LastCxFileData lastOutgoingData;


static CharacterBuffer cxFileCharacterBuffer;

static unsigned fileNumberMapping[MAX_FILES];

static FILE *cxFile = NULL;

static FILE *currentCxFile;

typedef struct cxFileScanStep {
    int		recordCode;
    void    (*handlerFunction)(int size, int ri, CharacterBuffer *cb, CxFileScanOperation operation); /* TODO: Break out a type */
    int		argument;
} CxFileScanStep;


static CxFileScanStep normalScanSequence[];
static CxFileScanStep fullScanSequence[];
static CxFileScanStep fullUpdateSequence[];
static CxFileScanStep symbolMenuCreationSequence[];
static CxFileScanStep secondPassMacroUsageSequence[];
static CxFileScanStep globalUnusedDetectionSequence[];
static CxFileScanStep symbolSearchSequence[];

static void scanCxFileUsing(CxFileScanStep *scanSequence);


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

    if (options.cxFileCount <= 1)
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
    hash %= options.cxFileCount;
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
    assert(searchedStr);

    if (searchedStr[0] == '^') {
        // check for exact prefix
        return searchSingleStringEqual(searchedStr+1, cxtag);
    } else {
        char *cc = cxtag;
        int i;
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

static bool searchStringMatch(char *cxtag, int len) {
    if (containsWildcard(options.olcxSearchString))
        return shellMatch(cxtag, len, options.olcxSearchString, false);
    else
        return searchStringNonWildcardFitness(cxtag, len);
}


void searchSymbolCheckReference(ReferenceableItem *referenceableItem, Reference *reference) {
    char buffer[MAX_CX_SYMBOL_SIZE];
    char *s, *name;

    if (referenceableItem->type == TypeCppInclude)
        return;   // no %%i symbols (== "#include" ReferenceableItem)

    prettyPrintLinkName(buffer, referenceableItem->linkName, MAX_CX_SYMBOL_SIZE);
    name = buffer;

    // if completing without profile, cut profile
    if (options.searchKind==SEARCH_DEFINITIONS_SHORT || options.searchKind==SEARCH_FULL_SHORT) {
        s = strchr(name, '(');
        if (s!=NULL)
            *s = 0;
    }

    // Remove any possible struct/union member name so that the reference refers to the same referenceable
    do {
        s = strchr(name, '.');
        if (s!=NULL)
            name = s+1;
    } while (s!=NULL);

    if (searchStringMatch(name, strlen(name))) {
        sessionData.searchingStack.top->matches = prependToMatches(
            sessionData.searchingStack.top->matches, name, NULL, NULL, referenceableItem,
            reference, referenceableItem->includeFileNumber);

        // compact completions from time to time
        static int count = 0;
        count ++;
        if (count > COMPACT_TAGS_AFTER_SEARCH_COUNT) {
            sortMatchListByName(&sessionData.searchingStack.top->matches);
            compactSearchResultsShort();
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
    ReferenceableItem *r = lastOutgoingData.referenceableItem;
    writeOptionalCompactRecord(CXFI_SYMBOL_TYPE, r->type, "\n"); /* Why newline in the middle of all this? */
    writeOptionalCompactRecord(CXFI_INCLUDEFILENUMBER, r->includeFileNumber, ""); /* TODO - not used, but are actually include file refence */
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
    writeOptionalCompactRecord(CXFI_FILE_UMTIME, fileItem->lastParsedMtime, " ");
    writeOptionalCompactRecord(CXFI_FILE_FUMTIME, fileItem->lastFullUpdateMtime, " ");
    writeOptionalCompactRecord(CXFI_COMMAND_LINE_ARGUMENT, fileItem->isArgument, "");
    writeStringRecord(CXFI_FILE_NAME, fileItem->name, " ");
}

/* *************************************************************** */

static void writeReferenceableItem(ReferenceableItem *referenceableItem) {
    log_trace("generate cxref for symbol '%s'", referenceableItem->linkName);
    assert(strlen(referenceableItem->linkName)+1 < MAX_CX_SYMBOL_SIZE);

    strcpy(lastOutgoingData.cachedSymbolName, referenceableItem->linkName);
    lastOutgoingData.cachedReferenceableItem = makeReferenceableItem(lastOutgoingData.cachedSymbolName,
                                                                 referenceableItem->type, referenceableItem->storage,
                                                                 referenceableItem->scope, referenceableItem->visibility,
                                                                 referenceableItem->includeFileNumber);
    lastOutgoingData.referenceableItem   = &lastOutgoingData.cachedReferenceableItem;
    lastOutgoingData.referenceableItem   = &lastOutgoingData.cachedReferenceableItem;
    lastOutgoingData.symbolIsWritten = false;

    if (referenceableItem->visibility == VisibilityLocal)
        return;

    for (Reference *reference = referenceableItem->references; reference != NULL; reference = reference->next) {
        FileItem *fileItem = getFileItemWithFileNumber(reference->position.file);
        log_trace("checking ref: loading=%d --< %s:%d", fileItem->cxLoading,
                  fileItem->name, reference->position.line);
        if (options.update==UPDATE_DEFAULT || fileItem->cxLoading) {
            writeCxReference(reference);
        } else {
            log_trace("Update mode (%d) and file not loading (%d), skipping (will be copied from old CXrefs)",
                      options.update, fileItem->cxLoading);
            /* During updates, references for unchanged files will be copied from the old CXrefs file */
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
    writeCompactRecord(CXFI_REFNUM, options.cxFileCount, " ");
    writeCompactRecord(CXFI_CHECK_NUMBER, composeCxfiCheckNum(MAX_FILES, options.exactPositionResolve), " ");
}

static char tmpFileName[MAX_FILE_NAME_SIZE];

static void openInOutCxFile(bool updating, char *cxFileName) {
    if (updating) {
        if (fileExists(cxFileName)) {
            char *tempname = create_temporary_filename();
            strcpy(tmpFileName, tempname);
            copyFileFromTo(cxFileName, tmpFileName);
        } else
            tmpFileName[0] = '\0';
    }

    assert(cxFileName);
    cxFile = openFile(cxFileName,"w");
    if (cxFile == NULL)
        FATAL_ERROR(ERR_CANT_OPEN, cxFileName, XREF_EXIT_ERR);

    if (updating) {
        if (tmpFileName[0] != '\0') {
            currentCxFile = openFile(tmpFileName, "r");
            if (currentCxFile==NULL)
                warningMessage(ERR_CANT_OPEN_FOR_READ, tmpFileName);
        } else {
            currentCxFile = NULL;
        }
    } else {
        currentCxFile = NULL;
    }
}

static void closeCurrentCxFile(void) {
    if (currentCxFile != NULL) {
        closeFile(currentCxFile);
        currentCxFile = NULL;
        removeFile(tmpFileName);
    }
    closeFile(cxFile);
    cxFile = NULL;
}

/* suffix contains '/' at the beginning */
static void writePartialCxFile(bool updateFlag, char *dirname, char *suffix,
                               void mapfun(FileItem *, int), void mapfun2(FileItem *, int)) {
    char cxFileName[MAX_FILE_NAME_SIZE];

    sprintf(cxFileName, "%s%s", dirname, suffix);
    assert(strlen(cxFileName) < MAX_FILE_NAME_SIZE-1);
    openInOutCxFile(updateFlag, cxFileName);
    writeCxFileHead();
    mapOverFileTableWithIndex(mapfun);
    if (mapfun2!=NULL)
        mapOverFileTableWithIndex(mapfun2);
    scanCxFileUsing(fullScanSequence);
    closeCurrentCxFile();
}

static void writeReferencesFromMemoryIntoCxFile(int partitionNumber) {
    for (int i=getNextExistingReferenceableItem(0); i != -1; i = getNextExistingReferenceableItem(i+1)) {
        for (ReferenceableItem *r=getReferenceableItem(i); r!=NULL; r=r->next) {
            if (r->visibility == VisibilityLocal)
                continue;
            if (r->references == NULL)
                continue;
            if (cxFileHashNumberForSymbol(r->linkName) == partitionNumber)
                writeReferenceableItem(r);
            else
                log_trace("Skipping reference with linkname \"%s\"", r->linkName);
        }
    }
}

static void writeSingleCxFile(bool updating, char *filename) {
    openInOutCxFile(updating, filename);
    writeCxFileHead();
    mapOverFileTableWithIndex(writeFileNumberItem);
    scanCxFileUsing(fullScanSequence);
    mapOverReferenceableItemTable(writeReferenceableItem);
    closeCurrentCxFile();
}

static void writeMultipeCxFiles(bool updating, char *dirName) {
    char  cxFileName[MAX_FILE_NAME_SIZE];

    createDirectory(dirName);
    writePartialCxFile(updating, dirName, CXFILENAME_FILES, writeFileNumberItem, NULL);
    for (int i = 0; i < options.cxFileCount; i++) {
        sprintf(cxFileName, "%s%s%04d", dirName, CXFILENAME_PREFIX, i);
        assert(strlen(cxFileName) < MAX_FILE_NAME_SIZE - 1);
        openInOutCxFile(updating, cxFileName);
        writeCxFileHead();
        scanCxFileUsing(fullScanSequence);
        writeReferencesFromMemoryIntoCxFile(i);
        closeCurrentCxFile();
    }
}

static void writeCxFile(int updating, char *fileName) {
    if (!updating)
        removeFile(fileName);

    recursivelyCreateFileDirIfNotExists(fileName);

    if (options.cxFileCount <= 1) {
        writeSingleCxFile(updating, fileName);
    } else {
        writeMultipeCxFiles(updating, fileName);
    }
}

void saveReferencesToStore(bool updating, char *fileName) {
    writeCxFile(updating, fileName);
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
                                  CxFileScanOperation operation
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
                                      CxFileScanOperation operation
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
                                     CxFileScanOperation operation
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

    log_trace("re-read info from '%s' for '%s'?", options.cxFileLocation, fileItem->name);
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
              options.cxFileLocation, fileItem->name);

    return updateFromCxFile;
}

static void scanFunction_ReadFileName(int fileNameLength,
                                      int key,
                                      CharacterBuffer *cb,
                                      CxFileScanOperation operation
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
        if (fileItem->lastParsedMtime == 0)
            fileItem->lastParsedMtime=umtime;
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
        if (fileItem->lastParsedMtime == 0)
            fileItem->lastParsedMtime=umtime;
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
                                                         CxFileScanOperation operation
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

    ReferenceableItem *referenceableItem = &lastIncomingData.cachedReferenceableItem;
    lastIncomingData.referenceableItem = referenceableItem;

    int includedFileNumber;
    getIncludedFileNumber(&includedFileNumber);
    *referenceableItem = makeReferenceableItem(id, symbolType, storage, GlobalScope, VisibilityGlobal, includedFileNumber);

    ReferenceableItem *foundReferenceableItem;
    if (!isMemberInReferenceableItemTable(referenceableItem, NULL, &foundReferenceableItem)) {
        // TODO: This is more or less the body of a newReferenceableItem()
        char *ss = cxAlloc(len+1);
        strcpy(ss,id);
        foundReferenceableItem = cxAlloc(sizeof(ReferenceableItem));
        *foundReferenceableItem = makeReferenceableItem(ss, symbolType, storage,
                                                    GlobalScope, VisibilityGlobal, includedFileNumber);
        addToReferenceableItemTable(foundReferenceableItem);
    }
    lastIncomingData.referenceableItem = foundReferenceableItem;
    lastIncomingData.onLineReferencedSym = 0;
}

static void cxfileCheckLastSymbolDeadness(void) {
    if (lastIncomingData.symbolToCheckForDeadness != -1
        && lastIncomingData.deadSymbolIsDefined
    ) {
        addReferenceableToBrowserMenu(&sessionData.browsingStack.top->hkSelectedSym,
                                      lastIncomingData.referenceableItem,
                                      true, true, 0, (SymbolRelation){.sameFile = false},
                                      UsageDefined, NO_POSITION, UsageDefined);
    }
}


static bool referenceableIsReportableAsUnused(ReferenceableItem *referenceableItem) {
    if (referenceableItem==NULL || referenceableItem->linkName[0]==' ')
        return false;

    // you need to be strong here, in fact struct record can be used
    // without using struct explicitly
    if (referenceableItem->type == TypeStruct)
        return false;

    // in this first approach restrict this to variables and functions
    if (referenceableItem->type == TypeMacro)
        return false;
    return true;
}

static void scanFunction_SymbolName(int size,
                                    int key,
                                    CharacterBuffer *cb,
                                    CxFileScanOperation operation
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

    ReferenceableItem *referenceableItem = &lastIncomingData.cachedReferenceableItem;
    lastIncomingData.referenceableItem = referenceableItem;

    int includedFileNumber;
    getIncludedFileNumber(&includedFileNumber);
    *referenceableItem = makeReferenceableItem(id, symbolType, storage, GlobalScope, VisibilityGlobal, includedFileNumber);

    ReferenceableItem *foundMemberP;
    bool isMember = isMemberInReferenceableItemTable(referenceableItem, NULL, &foundMemberP);
    while (isMember && foundMemberP->visibility!=VisibilityGlobal)
        isMember = referenceableItemTableNextMember(referenceableItem, &foundMemberP);

    assert(options.mode);
    if (options.mode == XrefMode) {
        if (foundMemberP==NULL)
            foundMemberP=referenceableItem;
        writeReferenceableItem(foundMemberP);
        referenceableItem->references = foundMemberP->references; // note references to not generate multiple
        foundMemberP->references = NULL;      // HACK, remove them, to not be regenerated
    }
    if (options.mode == ServerMode) {
        if (operation == CXSF_DEAD_CODE_DETECTION) {
            if (referenceableIsReportableAsUnused(lastIncomingData.referenceableItem)) {
                lastIncomingData.symbolToCheckForDeadness = 0;
                lastIncomingData.deadSymbolIsDefined = 0;
            } else {
                lastIncomingData.symbolToCheckForDeadness = -1;
            }
        } else if (options.serverOperation!=OLO_TAG_SEARCH) {
            int ols = 0;
            BrowserMenu *menu = NULL;
            if (operation == CXSF_MENU_CREATION) {
                menu = createSelectionMenu(referenceableItem);
                if (menu == NULL) {
                    ols = 0;
                } else {
                    if (isBestFitMatch(menu))
                        ols = 2;
                    else
                        ols = 1;
                }
            }
            lastIncomingData.onLineRefMenuItem = menu;
            if (ols) {
                lastIncomingData.onLineReferencedSym = 0;
                log_trace("symbol %s is O.K. for %s (ols==%d)", referenceableItem->linkName, options.browsedName, ols);
            } else {
                if (lastIncomingData.onLineReferencedSym == 0) {
                    lastIncomingData.onLineReferencedSym = -1;
                }
            }
        }
    }
}

static void scanFunction_Reference_ForFullUpdateSchedule(int size,
                                                        int key,
                                                        CharacterBuffer *cb,
                                                        CxFileScanOperation operation
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
        addToReferenceList(&lastIncomingData.referenceableItem->references, pos, usage);
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
                                   CxFileScanOperation operation
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
            copyrefFl = !isInReferenceList(lastIncomingData.referenceableItem->references,
                                     usage, makePosition(file, line, col));
        } else {
            copyrefFl = !fileItem->cxLoading;
        }
        if (copyrefFl)
            writeCxReferenceBase(usage, file, line, col);
    } else if (options.mode == ServerMode) {
        Reference reference = makeReference(makePosition(file, line, col), usage, NULL);
        FileItem *cxFileItem = getFileItemWithFileNumber(reference.position.file);
        if (operation == CXSF_DEAD_CODE_DETECTION) {
            if (isVisibleUsage(reference.usage)) {
                // restrict reported symbols to those defined in project input file
                if (isDefinitionUsage(reference.usage)
                    && cxFileItem->isArgument
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
                    searchSymbolCheckReference(lastIncomingData.referenceableItem, &reference);
                }
            } else {
                if (lastIncomingData.onLineReferencedSym == lastIncomingData.data[CXFI_SYMBOL_INDEX]) {
                    if (operation == CXSF_MENU_CREATION) {
                        assert(lastIncomingData.onLineRefMenuItem);
                        if (file != topLevelFileNumber || !fileItem->isArgument
                            || options.serverOperation == OLO_GOTO
                            || options.serverOperation == OLO_COMPLETION_GOTO
                            || options.serverOperation == OLO_PUSH_NAME
                        ) {
                            log_trace (":adding reference %s:%d", cxFileItem->name, reference.position.line);
                            addReferenceToBrowserMenu(lastIncomingData.onLineRefMenuItem, &reference);
                        }
                    } else {
                        addReferenceToList(&reference, &sessionData.browsingStack.top->references);
                    }
                }
            }
        }
    }
}


static void scanFunction_CxFileCountCheck(int fileCountInCxFile,
                                          int key,
                                          CharacterBuffer *cb,
                                          CxFileScanOperation operation
) {
    if (!currentCxFileCountMatches(fileCountInCxFile)) {
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

static void setupRecordKeyHandlersFromTable(CxFileScanStep scanFunctionTable[]) {
    /* Set up the keys and handlers from the provided table */
    for (int i = 0; scanFunctionTable[i].recordCode > 0; i++) {
        assert(scanFunctionTable[i].recordCode < MAX_CHARS);
        int ch = scanFunctionTable[i].recordCode;
        lastIncomingData.handlerFunction[ch] = scanFunctionTable[i].handlerFunction;
        lastIncomingData.argument[ch] = scanFunctionTable[i].argument;
    }
}

static void scanCxFileUsing(CxFileScanStep *scanSequence) {
    ENTER();
    if (currentCxFile == NULL) {
        log_trace("No reference file opened");
        LEAVE();
        return;
    }

    resetIncomingData();

    setupRecordKeyHandlersFromTable(scanSequence);

    initCharacterBufferFromFile(&cxFileCharacterBuffer, currentCxFile);
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
static bool scanCxFile(char *cxFileLocation, char *element1, char *element2, CxFileScanStep *scanSequence) {
    char fn[MAX_FILE_NAME_SIZE];

    sprintf(fn, "%s%s%s", cxFileLocation, element1, element2);
    assert(strlen(fn) < MAX_FILE_NAME_SIZE-1);
    log_trace(":scanning file %s", fn);
    currentCxFile = openFile(fn, "r");
    if (currentCxFile==NULL) {
        return false;
    } else {
        scanCxFileUsing(scanSequence);
        closeFile(currentCxFile);
        currentCxFile = NULL;
        return true;
    }
}

static void scanCxFiles(char *cxFileLocation, CxFileScanStep *scanSequence) {

    if (options.cxFileCount <= 1) {
        scanCxFile(cxFileLocation, "", "", scanSequence);
    } else {
        scanCxFile(cxFileLocation,CXFILENAME_FILES,"",scanSequence);
        for (int i=0; i<options.cxFileCount; i++) {
            char partitionNumber[MAX_FILE_NAME_SIZE];
            sprintf(partitionNumber, "%04d", i);
            scanCxFile(cxFileLocation, CXFILENAME_PREFIX, partitionNumber, scanSequence);
        }
    }
}

bool loadFileNumbersFromStore(void) {
    static time_t savedModificationTime = 0; /* Cache previously read file data... */
    static off_t savedFileSize = 0;
    static char previouslyReadFileName[MAX_FILE_NAME_SIZE] = ""; /* ... and name */
    char cxFileName[MAX_FILE_NAME_SIZE];

    if (options.cxFileCount <= 1) {
        sprintf(cxFileName, "%s", options.cxFileLocation);
    } else {
        sprintf(cxFileName, "%s%s", options.cxFileLocation, CXFILENAME_FILES);
    }
    if (editorFileExists(cxFileName)) {
        size_t currentSize = editorFileSize(cxFileName);
        time_t currentModificationTime = editorFileModificationTime(cxFileName);
        if (strcmp(previouslyReadFileName, cxFileName) != 0
            || savedModificationTime != currentModificationTime
            || savedFileSize != currentSize)
        {
            log_trace(":(re)reading reference file '%s'", cxFileName);
            if (scanCxFile(cxFileName, "", "", normalScanSequence)) {
                strcpy(previouslyReadFileName, cxFileName);
                savedModificationTime = currentModificationTime;
                savedFileSize = currentSize;
            }
        } else {
            log_trace(":skipping (re)reading reference file '%s'", cxFileName);
        }
        return true;
    }
    return false;
}

// symbolName can be NULL !!!!!!
static void readOneAppropiateCxFile(char *symbolName, CxFileScanStep *scanSequence) {
    if (options.cxFileLocation == NULL)
        return;
    cxFile = stdout;
    if (options.cxFileCount <= 1) {
        scanCxFile(options.cxFileLocation, "", "", scanSequence);
    } else {
        if (!loadFileNumbersFromStore())
            return;
        if (symbolName == NULL)
            return;

        /* following must be after reading XFiles*/
        char partitionNumber[MAX_FILE_NAME_SIZE];
        int i = cxFileHashNumberForSymbol(symbolName);

        sprintf(partitionNumber, "%04d", i);
        assert(strlen(partitionNumber) < MAX_FILE_NAME_SIZE-1);
        scanCxFile(options.cxFileLocation, CXFILENAME_PREFIX, partitionNumber, scanSequence);
    }
}


protected void normalScanCxFile(char *name) {
    scanCxFile(options.cxFileLocation, name, "", normalScanSequence);
}

void ensureReferencesAreLoadedFor(char *symbolName) {
    readOneAppropiateCxFile(symbolName, fullUpdateSequence);
}

void scanReferencesToCreateMenu(char *symbolName){
    readOneAppropiateCxFile(symbolName, symbolMenuCreationSequence);
}

void scanForMacroUsage(char *symbolName) {
    readOneAppropiateCxFile(symbolName, secondPassMacroUsageSequence);
}

void scanForGlobalUnused(char *cxrefLocation) {
    scanCxFiles(cxrefLocation, globalUnusedDetectionSequence);
}

void scanForSearch(char *cxrefLocation) {
    scanCxFiles(cxrefLocation, symbolSearchSequence);
}


/* ***************************************************************************************

   These tables are dispatching tables for reading the disk store.  The first element is
   a marker indicating the type of the field, e.g. a 'f' for a file number or 'l' for
   line number.

   Reading will gather a data field and then its type, e.g. 3548f. The value (3548) in
   the structure `lastIcoming` so that it can be reused if left out, for example in the
   next position (file, line, column). So 3548f3lc15l is actually two positions in file
   3548, one on line 3, column 1 and another in the same file, line 5, also in column 1.

   Then the scan sequence used will be consulted to see if there is an entry for the
   marker, if so it will call the scan function.

 */


static CxFileScanStep normalScanSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_CxFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static CxFileScanStep fullScanSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_GENERATE_OUTPUT},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_DEFAULT},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_FIRST_PASS},
    {CXFI_REFNUM, scanFunction_CxFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static CxFileScanStep symbolMenuCreationSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_MENU_CREATION},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_MENU_CREATION},
    {CXFI_REFNUM, scanFunction_CxFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static CxFileScanStep fullUpdateSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_VERSION, scanFunction_VersionCheck, CXSF_NOP},
    {CXFI_CHECK_NUMBER, scanFunction_CheckNumber, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolNameForFullUpdateSchedule, CXSF_NOP},
    {CXFI_REFERENCE, scanFunction_Reference_ForFullUpdateSchedule, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_CxFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static CxFileScanStep secondPassMacroUsageSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_PASS_MACRO_USAGE},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_PASS_MACRO_USAGE},
    {CXFI_REFNUM, scanFunction_CxFileCountCheck, CXSF_NOP},
    {-1,NULL, 0},
};

static CxFileScanStep symbolSearchSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_CxFileCountCheck, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, SEARCH_SYMBOL},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_FIRST_PASS},
    {-1,NULL, 0},
};

static CxFileScanStep globalUnusedDetectionSequence[]={
    {CXFI_KEY_LIST, scanFunction_ReadKeys, CXSF_NOP},
    {CXFI_REFNUM, scanFunction_CxFileCountCheck, CXSF_NOP},
    {CXFI_FILE_NAME, scanFunction_ReadFileName, CXSF_JUST_READ},
    {CXFI_SYMBOL_NAME, scanFunction_SymbolName, CXSF_DEAD_CODE_DETECTION},
    {CXFI_REFERENCE, scanFunction_Reference, CXSF_DEAD_CODE_DETECTION},
    {-1,NULL, 0},
};

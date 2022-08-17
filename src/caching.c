#include "caching.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "protocol.h"
#include "yylex.h"
#include "editor.h"
#include "reftab.h"
#include "javafqttab.h"
#include "classhierarchy.h"
#include "jsemact.h"
#include "filedescriptor.h"
#include "filetable.h"

#include "log.h"

Cache cache;


bool checkFileModifiedTime(int fileIndex) {
    time_t now = time(NULL);
    FileItem *fileItem = getFileItem(fileIndex);

    if (fileItem->lastInspected >= fileProcessingStartTime
        && fileItem->lastInspected <= now) {
        /* Assuming that files cannot change during one execution */
        return true;
    }
    if (!editorFileExists(fileItem->name)) {
        return false;
    }

    fileItem->lastInspected = now;
    time_t modificationTime = editorFileModificationTime(fileItem->name);
    if (modificationTime == fileItem->lastModified) {
        return true;
    } else {
        fileItem->lastModified = modificationTime;
        return false;
    }
}


static void deleteReferencesOutOfMemory(Reference **referenceP) {
    while (*referenceP!=NULL) {
        if (CX_FREED_POINTER(*referenceP)) {
            log_trace("deleting reference on %s:%d", getFileItem((*referenceP)->position.file)->name,
                      (*referenceP)->position.line);
            *referenceP = (*referenceP)->next;
            continue;
        }
        referenceP = &(*referenceP)->next;
    }
}

static void cxrefTabDeleteOutOfMemory(int index) {
    ReferencesItem *item;
    ReferencesItem **itemP;

    item = getReferencesItem(index);
    itemP = &item;

    while (*itemP!=NULL) {
        if (CX_FREED_POINTER(*itemP)) {
            /* out of memory, delete it */
            log_trace("deleting all references on %s", (*itemP)->name);
            *itemP = (*itemP)->next;  /* Unlink it and look at next */
            continue;
        } else {
            /* in memory, examine all refs */
            deleteReferencesOutOfMemory(&((*itemP)->references));
        }
        itemP = &((*itemP)->next);
    }
    setReferencesItem(index, item);
}

static void fileTabDeleteOutOfMemory(FileItem *fileItem) {
    ClassHierarchyReference **h;
    h = &fileItem->superClasses;
    while (*h!=NULL) {
        if (CX_FREED_POINTER(*h)) {
            *h = (*h)->next;
            goto contlabel;     /* TODO: continue? */
        }
        h= &(*h)->next;       /* TODO: else? */
    contlabel:;
    }
    h = &fileItem->inferiorClasses;
    while (*h!=NULL) {
        if (CX_FREED_POINTER(*h)) {
            *h = (*h)->next;
            goto contlabel2;    /* TODO: continue? */
        }
        h= &(*h)->next;       /* TODO: else? */
    contlabel2:;
    }
}

static void structCachingFree(Symbol *symbol) {
    SymbolList **superList;
    assert(symbol->u.structSpec);
    if (freedPointer(symbol->u.structSpec->records) ||
        PPM_FREED_POINTER(symbol->u.structSpec->records)) {
        symbol->u.structSpec->records = NULL;
    }
    if (freedPointer(symbol->u.structSpec->casts.node) ||
        PPM_FREED_POINTER(symbol->u.structSpec->casts.node)) {
        symbol->u.structSpec->casts.node = NULL;
    }
    if (freedPointer(symbol->u.structSpec->casts.sub) ||
        PPM_FREED_POINTER(symbol->u.structSpec->casts.sub)) {
        symbol->u.structSpec->casts.sub = NULL;
    }

    superList = &symbol->u.structSpec->super;
    while (*superList!=NULL) {
        if (freedPointer(*superList) ||
            PPM_FREED_POINTER(*superList)) {
            *superList = (*superList)->next;
            goto contlabel;
        }
        superList = &(*superList)->next;
    contlabel:;
    }
}

static void symbolTableDeleteOutOfMemory(int i) {
    Symbol **pp;
    pp = &symbolTable->tab[i];
    while (*pp!=NULL) {
        switch ((*pp)->type) {
        case TypeMacro:
            if (PPM_FREED_POINTER(*pp)) {
                *pp = (*pp)->next;
                continue;
            }
            break;
        case TypeStruct:
        case TypeUnion:
            if (freedPointer(*pp) || PPM_FREED_POINTER(*pp)) {
                *pp = (*pp)->next;
                continue;
            } else {
                structCachingFree(*pp);
            }
            break;
        case TypeEnum:
            if (freedPointer(*pp)) {
                *pp = (*pp)->next;
                continue;
            } else if (freedPointer((*pp)->u.enums)) {
                (*pp)->u.enums = NULL;
            }
            break;
        default:
            if (freedPointer(*pp)) {
                *pp = (*pp)->next;
                continue;
            }
            break;
        }
        pp= &(*pp)->next;
    }
}

static void javaFqtTabDeleteOutOfMemory(int i) {
    SymbolList **symbolListP;
    symbolListP = &javaFqtTable.tab[i];
    while (*symbolListP != NULL) {
        if (PPM_FREED_POINTER(*symbolListP)) {
            *symbolListP = (*symbolListP)->next;
            continue;
        } else if (freedPointer((*symbolListP)->element) || PPM_FREED_POINTER((*symbolListP)->element)) {
            *symbolListP = (*symbolListP)->next;
            continue;
        } else {
            structCachingFree((*symbolListP)->element);
        }
        symbolListP = &(*symbolListP)->next;
    }
}

static void trailDeleteOutOfMemory(void) {
    FreeTrail **pp;
    pp = &currentBlock->trail;
    while (freedPointer(*pp)) {
        *pp = (*pp)->next;
    }
}

static void includeListDeleteOutOfMemory(void) {
    StringList **pp;
    pp = & options.includeDirs;
    while (*pp!=NULL) {
        if (PPM_FREED_POINTER(*pp)) {
            *pp = (*pp)->next;
            continue;
        }
        pp= &(*pp)->next;
    }
}

static int cachedIncludedFilePass(int cpi) {
    int mi,mt;
    assert (cpi > 0);
    mi = cache.cachePoints[cpi].ibi;
    for (int i=cache.cachePoints[cpi-1].ibi; i<mi; i++) {
        mt = checkFileModifiedTime(cache.ib[i]);
        log_debug("mtime of %s eval to %d", getFileItem(cache.ib[i])->name, mt);
        if (mt == 0)
            return 0;
    }
    return 1;
}

static void recoverCxMemory(char *cxMemFreeBase) {
    CX_FREE_UNTIL(cxMemFreeBase);
    mapOverFileTable(fileTabDeleteOutOfMemory);
    mapOverReferenceTableWithIndex(cxrefTabDeleteOutOfMemory);
}

static void fillCache(Cache *cache, bool cachingActive, int cachePointIndex, int ibi, char *lbcc, char *lexcc,
                      char *cc, char *cfin) {
    cache->cachingActive   = cachingActive;
    cache->cachePointIndex = cachePointIndex;
    cache->ibi             = ibi;
    cache->lbcc            = lbcc;
    cache->lexcc           = lexcc;
    cache->currentLexemP   = cc;
    cache->endOfBuffer     = cfin;
}

// before allowing it, fix problem when modifying .xrefrc during run!!!!
#define CACHING_CLASSES 1

static bool canContinueCachingClasses() {
    return CACHING_CLASSES && LANGUAGE(LANG_JAVA)
        && options.mode == XrefMode
        && ppmMemoryIndex < (SIZE_ppmMemory/3)*2;
}

void recoverCachePointZero(void) {
    ppmMemoryIndex = cache.cachePoints[0].ppmMemoryIndex;
    recoverCachePoint(0, cache.cachePoints[0].currentLexemP, false);
}

void recoverMemoriesAfterOverflow(char *cxMemFreeBase) {
    recoverCxMemory(cxMemFreeBase);
    initAllInputs();
    recoverCachePointZero();
}

void recoverCachePoint(int cachePointIndex, char *readUntil, bool activeCaching) {
    CachePoint *cachePoint;

    log_trace("recovering cache point %d", cachePointIndex);
    cachePoint = &cache.cachePoints[cachePointIndex];
    if (!canContinueCachingClasses()) {
        ppmMemoryIndex = cachePoint->ppmMemoryIndex;
        if (CACHING_CLASSES)
            log_debug("flushing classes");
    }
    mbMemoryIndex = cachePoint->mbMemoryIndex;
    currentBlock = cachePoint->topBlock;
    *currentBlock = cachePoint->topBlockContent;
    javaStat = cachePoint->javaStat;
    counters = cachePoint->counters;
    trailDeleteOutOfMemory();
    assert(options.mode);
    if (options.mode==ServerMode && currentPass==1) {
        /* remove old references, only on first pass of edit server */
        log_trace("removing references");
        cxMemory->index = cachePoint->cxMemoryIndex;
        mapOverReferenceTableWithIndex(cxrefTabDeleteOutOfMemory);
        mapOverFileTable(fileTabDeleteOutOfMemory);
    }
    log_trace("recovering 0");
    symbolTableMapWithIndex(symbolTable, symbolTableDeleteOutOfMemory);
    log_trace("recovering 1");
    javaFqtTableMapWithIndex(&javaFqtTable, javaFqtTabDeleteOutOfMemory);
    log_trace("recovering 2");

    // do not forget that includes are listed in PP_MEMORY too.
    includeListDeleteOutOfMemory();

    currentFile.lineNumber = cachePoint->lineNumber;
    currentFile.ifDepth = cachePoint->ifDepth;
    currentFile.ifStack = cachePoint->ifStack;
    fillLexInput(&currentInput, cachePoint->currentLexemP, readUntil, cache.beginningOfBuffer, NULL, INPUT_CACHE);
    fillCache(&cache, activeCaching, cachePointIndex + 1, cachePoint->ibi, cachePoint->currentLexemP,
              currentInput.currentLexemP, currentInput.currentLexemP, currentInput.endOfBuffer);
    log_trace("finished recovering");
}

/* ******************************************************************* */
/*                         recover from cache                          */
/* ******************************************************************* */

void recoverFromCache(void) {
    int i;
    char *readUntil;

    assert(cache.cachePointIndex >= 1);
    cache.cachingActive = false;
    log_debug("reading from cache");
    readUntil = cache.cachePoints[0].currentLexemP;
    for (i = 1; i < cache.cachePointIndex; i++) {
        log_trace("trying to recover cache point %d", i);
        if (cachedInputPass(i, &readUntil) == 0)
            break;
        if (cachedIncludedFilePass(i) == 0)
            break;
    }
    assert(i > 1);
    /* now, recover state from the cache point 'i-1' */
    log_debug("recovering cache point %d", i-1);
    recoverCachePoint(i-1, readUntil, true);
}

void setupCaching(void) {
    fillCache(&cache, 0, 0, 0, NULL, NULL, NULL, NULL);
}

void initCaching(void) {
    fillCache(&cache, 1, 0, 0, cache.beginningOfBuffer, currentFile.lexemBuffer.next, NULL, NULL);
    placeCachePoint(false);
    cache.cachingActive = false;
}

/* ****************************************************************** */
/*        caching of input from 'cache.lexcc' to 'cInput.cc'          */
/* ****************************************************************** */

void cacheInput(void) {
    int size;

    ENTER();
    if (!cache.cachingActive) {
        log_trace("Caching is not active");
        LEAVE();
        return;
    }
    if (includeStackPointer != 0 || macroStackIndex != 0) {
        log_trace("In include or macro");
        LEAVE();
        return;
    }
    size = currentInput.currentLexemP - cache.lexcc;
    if (cache.lbcc - cache.beginningOfBuffer + size >= LEX_BUF_CACHE_SIZE) {
        cache.cachingActive = false;
        LEAVE();
        return;
    }
    /* if from cache, don't copy on the same place */
    if (currentInput.inputType != INPUT_CACHE)
        memcpy(cache.lbcc, cache.lexcc, size);
    cache.lbcc += size;
    cache.lexcc = currentInput.currentLexemP;
    LEAVE();
}

void cacheInclude(int fileNum) {
    if (!cache.cachingActive)
        return;
    log_debug("caching include of file %d: %s",
              cache.ibi, getFileItem(fileNum)->name);
    checkFileModifiedTime(fileNum);
    assert(cache.ibi < INCLUDE_CACHE_SIZE);
    cache.ib[cache.ibi] = fileNum;
    cache.ibi ++;
    if (cache.ibi >= INCLUDE_CACHE_SIZE)
        cache.cachingActive = false;
}

static void fillCachePoint(CachePoint *cachePoint, CodeBlock *topBlock, int ppmMemoryIndex,
                           int cxMemoryIndex, int mbMemoryIndex, char *lbcc, short int ibi,
                           short int lineNumber, short int ifDepth, CppIfStack *ifStack,
                           JavaStat *javaCached, Counters counters) {
    cachePoint->topBlock = topBlock;
    cachePoint->topBlockContent = *topBlock;
    cachePoint->ppmMemoryIndex = ppmMemoryIndex;
    cachePoint->cxMemoryIndex = cxMemoryIndex;
    cachePoint->mbMemoryIndex = mbMemoryIndex;
    cachePoint->currentLexemP   = lbcc;
    cachePoint->ibi = ibi;
    cachePoint->lineNumber = lineNumber;
    cachePoint->ifDepth = ifDepth;
    cachePoint->ifStack = ifStack;
    cachePoint->javaStat = javaCached;
    cachePoint->counters = counters;
}

void placeCachePoint(bool inputCaching) {
    CachePoint *cachePoint;
    if (!cache.cachingActive)
        return;
    if (includeStackPointer != 0 || macroStackIndex != 0)
        return;
    if (cache.cachePointIndex >= MAX_CACHE_POINTS) {
        cache.cachingActive = false;
        return;
    }
    if (inputCaching)
        cacheInput();
    if (!cache.cachingActive)
        return;
    cachePoint = &cache.cachePoints[cache.cachePointIndex];
    log_debug("placing cache point %d", cache.cachePointIndex);
    fillCachePoint(cachePoint, currentBlock, ppmMemoryIndex, cxMemory->index, mbMemoryIndex, cache.lbcc,
                   cache.ibi, currentFile.lineNumber, currentFile.ifDepth,
                   currentFile.ifStack, javaStat, counters);
    cache.cachePointIndex++;
}

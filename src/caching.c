#include "caching.h"

#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "globals.h"
#include "memory.h"
#include "stackmemory.h"
#include "options.h"
#include "input.h"
#include "yylex.h"
#include "editor.h"
#include "reftab.h"
#include "symboltable.h"
#include "filedescriptor.h"
#include "filetable.h"

#include "log.h"

Cache cache;


bool checkFileModifiedTime(int fileNumber) {
    time_t now = time(NULL);
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);

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
        if (cxMemoryIsFreed(*referenceP)) {
            log_trace("deleting reference on %s:%d", getFileItemWithFileNumber((*referenceP)->position.file)->name,
                      (*referenceP)->position.line);
            *referenceP = (*referenceP)->next;
            continue;
        }
        referenceP = &(*referenceP)->next;
    }
}

static void refTabDeleteOutOfMemory(int index) {
    ReferenceItem *item;
    ReferenceItem **itemP;

    /* Since we always push older Reference items down the list on the same index we can
       pop them off until we get to one that will not be flushed. If any left we
       can just hook them in at the index.
    */
    item = getReferenceItem(index);
    itemP = &item;

    while (*itemP!=NULL) {
        if (cxMemoryIsFreed(*itemP)) {
            /* Out of memory, or would be flushed, un-hook it */
            log_trace("deleting all references on %s", (*itemP)->linkName);
            *itemP = (*itemP)->next;  /* Unlink it and look at next */
            /* And since all references related to the referenceItem are allocated after
             * the item itself they will automatically be flushed too */
            continue;
        } else {
            /* The referenceItem is still in memory, but references might be flushed */
            deleteReferencesOutOfMemory(&((*itemP)->references));
        }
        itemP = &((*itemP)->next);
    }
    setReferenceItem(index, item);
}

// Deliberate NO-OP
static void fileTableDeleteOutOfMemory(FileItem *fileItem) {
}

static void structCachingFree(Symbol *symbol) {
    assert(symbol->u.structSpec);
    if (isFreedStackMemory(symbol->u.structSpec->members)
        || ppmIsFreedPointer(symbol->u.structSpec->members)) {
        symbol->u.structSpec->members = NULL;
    }
}

static void symbolTableDeleteOutOfMemory(int i) {
    Symbol **pp;
    pp = &symbolTable->tab[i];
    while (*pp!=NULL) {
        switch ((*pp)->type) {
        case TypeMacro:
            if (ppmIsFreedPointer(*pp)) {
                *pp = (*pp)->next;
                continue;
            }
            break;
        case TypeStruct:
        case TypeUnion:
            if (isFreedStackMemory(*pp) || ppmIsFreedPointer(*pp)) {
                *pp = (*pp)->next;
                continue;
            } else {
                structCachingFree(*pp);
            }
            break;
        case TypeEnum:
            if (isFreedStackMemory(*pp)) {
                *pp = (*pp)->next;
                continue;
            } else if (isFreedStackMemory((*pp)->u.enums)) {
                (*pp)->u.enums = NULL;
            }
            break;
        default:
            if (isFreedStackMemory(*pp)) {
                *pp = (*pp)->next;
                continue;
            }
            break;
        }
        pp= &(*pp)->next;
    }
}

static void deleteFrameAllocationsWhenOutOfMemory(void) {
    FrameAllocation **pp;
    pp = &currentBlock->frameAllocations;
    while (isFreedStackMemory(*pp)) {
        *pp = (*pp)->next;
    }
}

static void includeListDeleteOutOfMemory(void) {
    StringList **pp;
    pp = & options.includeDirs;
    while (*pp!=NULL) {
        if (ppmIsFreedPointer(*pp)) {
            *pp = (*pp)->next;
            continue;
        }
        pp= &(*pp)->next;
    }
}

static bool cachedIncludedFilePass(int index) {
    int includeTop;
    assert(index > 0);
    includeTop = cache.points[index].includeStackTop;
    for (int i = cache.points[index - 1].includeStackTop; i < includeTop; i++) {
        bool fileIsCurrent = checkFileModifiedTime(cache.includeStack[i]);
        log_debug("mtime of %s eval to %s", getFileItemWithFileNumber(cache.includeStack[i])->name,
                  fileIsCurrent ? "true" : "false");
        if (!fileIsCurrent)
            return false;
    }
    return true;
}

static void recoverCxMemory(void *cxMemoryFlushPoint) {
#ifdef USE_NEW_CXMEMORY
    markCxMemoryForFlushing(cxMemoryFlushPoint);
#else
    cxFreeUntil(cxMemoryFlushPoint);
#endif
    mapOverFileTable(fileTableDeleteOutOfMemory);
    mapOverReferenceTableWithIndex(refTabDeleteOutOfMemory);
#ifdef USE_NEW_CXMEMORY
    flushPendingCxMemory();
#endif
}

static void fillCache(Cache *cache, bool cachingActive, int cachePointIndex, int includeStackTop, char *free,
                      char *next, char *read, char *write) {
    cache->active          = cachingActive;
    cache->index           = cachePointIndex;
    cache->includeStackTop = includeStackTop;
    cache->free            = free;
    cache->nextToCache     = next;
    cache->read   = read;
    cache->write  = write;
}

void deactivateCaching(void) {
    cache.active = false;
}

void activateCaching(void) {
    cache.active = true;
}

bool cachingIsActive(void) {
    return cache.active;
}

void recoverCachePointZero(void) {
    ppmMemory.index = cache.points[0].ppmMemoryIndex;
    recoverCachePoint(0, cache.points[0].nextLexemP, false);
}

void recoverMemoriesAfterOverflow(char *cxMemFreeBase) {
    recoverCxMemory(cxMemFreeBase);
    initAllInputs();
    recoverCachePointZero();
}

void recoverCachePoint(int cachePointIndex, char *readUntil, bool cachingActive) {
    CachePoint *cachePoint;

    log_trace("recovering cache point %d", cachePointIndex);
    cachePoint = &cache.points[cachePointIndex];

    /* TODO What does this do? */
    log_debug("flushing classes or whatever");
    ppmMemory.index = cachePoint->ppmMemoryIndex;

    setMacroBodyMemoryIndex(cachePoint->macroBodyMemoryIndex);
    currentBlock = cachePoint->topBlock;
    *currentBlock = cachePoint->topBlockContent;
    counters = cachePoint->counters;
    deleteFrameAllocationsWhenOutOfMemory();
    assert(options.mode);
    if (options.mode==ServerMode && currentPass==1) {
        /* remove old references, only on first pass of edit server */
        log_trace("removing references");
#ifdef USE_NEW_CXMEMORY
        assert(0);
#else
        cxMemory.index = cachePoint->cxMemoryIndex;
#endif
        mapOverReferenceTableWithIndex(refTabDeleteOutOfMemory);
        mapOverFileTable(fileTableDeleteOutOfMemory);
    }
    log_trace("recovering symbolTable");
    symbolTableMapWithIndex(symbolTable, symbolTableDeleteOutOfMemory);

    log_trace("recovering finished");

    // do not forget that includes are listed in PP_MEMORY too.
    includeListDeleteOutOfMemory();

    currentFile.lineNumber = cachePoint->lineNumber;
    currentFile.ifDepth = cachePoint->ifDepth;
    currentFile.ifStack = cachePoint->ifStack;
    fillLexInput(&currentInput, cachePoint->nextLexemP, cache.lexemStream, readUntil, NULL, INPUT_CACHE);
    fillCache(&cache, cachingActive, cachePointIndex + 1, cachePoint->includeStackTop, cachePoint->nextLexemP,
              currentInput.read, currentInput.read, currentInput.write);
    log_trace("finished recovering");
}

/* ******************************************************************* */
/*                         recover from cache                          */
/* ******************************************************************* */

void recoverFromCache(void) {
    int i;
    char *readUntil;

    assert(cache.index >= 1);
    deactivateCaching();
    log_trace("reading from cache");
    readUntil = cache.points[0].nextLexemP;
    for (i = 1; i < cache.index; i++) {
        log_trace("trying to recover cache point %d", i);
        if (cachedInputPass(i, &readUntil) == 0)
            break;
        if (!cachedIncludedFilePass(i))
            break;
    }
    assert(i > 1);
    /* now, recover state from the cache point 'i-1' */
    log_trace("recovering cache point %d", i-1);
    recoverCachePoint(i-1, readUntil, true);
}

void initCaching(void) {
    fillCache(&cache, true, 0, 0, cache.lexemStream, currentFile.lexemBuffer.read, NULL, NULL);
    placeCachePoint(false);
    deactivateCaching();
}

/* ****************************************************************** */
/*        Caching of input from LexemBuffer to LexInput               */
/* ****************************************************************** */

void cacheInput(LexInput *input) {
    int size;

    ENTER();
    if (!cachingIsActive()) {
        log_trace("Caching is not active");
        LEAVE();
        return;
    }
    if (includeStack.pointer != 0 || macroStackIndex != 0) {
        log_trace("In include or macro, will not cache now");
        LEAVE();
        return;
    }
    /* How much needs to be cached? input vs. cache?!? */
    size = input->read - cache.nextToCache;

    /* Is there space enough? */
    if (cache.free - cache.lexemStream + size >= LEXEM_STREAM_CACHE_SIZE) {
        deactivateCaching();
        LEAVE();
        return;
    }

    /* Avoid copying if we are already reading from cached data */
    if (input->inputType != INPUT_CACHE)
        /* Copy from next un-cached to the free area of the cache buffer */
        memcpy(cache.free, cache.nextToCache, size);
    cache.free += size;
    cache.nextToCache = input->read;
    LEAVE();
}

void cacheInclude(int fileNum) {
    if (!cachingIsActive())
        return;
    log_debug("caching include of file %d: %s",
              cache.includeStackTop, getFileItemWithFileNumber(fileNum)->name);
    checkFileModifiedTime(fileNum);
    assert(cache.includeStackTop < INCLUDE_STACK_CACHE_SIZE);
    cache.includeStack[cache.includeStackTop] = fileNum;
    cache.includeStackTop ++;
    if (cache.includeStackTop >= INCLUDE_STACK_CACHE_SIZE)
        deactivateCaching();
}

static void fillCachePoint(CachePoint *cachePoint, CodeBlock *topBlock, int ppmMemoryIndex,
                           int cxMemoryIndex, int macroBodyMemoryIndex, char *lbcc, short int includeStackTop,
                           short int lineNumber, short int ifDepth, CppIfStack *ifStack, Counters counters) {
    cachePoint->topBlock = topBlock;
    cachePoint->topBlockContent = *topBlock;
    cachePoint->ppmMemoryIndex = ppmMemoryIndex;
    cachePoint->cxMemoryIndex = cxMemoryIndex;
    cachePoint->macroBodyMemoryIndex = macroBodyMemoryIndex;
    cachePoint->nextLexemP   = lbcc;
    cachePoint->includeStackTop = includeStackTop;
    cachePoint->lineNumber = lineNumber;
    cachePoint->ifDepth = ifDepth;
    cachePoint->ifStack = ifStack;
    cachePoint->counters = counters;
}

void placeCachePoint(bool inputCaching) {
    CachePoint *cachePoint;
    if (!cachingIsActive())
        return;
    if (includeStack.pointer != 0 || macroStackIndex != 0)
        return;
    if (cache.index >= MAX_CACHE_POINTS) {
        deactivateCaching();
        return;
    }
    if (inputCaching)
        cacheInput(&currentInput);
    if (!cachingIsActive())
        return;
    cachePoint = &cache.points[cache.index];
    log_debug("placing cache point %d", cache.index);
#ifdef USE_NEW_CXMEMORY
    fillCachePoint(cachePoint, currentBlock, ppmMemory.index, cxMemory.top, getMacroBodyMemoryIndex(), cache.free,
                   cache.includeStackTop, currentFile.lineNumber, currentFile.ifDepth, currentFile.ifStack, counters);
#else
    fillCachePoint(cachePoint, currentBlock, ppmMemory.index, cxMemory.index, getMacroBodyMemoryIndex(), cache.free,
                   cache.includeStackTop, currentFile.lineNumber, currentFile.ifDepth, currentFile.ifStack, counters);
#endif
    cache.index++;
}

/**
 * @file caching.c
 * @brief Incremental parsing cache system
 * 
 * This file implements the caching system that enables incremental parsing
 * in c-xrefactory. The cache stores tokenized input and parser state snapshots
 * to avoid re-parsing unchanged code sections, significantly improving
 * performance for large codebases.
 * 
 * Key concepts:
 * - Cache Points: Complete parser state snapshots that can be restored
 * - Input Caching: Storage of tokenized lexical input in a buffer
 * - File Modification Tracking: Validation that included files haven't changed
 * - Memory Recovery: Restoration of various memory pools to cached states
 */

#include "caching.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

/* ========================================================================== */
/*                              Global State                                 */
/* ========================================================================== */

Cache cache;

/* ========================================================================== */
/*                        File Modification Tracking                        */
/* ========================================================================== */

/**
 * Check if a file's modification time indicates it hasn't changed since last cache.
 * 
 * This function implements file modification tracking for cache validation.
 * Files are considered current if they were inspected during this execution
 * or if their modification time matches the cached value.
 * 
 * @param fileNumber The file number to check
 * @return true if file is current (unchanged), false if modified or missing
 */
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

/* ========================================================================== */
/*                           Memory Recovery                                 */
/* ========================================================================== */

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

static void recoverMemoryFromReferenceTableEntry(int index) {
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
static void recoverMemoryFromFileTableEntry(FileItem *fileItem) {
}

static void recoverMemoryFromTypeStructOrUnion(Symbol *symbol) {
    assert(symbol->structSpec);
    if (isFreedStackMemory(symbol->structSpec->members)
        || ppmIsFreedPointer(symbol->structSpec->members)) {
        symbol->structSpec->members = NULL;
    }
}

static void recoverMemoryFromSymbolTableEntry(int i) {
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
                recoverMemoryFromTypeStructOrUnion(*pp);
            }
            break;
        case TypeEnum:
            if (isFreedStackMemory(*pp)) {
                *pp = (*pp)->next;
                continue;
            } else if (isFreedStackMemory((*pp)->enums)) {
                (*pp)->enums = NULL;
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

static void recoverMemoryFromFrameAllocations(void) {
    FrameAllocation **pp;
    pp = &currentBlock->frameAllocations;
    while (isFreedStackMemory(*pp)) {
        *pp = (*pp)->next;
    }
}

static void recoverMemoryFromIncludeList(void) {
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

/**
 * Validate that all files included between two cache points are still current.
 * 
 * This function checks file modification times for all files that were
 * included between the previous cache point and the specified cache point.
 * If any file has been modified, the cache is invalid.
 * 
 * @param index Cache point index to validate files for
 * @return true if all included files are current, false if any were modified
 */
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
    cxFreeUntil(cxMemoryFlushPoint);
    mapOverFileTable(recoverMemoryFromFileTableEntry);
    mapOverReferenceTableWithIndex(recoverMemoryFromReferenceTableEntry);
}

/* ========================================================================== */
/*                         Cache State Management                            */
/* ========================================================================== */

/**
 * Initialize cache structure with specified state values.
 * 
 * This utility function sets all the cache state fields in one operation.
 * Used when initializing the cache or updating its state during recovery.
 * 
 * @param cache Cache structure to fill
 * @param cachingActive Whether caching should be active
 * @param cachePointIndex Next available cache point index
 * @param includeStackTop Current include stack depth
 * @param free Next free position in lexem stream
 * @param next Next input position to cache
 * @param read Current read position in cache
 * @param write Current write position in cache
 */
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
    assert(cache.points[0].topBlock == (CodeBlock*)stackMemory);
    assert(cache.points[0].macroBodyMemoryIndex == 0);
    assert(cache.points[0].includeStackTop == 0);
    /* assert(cache.points[0].ppmMemoryIndex == 0); This won't always be true because
                                                  * definitions from command line or
                                                  * option file will reside here */
    recoverCachePoint(0, cache.points[0].nextLexemP, false);
}

void recoverMemoriesAfterOverflow(char *cxMemFreeBase) {
    recoverCxMemory(cxMemFreeBase);
    initAllInputs();
    recoverCachePointZero();
}

/**
 * Restore parser state from a specific cache point.
 * 
 * This function performs a complete restoration of the parser state from
 * a previously saved cache point, including memory pools, parsing context,
 * and input stream position.
 * 
 * @param cachePointIndex Index of the cache point to restore from
 * @param readUntil Input position to read until in cached stream  
 * @param cachingActive Whether to activate caching after recovery
 */
void recoverCachePoint(int cachePointIndex, char *readUntil, bool cachingActive) {
    log_trace("Recovering cache point %d", cachePointIndex);
    CachePoint *cachePoint = &cache.points[cachePointIndex];

    log_debug("Flushing memories");
    ppmMemory.index = cachePoint->ppmMemoryIndex;

    setMacroBodyMemoryIndex(cachePoint->macroBodyMemoryIndex);
    currentBlock = cachePoint->topBlock;
    *currentBlock = cachePoint->topBlockContent;
    counters = cachePoint->counters;
    recoverMemoryFromFrameAllocations();
    assert(options.mode);
    if (options.mode==ServerMode && currentPass==1) {
        /* remove old references, only on first pass of edit server */
        log_trace("removing references");
        cxMemory.index = cachePoint->cxMemoryIndex;
        mapOverReferenceTableWithIndex(recoverMemoryFromReferenceTableEntry);
        mapOverFileTable(recoverMemoryFromFileTableEntry);
    }
    log_trace("recovering symbolTable");
    symbolTableMapWithIndex(symbolTable, recoverMemoryFromSymbolTableEntry);

    log_trace("recovering finished");

    // do not forget that includes are listed in PP_MEMORY too.
    recoverMemoryFromIncludeList();

    currentFile.lineNumber = cachePoint->lineNumber;
    currentFile.ifDepth = cachePoint->ifDepth;
    currentFile.ifStack = cachePoint->ifStack;
    currentInput = makeLexInput(cache.lexemStream, cachePoint->nextLexemP, readUntil, NULL, INPUT_CACHE);
    fillCache(&cache, cachingActive, cachePointIndex + 1, cachePoint->includeStackTop, cachePoint->nextLexemP,
              currentInput.read, currentInput.read, currentInput.write);
    log_trace("finished recovering");
}

/* ========================================================================== */
/*                           Cache Recovery                                  */
/* ========================================================================== */

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

/* ========================================================================== */
/*                            Input Caching                                  */
/* ========================================================================== */

/**
 * Check if input caching should be skipped due to current parsing context.
 * 
 * @return true if caching should be skipped, false if it can proceed
 */
static bool shouldSkipInputCaching(void) {
    if (!cachingIsActive()) {
        log_trace("Caching is not active");
        return true;
    }
    if (includeStack.pointer != 0 || macroStackIndex != 0) {
        log_trace("In include or macro, will not cache now");
        return true;
    }
    return false;
}

/**
 * Check if there's enough space in the cache buffer for the input.
 * 
 * @param size Number of bytes to cache
 * @return true if there's enough space, false if cache is full
 */
static bool hasEnoughCacheSpace(int size) {
    return (cache.free - cache.lexemStream + size < LEXEM_STREAM_CACHE_SIZE);
}

/**
 * Copy input data to the cache buffer if not already cached.
 * 
 * @param input The input to cache
 * @param size Number of bytes to copy
 */
static void copyInputToCache(LexInput *input, int size) {
    /* Avoid copying if we are already reading from cached data */
    if (input->inputType != INPUT_CACHE) {
        /* Copy from next un-cached to the free area of the cache buffer */
        memcpy(cache.free, cache.nextToCache, size);
    }
    cache.free += size;
    cache.nextToCache = input->read;
}

/**
 * Store lexical input in the cache buffer.
 * 
 * This function copies input tokens to the cache buffer for later reuse.
 * Caching is skipped when inside includes/macros or when the buffer is full.
 * 
 * @param input The lexical input to cache
 */
void cacheInput(LexInput *input) {
    ENTER();
    
    if (shouldSkipInputCaching()) {
        LEAVE();
        return;
    }
    
    /* Calculate how much needs to be cached */
    int size = input->read - cache.nextToCache;
    
    /* Check if there's enough space */
    if (!hasEnoughCacheSpace(size)) {
        deactivateCaching();
        LEAVE();
        return;
    }
    
    copyInputToCache(input, size);
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
    log_trace("currentBlock=%d, ppmMemory.index=%d, cxMemory.index=%d, macroBodyMemoryIndex=%d, cache.free =%d, \
cache.includeStackTop=%d, currentFile.lineNumber=%d, currentFile.ifDepth=%d, currentFile.ifStack=%d",
              currentBlock, ppmMemory.index, cxMemory.index, getMacroBodyMemoryIndex(), cache.free,
              cache.includeStackTop, currentFile.lineNumber, currentFile.ifDepth, currentFile.ifStack);
    fillCachePoint(cachePoint, currentBlock, ppmMemory.index, cxMemory.index, getMacroBodyMemoryIndex(), cache.free,
                   cache.includeStackTop, currentFile.lineNumber, currentFile.ifDepth, currentFile.ifStack, counters);
    cache.index++;
}

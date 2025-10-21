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
#include "yylex.h"
#include "reftab.h"
#include "filedescriptor.h"
#include "filetable.h"

#include "log.h"

/* ========================================================================== */
/*                              Global State                                 */
/* ========================================================================== */

Cache cache;

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

static void recoverMemoryFromReferenceTable(void) {
    mapOverReferenceTableWithIndex(recoverMemoryFromReferenceTableEntry);
}

void recoverCxMemory(void *cxMemoryFlushPoint) {
    cxFreeUntil(cxMemoryFlushPoint);
    recoverMemoryFromFileTable();
    recoverMemoryFromReferenceTable();
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

void recoverMemoriesAfterOverflow(char *cxMemFreeBase) {
    recoverCxMemory(cxMemFreeBase);
    initAllInputs();
}

void initCaching(void) {
    fillCache(&cache, true, 0, 0, cache.lexemStream, currentFile.lexemBuffer.read, NULL, NULL);
    placeCachePoint(false);
    deactivateCaching();
}

/* ========================================================================== */
/*                            Input Caching                                  */
/* ========================================================================== */

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

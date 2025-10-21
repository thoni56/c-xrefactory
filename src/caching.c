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

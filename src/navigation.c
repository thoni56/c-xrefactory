#include "navigation.h"

#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "browsermenu.h"
#include "commons.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "parsing.h"
#include "ppc.h"
#include "referenceableitemtable.h"
#include "startup.h"
#include "yylex.h"


static bool positionIsLessThanByFilename(Position p1, Position p2) {
    char fn1[MAX_FILE_NAME_SIZE];
    strcpy(fn1, simpleFileNameFromFileNum(p1.file));
    char *fn2 = simpleFileNameFromFileNum(p2.file);
    int fc = strcmp(fn1, fn2);
    if (fc < 0) return true;
    if (fc > 0) return false;
    if (p1.file < p2.file) return true;
    if (p1.file > p2.file) return false;
    if (p1.line < p2.line) return true;
    if (p1.line > p2.line) return false;
    return p1.col < p2.col;
}

static Reference *findLastReference(SessionStackEntry *sessionEntry, int filterLevel) {
    Reference *last = NULL;
    for (Reference *r = sessionEntry->references; r != NULL; r = r->next) {
        if (isMoreImportantUsageThan(r->usage, filterLevel))
            last = r;
    }
    return last;
}

static Reference *findPreviousReference(SessionStackEntry *sessionEntry, int filterLevel) {
    // Find the reference before current that passes the filter
    Reference *previous = NULL;
    if (sessionEntry->current != NULL) {
        for (Reference *r = sessionEntry->references; r != sessionEntry->current && r != NULL;
             r = r->next) {
            if (isMoreImportantUsageThan(r->usage, filterLevel))
                previous = r;
        }
    }
    return previous;
}

static Position getCurrentPosition(SessionStackEntry *sessionEntry) {
    return sessionEntry->current ? sessionEntry->current->position : makePosition(NO_FILE_NUMBER, 0, 0);
}

static void setCurrentReferenceToPreviousOrLast(SessionStackEntry *sessionEntry, Reference *previousReference,
                                                int filterLevel) {
    if (previousReference != NULL) {
        sessionEntry->current = previousReference;
    } else {
        ppcBottomInformation("Moving to the last reference");
        sessionEntry->current = findLastReference(sessionEntry, filterLevel);
    }
}

void setCurrentReferenceToFirstAfterCallerPosition(SessionStackEntry *sessionEntry) {
    Reference *reference;
    for (reference = sessionEntry->references; reference != NULL; reference = reference->next) {
        log_debug("checking %d:%d:%d to %d:%d:%d", reference->position.file, reference->position.line, reference->position.col,
                  sessionEntry->callerPosition.file, sessionEntry->callerPosition.line,
                  sessionEntry->callerPosition.col);
        if (!positionIsLessThanByFilename(reference->position, sessionEntry->callerPosition))
            break;
    }
    // it should never be NULL, but one never knows - DUH! We have coverage to show that you are wrong
    if (reference == NULL) {
        sessionEntry->current = sessionEntry->references;
    } else {
        sessionEntry->current = reference;
    }
}

void setCurrentReferenceToNextOrFirst(SessionStackEntry *sessionEntry, Reference *reference) {
    int rlevel = usageFilterLevels[sessionEntry->refsFilterLevel];

    while (reference != NULL && isAtMostAsImportantAs(reference->usage, rlevel))
        reference = reference->next;

    if (reference != NULL) {
        sessionEntry->current = reference;
    } else {
        assert(options.xref2);
        ppcBottomInformation("Moving to the first reference");
        reference = sessionEntry->references;
        while (reference != NULL && isAtMostAsImportantAs(reference->usage, rlevel))
            reference = reference->next;
        sessionEntry->current = reference;
    }
}

/*
 * Staleness handling for NEXT/PREVIOUS:
 *
 * The Emacs client sends preload (modified buffer content) only for the CURRENT file
 * (where the cursor is when the command is issued). We leverage this in two ways:
 *
 * 1. Source file refresh: When user navigates (NEXT/PREVIOUS), they're usually at a
 *    reference they previously navigated to. If they edited that file, we refresh it
 *    first, keeping current position.
 *
 * 2. Target file refresh: If navigating within the same file, the target reference's
 *    file also has preload, so we can refresh and find the correct next/previous.
 *
 * For cross-file navigation (target != source), the target won't have a preload,
 * so we can't detect its staleness. The user can recover by doing any operation
 * from the target file, which will then send its preload.
 */

static bool fileNumberIsStale(int fileNumber) {
    if (fileNumber == NO_FILE_NUMBER)
        return false;

    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileItem->name);

    // No preload = not stale
    if (buffer == NULL || buffer->preLoadedFromFile == NULL)
        return false;

    // Already refreshed? (lastParsedMtime updated to buffer's mtime after refresh)
    if (fileItem->lastParsedMtime >= buffer->modificationTime)
        return false;

    return true;
}

/* TODO: Technical Debt from extracting `navigation` */
extern void recomputeSelectedReferenceable(SessionStackEntry *entry);

void refreshStaleReferencesInSession(SessionStackEntry *sessionEntry, int fileNumber) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);

    // Update in-memory table: remove old, parse fresh
    removeReferenceableItemsForFile(fileNumber);

    // Reset macro state before re-parsing to clear header guards from previous parse.
    // Without this, #ifndef guards would prevent parsing the file body.
    restoreMemoryCheckPoint();
    initAllInputs();

    parseToCreateReferences(fileItem->name);

    // Remove refs for the stale file from menu - the menu already has cross-file refs
    // from the original PUSH operation. We only need to remove the stale file's refs
    // (which have old positions) and replace them with fresh refs from in-memory table.
    // NOTE: We do NOT clear all refs and scan from disk because the scan creates new
    // menu items (with selected=false) due to differing includeFileNumber, which
    // breaks the session's reference list (only selected items contribute refs).
    for (BrowserMenu *menu = sessionEntry->menu; menu != NULL; menu = menu->next) {
        Reference **refP = &menu->referenceable.references;
        while (*refP != NULL) {
            if ((*refP)->position.file == fileNumber) {
                Reference *toFree = *refP;
                *refP = (*refP)->next;
                free(toFree);
            } else {
                refP = &(*refP)->next;
            }
        }
    }

    // Merge fresh refs from in-memory table. This adds refs for the stale file
    // with updated positions (from preloaded content).
    // NOTE: We match by linkName only, not by all ReferenceableItem fields, because
    // the menu's item may have different includeFileNumber/type/etc. than what the
    // parser creates. The hash table lookup requires exact match of all fields, so
    // we iterate over the table and match by linkName.
    for (BrowserMenu *menu = sessionEntry->menu; menu != NULL; menu = menu->next) {
        for (int i = getNextExistingReferenceableItem(0); i >= 0;
             i = getNextExistingReferenceableItem(i + 1)) {
            for (ReferenceableItem *item = getReferenceableItem(i); item != NULL; item = item->next) {
                if (strcmp(item->linkName, menu->referenceable.linkName) == 0) {
                    extendBrowserMenuWithReferences(menu, item->references);
                }
            }
        }
    }

    // Mark file as freshly parsed so we don't re-refresh in this request
    EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileItem->name);
    if (buffer != NULL) {
        fileItem->lastParsedMtime = buffer->modificationTime;
    }

    // Rebuild session's reference list from updated menu
    recomputeSelectedReferenceable(sessionEntry);
    log_debug("After recompute: sessionEntry->references=%p", (void*)sessionEntry->references);
    for (Reference *r = sessionEntry->references; r != NULL; r = r->next) {
        log_debug("  ref: %d:%d:%d", r->position.file, r->position.line, r->position.col);
    }
}

/* Restore current reference to the reference matching savedPos (for "stay put" after refresh).
 *
 * Note: The reference list is sorted by filename, but Position comparison uses file numbers.
 * These orderings can differ, so we find an exact match by file number rather than using
 * position ordering for "nearest" lookup.
 *
 * If exact match not found (e.g., lines shifted due to edits), find nearest in same file.
 */
void restoreToNearestReference(SessionStackEntry *sessionEntry, Position savedPos, int filterLevel) {
    log_debug("restoreToNearestReference: savedPos=%d:%d:%d, references=%p",
              savedPos.file, savedPos.line, savedPos.col, (void*)sessionEntry->references);
    Reference *exactMatch = NULL;
    Reference *nearestInSameFile = NULL;
    int nearestDistance = INT_MAX;
    Reference *firstVisible = NULL;

    for (Reference *r = sessionEntry->references; r != NULL; r = r->next) {
        if (!isMoreImportantUsageThan(r->usage, filterLevel))
            continue;
        if (firstVisible == NULL)
            firstVisible = r;

        if (r->position.file == savedPos.file) {
            if (r->position.line == savedPos.line && r->position.col == savedPos.col) {
                exactMatch = r;
                break;
            }
            int distance = abs(r->position.line - savedPos.line);
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestInSameFile = r;
            }
        }
    }

    sessionEntry->current = exactMatch ? exactMatch : nearestInSameFile ? nearestInSameFile : firstVisible;
}

static void restoreToNextReferenceAfterRefresh(SessionStackEntry *sessionEntry, Position savedPos,
                                               int filterLevel) {
    ENTER();

    // First restore to where we were, then advance to next
    restoreToNearestReference(sessionEntry, savedPos, filterLevel);
    if (sessionEntry->current != NULL)
        setCurrentReferenceToNextOrFirst(sessionEntry, sessionEntry->current->next);

    LEAVE();
}

static void restoreToPreviousReferenceAfterRefresh(SessionStackEntry *sessionEntry, Position savedPos,
                                                   int filterLevel) {
    ENTER();

    // First restore to where we were, then step back to previous
    restoreToNearestReference(sessionEntry, savedPos, filterLevel);
    setCurrentReferenceToPreviousOrLast(sessionEntry, findPreviousReference(sessionEntry, filterLevel),
                                        filterLevel);

    LEAVE();
}

static void gotoCurrentReference(SessionStackEntry *sessionEntry) {
    if (sessionEntry->current != NULL) {
        ppcGotoPosition(sessionEntry->current->position);
    } else {
        ppcIndicateNoReference();
    }
}

void gotoNextReference(SessionStackEntry *sessionEntry) {
    ENTER();

    int filterLevel = usageFilterLevels[sessionEntry->refsFilterLevel];

    if (fileNumberIsStale(requestFileNumber)) {
        Position savedPos = getCurrentPosition(sessionEntry);
        log_debug("Refreshing stale source file %d: %s", requestFileNumber,
                  getFileItemWithFileNumber(requestFileNumber)->name);
        refreshStaleReferencesInSession(sessionEntry, requestFileNumber);
        restoreToNearestReference(sessionEntry, savedPos, filterLevel);
    }

    // Determine the next reference we would navigate to
    Reference *nextReference =
        (sessionEntry->current == NULL) ? sessionEntry->references : sessionEntry->current->next;

    // Save position and refresh if stale - position saved BEFORE refresh since
    // current pointer becomes dangling after refresh
    if (nextReference != NULL && fileNumberIsStale(nextReference->position.file)) {
        int fileNumber = nextReference->position.file;
        Position savedPos = getCurrentPosition(sessionEntry);
        log_debug("Refreshing stale file %d: %s", fileNumber, getFileItemWithFileNumber(fileNumber)->name);
        refreshStaleReferencesInSession(sessionEntry, fileNumber);
        restoreToNextReferenceAfterRefresh(sessionEntry, savedPos, filterLevel);
    } else {
        // Normal navigation - advance to next reference
        if (sessionEntry->current == NULL)
            sessionEntry->current = sessionEntry->references;
        else {
            setCurrentReferenceToNextOrFirst(sessionEntry, nextReference);
        }
    }

    gotoCurrentReference(sessionEntry);
    LEAVE();
}

void gotoPreviousReference(SessionStackEntry *sessionEntry) {
    ENTER();

    int filterLevel = usageFilterLevels[sessionEntry->refsFilterLevel];

    if (fileNumberIsStale(requestFileNumber)) {
        Position savedPos = getCurrentPosition(sessionEntry);
        log_debug("Refreshing stale source file %d: %s", requestFileNumber,
                  getFileItemWithFileNumber(requestFileNumber)->name);
        refreshStaleReferencesInSession(sessionEntry, requestFileNumber);
        restoreToNearestReference(sessionEntry, savedPos, filterLevel);
    }

    // Determine the previous reference we would navigate to
    Reference *previousReference = findPreviousReference(sessionEntry, filterLevel);

    // Save position and refresh if stale - position saved BEFORE refresh since
    // current pointer becomes dangling after refresh
    if (previousReference != NULL && fileNumberIsStale(previousReference->position.file)) {
        Position savedPos = getCurrentPosition(sessionEntry);
        int fileNumber = previousReference->position.file;
        log_debug("Refreshing stale file %d: %s", fileNumber, getFileItemWithFileNumber(fileNumber)->name);
        refreshStaleReferencesInSession(sessionEntry, fileNumber);
        restoreToPreviousReferenceAfterRefresh(sessionEntry, savedPos, filterLevel);
    } else {
        // Normal navigation
        if (sessionEntry->current == NULL)
            sessionEntry->current = sessionEntry->references;
        else {
            setCurrentReferenceToPreviousOrLast(sessionEntry, previousReference, filterLevel);
        }
    }

    gotoCurrentReference(sessionEntry);
    LEAVE();
}

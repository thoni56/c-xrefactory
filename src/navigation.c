#include "navigation.h"

#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "commons.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"


bool positionIsLessThanByFilename(Position p1, Position p2) {
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

void restoreToNextReferenceAfterRefresh(SessionStackEntry *sessionEntry, Position savedPos,
                                        int filterLevel) {
    ENTER();

    // After a refresh for "next", find the first reference AFTER savedPos
    sessionEntry->current = sessionEntry->references;
    while (sessionEntry->current != NULL) {
        if (positionIsLessThanByFilename(savedPos, sessionEntry->current->position)
            && isMoreImportantUsageThan(sessionEntry->current->usage, filterLevel))
            break; // Found first reference after where we were
        sessionEntry->current = sessionEntry->current->next;
    }

    // If no reference found after savedPos, wrap to first reference
    if (sessionEntry->current == NULL) {
        ppcBottomInformation("Moving to the first reference");
        sessionEntry->current = sessionEntry->references;
        while (sessionEntry->current != NULL
               && isAtMostAsImportantAs(sessionEntry->current->usage, filterLevel))
            sessionEntry->current = sessionEntry->current->next;
    }
    LEAVE();
}

void restoreToPreviousReferenceAfterRefresh(SessionStackEntry *sessionEntry, Position savedPos,
                                            int filterLevel) {
    ENTER();
    // After a refresh for "previous", find the last reference BEFORE savedPos
    sessionEntry->current = NULL;
    for (Reference *r = sessionEntry->references; r != NULL; r = r->next) {
        if (!positionIsLessThanByFilename(r->position, savedPos))
            break; // At or past saved position
        if (isMoreImportantUsageThan(r->usage, filterLevel))
            sessionEntry->current = r;
    }

    // If no reference found before savedPos, wrap to last reference
    if (sessionEntry->current == NULL) {
        ppcBottomInformation("Moving to the last reference");
        sessionEntry->current = findLastReference(sessionEntry, filterLevel);
    }
    LEAVE();
}

Position getCurrentPosition(SessionStackEntry *sessionEntry) {
    return sessionEntry->current ? sessionEntry->current->position : makePosition(NO_FILE_NUMBER, 0, 0);
}

Reference *findPreviousReference(SessionStackEntry *sessionEntry, int filterLevel) {
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

Reference *findLastReference(SessionStackEntry *sessionEntry, int filterLevel) {
    Reference *last = NULL;
    for (Reference *r = sessionEntry->references; r != NULL; r = r->next) {
        if (isMoreImportantUsageThan(r->usage, filterLevel))
            last = r;
    }
    return last;
}

void setCurrentToFirstReferenceAfterCallerPosition(SessionStackEntry *sessionStackEntry) {
    Reference *r;
    for (r = sessionStackEntry->references; r != NULL; r = r->next) {
        log_debug("checking %d:%d:%d to %d:%d:%d", r->position.file, r->position.line, r->position.col,
                  sessionStackEntry->callerPosition.file, sessionStackEntry->callerPosition.line,
                  sessionStackEntry->callerPosition.col);
        if (!positionIsLessThanByFilename(r->position, sessionStackEntry->callerPosition))
            break;
    }
    // it should never be NULL, but one never knows - DUH! We have coverage to show that you are wrong
    if (r == NULL) {
        sessionStackEntry->current = sessionStackEntry->references;
    } else {
        sessionStackEntry->current = r;
    }
}

void setCurrentReferenceToPreviousOrLast(SessionStackEntry *refs, Reference *previousReference,
                                         int filterLevel) {
    if (previousReference != NULL) {
        refs->current = previousReference;
    } else {
        ppcBottomInformation("Moving to the last reference");
        refs->current = findLastReference(refs, filterLevel);
    }
}

void setCurrentReferenceToFirstVisible(SessionStackEntry *refs, Reference *r) {
    int rlevel = usageFilterLevels[refs->refsFilterLevel];

    while (r != NULL && isAtMostAsImportantAs(r->usage, rlevel))
        r = r->next;

    if (r != NULL) {
        refs->current = r;
    } else {
        assert(options.xref2);
        ppcBottomInformation("Moving to the first reference");
        r = refs->references;
        while (r != NULL && isAtMostAsImportantAs(r->usage, rlevel))
            r = r->next;
        refs->current = r;
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

extern void refreshStaleReferencesInSession(SessionStackEntry *sessionEntry, int fileNumber);

/* Restore current reference to the reference matching savedPos (for "stay put" after refresh).
 *
 * Note: The reference list is sorted by filename, but Position comparison uses file numbers.
 * These orderings can differ, so we find an exact match by file number rather than using
 * position ordering for "nearest" lookup.
 *
 * If exact match not found (e.g., lines shifted due to edits), find nearest in same file.
 */
void restoreToNearestReference(SessionStackEntry *sessionEntry, Position savedPos, int filterLevel) {
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

extern bool sessionHasReferencesValidForOperation(SessionData *session, SessionStackEntry **entryP,
                                                  CheckNull checkNull);

static void gotoCurrentReference(SessionStackEntry *sessionEntry) {
    if (sessionEntry->current != NULL) {
        ppcGotoPosition(sessionEntry->current->position);
    } else {
        ppcIndicateNoReference();
    }
}

void gotoNextReference(void) {
    ENTER();

    SessionStackEntry *sessionEntry;
    if (!sessionHasReferencesValidForOperation(&sessionData, &sessionEntry, CHECK_NULL_YES)) {
        LEAVE();
        return;
    }

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
            setCurrentReferenceToFirstVisible(sessionEntry, nextReference);
        }
    }

    gotoCurrentReference(sessionEntry);
    LEAVE();
}

void gotoPreviousReference(void) {
    ENTER();

    SessionStackEntry *sessionEntry;
    if (!sessionHasReferencesValidForOperation(&sessionData, &sessionEntry, CHECK_NULL_YES)) {
        LEAVE();
        return;
    }

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

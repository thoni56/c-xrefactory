#include "navigation.h"

#include <stdbool.h>
#include <string.h>

#include "head.h"
#include "log.h"
#include "misc.h"
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

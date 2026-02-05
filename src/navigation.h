#ifndef _NAVIGATION_H_INCLUDED
#define _NAVIGATION_H_INCLUDED

#include "position.h"
#include "session.h"
#include "reference.h"


extern bool positionIsLessThanByFilename(Position p1, Position p2);
extern void restoreToNextReferenceAfterRefresh(SessionStackEntry *sessionEntry, Position savedPos,
                                               int filterLevel);
extern void restoreToPreviousReferenceAfterRefresh(SessionStackEntry *sessionEntry, Position savedPos,
                                                   int filterLevel);
extern Position getCurrentPosition(SessionStackEntry *sessionEntry);
extern Reference *findPreviousReference(SessionStackEntry *sessionEntry, int filterLevel);
extern Reference *findLastReference(SessionStackEntry *sessionEntry, int filterLevel);
extern void setCurrentToFirstReferenceAfterCallerPosition(SessionStackEntry *sessionStackEntry);
extern void setCurrentReferenceToFirstVisible(SessionStackEntry *refs, Reference *r);

#endif

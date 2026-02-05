#ifndef _NAVIGATION_H_INCLUDED
#define _NAVIGATION_H_INCLUDED

#include "position.h"
#include "session.h"


extern bool positionIsLessThanByFilename(Position p1, Position p2);
extern void restoreToNextReferenceAfterRefresh(SessionStackEntry *sessionEntry, Position savedPos,
                                               int filterLevel);
extern void restoreToPreviousReferenceAfterRefresh(SessionStackEntry *sessionEntry, Position savedPos,
                                                   int filterLevel);
extern Position getCurrentPosition(SessionStackEntry *sessionEntry);

#endif

#ifndef _NAVIGATION_H_INCLUDED
#define _NAVIGATION_H_INCLUDED

#include "session.h"
#include "reference.h"


extern void setCurrentReferenceToFirstAfterCallerPosition(SessionStackEntry *sessionEntry);
extern void setCurrentReferenceToFirstVisible(SessionStackEntry *sessionEntry, Reference *reference);
extern void gotoNextReference(SessionStackEntry *sessionEntry);
extern void gotoPreviousReference(SessionStackEntry *sessionEntry);
extern void refreshStaleReferencesInSession(SessionStackEntry *sessionEntry, int fileNumber);

#endif

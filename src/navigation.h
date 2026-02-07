#ifndef _NAVIGATION_H_INCLUDED
#define _NAVIGATION_H_INCLUDED

#include "session.h"
#include "reference.h"


extern void setCurrentReferenceToFirstAfterCallerPosition(SessionStackEntry *sessionEntry);
extern void setCurrentReferenceToFirstVisible(SessionStackEntry *sessionEntry, Reference *reference);
extern void gotoNextReference(void);
extern void gotoPreviousReference(void);

#endif

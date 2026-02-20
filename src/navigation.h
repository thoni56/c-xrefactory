#ifndef _NAVIGATION_H_INCLUDED
#define _NAVIGATION_H_INCLUDED

#include "argumentsvector.h"
#include "session.h"
#include "reference.h"


extern void setCurrentReferenceToFirstAfterCallerPosition(SessionStackEntry *sessionEntry);
extern void setCurrentReferenceToNextOrFirst(SessionStackEntry *sessionEntry, Reference *reference);
extern void gotoNextReference(SessionStackEntry *sessionEntry);
extern void gotoPreviousReference(SessionStackEntry *sessionEntry);
extern bool fileNumberIsStale(int fileNumber);
extern void parseFileWithFullInit(char *fileName, ArgumentsVector baseArgs);
extern void reparseStaleFile(int fileNumber, ArgumentsVector baseArgs);

#endif

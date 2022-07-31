#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <stdbool.h>

#include "head.h" /* For Language type */

// Used by server.c & xref.c but not main itself
extern bool initializeFileProcessing(bool *firstPass, int argc, char **argv, int nargc, char **nargv,
                                     Language *outLanguage);
//------

// Required for xref.c
extern void checkExactPositionUpdate(bool printMessage);
//------

extern int mainHandleSetOption(int argc, char **argv, int i );
extern void mainTaskEntryInitialisations(int argc, char **argv);
extern void mainOpenOutputFile(char *ofile);

#endif

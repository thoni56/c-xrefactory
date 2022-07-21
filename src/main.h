#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "proto.h"
#include "options.h"


// Required for server.c extraction...
extern bool fileProcessingInitialisations(bool *firstPass, int argc, char **argv, int nargc, char **nargv,
                                          Language *outLanguage);
//------

// Required for xref.c extraction
extern void checkExactPositionUpdate(bool printMessage);
//------

extern void searchStandardOptionsFileFor(char *filename, char *options_filename, char *section);
extern void getPipedOptions(int *outNargc,char ***outNargv);

extern void writeRelativeProgress(int progress);

extern int mainHandleSetOption(int argc, char **argv, int i );
extern void mainTaskEntryInitialisations(int argc, char **argv);
extern void mainOpenOutputFile(char *ofile);

#endif

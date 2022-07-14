#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "proto.h"
#include "options.h"

// TODO: Where does this belong?
extern void aboutMessage(void);

// Required for server.c extraction...
bool fileProcessingInitialisations(bool *firstPass, int argc, char **argv, int nargc, char **nargv,
                                   Language *outLanguage);
void closeInputFile(void);
//------

extern void searchDefaultOptionsFile(char *filename, char *options_filename, char *section);
extern void getPipedOptions(int *outNargc,char ***outNargv);

extern char *getNextScheduledFile(int *fArgCount);
extern void writeRelativeProgress(int progress);

extern int mainHandleSetOption(int argc, char **argv, int i );
extern void mainCallXref(int argc, char **argv);
extern void mainTaskEntryInitialisations(int argc, char **argv);
extern void mainOpenOutputFile(char *ofile);

#endif

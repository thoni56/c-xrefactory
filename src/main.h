#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "proto.h"
#include "options.h"


extern void dirInputFile(MAP_FUN_SIGNATURE);

extern void createOptionString(char **optAddress, char *text);
extern void copyOptions(Options *dest, Options *src);
extern void searchDefaultOptionsFile(char *filename, char *options_filename, char *section);
extern void processOptions(int argc, char **argv, ProcessFileArguments infilesFlag);
extern void getPipedOptions(int *outNargc,char ***outNargv);

extern char *getNextInputFile(int *fArgCount);
extern void writeRelativeProgress(int progress);

extern int mainHandleSetOption(int argc, char **argv, int i );
extern void mainSetLanguage(char *inFileName, Language *outLanguage);
extern void mainCallEditServerInit(int nargc, char **nargv);
extern void mainCallEditServer(int argc, char **argv,
                               int nargc, char **nargv,
                               bool *firstPass);
extern void mainCallXref(int argc, char **argv);
extern void mainTaskEntryInitialisations(int argc, char **argv);
extern void mainOpenOutputFile(char *ofile);

#endif

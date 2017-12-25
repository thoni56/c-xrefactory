#ifndef MAIN_H
#define MAIN_H

#include "proto.h"

extern int creatingOlcxRefs();
extern void dirInputFile(MAP_FUN_PROFILE);
extern void xrefSetenv(char *name, char *val);

extern void createOptionString(char **dest, char *text);
extern void copyOptions(S_options *dest, S_options *src);
extern void searchDefaultOptionsFile(char *file, char *ttt, char *sect);
extern void processOptions(int argc, char **argv, int infilesFlag);
extern void getPipedOptions(int *outNargc,char ***outNargv);

extern char *getInputFile(int *fArgCount);
extern void writeRelativeProgress(int progress);

extern int mainHandleSetOption(int argc, char **argv, int i );
extern void mainSetLanguage(char *inFileName, int *outLanguage);
extern void mainCallEditServerInit(int nargc, char **nargv);
extern void mainCallEditServer(int argc, char **argv,
                               int nargc, char **nargv,
                               int *firstPassing
                               );
extern void mainCallXref(int argc, char **argv);
extern void mainTaskEntryInitialisations(int argc, char **argv);
extern void mainOpenOutputFile(char *ofile);
extern void mainCloseOutputFile();

#endif

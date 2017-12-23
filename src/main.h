#ifndef MAIN_H
#define MAIN_H

#include "proto.h"

extern int creatingOlcxRefs();
extern void dirInputFile(MAP_FUN_PROFILE);
extern void createOptionString(char **dest, char *text);
extern void xrefSetenv(char *name, char *val);
extern int mainHandleSetOption(int argc, char **argv, int i );
extern void copyOptions(S_options *dest, S_options *src);
extern void resetPendingSymbolMenuData();
extern char *presetEditServerFileDependingStatics();
extern void searchDefaultOptionsFile(char *file, char *ttt, char *sect);
extern void processOptions(int argc, char **argv, int infilesFlag);
extern void mainSetLanguage(char *inFileName, int *outLanguage);
extern void getAndProcessXrefrcOptions(char *dffname, char *dffsect, char *project);
extern char * getInputFile(int *fArgCount);
extern void getPipedOptions(int *outNargc,char ***outNargv);
extern void mainCallEditServerInit(int nargc, char **nargv);
extern void mainCallEditServer(int argc, char **argv,
                               int nargc, char **nargv,
                               int *firstPassing
                               );
extern void mainCallXref(int argc, char **argv);
extern void mainXref(int argc, char **argv);
extern void writeRelativeProgress(int progress);
extern void mainTaskEntryInitialisations(int argc, char **argv);
extern void mainOpenOutputFile(char *ofile);
extern void mainCloseOutputFile();

#endif

#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include "stdinc.h"
#include "stdbool.h"

extern void addSourcePathsCut(void);
extern void getXrefrcFileName(char *ttt);
extern char *getJavaHome(void);
extern void getJavaClassAndSourcePath(void);
extern bool packageOnCommandLine(char *packageName);
extern char *expandSpecialFilePredefinedVariables_st(char *tt, char *inputFilename);
extern bool readOptionFromFile(FILE *ff, int *nargc, char ***nargv,
                               int memFl, char *sectionFile, char *project, char *resSection);
extern void readOptionFile(char *name, int *nargc, char ***nargv, char *sectionFile, char *project);
extern void readOptionPipe(char *command, int *nargc, char ***nargv, char *sectionFile);
extern void javaSetSourcePath(int defaultCpAllowed);
extern bool checkReferenceFileCountOption(int newRefNum);

#endif

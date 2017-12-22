#ifndef OPTIONS_H
#define OPTIONS_H

extern void addSourcePathesCut();
extern void getXrefrcFileName( char *ttt );
extern int processInteractiveFlagOption(char **argv, int i);
extern char *getJavaHome();
extern char *getJdkClassPathFastly();
extern void getJavaClassAndSourcePath();
extern int packageOnCommandLine(char *fn);
extern void getStandardOptions(int *nargc, char ***nargv);
extern char *expandSpecialFilePredefinedVariables_st(char *tt);
extern int readOptionFromFile(FILE *ff, int *nargc, char ***nargv,
                              int memFl, char *sectionFile, char *project, char *resSection);
extern void readOptionFile(char *name, int *nargc, char ***nargv,char *sectionFile, char *project);
extern void readOptionPipe(char *command, int *nargc, char ***nargv,char *sectionFile);
extern void javaSetSourcePath(int defaultCpAllowed);
extern void getOptionsFromMessage(char *qnxMsgBuff, int *nargc, char ***nargv);
extern int changeRefNumOption(int newRefNum);

#endif

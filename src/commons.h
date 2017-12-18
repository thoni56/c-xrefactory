#ifndef COMMONS_H
#define COMMONS_H

extern void emergencyExit(int exitStatus);
extern void warning(int kod, char *sprava);
extern void error(int kod, char *sprava);
extern void fatalError(int kod, char *sprava, int exitCode);
extern void internalCheckFail(char *expr, char *file, int line);

#endif

#ifndef COMMONS_H
#define COMMONS_H

#define InternalCheck(expr) {\
    if (!(expr)) internalCheckFail(#expr, __FILE__, __LINE__);\
}

#undef assert
#define assert(expr) InternalCheck(expr)

extern void initCwd(void);
extern void reInitCwd(char *dffname, char *dffsect);
extern void emergencyExit(int exitStatus);
extern void warning(int kod, char *sprava);
extern void error(int kod, char *sprava);
extern void fatalError(int kod, char *sprava, int exitCode);
extern void internalCheckFail(char *expr, char *file, int line);

extern char *create_temporary_filename();
extern char *normalizeFileName(char *name, char *relativeto);

extern void copyFile(char *src, char *dest);
extern void createDir(char *dirname);
extern void copyDir(char *dest, char *s, int *i);
extern void removeFile(char *dirname);


#endif

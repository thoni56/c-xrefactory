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
extern void warningMessage(int code, char *message);
extern void errorMessage(int code, char *message);
extern void fatalError(int code, char *message, int exitCode);
extern void internalCheckFail(char *expr, char *file, int line);

extern char *create_temporary_filename(void);
extern void copyFileFromTo(char *src, char *dest);
extern void copyDir(char *dest, char *s, int *i);
extern char *normalizeFileName(char *name, char *relativeto);

extern void closeMainOutputFile(void);

#endif

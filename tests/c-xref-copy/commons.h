#ifndef COMMONS_H_INCLUDED
#define COMMONS_H_INCLUDED

#include <stdlib.h>


#define InternalCheck(expr) {\
    if (!(expr)) internalCheckFail(#expr, __FILE__, __LINE__);\
}

#undef assert
#define assert(expr) InternalCheck(expr)

extern void initCwd(void);
extern void reInitCwd(char *dffname, char *dffsect);

extern void infoMessage(char message[]);
extern void warningMessage(int code, char *message);
extern void errorMessage(int code, char *message);
#define FATAL_ERROR(code, message, exitCode) fatalError(code, message, EXIT_FAILURE, __FILE__, __LINE__);
extern void fatalError(int code, char *message, int exitCode, char *file, int line);
extern void internalCheckFail(char *expr, char *file, int line);

extern char *create_temporary_filename(void);
extern void copyFileFromTo(char *src, char *dest);
extern int extractPathInto(char *source, char *dest); /* Return length of path */
extern char *normalizeFileName_static(char *name, char *relativeto);


extern void openOutputFile(char *outfile);
extern void closeOutputFile(void);

#endif

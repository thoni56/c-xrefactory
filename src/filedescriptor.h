#ifndef _FILEDESCRIPTOR_H_
#define _FILEDESCRIPTOR_H_

#include "proto.h"
#include "lex.h"


typedef struct fileDesc {
    char                *fileName ;
    int                 lineNumber ;
    int					ifDepth;					/* depth of #ifs (C only)*/
    struct cppIfStack   *ifStack;					/* #if stack (C only) */
    struct lexBuf       lexBuffer;
} S_fileDesc;


extern S_fileDesc cFile;

extern struct fileDesc inStack[INCLUDE_STACK_SIZE];
extern int inStacki;


extern void fillFileDescriptor(S_fileDesc *fileDescriptor, char *name, char *bbase, int bsize, FILE *ff, unsigned filepos);

#endif

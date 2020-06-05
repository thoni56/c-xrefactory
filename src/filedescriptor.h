#ifndef _FILEDESCRIPTOR_H_
#define _FILEDESCRIPTOR_H_

#include "proto.h"
#include "lexer.h"


typedef struct fileDescriptor {
    char                *fileName ;
    int                 lineNumber ;
    int					ifDepth;					/* depth of #ifs (C only)*/
    struct cppIfStack   *ifStack;					/* #if stack (C only) */
    struct lexBuf       lexBuffer;
} FileDescriptor;


extern FileDescriptor currentFile;

extern FileDescriptor inStack[INCLUDE_STACK_SIZE];
extern int inStacki;


extern void fillFileDescriptor(FileDescriptor *fileDescriptor, char *name, char *bbase, int bsize, FILE *ff, unsigned filepos);

#endif

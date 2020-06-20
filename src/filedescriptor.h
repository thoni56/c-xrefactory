#ifndef _FILEDESCRIPTOR_H_
#define _FILEDESCRIPTOR_H_

#include "proto.h"
#include "lexembuffer.h"


typedef struct fileDescriptor {
    char *fileName ;
    int lineNumber ;
    int ifDepth;                  /* depth of #ifs (C only)*/
    struct cppIfStack *ifStack; /* #if stack (C only) */
    LexemBuffer lexBuffer;
} FileDescriptor;


extern FileDescriptor currentFile;

extern FileDescriptor includeStack[INCLUDE_STACK_SIZE];
extern int includeStackPointer;


extern void fillFileDescriptor(FileDescriptor *fileDescriptor, char *name, char *bbase, int bsize, FILE *ff, unsigned filepos);

#endif

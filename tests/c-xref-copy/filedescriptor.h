#ifndef FILEDESCRIPTOR_H_INCLUDED
#define FILEDESCRIPTOR_H_INCLUDED

#include "characterreader.h"
#include "lexembuffer.h"


typedef struct fileDescriptor {
    char *fileName ;
    int lineNumber ;
    int ifDepth;                  /* Depth of #ifs */
    struct cppIfStack *ifStack;   /* #if stack */
    LexemBuffer        lexemBuffer;
    CharacterBuffer    characterBuffer;
} FileDescriptor;

typedef struct {
    FileDescriptor stack[INCLUDE_STACK_SIZE];
    int pointer;
} IncludeStack;


extern FileDescriptor currentFile;

extern IncludeStack includeStack;


extern void fillFileDescriptor(FileDescriptor *fileDescriptor, char *name, char *bbase, int bsize,
                               FILE *file, unsigned filepos);

#endif

#ifndef _FILEDESCRIPTOR_H_
#define _FILEDESCRIPTOR_H_

#include "proto.h"

typedef struct fileDesc {
    char                *fileName ;
    int                 lineNumber ;
    int					ifDeep;						/* deep of #ifs (C only)*/
    struct cppIfStack   *ifStack;					/* #if stack (C only) */
    struct lexBuf       lexBuffer;
} S_fileDesc;

extern S_fileDesc cFile;

extern struct fileDesc inStack[INSTACK_SIZE];
extern int inStacki;

#endif

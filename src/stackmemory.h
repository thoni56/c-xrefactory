#ifndef STACKMEMORY_H_INCLUDED
#define STACKMEMORY_H_INCLUDED

#include <stdbool.h>


/**********************************************************************

    Stack memory synchronized with program block structure.
*/


typedef struct stackFrame {
    void             (*action)(void*);
    void             *argument;
    struct stackFrame *next;
} FrameAllocation;

typedef struct codeBlock {
    int              firstFreeIndex;
    struct stackFrame *frameAllocations;
    struct codeBlock *outerBlock;
    /* A file scope block holds declarations that live for the whole file
       without being global. It is not a nesting level. */
    bool             isFileScope;
} CodeBlock;


extern char stackMemory[];

extern CodeBlock *currentBlock;


extern void setErrorHandlerForStackMemory(void (*function)(int code, char *message));

extern void *stackMemoryAlloc(int size);
extern char *stackMemoryPushString(char *s);

extern void beginBlock(void);
extern void beginFileScopeBlock(void);
extern void endBlock(void);
extern int nestingLevel(void);

extern bool isMemoryFromPreviousBlock(void *ppp);
extern bool isFreedStackMemory(void *ptr);

extern void addToFrame(void (*action)(void*), void *argument);
extern void removeFromFrameUntil(FrameAllocation *untilP);
extern void initOuterCodeBlock(void);

extern void stackMemoryStatistics(void);

#endif

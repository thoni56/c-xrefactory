#ifndef STACKMEMORY_H_INCLUDED
#define STACKMEMORY_H_INCLUDED

#include <stdbool.h>


/**********************************************************************

    Stack memory synchronized with program block structure.
*/

/* The "trail" seems to only be used for Java so ignore it for now... */
typedef struct freeTrail {
    void             (*action)(void*);
    void             *pointer;
    struct freeTrail *next;
} FreeTrail;

typedef struct codeBlock {
    int              firstFreeIndex;
    struct freeTrail *trail;
    struct codeBlock *outerBlock;
} CodeBlock;


extern char stackMemory[];

extern CodeBlock *currentBlock;


extern void *stackMemoryAlloc(int size);
extern char *stackMemoryPushString(char *s);

extern void beginBlock(void);
extern void endBlock(void);
extern int nestingLevel(void);

extern bool isMemoryFromPreviousBlock(void *ppp);
extern bool isFreedPointer(void *ptr);


extern void addToTrail(void (*action)(void*), void *p, bool needTrailOnTopLevel);
extern void removeFromTrailUntil(FreeTrail *untilP);
extern void initOuterCodeBlock(void);

#endif

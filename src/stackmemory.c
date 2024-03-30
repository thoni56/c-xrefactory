#include "constants.h"
#include "proto.h"
#include "commons.h"
#include "globals.h"

#include "stackmemory.h"

#include "log.h"


CodeBlock *currentBlock;


static bool memoryTrace = false;
#define mem_trace(...)  { if (memoryTrace) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__); }

/* Inject the function to call for error() */
static void (*error)(int code, char *message);
void setErrorHandlerForMemory(void (*function)(int code, char *message)) {
    error = function;
}

/* Stack memory - in this we allocate blocks which have separate free indices */
char stackMemory[SIZE_stackMemory];   /* Allocation using stackMemoryAlloc() et.al */


static void trailDump(void) {
    log_trace("*** begin trailDump");
    for (FreeTrail *t=currentBlock->trail; t!=NULL; t=t->next)
        log_trace("%p ", t);
    log_trace("*** end trailDump");
}


void addToTrail(void (*action)(void*), void *argument) {
    FreeTrail *t;

    /* no trail at level 0 in C, Yacc */
    if ((nestingLevel() == 0) && !(LANGUAGE(LANG_C)||LANGUAGE(LANG_YACC)))
        return;

    t = stackMemoryAlloc(sizeof(FreeTrail));
    t->action = action;
    t->argument = (void **) argument;
    t->next = currentBlock->trail;
    currentBlock->trail = t;
    if (memoryTrace)
        trailDump();
}

void removeFromTrailUntil(FreeTrail *untilP) {
    FreeTrail *p;
    for (p=currentBlock->trail; untilP<p; p=p->next) {
        assert(p!=NULL);
        (*(p->action))(p->argument);
    }
    if (p!=untilP) {
        error(ERR_INTERNAL, "block structure mismatch?");
    }
    currentBlock->trail = p;
    if (memoryTrace)
        trailDump();
}

static void fillCodeBlock(CodeBlock *block, int firstFreeIndex, FreeTrail *trail, CodeBlock *outerBlock) {
    block->firstFreeIndex = firstFreeIndex;
    block->trail = trail;
    block->outerBlock = outerBlock;
}

void initOuterCodeBlock(void) {
    /* Any use of stack memory will require that this is run first */
    currentBlock = (CodeBlock *) stackMemory;
    fillCodeBlock(currentBlock, sizeof(CodeBlock), NULL, NULL);
}

void *stackMemoryAlloc(int size) {
    int i;

    assert(currentBlock);
    mem_trace("stackMemoryAlloc: allocating %d bytes", size);
    i = currentBlock->firstFreeIndex;
    if (i+size < SIZE_stackMemory) {
        currentBlock->firstFreeIndex = i+size;
        return &stackMemory[i];
    } else {
        fatalError(ERR_ST,"i+size > SIZE_stackMemory,\n\tstack memory overflowed,\n\tread TROUBLES section of README file\n", XREF_EXIT_ERR, __FILE__, __LINE__);
        /* Should not return, but for testing and compilers sake return something */
        return NULL;
    }
}

static void *stackMemoryPush(void *pointer, int size) {
    void *new = stackMemoryAlloc(size);
    memcpy(new, pointer, size);
    return new;
}

char *stackMemoryPushString(char *string) {
    log_trace("Pushing string '%s'", string);
    return (char*)stackMemoryPush(string, strlen(string)+1);
}

void beginBlock(void) {
    CodeBlock *pushed, previous;
    log_trace("Begin block");
    previous = *currentBlock;
    pushed = stackMemoryPush(&previous, sizeof(CodeBlock));
    // trail can't be reset to NULL, because in case of syntax errors
    // this would avoid balancing of } at the end of class
    fillCodeBlock(currentBlock, currentBlock->firstFreeIndex, currentBlock->trail, pushed);
}

void endBlock(void) {
    log_trace("End block");
    //&removeFromTrailUntil(NULL);
    assert(currentBlock && currentBlock->outerBlock);
    removeFromTrailUntil(currentBlock->outerBlock->trail);
    *currentBlock =  *currentBlock->outerBlock;
    assert(currentBlock != NULL);
}

int nestingLevel(void) {
    int level = 0;
    CodeBlock *block = currentBlock;
    while (block->outerBlock != NULL) {
        block = block->outerBlock;
        level++;
    }
    return level;
}

bool isMemoryFromPreviousBlock(void *address) {
    return currentBlock->outerBlock != NULL &&
        (char*)address > stackMemory &&
        (char*)address < stackMemory + currentBlock->outerBlock->firstFreeIndex;
}

bool isFreedPointer(void *ptr) {
    return ((char*)ptr >= stackMemory + currentBlock->firstFreeIndex &&
            (char*)ptr < stackMemory + SIZE_stackMemory);
}

#include "stackmemory.h"

#include <string.h>
#include <stdlib.h>

#include "commons.h"
#include "constants.h"
#include "log.h"
#include "proto.h"



CodeBlock *currentBlock;


static bool memoryTrace = false;
#define mem_trace(...)  { if (memoryTrace) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__); }

/* Inject the function to call for error() */
static void (*error)(int code, char *message);
void setErrorHandlerForStackMemory(void (*function)(int code, char *message)) {
    error = function;
}

/* Stack memory - in this we allocate blocks which have separate free indices */
char stackMemory[StackMemorySize];   /* Allocation using stackMemoryAlloc() et.al */


static void frameDump(void) {
    log_debug("*** begin frameDump");
    for (FrameAllocation *f=currentBlock->frameAllocations; f!=NULL; f=f->next)
        log_debug("%p ", f);
    log_debug("*** end frameDump");
}


void addToFrame(void (*action)(void*), void *argument) {
    FrameAllocation *f;

    f = stackMemoryAlloc(sizeof(FrameAllocation));
    f->action = action;
    f->argument = (void **) argument;
    f->next = currentBlock->frameAllocations;
    currentBlock->frameAllocations = f;
    if (memoryTrace)
        frameDump();
}

void removeFromFrameUntil(FrameAllocation *untilP) {
    FrameAllocation *f;
    for (f=currentBlock->frameAllocations; untilP<f; f=f->next) {
        assert(f!=NULL);
        (*(f->action))(f->argument);
    }
    if (f!=untilP) {
        error(ERR_INTERNAL, "block structure mismatch?");
    }
    currentBlock->frameAllocations = f;
    if (memoryTrace)
        frameDump();
}

void recoverMemoryFromFrameAllocations(void) {
    FrameAllocation **pp;
    pp = &currentBlock->frameAllocations;
    while (isFreedStackMemory(*pp)) {
        *pp = (*pp)->next;
    }
}

static void fillCodeBlock(CodeBlock *block, int firstFreeIndex, FrameAllocation *allocation, CodeBlock *outerBlock) {
    block->firstFreeIndex = firstFreeIndex;
    block->frameAllocations = allocation;
    block->outerBlock = outerBlock;
}

void initOuterCodeBlock(void) {
    /* Any use of stack memory will require that this is run first */
    currentBlock = (CodeBlock *) stackMemory;
    fillCodeBlock(currentBlock, sizeof(CodeBlock), NULL, NULL);
}

static int stackMemoryMax = 0;

void *stackMemoryAlloc(int size) {
    int i;

    assert(currentBlock);
    mem_trace("stackMemoryAlloc: allocating %d bytes", size);
    i = currentBlock->firstFreeIndex;
    if (i+size < StackMemorySize) {
        currentBlock->firstFreeIndex = i+size;
        if (currentBlock->firstFreeIndex > stackMemoryMax)
            stackMemoryMax = currentBlock->firstFreeIndex;
        return &stackMemory[i];
    } else {
        fatalError(ERR_ST,"i+size > SIZE_stackMemory,\n\tstack memory overflowed,\n\tread TROUBLES section of README file\n", EXIT_FAILURE, __FILE__, __LINE__);
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
    log_debug("Pushing string '%s'", string);
    return (char*)stackMemoryPush(string, strlen(string)+1);
}

void beginBlock(void) {
    CodeBlock *pushed, previous;
    log_debug("Begin block");
    previous = *currentBlock;
    pushed = stackMemoryPush(&previous, sizeof(CodeBlock));
    // allocation can't be reset to NULL, because in case of syntax errors
    // this would avoid balancing of } at the end of class
    fillCodeBlock(currentBlock, currentBlock->firstFreeIndex, currentBlock->frameAllocations, pushed);
}

void endBlock(void) {
    log_debug("End block");
    //&removeFromFrameUntil(NULL);
    assert(currentBlock && currentBlock->outerBlock);
    removeFromFrameUntil(currentBlock->outerBlock->frameAllocations);
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

bool isFreedStackMemory(void *ptr) {
    return ((char*)ptr >= stackMemory + currentBlock->firstFreeIndex &&
            (char*)ptr < stackMemory + StackMemorySize);
}

void stackMemoryStatistics(void) {
    printf("Max memory use for stack: %d\n", stackMemoryMax);
}

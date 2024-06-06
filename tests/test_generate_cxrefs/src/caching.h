#ifndef CACHING_H_INCLUDED
#define CACHING_H_INCLUDED

#include "stdinc.h"
#include "counters.h"
#include "stackmemory.h"
#include "input.h"
#include "constants.h"

typedef struct {
    struct codeBlock  *topBlock;
    struct codeBlock   topBlockContent;
    int                ppmMemoryIndex;
    int                cxMemoryIndex;
    int                macroBodyMemoryIndex;
    char              *nextLexemP;
    short int          includeStackTop;
    short int          lineNumber;
    short int          ifDepth;
    struct cppIfStack *ifStack;
    Counters           counters;
} CachePoint;

typedef struct {
    bool       active; /* whether putting input to cache */
    CachePoint points[MAX_CACHE_POINTS];
    int        index;                                  /* Into points */
    int        includeStack[INCLUDE_STACK_CACHE_SIZE]; /* included files numbers */
    int        includeStackTop;
    char       lexemStream[LEXEM_STREAM_CACHE_SIZE]; /* lexems buffer */
    char      *free;                                 /* first free in lexemStream */
    char      *nextToCache;                          /* first not yet cached lexem */
    char      *read;                                 /* cc when input from cache */
    char      *write;                                /* end of cc, when input ... */
} Cache;

extern Cache cache;

extern void initCaching(void);
extern void activateCaching(void);
extern void deactivateCaching(void);
extern bool cachingIsActive(void);
extern void recoverCachePoint(int cachePointIndex, char *readUntil, bool cachingActive);
extern void recoverFromCache(void);
extern void cacheInput(LexInput *input);
extern void cacheInclude(int fileNum);
extern void placeCachePoint(bool inputCaching);
extern void recoverCachePointZero(void);
extern void recoverMemoriesAfterOverflow(char *cxMemFreeBase);
extern bool checkFileModifiedTime(int fileIndex);

#endif

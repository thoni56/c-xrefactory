#ifndef CACHING_H_INCLUDED
#define CACHING_H_INCLUDED

#include "globals.h" /* For Counters */
#include "memory.h"

typedef struct {
    struct codeBlock  *topBlock;
    struct codeBlock   topBlockContent;
    int                ppmMemoryIndex;
    int                cxMemoryIndex;
    int                mbMemoryIndex;
    char              *nextLexemP; /* caching lbcc */
    short int          includeStackTop;  /* caching ibi */
    short int          lineNumber;
    short int          ifDepth;
    struct cppIfStack *ifStack;
    struct javaStat   *javaStat;
    struct counters    counters;
} CachePoint;

typedef struct {
    bool       cachingActive; /* whether putting input to cache */
    CachePoint cachePoints[MAX_CACHE_POINTS];
    int        cachePointIndex;
    int        includeStack[INCLUDE_STACK_CACHE_SIZE]; /* included files numbers */
    int        includeStackTop;
    char       lexemStream[LEXEM_STREAM_CACHE_SIZE]; /* lexems buffer */
    char      *lexemStreamFree;                      /* first free in lexemStream */
    char      *lexemStreamNext;                      /* first not yet cached lexem */
    char      *read;                        /* cc when input from cache */
    char      *write;                       /* end of cc, when input ... */
} Cache;

extern Cache cache;

extern void initCaching(void);
extern void recoverCachePoint(int cachePointIndex, char *readUntil, bool cachingActive);
extern void recoverFromCache(void);
extern void cacheInput(void);
extern void cacheInclude(int fileNum);
extern void placeCachePoint(bool inputCache);
extern void recoverCachePointZero(void);
extern void recoverMemoriesAfterOverflow(char *cxMemFreeBase);
extern bool checkFileModifiedTime(int fileIndex);

#endif

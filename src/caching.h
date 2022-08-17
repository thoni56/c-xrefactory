#ifndef CACHING_H_INCLUDED
#define CACHING_H_INCLUDED

#include "globals.h" /* For Counters */
#include "memory.h"

typedef struct cachePoint {
    struct codeBlock  *topBlock;
    struct codeBlock   topBlockContent;
    int                ppmMemoryIndex;
    int                cxMemoryIndex;
    int                mbMemoryIndex;
    char              *currentLexemP; /* caching lbcc */
    short int          ibi;  /* caching ibi */
    short int          lineNumber;
    short int          ifDepth;
    struct cppIfStack *ifStack;
    struct javaStat   *javaStat;
    struct counters    counters;
} CachePoint;

typedef struct {
    bool       cachingActive; /* whether putting input to cache */
    int        cachePointIndex;
    CachePoint cachePoints[MAX_CACHE_POINTS];
    int        includeStackTop;
    int        includeStack[INCLUDE_STACK_CACHE_SIZE]; /* included files numbers */
    char      *lexemStreamEnd;                         /* first free in lexemStream */
    char       lexemStream[LEXEM_STREAM_CACHE_SIZE];   /* lexems buffer */
    char      *lexcc;                                  /* first not yet cached lexem */
    char      *currentLexemP;                          /* cc when input from cache */
    char      *endOfLexemStreamBuffer;                 /* end of cc, when input ... */
} Cache;

extern Cache cache;

extern void setupCaching(void);
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

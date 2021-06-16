#ifndef CACHING_H_INCLUDED
#define CACHING_H_INCLUDED

#include "globals.h"              /* For Counters */
#include "memory.h"


typedef struct cachePoint {
    struct topBlock			*topBlock;
    struct topBlock         topBlockContent;
    int                     ppmMemoryIndex;
    int                     cxMemoryIndex;
    int                     mbMemoryIndex;
    char					*lbcc;		/* caching lbcc */
    short int				ibi;		/* caching ibi */
    short int               lineNumber;
    short int               ifDepth;
    struct cppIfStack       *ifStack;
    struct javaStat			*javaCached;
    struct counters			counters;
} CachePoint;

typedef struct caching {
    bool activeCache;		/* whether putting input to cache */
    int					cpi;
    struct cachePoint	cp[MAX_CACHE_POINTS];
    int					ibi;
    int					ib[INCLUDE_CACHE_SIZE];	/* included files numbers */
    char				*lbcc;					/* first free of lb */
    char				lb[LEX_BUF_CACHE_SIZE];	/* lexems buffer */
    char				*lexcc;					/* first not yet cached lexem */
    char				*cc;					/* cc when input from cache */
    char				*cfin;					/* end of cc, when input ... */
} S_caching;


extern S_caching s_cache;


extern void setupCaching(void);
extern void initCaching(void);
extern void recoverCachePoint(int i, char *readUntil, int activeCaching);
extern void recoverFromCache(void);
extern void cacheInput(void);
extern void cacheInclude(int fileNum);
extern void placeCachePoint(bool inputCache);
extern void recoverCachePointZero(void);
extern void recoverMemoriesAfterOverflow(char *cxMemFreeBase);
extern bool checkFileModifiedTime(int fileIndex);

#endif

#ifndef _CACHING_H_
#define _CACHING_H_

#include "globals.h"              /* For Counters */
#include "memory.h"


typedef struct cachePoint {
    struct topBlock			*topBlock;
    struct topBlock         starTopBlock;
    int                     ppmMemoryi;
    int                     cxMemoryi;
    int                     mbMemoryi;
    char					*lbcc;		/* caching lbcc */
    short int				ibi;		/* caching ibi */
    short int               lineNumber;
    short int               ifDeep;
    struct cppIfStack       *ifstack;
    struct javaStat			*javaCached;
    struct counters			counts;
} CachePoint;

typedef struct caching {
    char				activeCache;		/* whether putting input to cache */
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
extern void recoverCachePoint(int i, char *readedUntil, int activeCaching);
extern void recoverFromCache(void);
extern void cacheInput(void);
extern void cacheInclude(int fileNum);
extern void placeCachePoint(int inputCacheFlag);
extern void recoverCachePointZero(void);
extern void recoverMemoriesAfterOverflow(char *cxMemFreeBase);
extern int checkFileModifiedTime(int ii);

#endif

#ifndef _CACHING_H_
#define _CACHING_H_

#include "proto.h"

typedef struct caching {
    char				activeCache;		/* whether putting input to cache */
    int					cpi;
    struct cachePoint	cp[MAX_CACHE_POINTS];
    int					ibi;
    int					ib[INCLUDE_CACHE_SIZE];	/* included files numbers */
    char				*lbcc;					/* first free of lb */
    char				lb[LEX_BUF_CACHE_SIZE];	/* lexems buffer */
    char				*lexcc;					/* first not yeat cached lexem */
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

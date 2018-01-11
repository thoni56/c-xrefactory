#ifndef _CACHING_H_
#define _CACHING_H_

extern int testFileModifTime(int ii);
extern void initCaching();
extern void recoverCachePoint(int i, char *readedUntil, int activeCaching);
extern void recoverFromCache();
extern void cacheInput();
extern void cacheInclude(int fileNum);
extern void poseCachePoint(int inputCacheFlag);
extern void recoverCachePointZero();
extern void recoverMemoriesAfterOverflow(char *cxMemFreeBase);

#endif

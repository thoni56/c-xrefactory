#ifndef CACHING_H
#define CACHING_H

extern int testFileModifTime(int ii);
extern void initCaching();
extern void recoverCachePoint(int i, char *readedUntil, int activeCaching);
extern void recoverFromCache();
extern void cacheInput();
extern void cacheInclude(int fileNum);
extern void poseCachePoint(int inputCacheFlag);
extern void recoverCxMemory(char *cxMemFreeBase);
extern void recoverCachePointZero();
extern void recoverMemoriesAfterOverflow(char *cxMemFreeBase);

#endif

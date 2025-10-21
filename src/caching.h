#ifndef CACHING_H_INCLUDED
#define CACHING_H_INCLUDED


/**
 * Recover memory state after a memory overflow occurred
 * @param cxMemFreeBase Base pointer for cross-reference memory recovery
 */
extern void recoverMemoriesAfterOverflow(char *cxMemFreeBase);

#endif

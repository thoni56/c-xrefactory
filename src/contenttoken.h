#ifndef CONTENTTOKEN_H_INCLUDED
#define CONTENTTOKEN_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "timestamp.h"

/* What stands for the content the server would read now, without reading
 * it: the modification time and the size from one stat (ADR-0036). A token
 * is only compared for being the same, never ordered, so do not reach into
 * it */
typedef struct {
    uint32_t hash;
} ContentToken;

extern ContentToken makeContentToken(FileTimestamp modificationTime, long size);
extern ContentToken missingContentToken(void);
extern ContentToken contentTokenOf(char *path);
extern bool contentTokensAreSame(ContentToken a, ContentToken b);
extern bool contentTokenIsMissing(ContentToken token);

/* For the snapshot, which keeps a token as a non-negative int */
extern int contentTokenAsInt(ContentToken token);
extern ContentToken contentTokenFromInt(int value);

#endif

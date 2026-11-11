#ifndef CONTENTTOKEN_H_INCLUDED
#define CONTENTTOKEN_H_INCLUDED

#include <stdbool.h>

#include "timestamp.h"

/* What stands for the content the server would read now, without reading
 * it: the modification time and the size from one stat (ADR-0036). A token
 * is only compared for being the same, never ordered, so do not reach into
 * the fields */
typedef struct {
    bool          exists;
    FileTimestamp modificationTime;
    long          size;
} ContentToken;

extern ContentToken makeContentToken(FileTimestamp modificationTime, long size);
extern ContentToken missingContentToken(void);
extern ContentToken contentTokenOf(char *path);
extern bool contentTokensAreSame(ContentToken a, ContentToken b);
extern bool contentTokenIsMissing(ContentToken token);

#endif

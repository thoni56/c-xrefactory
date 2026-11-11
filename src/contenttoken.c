#include "contenttoken.h"

#include "fileio.h"


ContentToken makeContentToken(FileTimestamp modificationTime, long size) {
    return (ContentToken){.exists = true, .modificationTime = modificationTime, .size = size};
}

/* Having none is a token of its own, the same as itself (ADR-0036) */
ContentToken missingContentToken(void) {
    return (ContentToken){.exists = false};
}

ContentToken contentTokenOf(char *path) {
    struct stat st;
    if (fileStatus(path, &st) != 0)
        return missingContentToken();
#ifdef __APPLE__
    return makeContentToken(st.st_mtimespec, st.st_size);
#else
    return makeContentToken(st.st_mtim, st.st_size);
#endif
}

bool contentTokensAreSame(ContentToken a, ContentToken b) {
    if (!a.exists || !b.exists)
        return a.exists == b.exists;
    return fileTimestampsEqual(a.modificationTime, b.modificationTime) && a.size == b.size;
}

bool contentTokenIsMissing(ContentToken token) {
    return !token.exists;
}

#include "contenttoken.h"

#include "fileio.h"


/* Missing is zero, so a token that was never recorded is missing */
#define MISSING_HASH 0

#define FNV_OFFSET_BASIS 2166136261u
#define FNV_PRIME        16777619u

/* FNV-1a, a byte at a time, least significant first, so that the hash is the
 * same on every platform and in every run, as it is kept in the snapshot */
static uint32_t hashInto(uint32_t hash, int64_t value) {
    for (int i = 0; i < 8; i++) {
        hash ^= (uint32_t)((uint64_t)value >> (8 * i)) & 0xff;
        hash *= FNV_PRIME;
    }
    return hash;
}

/* Unlike a hash table's, two tokens with the same hash are never compared
 * further, so a collision is a change not seen, about once in 2^31. 31 bits,
 * since the snapshot reads only non-negative ints */
ContentToken makeContentToken(FileTimestamp modificationTime, long size) {
    uint32_t hash = FNV_OFFSET_BASIS;
    hash = hashInto(hash, fileTimestampSeconds(modificationTime));
    hash = hashInto(hash, fileTimestampNanoseconds(modificationTime));
    hash = hashInto(hash, size) & 0x7fffffff;
    if (hash == MISSING_HASH)
        hash = 1;
    return (ContentToken){.hash = hash};
}

/* Having none is a token of its own, the same as itself (ADR-0036) */
ContentToken missingContentToken(void) {
    return (ContentToken){.hash = MISSING_HASH};
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
    return a.hash == b.hash;
}

bool contentTokenIsMissing(ContentToken token) {
    return token.hash == MISSING_HASH;
}

int contentTokenAsInt(ContentToken token) {
    return (int)token.hash;
}

ContentToken contentTokenFromInt(int value) {
    return (ContentToken){.hash = (uint32_t)value};
}

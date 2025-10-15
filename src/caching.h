#ifndef CACHING_H_INCLUDED
#define CACHING_H_INCLUDED

#include "constants.h"
#include "counters.h"
#include "input.h"
#include "stackmemory.h"

/**
 * CachePoint - Represents a complete parser state snapshot
 * 
 * This structure captures all the necessary state information to restore
 * the parser to a specific point during incremental parsing. Each cache
 * point allows the parser to resume from a known-good state when files
 * haven't changed since the last parse.
 */
typedef struct {
    struct codeBlock  *topBlock;            /* Current parsing block context */
    struct codeBlock   topBlockContent;     /* Content of the top block */
    int                ppmMemoryIndex;      /* Preprocessor memory position */
    int                cxMemoryIndex;       /* Cross-reference memory position */
    int                macroBodyMemoryIndex;/* Macro expansion memory position */
    char              *nextLexemP;          /* Next lexem to be parsed */
    short int          includeStackTop;     /* Include nesting depth */
    short int          lineNumber;          /* Current line number */
    short int          ifDepth;             /* Preprocessor #if nesting depth */
    struct cppIfStack *ifStack;             /* Preprocessor conditional stack */
    Counters           counters;            /* Various parsing counters */
} CachePoint;

/**
 * Cache - Main caching system state
 * 
 * This structure manages the incremental parsing cache, storing tokenized
 * input and parser state snapshots to enable fast re-parsing of unchanged
 * code sections.
 */
typedef struct {
    bool       active;                               /* Whether caching is currently enabled */
    CachePoint points[MAX_CACHE_POINTS];             /* Array of parser state snapshots */
    int        index;                                /* Next available cache point index */
    int        includeStack[INCLUDE_STACK_CACHE_SIZE]; /* File numbers of included files */
    int        includeStackTop;                      /* Current include stack depth */
    char       lexemStream[LEXEM_STREAM_CACHE_SIZE]; /* Tokenized input buffer */
    char      *free;                                 /* Next free position in lexemStream */
    char      *nextToCache;                          /* Next input position to cache */
    char      *read;                                 /* Read position when reading from cache */
    char      *write;                                /* Write position in cache buffer */
} Cache;

extern Cache cache;

/* ========== Cache Lifecycle Management ========== */

/** Initialize the caching system for a new parsing session */
extern void initCaching(void);

/** Enable input caching (tokens will be stored in cache buffer) */
extern void activateCaching(void);

/** Disable input caching (no new tokens stored) */
extern void deactivateCaching(void);

/** Check if caching is currently active */
extern bool cachingIsActive(void);

/* ========== Cache Point Management ========== */

/**
 * Create a new cache point storing current parser state
 * @param inputCaching Whether to cache current input before creating point
 */
extern void placeCachePoint(bool inputCaching);

/**
 * Restore parser state from a specific cache point
 * @param cachePointIndex Index of cache point to restore from
 * @param readUntil Position to read until in cached input
 * @param cachingActive Whether to activate caching after recovery
 */
extern void recoverCachePoint(int cachePointIndex, char *readUntil, bool cachingActive);

/** Restore parser state to the initial cache point (index 0) */
extern void recoverCachePointZero(void);

/* ========== Cache Input Management ========== */

/**
 * Store lexical input in the cache buffer
 * @param input The lexical input to cache
 */
extern void cacheInput(LexInput *input);

/**
 * Record an included file in the cache for modification tracking
 * @param fileNum File number of the included file
 */
extern void cacheInclude(int fileNum);

/* ========== Cache Recovery and Validation ========== */

/**
 * Attempt to recover parsing state from the most recent valid cache point
 * This validates cached input against current input and included file timestamps
 */
extern void recoverFromCache(void);

/**
 * Recover memory state after a memory overflow occurred
 * @param cxMemFreeBase Base pointer for cross-reference memory recovery
 */
extern void recoverMemoriesAfterOverflow(char *cxMemFreeBase);

/**
 * Check if a file has been modified since last cache
 * @param fileIndex Index of file to check
 * @return true if file is up-to-date, false if modified
 */
extern bool checkFileModifiedTime(int fileIndex);

#endif

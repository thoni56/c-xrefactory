# Remove lexem stream caching mechanism

Date: 2025-10-23

## Status

Implemented (completed 2025-11-26)

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

c-xrefactory used a complex lexem stream caching mechanism (implemented in `src/caching.c`, ~300 lines) that cached tokenized header files across multiple source files within a single parsing run. When parsing projects with many files (e.g., ffmpeg with 2,616 .c files), common headers like `<stdio.h>` would be tokenized once and reused for subsequent files.

The caching system included:
- Cache points storing complete parser state snapshots
- Include stack tracking (up to 1000 nested includes)
- Modification time validation
- Byte-for-byte token comparison for cache validation
- Memory recovery mechanisms tied to cache state

However, this caching system created significant architectural problems:

1. **Deep coupling**: Caching logic was intertwined with parsing, memory management, and file handling
2. **Memory complexity**: Cache points controlled memory recovery, making it difficult to understand memory lifecycle
3. **Macro spillover bugs**: Header guards and file-local macros were bleeding between files (see commit `950aacac`)
4. **Refactoring barrier**: The complexity prevented architectural improvements and modernization efforts
5. **Understanding difficulty**: After 5+ years of maintenance, even the maintainer struggled to fully understand the cache interactions

## Decision Outcome

**Remove the entire lexem stream caching mechanism**, accepting the performance trade-off to enable architectural refactoring.

Implementation commits (Oct-Nov 2025):
- `c19ff7b7` - Final removal of caching traces
- `f17b0864` - Remove `recoverCachePointZero()`
- `c6726579` - Remove `cacheInclude()`
- Earlier commits removed other cache functions incrementally

## Consequences and Risks

### Immediate Consequences

**Performance regression for multi-file parsing**:

Observed impact varies significantly by project size and header sharing patterns:

| Project | Before | After | Slowdown | Characteristics |
|---------|--------|-------|----------|-----------------|
| c-xrefactory self-build | 4.0s | 8.2s | 2.0× | ~100 files, moderate header sharing |
| ffmpeg + systemd tests | ~5min | ~32min | 6.4× | 3,258 files, heavy header sharing |

**Root cause**: Each source file now re-tokenizes all included headers independently. Without caching:
- c-xrefactory: ~100 files × ~15 headers = ~1,500 redundant parse operations (vs. ~115 with cache)
- ffmpeg: 2,616 files × ~30-50 headers = ~78,000-130,000 redundant operations (vs. ~2,700 with cache)

**Key insight**: The slowdown factor scales with project size. Most users working on typical C projects (~100-200 files) will experience 2-3× slowdown, not 6×. The extreme slowdown only appears in massive projects like ffmpeg.

**Memory management improvements**:
- Clearer memory lifecycle - no cache-driven recovery
- Explicit checkpoint/restore mechanism (`saveMemoryCheckPoint`, `restoreMemoryCheckPoint`)
- Fixed macro spillover between files
- Had to double `SIZE_ppmMemory` from 15MB to 30MB (commit `91931bc4`)

### Long-term Benefits

**Enables refactoring**:
- Can now restructure parsing without cache dependencies
- Memory management is explicit and understandable
- Separation of concerns becomes possible
- Foundation for on-demand parsing architecture

**Simplified codebase**:
- ~300 lines of complex code removed
- Fewer global state dependencies
- Easier for new contributors to understand

### Accepted Risks

**Performance is currently worse**: For large multi-file batch operations, parsing is 6× slower. This is acceptable because:

1. Most users don't work on 2,600-file projects
2. ffmpeg/systemd tests moved to "slow" test suite
3. Future on-demand architecture will eliminate most batch `-create` operations
4. For huge projects requiring batch parsing, future optimizations will be implemented on the cleaner architecture

**Future optimization path**: See ADR-0014 for on-demand parsing and potential simpler caching strategies.

## Considered Options

### Option 1: Keep caching and work around it

**Approach**: Maintain the existing cache while trying to refactor around it.

**Pros**:
- No performance regression
- Existing optimizations preserved

**Cons**:
- Continued maintenance burden
- Blocks architectural improvements
- Macro spillover bugs persist
- New contributors can't understand the code
- Prevents on-demand parsing implementation

**Verdict**: Rejected - the complexity cost is too high

### Option 2: Refactor caching first, then improve architecture

**Approach**: Clean up the caching system to make it more modular, then proceed with other refactorings.

**Pros**:
- Potentially preserves performance
- Incremental improvement

**Cons**:
- Would take months to properly refactor the cache
- Might still block architectural changes
- Unclear if clean caching is even compatible with on-demand parsing
- High risk of introducing subtle bugs in complex cache logic

**Verdict**: Rejected - too risky and time-consuming

### Option 3: Remove caching now, optimize later (CHOSEN)

**Approach**: Remove caching to unblock refactoring, accept temporary performance regression, re-introduce simpler caching after architecture is clean.

**Pros**:
- Immediate unblocking of architectural work
- Clearer code enables confident refactoring
- Future caching can be designed properly from scratch
- Performance regression is localized to large batch operations
- On-demand parsing will make batch operations rare

**Cons**:
- 6× slowdown for large projects
- Longer CI times temporarily
- Some users might notice slower initial indexing

**Verdict**: **ACCEPTED** - "Make it work, make it clear, then make it fast"

## Future Work

Potential optimizations once architecture is clear (see ADR-0014):

1. **Incremental `-create`**: Skip parsing files unchanged since last run
2. **Include graph caching**: Cache dependency relationships separately from parse results
3. **Selective header caching**: Cache only system headers, not project headers
4. **Parallel parsing**: Parse multiple files simultaneously

Any future caching must be:
- **Optional**: System works without it (cache is pure optimization)
- **Transparent**: Caching logic clearly separated from parsing logic
- **Simple**: No parser state snapshots or complex recovery mechanisms

# Lightweight file structure scanning replaces full-project reparse

Date: 2026-02-13

## Status

Draft

## Deciders

Thomas Nilefalk, Claude (AI pair programmer)

## Problem Statement and Context

The legacy c-xrefactory architecture relies on two expensive full-project operations:

1. **`-create` / `-update`**: Parses ALL project files, builds complete symbol database on disk. Required before any operations work. Conflates two concerns: discovering project structure (which files exist, who includes whom) and building symbol references (full parsing).

2. **`callXref()` before refactoring**: The refactoring subsystem re-enters the server to force a full reparse of affected files before performing operations like rename. This ensures reference data is fresh but does far more work than necessary.

Both operations treat the disk database as truth. Between runs, the data goes stale. The system has no mechanism to incrementally keep it fresh.

### What we learned from ADR 20

While implementing stale-header handling for ADR 20 (separate sync from dispatch), we discovered that:

- **Include structure** (which files include which) and **symbol references** (what symbols are defined/used where) are two separate layers of information with very different costs.
- Include structure can be discovered cheaply by scanning files for `#include` directives — no full parsing needed.
- The existing `TypeCppInclude` references in the reference table (link name `"%%%i"`) already represent include structure. Full parsing populates these as a side effect. Lightweight scanning can populate the same references directly.
- The existing `makeIncludeClosureOfFilesToUpdate()` in xref.c already queries these references. It doesn't care how they were populated.
- Mtime freshness checks can determine which files need rescanning or reparsing, avoiding redundant work.

### The core insight

The expensive part of `-create` is full parsing. But most of what the system needs to *function* — knowing which files exist and who includes whom — can be obtained by lightweight text scanning. Full parsing is only needed for files the user actually interacts with, and only when their content has changed.

## Decision Outcome

Replace the full-project reparse with a two-layer approach:

### Layer 1: Lightweight file structure scan

Discovers project structure without full parsing:

1. **CU discovery**: Glob the project directory for source files (`.c`, `.y`, etc. per `-csuffixes`)
2. **Include extraction**: For each CU, scan text for `#include` directives, resolve paths, populate `TypeCppInclude` references in the reference table
3. **File table population**: All discovered CUs and their included headers get file table entries

This provides the same structural information that `-create` provides, at a fraction of the cost. No symbol references are built — only include relationships.

### Layer 2: Incremental on-demand parsing

When symbol references are needed (navigation, refactoring):

1. **Mtime check**: Compare file's `lastParsedMtime` against current mtime
2. **Skip fresh files**: Files unchanged since last parse already have correct references
3. **Reparse stale files only**: Full parsing of changed files updates both symbol references and include structure

### Where this runs

The same scan/freshness/reparse machinery serves multiple trigger points:

- **Server entry-point** (ADR 20): Before dispatching any operation, check for stale files. Pass 1 reparses stale CUs (which also refreshes their include edges). Pass 2 finds CUs that include stale headers and reparses them.
- **Before refactoring**: Instead of `callXref()` forcing a full reparse, do a lightweight scan + incremental reparse of stale files. The include closure (needed for rename scope) comes from `TypeCppInclude` references.
- **Explicit rescan**: A "rescan" operation (replacing `-create`) runs Layer 1 to discover new files and updated include structure. Full parsing only happens on subsequent operations that need it.

### Pass ordering in the entry-point

Pass 1 (reparse stale CUs) must run before Pass 2 (handle stale headers). This is critical because:

- Pass 1 reparses stale CUs, which updates their `TypeCppInclude` refs as a side effect
- By the time Pass 2 queries "who includes this stale header?", the include refs are fresh
- A user can't create a new include edge (`#include "new.h"`) without editing the includer, so the includer is always stale when the edge is new — Pass 1 catches it

### Design boundary: known files only

The system operates on files it knows about (present in the file table). Files can become known through:

- **Disk db** (today): `loadFileNumbersFromStore()` seeds the file table on cold start
- **Editor interaction**: Client sends a file as preload — it enters the file table
- **Explicit rescan**: Lightweight scan discovers files from the project directory
- **Parsing**: When a CU is parsed, its included headers become known

Files that exist on disk but are unknown to the system are invisible. This is acceptable: they can't be in any browsing stack or reference list. An explicit "rescan" makes them known when the project structure changes.

## Consequences and Risks

### Benefits

- **Eliminates mandatory full reparse**: Structure discovery is cheap; full parsing happens incrementally
- **Replaces `callXref()`**: Refactoring gets fresh data through the same incremental mechanism as navigation
- **No new data structures**: Uses existing `TypeCppInclude` references and file table
- **Same machinery everywhere**: Entry-point sync, refactoring freshness, and explicit rescan all use the same scan/mtime/reparse pipeline
- **Catches new files**: Lightweight scan discovers files added to the project since last scan
- **Convergence point**: Works in both the current hybrid world (disk db provides baseline) and the future memory-only world (lightweight scan provides baseline)

### Risks

- **Basename ambiguity in include resolution**: Lightweight scanning resolves `#include` paths heuristically (dirname + `-I` directories). This may occasionally miss or over-match compared to full preprocessing. Full parsing corrects any errors when it runs.
- **Conditional includes**: `#ifdef`-guarded includes are visible to the text scanner but may not be active. This causes over-counting (scanning CUs that don't actually include the header). Harmless — full parsing resolves it correctly.
- **Large projects**: Scanning thousands of files for `#include` directives has I/O cost. Mtime checks can skip unchanged files on subsequent scans. In practice this is much faster than full parsing.

## Considered Options

### Option 1: Keep relying on disk db for include structure

Use `ensureReferencesAreLoadedFor(LINK_NAME_INCLUDE_REFS)` to load include refs from disk db, as `makeIncludeClosureOfFilesToUpdate()` already does.

**Pros**: No new code needed for include queries. Already works.

**Cons**: Depends on disk db being populated (requires prior `-create`). Can't discover new files. Include structure goes stale between `-create` runs.

**Verdict**: Works today but doesn't serve the on-demand future (ADR 14).

### Option 2: Ad-hoc text scanning on every request

Scan files for `#include` directives each time a stale header is detected. Don't persist results.

**Pros**: No disk db dependency. Always sees current file content.

**Cons**: Redundant work — rescans the same files repeatedly. Doesn't populate the reference table, so existing include-closure queries can't use the results.

**Verdict**:  There is currently an prototype implementation (the forward-walking DFS in server.c) of the right idea, but it should populate the reference table instead of being a parallel mechanism.

### Option 3: Lightweight scan populating existing reference table (CHOSEN)

Scan files for `#include` directives and populate `TypeCppInclude` references in the reference table. Use mtime to avoid redundant rescanning. Existing include-closure queries work unchanged.

**Pros**: Reuses existing infrastructure. Single source of truth for include structure. Works with and without disk db. Incremental via mtime.

**Cons**: Requires implementing include path resolution in the scanner (already done in the ad-hoc prototype).

**Verdict**: **ACCEPTED** — cleanest design, reuses existing infrastructure, serves both current and future architectures.

## Related Decisions

- **ADR 14**: Adopt on-demand parsing architecture — this ADR provides the mechanism for on-demand include-structure discovery
- **ADR 20**: Separate buffer sync from operation dispatch — the entry-point reparse loop is the first consumer of this scanning
- **ADR 13**: Limited extern detection — include closure is sufficient for rename scope, no need to scan all project files for extern declarations
- **ADR 21**: Single-project server — simplifies file discovery to one project directory

# Adopt on-demand parsing architecture

Date: 2025-12-04

## Status

Proposed

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

### Current Architecture: Batch Mode

c-xrefactory currently operates primarily in **batch mode**:

```
Step 1: User runs: c-xref -create
  → Parse ALL files in project (e.g., 2,616 files in ffmpeg)
  → Build complete symbol database (.cx files)
  → Takes 5-30 minutes depending on project size

Step 2: User performs operations: goto-definition, rename, etc.
  → Read from pre-built database
  → Instant response
```

This architecture has fundamental problems:

1. **Cold start problem**: Must run `-create` before any operations work
2. **Stale data**: Database gets out of sync with edited files
3. **Wasted work**: Parses files that may never be queried
4. **Poor LSP fit**: LSP clients expect zero-config, instant startup
5. **Legacy mindset**: Reflects 1990s batch cross-reference tools, not modern IDEs

### Modern Tool Expectations

Modern IDE tools (clangd, rust-analyzer, typescript-language-server) work on-demand:

- **Zero configuration**: Open project, start working immediately
- **Incremental parsing**: Parse files as needed, when needed
- **Modification tracking**: Re-parse only changed files
- **Fast response**: <100ms for common operations (goto-definition)
- **Memory efficiency**: Don't hold full project AST in memory

### The Vision

```
User opens file in editor (Emacs/VS Code/Vim)
  → LSP server starts (no -create needed)
  → Goto-definition on 'foo'
    → Parse current file if not already cached
    → Parse headers it includes
    → Find definition (0.1s)
  → Database builds incrementally, transparently
  → User never thinks about "creating references"
```

## Decision Outcome

**Adopt an on-demand parsing architecture** where c-xrefactory:

1. **Eliminates mandatory `-create`**: Operations work immediately on any project
2. **Parses incrementally**: Only parse files needed for the requested operation
3. **Uses `.cx` files as cache**: Persistent storage is an optimization, not a requirement
4. **Tracks dependencies**: Existing include tracking determines parsing closure
5. **Checks modification times**: Re-parse files changed since last analysis
6. **Builds database automatically**: Symbol database updates transparently during operations

The existing dependency tracking and modification checking infrastructure (already implemented) becomes the foundation for on-demand behavior.

### Core Principle

**The `.cx` database is a persistent cache of analysis results, not a fundamental requirement.**

c-xrefactory should always provide correct, up-to-date information by intelligently determining what needs re-parsing.

## Architecture

### On-Demand Operation Flow

```c
Symbol* lookupSymbol(const char* name, Position pos) {
    // 1. Check if cached information is current
    FileItem *file = getFileItem(pos.file);
    if (file->symbolInfo && !fileModifiedSince(file, file->lastParsed)) {
        Symbol *symbol = getCachedSymbol(name, pos);
        if (symbol) return symbol;
    }

    // 2. Parse minimal dependency closure
    FileList *closure = computeIncludeClosure(pos.file);
    for (FileItem *f : closure) {
        if (fileModifiedSince(f, f->lastParsed)) {
            parseFileAndUpdateCache(f);
        }
    }

    // 3. Return result (now guaranteed current)
    return getSymbol(name, pos);
}
```

### Refactoring Operation Flow

```c
void renameSymbol(const char* oldName, const char* newName, Position pos) {
    // 1. Parse current file if needed
    ensureFileParsed(pos.file);

    // 2. Find symbol definition
    Symbol *symbol = findSymbol(oldName, pos);

    // 3. Compute which files might reference it
    FileList *potentialReferences;
    if (symbolIsGlobal(symbol)) {
        // Global: find all files including the defining header
        potentialReferences = findFilesIncludingHeader(symbol->definedIn);
    } else {
        // Local: just current file
        potentialReferences = { pos.file };
    }

    // 4. Parse relevant files
    for (FileItem *f : potentialReferences) {
        ensureFileParsed(f);
    }

    // 5. Perform rename
    performRename(oldName, newName, potentialReferences);
}
```

### Dependency Closure Computation

c-xrefactory already has this infrastructure (reuse it):

```c
FileList* computeIncludeClosure(int fileNumber) {
    // Uses existing functions:
    // - getFileItem()
    // - checkFileModifiedTime()
    // - Existing include tracking

    FileList *result = createFileList();
    addFileToList(result, fileNumber);

    // Recursively add all #include'd files
    expandIncludeTransitiveClosure(result, fileNumber);

    return result;
}
```

## Consequences and Risks

### Benefits

**User Experience**:
- **Zero cold start**: Open file, start working immediately
- **Always up-to-date**: No manual `-update` needed
- **IDE-like feel**: Matches expectations from modern tools
- **No configuration**: Works on any C project automatically

**Architecture**:
- **Unified code path**: Same logic for Emacs and LSP
- **Simpler mental model**: No artificial "mode" distinctions
- **Enables long-running servers**: Can reset memory between files
- **Foundation for incremental parsing**: Natural fit for `textDocument/didChange`

**Performance** (for most users):
- **Typical projects** (50-200 files): Parse current file + 10-30 headers = 0.1-1s
- **Much faster than full `-create`**: Only parse what's needed
- **Subsequent operations**: Cache hits make operations instant

### Risks and Mitigations

**Risk 1: Slower first operation on huge projects**

*Scenario*: Open random file in ffmpeg (2,616 files), first goto-definition takes 5 seconds.

*Mitigation*:
- Most projects are <500 files (imperceptible latency)
- Background indexing on project open (future enhancement)
- Users of huge projects can still run optional `-create` for full indexing
- After first operation, cache makes subsequent operations fast

**Risk 2: Incomplete refactorings in edge cases**

*Scenario*: Archaic `extern` declarations in `.c` files without headers (see ADR-0013).

*Mitigation*:
- Documented limitation for non-modern C
- Workaround: Run full `-create` for legacy codebases
- Most modern C uses headers (ADR-0013 analysis: <5% of projects affected)

**Risk 3: Complex include graphs cause over-parsing**

*Scenario*: File includes 50 headers, each includes 50 more = parse 2,500 files.

*Reality check*:
- c-xrefactory already parses transitive includes (current behavior)
- Include guards prevent re-parsing same header
- Closures are typically 20-100 files, not thousands
- Clangd has same behavior - proven acceptable

**Risk 4: `.cx` file format needs changes**

*Scenario*: On-demand updates require different storage structure.

*Mitigation*:
- `.cx` format is already file-granular (one entry per file)
- Just add `lastParsed` timestamp if not present
- Auto-migration on version change
- Backward compatibility maintained

### Performance Expectations

| Project Size | First Operation | Subsequent Operations | Full `-create` (optional) |
|--------------|-----------------|----------------------|---------------------------|
| Small (50 files) | <0.1s | <0.01s | 2s |
| Medium (200 files) | 0.5s | <0.01s | 10s |
| Large (1000 files) | 2s | <0.01s | 60s |
| Huge (2600 files) | 5s | <0.01s | 300s |

For comparison, clangd has similar initial latency on large projects.

## Considered Options

### Option 1: Keep batch mode, add on-demand as separate mode

**Approach**: Maintain current `-create` workflow, add new `-lsp` mode with different behavior.

**Pros**:
- Backward compatible
- Lower risk
- Existing users unaffected

**Cons**:
- **Doubles complexity**: Two code paths to maintain
- **Mode confusion**: Users don't know which mode to use
- **Technical debt**: Batch mode blocks architectural improvements
- **Split development**: Features must be implemented twice

**Verdict**: Rejected - complexity not worth it

### Option 2: Make `-create` incremental only

**Approach**: Make `-create` skip unchanged files, but still require explicit invocation.

**Pros**:
- Smaller change
- Backward compatible command interface
- Faster iterative `-create` runs

**Cons**:
- **Still requires manual invocation**: User must remember to run it
- **Cold start problem persists**: First time still slow
- **Doesn't enable LSP**: LSP clients expect zero-config
- **Doesn't solve memory accumulation**: Still batch mindset

**Verdict**: Rejected - doesn't achieve LSP goals

### Option 3: Full on-demand, deprecate batch mode (CHOSEN)

**Approach**: Make all operations on-demand by default, keep `-create` as optional optimization.

**Pros**:
- **True zero-config**: Works immediately
- **Single code path**: Simpler to maintain
- **LSP-native**: Perfect fit for LSP architecture
- **Forces architectural cleanup**: Can't maintain batch-mode hacks
- **Modern UX**: Matches user expectations

**Cons**:
- **First operation latency** on huge projects
- **Requires architectural changes** to memory management
- **Risk of incomplete refactorings** in edge cases (ADR-0013)

**Verdict**: **ACCEPTED** - Aligns with long-term vision, enables LSP

### Option 4: Background indexing hybrid

**Approach**: On-demand by default, but automatically start background full index on project open.

**Pros**:
- Best of both worlds: instant startup + complete index eventually
- Transparent to user
- Full refactoring completeness after background index finishes

**Cons**:
- More complex implementation
- CPU/memory usage during background indexing
- Need cancellation if user closes project
- Need prioritization if user operation conflicts with background work

**Verdict**: Excellent **future enhancement**, but not required initially. Start with pure on-demand (Option 3), add background indexing later if needed.

## Implementation Plan

### Phase 1: Incremental `-create` (1-2 weeks)

Make existing `-create` skip files unchanged since last parse:

```c
for (FileItem *file : project) {
    if (file->lastParsed < file->lastModified) {
        parseFile(file);
    }
}
```

**Benefit**: Faster iterative development, prepares infrastructure for on-demand.

### Phase 2: On-Demand Goto-Definition (3-4 weeks)

Implement first on-demand operation as proof-of-concept:

```c
Position gotoDefinition(const char *symbol, Position pos) {
    ensureFileParsed(pos.file);
    FileList *closure = computeIncludeClosure(pos.file);
    for (FileItem *f : closure) {
        ensureFileParsed(f);
    }
    return findDefinition(symbol, pos);
}
```

**Benefit**: Validates architecture, provides immediate LSP value.

### Phase 3: On-Demand Refactorings (6-8 weeks)

Extend to rename, extract-function, parameter refactorings:

- Compute dependency closures based on symbol scope
- Parse only affected files
- Perform refactoring
- Update `.cx` cache

**Benefit**: Full feature parity with batch mode, but on-demand.

### Phase 4: Deprecate Mandatory `-create` (2 weeks)

- Make `-create` optional (auto-triggers on first operation if needed)
- Update documentation to reflect new workflow
- Add warnings if users run `-create` unnecessarily
- Provide migration guide for existing workflows

**Benefit**: User-facing completion of on-demand transition.

### Phase 5: Memory Management for Long-Running Servers (8-12 weeks)

Enable per-file stackMemory reset (see roadmap §3.5):

- Populate refTab from parsed file
- Reset stackMemory after refTab update
- Navigation uses refTab, not ephemeral symbol structures
- Enables `textDocument/didChange` incremental updates

**Benefit**: True LSP server capabilities without memory leaks.

## Success Criteria

- ✓ Goto-definition works without prior `-create` in <1s for typical projects
- ✓ Rename works correctly with automatic dependency tracking
- ✓ LSP server starts and responds immediately (no initialization delay)
- ✓ All 140+ tests pass with on-demand parsing
- ✓ Performance regression <5% vs. batch mode for already-cached operations
- ✓ `.cx` files update automatically, transparently
- ✓ Documentation reflects new zero-config workflow
- ✓ Emacs workflow unaffected (backward compatible)

## References

- ADR-0012: Remove lexem stream caching (enables this architecture)
- ADR-0013: Limited extern detection (accepted trade-off)
- Roadmap §3.3: Unified Symbol Database Architecture
- Roadmap §3.5: Long-term Vision - On-Demand Incremental Architecture for LSP

## Related Modern Tool Architectures

- **clangd**: Parses files on-demand using compilation database, incrementally updates
- **rust-analyzer**: Fully incremental, parses changed files only, uses query-based architecture
- **typescript-language-server**: Zero-config, parses on file open, uses in-memory AST cache
- **gopls**: Lazy loading, parses packages on-demand, incremental updates

c-xrefactory will follow similar patterns while leveraging its unique existing infrastructure (dependency tracking, file modification checks, .cx persistent cache).

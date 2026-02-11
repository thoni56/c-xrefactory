# Separate buffer synchronization from operation dispatch

Date: 2026-02-11

## Status

Draft

## Deciders

Thomas Nilefalk, Claude (AI pair programmer)

## Problem Statement and Context

The current server architecture interleaves buffer freshness checking with operation execution. When the editor client sends a request (e.g., BROWSE_NEXT), the operation handler discovers mid-execution that a file is stale, triggers a re-parse, then continues with refreshed data. This design has been a persistent source of bugs, particularly with the "auto-updating" browser stack where pointers become dangling after mid-operation refreshes.

The current flow:

```
Client request (with preloaded files for current buffer)
  -> determine operation
    -> during operation, discover staleness
      -> re-parse mid-operation
        -> continue operation with refreshed data
```

Additionally, the Emacs client currently only preloads the current buffer, meaning the server may have stale data for other modified-but-unsaved files.

### Relation to internal operations

This issue is compounded by the internal operation mechanism where `refactory.c` re-enters the server via `parseBufferUsingServer()` with command-line flags. These internal re-entries also face staleness issues. Separating sync from dispatch would simplify both external and internal operation handling.

## Decision Outcome

*To be determined.* The direction being explored is:

1. **Client sends all modified buffers** with every request, not just the current one. Buffers unchanged since the last request would still be sent but skipped by the server's existing timestamp-based freshness check (the cost is writing to temp files, not re-parsing).

2. **Server refreshes on entry**, before operation dispatch. A dedicated "sync phase" at the server entry point updates the in-memory reference database from all preloaded files.

3. **Operations assume fresh data.** Individual operation handlers (BROWSE_NEXT, etc.) no longer need staleness checks or mid-operation re-parsing.

Proposed flow:

```
Client request (with ALL modified buffers)
  -> sync phase: update reference database from modified files
    -> perform operation on guaranteed-fresh data
```

### Migration path

Each step is independently valuable and testable:

1. Fix Emacs client to always send all modified buffers (not just the current one)
2. Hoist the "refresh from preloads" phase to server entry, before operation dispatch
3. Remove staleness checks from individual operations
4. The LSP server gets the same "update phase then operate" structure naturally

### Alignment with LSP

This design aligns with LSP's model where `textDocument/didChange` notifications are separate from operation requests. The sync phase is conceptually equivalent to processing accumulated `didChange` events before handling the actual request.

## Consequences and Risks

**Benefits:**

- Eliminates the class of bugs caused by mid-operation refresh (dangling pointers, inconsistent state)
- Operation code becomes simpler - can trust its data
- Easier to test: sync and operations are independently testable
- Natural fit for LSP server architecture
- Internal operations (refactory.c re-entries) also benefit from guaranteed-fresh data

**Risks:**

- Performance: refreshing all modified files on every request may do more work than lazy per-file refresh. In practice, few files change between requests during normal editing.
- Radical rename scenario (many files modified) could be costly, but users typically save and test frequently during such operations.
- Requires changes to both client (Emacs) and server.

**Open questions:**

- What is the actual cost of the client writing all modified buffers to temporary files on every request?
- Could checksums or sequence numbers optimize the "send all modified buffers" step if disk I/O ever becomes a bottleneck?

## Considered Options

*To be explored further as this ADR matures from Draft to Proposed.*

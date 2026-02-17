# Separate buffer synchronization from operation dispatch

Date: 2026-02-11

## Status

Decided

## Deciders

Thomas Nilefalk, Claude (AI pair programmer)

## Problem Statement and Context

The server architecture interleaves buffer freshness checking with operation execution. When the editor client sends a request (e.g., BROWSE_NEXT), the operation handler discovers mid-execution that a file is stale, triggers a re-parse, then continues with refreshed data. This design has been a persistent source of bugs, particularly with the "auto-updating" browser stack where pointers become dangling after mid-operation refreshes.

The problematic flow:

```
Client request (with preloaded files for current buffer)
  -> determine operation
    -> during operation, discover staleness
      -> re-parse mid-operation
        -> continue operation with refreshed data
```

Additionally, the Emacs client originally only preloads the current buffer, meaning the server may have stale data for other modified-but-unsaved files.

### Relation to internal operations

This issue is compounded by the internal operation mechanism where `refactory.c` re-enters the server via `parseBufferUsingServer()` with command-line flags. These internal re-entries also face staleness issues. Separating sync from dispatch simplifies both external and internal operation handling.

## Decision Outcome

### 1. Client sends all modified buffers

The editor client sends all modified buffers with every request, not just the current one. Buffers unchanged since the last request are still sent but skipped by the server's existing timestamp-based freshness check (the cost is writing to temp files, not re-parsing).

### 2. Server refreshes on entry, before operation dispatch

A dedicated sync phase at the server entry point updates the in-memory reference database from all preloaded files. This is structured as two passes:

**Pass 1 — Reparse stale compilation units.** Iterate editor buffers. For any CU where `lastParsedMtime < buffer.modificationTime`, reparse it. This updates both symbol references and `TypeCppInclude` include-structure references for the CU.

**Pass 2 — Handle stale headers.** Iterate editor buffers again. For any stale file that is NOT a CU (i.e., a header), find compilation units that include it (via `TypeCppInclude` references) and reparse those CUs, which pulls in the fresh header content.

Pass ordering is critical: Pass 1 must complete before Pass 2 begins. When a user adds `#include "new.h"` to a CU, that CU is stale — Pass 1 reparses it, updating its include edges. By the time Pass 2 asks "who includes this stale header?", the include references are fresh. A new include edge cannot exist without the includer being stale, so Pass 1 always catches it first.

### 3. Operations assume fresh data

Individual operation handlers no longer need staleness checks or mid-operation re-parsing. The sync phase guarantees fresh data before dispatch.

### Resulting flow

```
Client request (with ALL modified buffers)
  -> sync phase:
    -> Pass 1: reparse stale CUs (updates symbols + include structure)
    -> Pass 2: for stale headers, find including CUs and reparse them
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
- Operation code becomes simpler — can trust its data
- Easier to test: sync and operations are independently testable
- Natural fit for LSP server architecture
- Internal operations (refactory.c re-entries) also benefit from guaranteed-fresh data
- Pass 1 refreshes include-structure as a side effect, keeping include references fresh for Pass 2 and for ADR 22 (lightweight file structure scanning)

**Risks:**

- Performance: refreshing all modified files on every request may do more work than lazy per-file refresh. In practice, few files change between requests during normal editing.
- Radical rename scenario (many files modified) could be costly, but users typically save and test frequently during such operations.
- Requires changes to both client (Emacs) and server.

## Considered Options

### Option 1: Keep lazy per-operation staleness checks

Each operation handler detects and handles staleness itself, as the legacy code does.

**Pros**: No architectural change needed. Each operation only refreshes what it touches.

**Cons**: Persistent source of bugs (dangling pointers after mid-operation refresh). Every operation must handle staleness correctly. Difficult to reason about system state.

**Verdict**: Rejected — the bug class is too costly.

### Option 2: Sync phase at server entry (CHOSEN)

Dedicated sync phase before dispatch. Two passes: stale CUs first, then stale headers.

**Pros**: Clean separation. Operations trust their data. Single place to handle freshness. Pass ordering naturally keeps include structure fresh.

**Cons**: May do slightly more work than lazy approach (refreshes files the operation might not touch). In practice negligible — few files change between requests.

**Verdict**: Accepted.

### Option 3: Event-driven sync (LSP-style didChange)

Process file changes as they arrive, not batched at request time.

**Pros**: Most responsive — data is always fresh. True LSP alignment.

**Cons**: Requires persistent server with event loop. More complex concurrency model. Not compatible with current request-response architecture.

**Verdict**: Future direction. The batch sync phase (Option 2) is a stepping stone — it can evolve into event-driven sync when the server architecture supports it.

## Related Decisions

- **ADR 14**: Adopt on-demand parsing architecture — sync phase is part of on-demand freshness
- **ADR 22**: Lightweight file structure scanning — Pass 2 depends on `TypeCppInclude` references that ADR 22's scanning populates

# Persistent content buffers with client-side change tracking

Date: 2026-02-22

## Status

Implemented (2026-02-23 in cdb9fa87)

## Deciders

Thomas Nilefalk, Claude (AI pair programmer)

## Problem Statement and Context

After implementing ADR 20 (separate sync from dispatch), the server correctly detects stale preloaded files and reparses them before dispatching operations. However, in practice **every navigation request triggers a reparse** of every modified buffer, even when the buffer content hasn't changed since the last request.

### Root cause

The Emacs client protocol works as follows per request:

1. Client writes each modified buffer to a **fresh tmp file** (incrementing counter)
2. Client sends `-preload real.c /tmp/cxref-42.tmp` to server
3. Server creates an EditorBuffer with the tmp file's filesystem mtime
4. Server compares `lastParsedMtime` with `buffer.modificationTime` — always different because the tmp file is new
5. Server reparses (expensive, unnecessary)
6. Server destroys all EditorBuffers at end of request
7. Client deletes tmp files

Steps 1 and 6 are the core problem: a new tmp file every time means a new mtime every time, and destroying buffers means the server has no memory of what it already parsed.

### Scale of impact

For a modified header file included by many CUs, this means a multi-second reparse on every PUSH, NEXT, or PREVIOUS — even when the user hasn't edited anything between operations. On the c-xrefactory sources this is noticeable; on a larger project it would be unusable.

### The EditorBuffer dual role

Investigation revealed that EditorBuffers serve two distinct roles:

1. **Editor content** — modified buffer content from the client, transported via tmp file (`preLoadedFromFile != NULL`)
2. **Disk file cache** — on-demand cache when the lexer reads a file during parsing (`preLoadedFromFile == NULL`)

The lexer always reads through `findOrCreateAndLoadEditorBufferForFile()`, which returns an existing buffer (preloaded or cached) or creates one from disk. This means preloaded editor content is transparently used during parsing — the parser doesn't know or care whether content came from the editor or disk.

The existing `bufferIsCloseable()` predicate already distinguishes these roles: it returns `false` for preloaded buffers (preserving editor content) and `true` for disk-read caches (allowing cleanup). But the server loop uses `closeAllEditorBuffers()` which ignores this distinction and destroys everything.

### LSP alignment

In LSP, document buffers follow a lifecycle:

| LSP event | Meaning |
|-----------|---------|
| `didOpen` | Editor opened a file — create buffer, parse |
| `didChange` | Editor modified content — update buffer, reparse |
| *(no event)* | Content unchanged — nothing happens |
| `didSave` / `didClose` | File saved/closed — can drop editor state |

The current Emacs protocol has no equivalent of "no event" — every request re-sends every modified buffer. Making buffers persistent and adding change detection creates this missing state.

## Decision Outcome

### Server: Keep editor content buffers alive across requests

Replace `closeAllEditorBuffers()` with `closeAllEditorBuffersIfClosable()` in the server request loop. This preserves preloaded editor content between requests while still freeing disk-read caches.

When `openEditorBufferFromPreload()` finds an existing buffer for a file:

- Compare the incoming preload file's mtime with the existing buffer's mtime
- **Same mtime**: Return existing buffer unchanged (content hasn't changed)
- **Different mtime**: Reload buffer from the new preload file (content changed)

This makes `fileNumberIsStale()` work correctly: same mtime means `lastParsedMtime >= buffer.modificationTime`, so no reparse.

### Client: Reuse tmp files for unchanged content

The Emacs client tracks per-buffer whether content has changed since the last preload:

- Assign each modified buffer a **persistent tmp file path** (buffer-local variable)
- Track `buffer-modified-tick` at last write (buffer-local variable)
- On each request:
  - Buffer modified AND tick changed since last write → rewrite tmp file, send preload
  - Buffer modified but tick unchanged → send preload with **existing tmp file** (don't rewrite)
  - Buffer not modified → don't send preload (existing behavior)
- Tmp file lifecycle: delete on `save-to-disk` or `kill-buffer`, not after each request

Since the tmp file isn't rewritten, its filesystem mtime stays the same. The server sees the same mtime as last time → not stale → no reparse.

### Invalidation: absence means "closed"

When a buffer was preloaded in a previous request but is absent from the current request's preload list, the editor has saved or closed it. The server can drop the preloaded buffer state and fall back to disk content.

### Mapping to LSP lifecycle

| LSP | Emacs preload equivalent |
|-----|------------------------|
| `didOpen` | First preload of a buffer |
| `didChange` | Preload with rewritten tmp file (new mtime) |
| *(no event)* | Preload with same tmp file (unchanged mtime) |
| `didSave` / `didClose` | Buffer absent from preload list |

This alignment means the server's buffer management works the same way for both protocols. The LSP path already keeps buffers alive (`handle_did_open` creates persistent buffers); the Emacs path converges to the same model.

## Consequences and Risks

### Benefits

- **Eliminates unnecessary reparsing** — modified-but-unchanged buffers are recognized as fresh
- **LSP alignment** — buffer lifecycle matches `didOpen`/`didChange`/`didClose` model
- **Leverages existing infrastructure** — `bufferIsCloseable()` already distinguishes editor content from disk caches; `closeAllEditorBuffersIfClosable()` already exists
- **Disk cache preserved** — in xref mode, header file caches survive across file processing (e.g., `commons.h` read once, reused). In server mode, disk caches are still freed between requests (appropriate since parsing patterns differ per request)
- **Foundation for LSP document management** — persistent buffers are a prerequisite for `didChange` support in the LSP adapter

### Risks

- **Client complexity** — tmp file lifecycle management moves from simple "create and delete per request" to tracked per-buffer state. `buffer-modified-tick` tracking adds a new concept.
- **Stale buffer accumulation** — if the client fails to signal that a buffer is no longer modified (e.g., Emacs crash), the server holds stale buffers. Mitigated: these are detected as stale on next request with a preload, or become irrelevant (disk content is used when no preload arrives).
- **Memory** — persistent buffers consume memory across requests. In practice this is small — only modified buffers are kept, and their content is already in the editor's memory.
- **No client tests** — the Emacs client has no test suite, making client-side changes riskier.

## Considered Options

### Option 1: Server-side content fingerprint

After parsing, store a content hash in FileItem. On next preload, compare hash before reparsing.

**Pros**: No client changes.

**Cons**: Still creates new tmp files, transfers data, loads buffer text every request — just avoids the parse. Doesn't solve the protocol inefficiency, only mitigates the worst symptom.

**Verdict**: A possible safety net but doesn't address the root cause.

### Option 2: Client-only change tracking (skip preload entirely)

Client uses `buffer-modified-tick` to skip sending preloads for unchanged buffers.

**Pros**: Most efficient — no data transfer at all.

**Cons**: Server destroys buffers after each request, so it needs the preload data every time to have correct positions for modified files. Skipping the preload means the server falls back to disk content, which has wrong positions.

**Verdict**: Doesn't work without also making server buffers persistent.

### Option 3: Persistent buffers + client reuse (CHOSEN)

Server keeps editor buffers alive. Client reuses tmp files for unchanged content. Both sides contribute to the "nothing changed, nothing happens" behavior.

**Pros**: Addresses root cause on both sides. Aligns with LSP model. Uses existing infrastructure.

**Cons**: Changes on both client and server. Most complex option.

**Verdict**: Accepted — the only option that both solves the problem and moves toward the LSP architecture.

## Related Decisions

- **ADR 20**: Separate sync from dispatch — the sync phase that this ADR optimizes
- **ADR 14**: On-demand parsing architecture — persistent buffers support the on-demand model
- **ADR 22**: Lightweight file structure scanning — scanning + persistent buffers together replace the full-project reparse

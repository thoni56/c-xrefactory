# Batch prime and config validation via CreateMode

Date: 2026-06-14

## Status

Proposed

## Deciders

Thomas Nilefalk

## Problem Statement and Context

_In the context of_
- the on-demand server architecture (ADR-0014, ADR-0022) having removed the
  *mandatory* `-create` step — the first session builds references on demand and
  the `.cx` snapshot warms later starts,
- a deliberate, whole-project parse still being wanted for three things: priming
  a large project up front, validating a project config, and giving system tests
  a pre-built snapshot,
- the legacy `XrefMode` (`-create`/`-update`) being the only thing that still
  offers that "parse everything up front" capability — via a divergent parse path
  (the `parseBufferUsingServer` bridge) slated for removal,

_facing the fact that_
- server mode deliberately stays quiet about recoverable parse errors, so a
  misconfigured `-I`/`-D` is invisible until navigation silently misses
  references,
- on a large project the first navigation pays cold-start latency that an upfront
  parse could move off the critical path,
- XrefMode cannot be deleted while it is the sole provider of upfront parsing,

_we decided to_
- introduce **CreateMode**: a standalone `c-xref -create [path]` batch mode that
  starts exactly as `-server` (auto-discovery, snapshot load), eagerly parses
  everything discoverable, reports parse errors, writes the snapshot, and exits,
- take **no command-line options** — the discovered `.c-xrefrc` is authoritative —
  and **not** offer a separate `-update`: always load the snapshot and let the
  reconcile auto-correct from whatever state it is in,
- build it as a **thin reuse of the existing server parse path**, introducing no
  new parse logic,

_disregarding the fact that_
- it adds a mode rather than reducing them,
- it reintroduces a `-create` command we had deliberately made unnecessary,
- a client-driven server *operation* could cover the test case without a new mode,

_because_
- giving `-create` a home on the server path is precisely what lets XrefMode (and
  its bridge) finally be deleted — net mode count stays flat, but the survivor is
  a thin shell, not a divergent engine,
- the `.cx` snapshot is a non-authoritative, disposable cache, so
  always-load-and-reconcile is always safe and a corrupt snapshot is handled by
  discard-and-rebuild — which is *why* create and update need not be distinct,
- a standalone mode (unlike a client-driven operation) serves the up-front user,
  CI/config-validation, and test `prepare` steps uniformly, with no editor client
  required.

## Decision Outcome

`c-xref -create [path]` discovers the project from the anchor (default `.`,
selecting *which* project; scope is always the whole detected project), brings
the snapshot fully up to date, persists it, and exits. It reports parse and
configuration errors on stderr and exits non-zero if any file fails to parse —
making it a config-sanity gate usable from CI. With no discoverable `.c-xrefrc`
it errors and exits non-zero (there is no client to prompt). A snapshot that
cannot be loaded is discarded and rebuilt.

## Consequences and Risks

**Benefits:**
- Unblocks the removal of XrefMode and the `parseBufferUsingServer` bridge.
- Optional upfront priming for large projects without per-request cold-start cost.
- A partial answer to "is my project config correct?", with a CI-usable exit code.
- Tests needing a primed snapshot migrate to a near-identical invocation instead
  of a client-driven script.

**Risks:**
- **Drift back into XrefMode** — the value holds only while every step dispatches
  into existing server internals; if CreateMode grows its own parse logic it has
  recreated the path it was meant to delete.
- **Init parity** — its initialization must match the server's, or a primed
  snapshot could differ from what the server would build.

**Mitigation:**
- Treat "reuses server internals, adds no parse logic" as an acceptance criterion.
- Migrate the XrefMode-`-create` tests onto CreateMode as the first consumer, so
  divergence shows up immediately.

## Related Decisions

- **ADR-0014**: On-demand parsing architecture — removed the *mandatory*
  `-create`; this reintroduces it as an *optional* server-path batch.
- **ADR-0021**: Single-project server — CreateMode inherits the one-project lock.
- **ADR-0022**: Lightweight file structure scanning — the discovery CreateMode
  parses over.
- **Roadmap, "Memory as Truth"**: CreateMode is the enabler for the
  XrefMode-removal step.

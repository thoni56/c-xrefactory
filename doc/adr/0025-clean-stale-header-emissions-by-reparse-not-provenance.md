# Clean stale header-emitted references by include-structure reparse, not provenance

Date: 2026-06-19

## Status

Accepted

## Deciders

Thomas Nilefalk

## Problem Statement and Context

_In the context of_
- the memory-as-truth architecture (ADR-0014), where the in-memory
  `referenceableItemTable` is authoritative and the `.cx` snapshot is a disposable
  cache,
- entry refresh (ADR-0020) reparsing the minimal set of compilation units needed to
  keep that table consistent before each operation,
- `removeReferenceableItemsForFile` being **position-scoped** — it strips only
  references whose `position.file == fileNumber`,

_facing the fact that_
- a compilation unit emits declarations *into the headers it includes*, at header
  positions, and *which* declarations it emits depends on the CU's own preprocessor
  state,
- when such a CU changes that state (e.g. drops a `#define`) and is reparsed, the
  position-scoped strip does not reach the header-position declaration it previously
  emitted, and the reparse gates it out rather than re-emitting it — leaving an orphan
  (the confirmed "variant B" gap, reproduced in
  `test_preprocess_edit_removes_ifdef_define`),
- a fully incremental fix would need **provenance**: knowing which CU emitted each
  header-position reference, so a reparse can strip exactly its own past contributions,

_we decided to_
- **reject per-CU provenance** in any form,
- close the gap by **reparsing by include structure** — when a CU's reparse changes
  what it emits into a header, strip and rebuild that header's contributions from its
  includers; this is always safe because it rebuilds from the include graph,
- **gate** that work on a cheap **per-CU fingerprint** of the CU's header emissions:
  recompute it on reparse, and only when it changes do the include-structure reparse;
  scope it to project headers and bound it by the reverse-include graph,

_disregarding the fact that_
- provenance would be surgical and incremental, touching only the references that
  actually changed, and would make correctness independent of pass ordering,
- the reparse approach pays CPU on the rare preprocessor-affecting edit, worst-case
  reparsing many includers of a widely-shared header,

_because_
- provenance's cost is prohibitive at scale: measured on a full ffmpeg index (2,616
  CUs, 1.24M references ≈ 50–90 MB, of which 190,843 are at header positions),
  provenance incidences run ~10–60M — **3–20× the entire current index** — whether
  stored as per-CU pointer lists (~0.2–1 GB) or as de-duplicated-away duplicate
  references (~0.5–2.5 GB),
- it also introduces a new web of in-memory cross-reference pointers, a bug-prone
  class the project deliberately avoids,
- preprocessor-affecting edits are *rare* — most edits touch function bodies — so the
  fingerprint gate makes the common case free and confines the reparse cost to the
  uncommon case, where the reverse-include graph already bounds the blast radius.

## Decision Outcome

No provenance. The variant-B orphan is cleaned by include-structure reparse, gated by a
per-CU fingerprint of the CU's header emissions. Unconditional declarations fingerprint
identically for every includer, so only `#ifdef`-gated content — the actual source of
the gap — can trip the gate. **Not yet implemented**; this records the agreed direction.
The mechanism is described in the Algorithms chapter, Entry Refresh → _Cleaning Stale
Emissions_.

## Consequences and Risks

- No added steady-state memory and no new pointer structure; the table footprint is
  unchanged.
- Correctness does not depend on per-CU bookkeeping staying consistent — the reparse
  rebuilds from the include graph, and the fingerprint gate affects only *when* the work
  runs, never correctness: a fingerprint mismatch always takes the safe path.
- Risk: a worst-case edit (toggling a macro that gates a widely-included header) can
  reparse many includer CUs and be slow. Mitigated by rarity, the fingerprint gate,
  project-header scoping, and reverse-include bounding; parallel parsing (a separate
  roadmap item) would cap the tail.
- Risk: a hash-only fingerprint cannot distinguish an *added* emission (harmless) from a
  *dropped* one (the orphan case), so a mismatch always takes the full safe reparse
  rather than a surgical strip. Accepted until profiling shows the mismatch path is hot.

## Considered Options

- Provenance A — refcount plus per-CU foreign-reference pointer lists
- Provenance B — `owner` field on each reference plus strip-by-owner
- Reparse-all-includers, ungated
- Reparse-by-include-structure plus per-CU fingerprint gate (chosen)

### Provenance A — refcount + per-CU pointer lists

Each shared header-position reference carries a refcount; each CU keeps a list of
pointers to the foreign references it emitted. A reparse decrements via the list and
deletes the reference at zero. Surgical, incremental, and pass-order-independent.
Downsides: it introduces the cross-reference pointer web, and the per-CU lists cost
O(Σ CU × header-declarations-seen) — ~10–60M incidences on ffmpeg (~0.2–1 GB).

### Provenance B — owner field + strip-by-owner

Give each reference an `owner` (the emitting CU) and change the strip predicate from
`position.file == X` to `owner == X` — no new pointers, a clean generalization of the
existing strip. Downside: it breaks the position dedup. A header declaration emitted by
N includers becomes N duplicate references at one position, so the same ~10–60M blow-up
reappears as duplicate nodes (~0.5–2.5 GB), and every reader of header positions must
de-duplicate at read time.

### Reparse-all-includers, ungated

On any CU reparse that might change header emissions, strip the affected header's
references and reparse all its includers. Zero stored state, always correct. Downside:
pays the full reparse on *every* qualifying edit, including the common case where nothing
about the header's emission actually changed — wasteful on widely-shared headers.

### Reparse-by-include-structure + per-CU fingerprint gate (chosen)

As reparse-all, but gated by a cheap per-CU fingerprint of header emissions so the
reparse fires only when those emissions actually change. One small hash per CU
(negligible storage), no pointer web, no duplicate references. Trades a little compute on
rare edits for none of provenance's memory cost.

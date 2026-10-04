# References are removed when every CU reaching their file has been parsed again

Date: 2026-10-03

## Status

Accepted (2026-10-04)

Supersedes the include-structure reparse and fingerprint gate of
[ADR-0025](0025-clean-stale-header-emissions-by-reparse-not-provenance.md), keeping its
rejection of provenance
Goes with [ADR-0032](0032-knowledge-is-rebuilt-like-make-rebuilds-targets.md)

## Deciders

Thomas Nilefalk (maintainer)

## Terms

Used in this decision, beside those of ADR-0032, and to move to Terminology in
`doc/docs/06-principles.adoc` when it is implemented:

- **Generation**: the number of a CU parse, from a counter incremented once per parse.
- **Knowledge generation**: the generation of a CU's latest parse.
- **Refresh**: a parse emits a reference that is already in the table, and the reference
  gets that parse's generation.
- **Reaching CU** (of a file): a CU whose include closure contains the file, or that is
  the file.

## Problem Statement and Context

_In the context of_
- a CU's knowledge being rebuilt when it is out of date (ADR-0032),
- a CU emitting references at positions in the headers it includes, so that the
  references in a header are contributed by all its includers,
- `removeReferenceableItemsForFile()` removing references by the file of their
  position, which is the only way references leave the table,
- ADR-0025 rejecting provenance, i.e. knowing which CU emitted which reference, because
  it would cost 3-20 times the whole index on ffmpeg,

_facing the fact that_
- a header's references can only be removed as a whole, and are then rebuilt only by the
  includers that get parsed. With the cap, or a "no" to the completeness question, the
  rest are lost, and nothing notices: a rename skips them and reports success,
- ADR-0032 therefore has to rebuild a header with all its includers or not at all, so
  partial requests never add up and an out-of-date header stays out of date,
- a CU that stops emitting a declaration into a header, e.g. after dropping a
  `#define`, leaves it behind, since the CU's reparse strips only its own file
  (`tests/test_preprocess_edit_removes_ifdef_define`, the Variant-B gap). ADR-0025's
  answer is a reparse by include structure, gated on a fingerprint of each CU's header
  emissions, which is not built,

_we propose to_
- give every CU parse a **generation**, and every CU a **knowledge generation**,
- let each reference carry the generation of the **latest parse that emitted it**. A
  parse that emits a reference already in the table (same symbol, position and usage)
  **refreshes** it, which says the reference is still claimed. The table gets references
  from the parser and the snapshot load, both through `addToReferenceList()`, which has
  one branch for a reference already present, so that is the one place to refresh it,
- let a parse **add and refresh, and strip nothing in advance**,
- **remove** a reference when every CU that reaches its file, through the include graph,
  has a newer knowledge generation than the reference: all of them have been parsed since
  it was last emitted, and none emitted it again. Removing unlinks it from its item's
  list, as `removeReferenceableItemsForFile()` does, but chosen by generation instead of
  by file. It is the sweep of a mark-and-sweep collector. A CU that no longer reaches the
  file drops out of the set, and a file no CU reaches loses all its references. The rule
  is the same for a header and for a CU's own file,
- remove in one walk over the table after a request that parsed something,
- **not remove anything in a file whose reverse walk was capped**, since a CU left out of
  the walk may still claim its references,
- not persist generations in the snapshot. On load every reference and every CU gets the
  same generation, so nothing is removed until everything reaching a file has been parsed
  again,

_disregarding the fact that_
- between a change and the removal, references at old positions are still there and show
  as ghosts. They are visible, and the rename precheck refuses them, where a lost
  reference is neither,
- a CU that includes a header only under an `#ifdef` it does not take is still counted as
  reaching it, which delays the removal but loses nothing,

_because_
- every CU that ever contributed to a file is in the include graph: it recorded an
  include reference to the file when it was parsed (`addIncludeReference()`). So the
  removal condition is exact without knowing who emitted what,
- it is not provenance: one number per reference and no CU identity, so the cost
  ADR-0025 measured does not apply,
- partial requests narrow the gap step by step, and the removal happens by itself when the
  last includer has been parsed, which lets ADR-0032's time budget and the remembered
  "no" work for headers too,
- the same rule removes what a CU stops emitting, which closes the Variant-B gap without
  a separate fingerprint gate.

## Decision Outcome

A parse adds and refreshes references, marked with its generation, and removes nothing
first. A reference is removed when every CU that reaches its file has been parsed after
the reference was last emitted. Knowledge in a file is never lost to a partial rebuild;
until the removal, out-of-date references show as ghosts.

## Consequences and Risks

**Benefits:**
- ADR-0032's all-or-nothing rule for headers goes; partial work accumulates.
- `test_preprocess_edit_removes_ifdef_define` gets a fix that needs no fingerprint.
- One table walk per request replaces Pass 2's walk per stale header.

**Risks:**
- *A missed refresh loses a live reference.* This rests on an invariant: every reference
  enters the table through `addToReferenceList()`. It holds today, with two callers, and
  should get a test so that it keeps holding.
- *The reverse walk is capped today* (`MAX_INCLUDE_WALK_FILES`, 256). An uncapped walk
  may cost on a large project; until it is measured, a capped walk means no removal in
  that file.
- *Memory.* Each reference grows by a generation, at most 8 bytes with padding. On
  ffmpeg's 1.24 million references that is about 10 MB. How it is stored is left to the
  implementation.
- *A `.c` file `#include`d by another CU* is reached by both. Today its reparse strips
  all its positions first, including those the other CU emitted. That looks like the
  same loss on a small scale and is not verified.

## Considered Options

- **Add now, remove when every reaching CU is parsed again** — proposed.
- **All or nothing per header** (ADR-0032 without this decision). Correct, but an
  out-of-date header stays so until someone pays for every includer at once.
- **Include-structure reparse gated on a fingerprint** (ADR-0025). Handles the Variant-B
  gap, but still strips before rebuilding, so it does not help partial rebuilds.
- **Provenance.** Exact and incremental, rejected by ADR-0025 for its size.

## Origin

Discussed on the Linux machine, session `76735541`, 2026-10-03, right after ADR-0032.
The all-or-nothing rule for headers meant a "no" to the completeness question would
never narrow, where Thomas expected continued browsing to narrow it gradually. A mark on
each reference, refreshed when it is emitted again, lets a header's references be
removed only when nobody can still claim them.

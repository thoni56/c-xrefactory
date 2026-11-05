# A file's references are removed when its content changes

Date: 2026-11-03

## Status

Accepted (2026-11-03)

Amended (2026-11-03): a change of a file is a change of every file that includes it.

Supersedes [ADR-0033](0033-references-are-removed-when-every-cu-reaching-their-file-is-parsed-again.md),
and with it the include-structure reparse and fingerprint gate of
[ADR-0025](0025-clean-stale-header-emissions-by-reparse-not-provenance.md), keeping the
rejection of provenance
Refines *change time* in [ADR-0032](0032-knowledge-is-rebuilt-like-make-rebuilds-targets.md),
and goes with it

## Deciders

Thomas Nilefalk (maintainer)

## Terms

Used beside those of ADR-0032.

- **Change time**: the time the content the server reads for a file last changed. For a
  file on disk that is its modification time, for a held buffer the buffer's. When the
  server stops reading a held buffer and reads the disk again, the content changes at
  that moment, although the file on disk is older.
- **Reaching CU** (of a file): a CU whose include closure contains the file, or that is
  the file.

## Problem Statement and Context

_In the context of_
- knowledge being rebuilt when an input changed after it (ADR-0032),
- a CU emitting references at positions in the headers it includes, so that the
  references in a header come from all its includers,
- `removeReferenceableItemsForFile()` removing references by the file of their position,
  which is the only way references leave the table,
- provenance, i.e. knowing which CU emitted which reference, costing 3-20 times the whole
  index on ffmpeg (ADR-0025),
- ADR-0033 deciding to remove late: a parse removes nothing first, and a reference goes
  when every reaching CU has been parsed since it was last emitted, with ghosts until
  then. Its generations were never built,

_facing the fact that_
- what was built instead is ADR-0032's all or nothing per header, plus a flag. A header
  is stripped only when every includer is built in the same round. A round with only
  some of them marks the header *left behind*, and the mark makes every includer out of
  date until such a round happens,
- the mark is state that has to be cleared, and clearing it fails in two ways:
  - a round capped at 128 CUs leaves the same CUs out every time, so a header with more
    includers than that is never cleared. Every request then builds 128 CUs again
    (`tests/test_capped_round_is_not_repeated`),
  - stripping a header removes its own `#include` edges, so a header it includes has no
    includers left when it is looked at later in the same loop, and is never cleared
    (`tests/test_a_header_let_go_is_current_again_through_another`),
- together they kept the daily server on c-xrefactory's own `src/` (142 CUs) building
  128 CUs on every request, also after a restart, with or without the snapshot,
- reverting a held buffer is the one change the change time does not show, since the
  file on disk is older than the knowledge built from the buffer. The mark was also
  used for that (`src/editor.c`),

_we propose to_
- **remove a file's references when the server sees its content change**: a newer
  modification time on disk, new content in a held buffer, or a held buffer it stops
  reading. Its change time is then the moment of that change,
- let a **change of a file be a change of every file that includes it**, directly or
  through other headers. What a header emits depends on what it includes, e.g. an
  `#ifdef` on a macro the included file defines. Every CU that reaches the including
  file also reaches the changed one, so no other CU becomes out of date,
- let a **CU be out of date** when its knowledge is older than the change time of an
  input, as ADR-0032 already says. Every CU that reaches a changed header is then out of
  date, and building one puts its part of the header back,
- let **partial work add up**: each CU built puts back its part, and the header is
  complete when every reaching CU has been built since its change time. Nothing has to
  be cleared, so a capped round, or a "no" to the completeness question, is finished by
  later requests,
- let **missing references be known**: the reaching CUs that are out of date are the
  ones whose references are missing, and the completeness question asks about them as
  it does today,
- **drop the mark**: `isLeftBehind`, `markHeadersLeftBehind()`,
  `stripHeadersWhoseIncludersAreAllBuilt()`, `forgetKnowledgeOfHeadersLeftBehind()` and
  the all-or-nothing rule for headers go,
- let the **config be an input of every file** (backlog #36): its definitions and passes
  decide what every CU emits into every file. A config change is a change of every file,
  so every file's references are removed, and every CU is out of date. That replaces
  `markAllCompilationUnitsStale()`, which only made the CUs out of date,
- let the **snapshot follow the same rule**: at exit the server stops reading every held
  buffer, which is a change for one whose content is not on disk, so the CUs reaching
  such a file are saved without knowledge,
- keep **rejecting provenance**: nothing records which CU emitted a reference,

_disregarding the fact that_
- between a change and the last reaching CU being built, a header lacks the references of
  the CUs not yet built. ADR-0033 chose ghosts over that, because a lost reference was
  lost silently. Here the loss is not silent, since the CUs it waits for are out of date,
  but a browse in that state shows fewer references than there are. Measured on `src/`
  (`doc/freshness-after-adr-0035.md`), about 89% of a header's references are
  emitted by every includer, so one CU built puts most of them back. The rest are almost
  all macro expansions, at positions in the macro body, and wait for the CUs that expand
  the macro,
- removing a header's references also removes its `#include` edges until a reaching CU
  is built again, so the include graph below a changed header is incomplete for a while.
  The CUs that would follow those edges are out of date anyway, through the changed
  header, but *visible* (ADR-0031) may answer *outside* for a header only reached
  through it. The files that include a changed file are found before anything is
  removed,
- a CU that stops emitting into a header that did not change, the Variant-B gap
  (`tests/test_preprocess_edit_removes_ifdef_define`), is not handled. What a CU emits
  into a header is the union over all passes. It shrinks when the preprocessor state on
  the way into the header changes, and the cause can be the CU itself or a header
  included before it but not by it, e.g. one that stops defining `FEATURE`. Nothing
  about the header changed, so nothing removes what the CU no longer emits. A file the
  header includes, and the config, are the other causes, and are handled above.
  ADR-0033's generations would close the remaining two, and stay one option for that,
- a CU that includes a header only under an `#ifdef` it does not take is still counted as
  reaching it, so it is built when the header changes, which costs a build but loses
  nothing,

_because_
- a file's content changing is a fact about the file. A change time is compared like any
  other, and never has to be cleared, so there is no order and no round size that can
  keep it stuck,
- it is make: a target is out of date when an input is newer, and a header that changed
  is such an input to every CU that reaches it,
- every CU that ever emitted into a file recorded an include reference to it when it was
  parsed (`addIncludeReference()`), so the reaching CUs say which parts are missing
  without knowing who emitted what,
- it is not provenance: one change time per file, which is already kept, so the cost
  ADR-0025 measured does not apply,
- references that are missing and known can be asked about or refused. Ghosts can be
  edited by a refactoring that does not see they are old.

## Decision Outcome

A file's references are removed when the server sees its content change, and the change
time is when that happened, including when a held buffer is let go. A change of a file
is a change of every file that includes it, and a config change is a change of every
file. A CU is out of date when an input changed after its knowledge, and
building it puts its references back. There is no left-behind mark. Until every reaching
CU has been built again, a changed header's references are incomplete, and the
out-of-date CUs say which are missing.

## Consequences and Risks

**Benefits:**
- `test_capped_round_is_not_repeated` and `test_a_header_let_go_is_current_again_through_another`
  pass without a fix of their own.
- A header's state is its change time and the knowledge times of its reaching CUs, both
  already kept.
- Partial requests add up for headers too, which is what ADR-0033 wanted from
  generations.

**Risks:**
- *Seeing a change.* The removal happens only when the server notices the change. A file
  changed behind its back between requests is noticed when its change time is next
  read, as today. One changed during a parse is noticed on the next request, since the
  knowledge time is taken before the parse.
- *A revert needs a time of its own.* The disk's modification time is older than the
  knowledge, so the moment the server lets go of the buffer has to be kept as the
  file's change time. `test_reverted_held_header_keeps_its_references` pins the case.
- *A missing include edge.* The reaching CUs are found through the include references. A
  CU whose edge to a changed header is missing is not out of date for it, so its part of
  the header is missing without being known to be. Edges come from the parse, the
  lightweight scan and the snapshot, and they can disagree
  (`tests/test_restart_keeps_includes_as_parsed`). The scan records no `<...>` includes.
  Which writer wins is backlog item `include-edges`.
- *The reverse walk is capped* (`MAX_INCLUDE_WALK_FILES`, 256). A capped walk misses
  reaching CUs in the same way as a missing edge.
- *A `.c` file `#include`d by another CU* is a CU itself, and building it alone removes
  what its includer put in it (backlog #105). Not changed by this decision.
- *Cost.* `removeReferenceableItemsForFile()` walks the whole table, once per changed
  file. A request that sees many headers change at once, e.g. after a `git checkout`,
  walks it many times.

## Considered Options

- **Remove on change** — proposed.
- **Generations** (ADR-0033). Keep every reference until every reaching CU has been
  parsed again, then sweep. Ghosts until the sweep, about 10 MB more on ffmpeg, never
  built. Also closes the Variant-B gap.
- **All or nothing plus a mark** (today). Needs every includer in one round, which the
  cap and the clearing order prevent.
- **Fix the mark**: clear it over several rounds, and collect includers before
  stripping. Removes both defects, but keeps state that has to be managed.
- **Include-structure reparse gated on a fingerprint** (ADR-0025). Handles the Variant-B
  gap, but still strips before rebuilding, so it does not help partial rebuilds.
- **Provenance.** Exact and incremental, rejected by ADR-0025 for its size.

## Origin

Found on WSL, session `d25255b8`, 2026-11-02 and 2026-11-03. After a `.c-xrefrc` change
the daily server built 128 of 140 CUs on every request, for system headers like
`stdarg.h` that nothing had edited. A restart, also without the snapshot, did not help.
A debug log showed the second defect. Thomas asked what the mark was, and whether the
make analogy needed it at all. The measurement of what includers emit was made on the
Mac, session `4839a129`, the same day.

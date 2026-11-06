# A change is a difference, and the server orders what it saw

Date: 2026-11-06

## Status

Proposed

Refines [ADR-0032](0032-knowledge-is-rebuilt-like-make-rebuilds-targets.md) and
[ADR-0035](0035-a-files-references-are-removed-when-its-content-changes.md)
Takes the per-CU half of the generations in
[ADR-0033](0033-references-are-removed-when-every-cu-reaching-their-file-is-parsed-again.md)

## Deciders

Thomas Nilefalk (maintainer)

## Terms

Used beside those of ADR-0032 and ADR-0035.

- **Content token** (of a file): what stands for the content the server would read now.
  For a file on disk its modification time and size. For a held buffer the client's
  version of it. A file that does not exist has none.
- **Seen token**: the content token recorded when the server last looked at the file.
- **Changed** (of a file): its content token is not the same as its seen token, or it has
  none of either. A file the server has not seen before has changed.
- **Order**: a counter the server increments when it notices a change and when it takes
  knowledge. It is the server's own, kept in the snapshot, and is not a time.
- **Noticed** (of a change): the order at which the server found a file changed.
- **Racy** (of a seen token): recorded so soon after the file's modification time that an
  edit in the same tick of the filesystem clock would leave the time as it is. The term
  is git's.

## Where it fits

In the shape (`doc/docs/20-insights.adoc`, "The shape it converges to") this decides the
*changed* line, and what the *build* line compares. Discover, scan and remove use
*changed* as it is defined here.

## Problem Statement and Context

_In the context of_
- knowledge being rebuilt like `make` rebuilds targets (ADR-0032), and a file's references
  being removed when the server sees its content change (ADR-0035),
- the shape finding the changed files once, and the scan and the removal both using them,

_facing the fact that_
- "changed" is read in three places, three ways:
  - `rescanChangedFiles()` (`src/projectstructure.c`) reads the disk only, and compares
    with `scanTime`, a clock time, newer by more than the coarse clock's 10 ms,
  - `removeReferencesOfChangedFiles()` (`src/rebuild.c`) reads a held buffer or the disk
    through `contentTimeOf()`, raised to `letGoTime`, and compares with `changeTime`,
    strictly newer,
  - `changeTimeOf()` (`src/freshness.c`) reads a held buffer or the disk, raised to
    `changeTime`, for the build,
- so only the removal knows a buffer let go, only the build counts a vanished file as
  changed, and the scan does not know held buffers,
- "newer" does not see a file that vanished, whose time reads as zero. A header deleted
  on disk keeps its references and its edges (`tests/test_a_deleted_header_leaves_no_references`,
  suspended),
- "newer" does not see a file restored with an older time, by `cp -p`, `tar` or a
  checkout. ADR-0032 accepted that, since `make` has the same deficiency,
- a held buffer let go is read from a disk file older than the knowledge built from the
  buffer, so it needed a time of its own, `letGoTime`,
- the build compares a file's time, from the filesystem's coarse clock, with a knowledge
  time from the system clock, which needs a gap of 10 ms. A preload written just before
  the request lands inside it, so a held CU is built once more after each edit,
- a file time and a clock time are both moved by the system clock being set back, by NTP
  or a suspend, which can make a change look older than knowledge built before it,

_we propose to_
- **find a change by difference**: a file has changed when its content token is not the
  same as its seen token. That is one reading, once per request, used by the scan and the
  removal, each with its own seen token,
- **make the content token the modification time and the size**, from the same `stat`,
  so that most edits within one tick of the filesystem clock are seen,
- **treat a racy seen token as unknown**, so that the file counts as changed on the next
  request, as git does for a file whose time is too close to when its index was written,
- **let the client send its version of a held buffer** with `-preload`, e.g. Emacs's
  `buffer-modified-tick`, which it already keeps to decide whether to write the preload,
  so that no file time is involved for held content,
- **order what the server saw with a counter of its own**: noticing a change and taking
  knowledge each take the next value. A CU is out of date when a prerequisite was noticed
  changed after its knowledge was taken. The counter's highest value, each file's noticed
  change and each CU's knowledge are kept in the snapshot,
- **keep the scan's seen token per session**, since the edges are not in the snapshot, and
  the removal's in the snapshot. In a new session every known file is changed for the
  scan and none for the removal,

_disregarding the fact that_
- a token that changes without the content changing, e.g. after `touch` or a checkout that
  rewrites a file as it was, costs a build. "Newer" costs the same,
- content changed twice within one tick, with the same size, is not seen. A checksum would
  see it, at the cost of reading every file each request. The racy rule covers the case
  where the server has just read the file,
- a filesystem whose times differ between two `stat`s of an unchanged file would make
  every request see it changed. Not known to happen. WSL's `/mnt/c` is the one to check,

_because_
- `make` compares a target's time with its prerequisites' times. It never asks whether one
  file changed, and that question is what detection is. ADR-0032 kept the rules from
  `make` and, with them, "newer" for detection too, where it does not fit,
- a difference sees a vanished file, a restored older file and a buffer let go with the
  same comparison, so `letGoTime` and the zero special cases go,
- the token stands for the content, as a checksum would, without reading the file,
- once a change time is "when the server noticed", every value the build orders is the
  server's own. A counter orders them exactly, needs no gap, and is not moved by the
  system clock,
- the counter is the per-CU half of ADR-0033's generations, the knowledge generation. The
  per-reference half, the mark, the refresh and the sweep, was a removal mechanism, which
  ADR-0035 replaced. If the Variant-B gap is closed with generations, as ADR-0035 leaves
  open, it uses this counter.

## Decision Outcome

A file has changed when its content token, its modification time and size or the
client's version of a held buffer, is not the same as when the server last saw it. A
vanished file and a new one have changed. The server orders what it saw with a counter
of its own, kept in the snapshot: a CU is out of date when a prerequisite was noticed
changed after its knowledge was taken. "Newer" is no longer used to find a change, and
no file time is compared with a clock.

## Consequences and Risks

**Benefits:**
- One reading of "changed", shared by the scan and the removal, as the shape says.
- A deleted header is seen, and `test_a_deleted_header_leaves_no_references` has its fix
  in the comparison.
- A restored older file is seen. ADR-0032's accepted deficiency goes.
- `letGoTime` goes: a buffer let go is read from the disk, whose token is not the
  buffer's, so it has changed.
- The coarse clock gap goes, with the extra build of a held CU after each edit.
- The order survives a restart exactly, also when the system clock moved.

**Risks:**
- *A new snapshot format.* The seen tokens, the noticed changes, the knowledge values and
  the counter's highest value are kept. An older snapshot is ignored, as for 1.15.0.
- *The racy rule depends on the filesystem's tick.* Too narrow and an edit is missed, too
  wide and files are scanned again needlessly. The first errs towards a missed change,
  so it should err wide.
- *The client's version is a protocol change.* Until the client sends it, a held buffer's
  token is the preload file's modification time and size, which is racy as described.
- *An unknown filesystem behaviour.* A filesystem that reports different times for an
  unchanged file makes every request rebuild. It costs builds, not correctness.

## Considered Options

- **A difference of modification time and size, and the server's own order** — proposed.
- **Newer, as today.** Misses vanished and restored files, needs `letGoTime` and the clock
  gap, and is read three ways.
- **A difference of modification time alone.** Misses same-tick edits of equal times
  more often, for no saving: the size comes from the same `stat`.
- **A checksum of the content.** Exact, but every file is read on every request. Possible
  later for racy files only.
- **Times for the order, the noticed time from the system clock.** Works, but is moved by
  the clock being set back, and keeps the comparison between two clocks for held buffers.

## Origin

Discussed on the Mac, session `4839a129`, 2026-11-06, while refining the shape's
*changed* line and looking for how added and deleted files are seen (backlog
`files-appear-vanish`). That the code reads "changed" three ways was found by comparing
the scan with the removal. Thomas noted that the token stands in for the content, as the
checksum once considered would, and that the make analogy had put "newer" both into the
code and into how we thought about it. The racy rule is git's ("racily clean" files).

# A change is a difference, and the server orders what it saw

Date: 2026-11-06

## Status

Proposed

Refines [ADR-0032](0032-knowledge-is-rebuilt-like-make-rebuilds-targets.md) and
[ADR-0035](0035-a-files-references-are-removed-when-its-content-changes.md)

## Deciders

Thomas Nilefalk (maintainer)

## Terms

Used beside those of ADR-0032 and ADR-0035.

- **Content token** (of a file): what stands for the content the server would read now.
  For a file on disk its modification time and size. For a held buffer those of its
  preload. A file that does not exist has none.
- **Seen token**: the content token recorded when the server last looked at the file.
- **Changed** (of a file): its content token is not the same as its seen token. Having
  none is a token of its own, so a file that vanished, or that the server has not seen
  before, has changed, and one that is still missing has not.
- **Noticed** (of a change): when the server found a file changed, by its own clock.

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
- a header held at an exit keeps the buffer's references after a restart, since
  `forgetKnowledgeForHeldContent()` clears only knowledge times, which the removal does
  not read (`tests/test_a_header_held_at_an_exit_is_changed_after_a_restart`, suspended),
- "newer" does not see a file restored with an older time, by `cp -p`, `tar` or a
  checkout. ADR-0032 accepted that, since `make` has the same deficiency,
- a held buffer let go is read from a disk file older than the knowledge built from the
  buffer, so it needed a time of its own, `letGoTime`,
- the build compares a file's time, from the filesystem's coarse clock, with a knowledge
  time from the system clock, which needs a gap of 10 ms. A preload written just before
  the request lands inside it, so a held CU is built once more after each edit,

_we propose to_
- **find a change by difference**: a file has changed when its content token is not the
  same as its seen token. That is one reading, once per request, used by the scan and the
  removal, each with its own seen token,
- **make the content token the modification time and the size**, from the same `stat`,
  so that most edits within one tick of the filesystem clock are seen. The `stat` comes
  before the content is read, so that a write in between is seen on the next request,
- **order what the server saw by its own clock**: a change is stamped when it is noticed,
  and knowledge when it is taken. A CU is out of date when a prerequisite was noticed
  changed after its knowledge was taken. A change is stamped at the `stat` that finds
  it, not when the file is used, so a file first reached by a parse, e.g. through an
  include the scan does not record, is stamped after the knowledge being built, and costs
  one build more rather than a change missed. The noticed changes and the knowledge are
  kept in the snapshot,
- **keep the scan's seen token per session**, since the edges are not in the snapshot, and
  the removal's in the snapshot. In a new session every known file is changed for the
  scan, and for the removal those whose token differs from the snapshot's, which
  includes every file that was held,
- **save a held file with its preload's token**. After a restart the server reads the
  disk, or finds the file missing, and neither has the preload's token, so the file has
  changed, as when a buffer is let go during a session. That is so also for the snapshot
  written during a long build. Its references are saved as they are, and the restarted
  server removes them,

_disregarding the fact that_
- a token that changes without the content changing, e.g. after `touch` or a checkout that
  rewrites a file as it was, costs a build. "Newer" costs the same,
- saving a held buffer costs a build: the server lets go of it and reads the disk, whose
  token is never the preload's, although the content is the same. `letGoTime` costs the
  same today,
- content written, `stat`ed by the server and written again within one tick of the
  filesystem clock, with the same size, is not seen until the file changes again. The
  tick is a few milliseconds on Linux, where modification times come from the kernel's
  coarse clock. Emacs writes a preload only before a request, and a save is a command, so a
  human needs two requests within the tick. Only a tool rewriting a file twice while a
  request runs can meet it, e.g. a generator or a formatter, and only at the same size,
- on a filesystem with a tick of a second or more, e.g. HFS+, FAT or some network
  mounts, a human can meet it too: an edit, a request and an edit of the same size
  within the tick,
- a filesystem whose times differ between two `stat`s of an unchanged file would make
  every request see it changed. Not known to happen. WSL's `/mnt/c` is the one to check,

_because_
- `make` compares a target's time with its prerequisites' times. It never asks whether one
  file changed, and that question is what detection is. ADR-0032 kept the rules from
  `make` and, with them, "newer" for detection too, where it does not fit,
- a difference sees a vanished file, a restored older file and a buffer let go with the
  same comparison, so `letGoTime` and the zero special cases go,
- the token stands for the content, as a checksum would, without reading the file,
- once a change time is "when the server noticed", every time the build orders is read
  from the server's own clock, and two readings of one clock need no gap.

## Decision Outcome

A file has changed when its content token, its modification time and size or those of
a held buffer's preload, is not the same as when the server last saw it. A
vanished file and a new one have changed. A held file is saved with its preload's token.
The server orders what it saw by its own clock:
a CU is out of date when a prerequisite was noticed changed after its knowledge was
taken. "Newer" is no longer used to find a change, and no file time is compared with
the clock.

## Consequences and Risks

**Benefits:**
- One reading of "changed", shared by the scan and the removal, as the shape says.
- A deleted header is seen, and `test_a_deleted_header_leaves_no_references` has its fix
  in the comparison.
- A restored older file is seen. ADR-0032's accepted deficiency goes.
- A held file has changed after a restart, header or CU alike, and
  `test_a_header_held_at_an_exit_is_changed_after_a_restart` has its fix.
  `forgetKnowledgeForHeldContent()` goes.
- `letGoTime` goes: a buffer let go is read from the disk, whose token is not the
  buffer's, so it has changed.
- The coarse clock gap goes, with the extra build of a held CU after each edit.

**Risks:**
- *A new snapshot format.* The seen tokens and the noticed changes are kept. An older
  snapshot is ignored, as for 1.15.0.
- *The system clock set back.* A change noticed soon after the clock was set back can
  look older than knowledge taken before it. That CU then looks current and lacks its
  part of the changed file, until it or a file it includes changes again. It is rare, so
  it is accepted. If it is seen, the server can stamp no time lower than the highest it
  has stamped, i.e. `max(now, highest)`, taking the highest from the snapshot's times
  after a restart. That needs no new value in the snapshot.
- *An unknown filesystem behaviour.* A filesystem that reports different times for an
  unchanged file makes every request rebuild. It costs builds, not correctness.

## Considered Options

- **A difference of modification time and size, ordered by the server's clock** — proposed.
- **Newer, as today.** Misses vanished and restored files, needs `letGoTime` and the clock
  gap, and is read three ways.
- **A difference of modification time alone.** Misses same-tick edits of equal times
  more often, for no saving: the size comes from the same `stat`.
- **A checksum of the content.** Exact, but every file is read on every request.
- **Git's rule for racy tokens**, a token recorded within a tick of the file's time
  counting as unknown, with the client backdating its preloads. Closes the same-tick
  window, but needs a third state beside a token and none, which unlike none is not
  the same as itself, and compares a file time with the clock again, for a window that
  human edits do not reach.
- **The client's version sent with `-preload`**, e.g. Emacs's `buffer-modified-tick`,
  which it already keeps to decide whether to write the preload. Takes file times out of
  held content altogether, but changes the protocol on both sides.
- **A counter of the server's own for the order.** Exact, also when the clock is set
  back, but one more value to keep and save, for a case that is rare.

## Origin

Discussed on the Mac, session `4839a129`, 2026-11-06, while refining the shape's
*changed* line and looking for how added and deleted files are seen (backlog
`files-appear-vanish`). That the code reads "changed" three ways was found by comparing
the scan with the removal. Thomas noted that the token stands in for the content, as the
checksum once considered would, and that the make analogy had put "newer" both into the
code and into how we thought about it.
The counter first proposed for the order was dropped on WSL, session `d25255b8`,
2026-11-09, since a clock set back is rare. The racy rule, the backdating and saving
a held file without a token were dropped on the Mac, session `4839a129`, 2026-11-10,
since they needed a third state for a window that human edits do not reach.

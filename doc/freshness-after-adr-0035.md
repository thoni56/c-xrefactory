# Freshness after ADR-0035: where it is heading, and the include graph

Mac, session `4839a129`, 2026-11-05, at `3ca09bc3`. An opinion on the direction after
ADR-0035, and what follows for the include graph, for the WSL discussion. It replaces
`doc/freshness-make-model-assessment.md`, whose assessment has been worked into ADR-0035,
the backlog and the shape in `doc/docs/20-insights.adoc`. Its measurement is kept below,
since ADR-0035 and the backlog cite it.

## The direction is right

- **Removal is a fact about a file.** A change time is compared, never cleared, so whole
  classes of defect cannot happen any more: the capped round, the clearing order, a
  restart. The chain of rules that made up for each other, the left-behind mark and its
  helpers, is gone rather than patched.
- **Missing and known is better than ghosts.** Partial work adds up, and the out-of-date
  CUs say what is missing. The cost, macro-body references missing until the CUs that
  expand the macro are built, is stated openly in ADR-0035.
- **The process change matters most.** The shape is written down, and CLAUDE.md requires
  every fix to say which line of the shape it belongs to. That is the guard that was
  missing while the left-behind mark grew: each fix was locally right, and none was
  checked against where the design was going.

## What still fits no line of the shape

`callServer()` against the shape (remove, goal, build, answer):

| Step | Line | Note |
|---|---|---|
| `removeReferencesOfChangedFiles()` | remove | Headers only. A CU loses its references when it is built. |
| `closeEditorBuffersNoLongerHeld()` | none | A CU let go goes onto the build list, a header gets a `letGoTime`: two mechanisms for one event. |
| `markAllCompilationUnitsStale()` | none | The config is not an input yet, and no references are removed (#36). |
| `collectOutOfDateHeldCUs()` | goal | "A held CU is always in the goal" is a policy, but not stated as one. |
| `collectRequestCUForStripping()` / `ToBuild()`, `stripRequestCU()` | none | The parse that answers is also the knowledge build (`cursor-parse-reuse`). |
| `collectSiblingCUs()` | goal | The request file's reach, not the symbol's (`request-target`). |
| `analyseAllOutOfDateCUs()` | goal and build | A second build list for an operation on a name. |

Most of the rows marked "none" go back to one root: the include edges.

## The centre of gravity is the include edges

A `TypeCppInclude` reference is both knowledge and an edge (`20-insights`, "The include
edges are inputs, not knowledge"). Because of that:

- a changed CU cannot lose its references when it changes, since its edges would go with
  them, so removal is not the same for CUs and headers,
- the reverse question, which CUs reach this file, is answered from the previous build
  (`test_a_cu_that_starts_including_a_header_on_disk_is_among_its_includers`),
- the parse, the scan and the snapshot all write edges and can disagree
  (`test_restart_keeps_includes_as_parsed`, backlog `include-edges`).

**Proposal: take the edges out of the reference table.** A file's includes become a fact
about the file, like its change time, written by the lightweight scan from its current
content. The parse's include references stay as knowledge, for browsing and Rename
Module.

What it gives:

- **Uniform removal.** Removal on change becomes the same for CUs and headers, which
  retires both special cases for a CU: going onto the build list when let go, and the
  request-CU stripping split.
- **One writer.** `include-edges` is decided by construction.
- **Correct reverse answers.** A CU that starts including a header is found.
- **A cheaper request.** `buildIncludeGraph()` no longer walks the whole table, about
  38 ms on a fully parsed ffmpeg, twice per request.
- **Errors in the safe direction.** The scan also takes includes inside an untaken
  `#ifdef`, so it over-approximates. That costs builds, never correctness.

## The graph is not in the snapshot

It is derived from the files every session, as the change times are.

- **The scan already runs** on the first request of every session (`scanDone` in
  `callServer()`), with or without a snapshot, and reads every file's current content.
- **A saved graph would be the previous session's.** Anything changed while no server ran
  makes it wrong, and the scan reads those files again anyway. Keeping it brings back a
  second writer, and `test_restart_keeps_includes_as_parsed` is what two writers look
  like.
- **Both questions are served by the current content.**
  - The reverse question needs it, and only a scan of the current content gives it.
  - For the forward question, is this CU out of date, make uses the previous build's
    dependencies, and the current content is at least as good: a new edge appears only
    through a change in the CU or a header on its way, which makes the CU out of date
    already.
- **Incremental during a session.** Rescan a file's includes whenever its change time
  moves, at the point where `removeReferencesOfChangedFiles()` removes its references. A
  file that appears through a new `#include` is found when its includer is rescanned. A
  new CU that nothing includes and no request names is found at the next restart or
  config change, as today.

### The edges' own up-to-date rule

WSL, session `d25255b8`, 2026-11-05.

A file's own edges, the includes it names, depend only on its content and the search
path. So they are out of date exactly when its change time moves, or the config changes.
That is the set the remove line already finds.

The rule is the one used everywhere else, a time and not a flag:

- each file records when its edges were scanned,
- its edges are current if its change time is not later than that,
- an incremental scan rescans every file whose edges are not current.

That is make again. The edges are a target, the file's content and the config are its
prerequisites, and the scan is the recipe. A capped round, an exit or a restart cannot
lose a rescan, as for the removal.

The edges cannot be lazy, since the goal is found from them. They have to be current
before the goal, so the rescan belongs on the remove line. The scan is cheap, so doing it
for every changed file should be fine. The whole scan stays as today, on the first
request and on a config change.

Not covered: which file an include resolves to also depends on which files exist on the
search path. A header created later that an unchanged file already includes gets no edge,
and a new header earlier on the search path is not seen to shadow the old one. Make has the
same pitfall, since `gcc -MD` only records what it found. If it matters, a file could keep
its unresolved includes, and a new file could be matched against them.

### What the scan has to get right, as the only source

1. **`<...>` includes.** `scanFileForIncludes()` (`src/projectstructure.c`) matches only
   `#include "..."`. A project that includes its own headers with angle brackets gets no
   edges, and reach misses CUs. This stops being optional.
2. **Which directories it searches.** `resolveIncludePath()` tries the includer's
   directory, then `options.includeDirs`. Not checked: whether the system directories
   found by interrogating the compiler are in that list. If not, edges between system
   headers have no writer, and a request in a system header finds no CUs.
3. **Computed includes** (`#include MACRO`). The scan cannot see them, the parse can.
   Accept the gap, and perhaps log when a parse takes an include the scan did not record.

## Risks to keep in view

- **Cost per request.** `removeReferencesOfChangedFiles()` checks the time of every
  non-CU file the server knows, system headers included, on every request. Each changed
  file costs a walk over the whole table, which is heavy after a `git checkout`. Both
  want measuring on ffmpeg before anything is built on top.
- **Known only helps if it is said.** The server knows a header is incomplete, but a
  browse does not tell the user. With the removal honest, the goal being too small
  (`request-target`) is the largest correctness gap: the answer reads references from
  CUs outside the goal, and nothing says so.
- **Variant B is open by decision.** `test_preprocess_edit_removes_ifdef_define` stays
  suspended as its marker.

## Where this is taking us

The end state is small enough to say in a paragraph:

- each file has two facts, its change time and its includes, both derived from the file,
- each CU has one, its knowledge time,
- references are knowledge and nothing else,
- each request is remove, goal, build, answer.

That is make, and every step left over is visibly either policy or a workaround.
Suggested order: the edges, then the goal (`request-target`). #54 and #1 come after,
on a request path that is simpler by then.

## Measurement: what every includer emits into a header

Made on the Mac, session `4839a129`, 2026-11-03, at `643f3cec`.

**How:**

- A temporary hook in `addToReferenceList()`'s caller in `src/cxref.c` logged every
  reference a CU parse emitted at a header position: the CU, link name, line, column,
  usage, type and header.
- One cold build of `src/` (its own `.c-xrefrc`: `.`, `-I3pp`), with no snapshot and
  `MAX_CUS_TO_REPARSE` raised so that one `-browse-push` in `options.c` built every CU
  that reaches it.
- It ran in a scratch worktree, since removed, and nothing was committed.
- Every emission was counted, not the table after deduplication. A reference is
  identified by (link name, line, column, usage).
- One emission with an empty `inputFileName` (`%%%i` at 1:0, probably the scan) is left
  out.

**`options.h`, 62 reaching CUs:** 161 references in the union, 160 (99.4%) emitted by
every includer. The one exception is the include guard, `OPTIONS_H_INCLUDED` at line 1,
which only 26 CUs emit.

**All headers with at least two includers:**

| | Headers | References | Emitted by every includer |
|---|---|---|---|
| The project's own | 91 | 4998 | 88.8% |
| System | 175 | 21674 | 89.4% |

**The headers where the share is lowest:**

| Header | Includers | References | Emitted by every includer | One includer restores, median |
|---|---|---|---|---|
| `yystype.h` | 17 | 141 | 14.9% | 29% |
| `list.h` | 21 | 404 | 36.1% | 49% |
| `protocol.h` | 16 | 30 | 50.0% | 70% |
| `misc.h` | 47 | 101 | 65.3% | 66% |
| `log.h` | 84 | 81 | 69.1% | 74% |
| `head.h` | 76 | 128 | 76.6% | 81% |
| cgreen's `unit_implementation.h` | 57 | 2143 | 6.3% | 8% |
| clang's `stddef.h` | 121 | 77 | 51.9% | 99% |

**Where the references that not every includer emits are:** inside `#define` bodies, 258
of 258 in `list.h`, 35 of 35 in `misc.h`, 24 of 25 in `log.h` and 29 of 30 in `head.h`. A
macro expansion puts its references at the macro body's positions, and only the CUs that
expand the macro emit them. Some depend on the CU's own types, e.g. the `next` field of
whichever struct a CU hands to `LIST_APPEND`. There are also locals declared inside macro
bodies. `#ifdef` barely shows in `src/`.

**Not measured:** a project with real `#ifdef` variance, e.g. ffmpeg; how often a real
edit hits a macro body rather than a declaration; and whether the missing macro-body
references are ones a user browses to.

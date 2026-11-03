# Freshness as make: where the implementation stands, and ghosts against losses

Mac, session `4839a129`, 2026-11-03, at `643f3cec`. Written for the WSL discussion of
what the implementation lacks or does differently from ADR-0032, and whether ADR-0033
was right to prefer ghosts over missing references. Read from `src/freshness.c`,
`src/rebuild.c`, `callServer()` in `src/server.c`, `closeEditorBuffersNoLongerHeld()` in
`src/editor.c`, ADR-0032, ADR-0033 and backlog §7. Part 2 is a measurement, part 1 is
reading.

## Part 1: Is it getting complicated?

Yes, but the special cases are not spread evenly. The predicate is close to the ADR. The
special cases sit in `rebuild.c` and `callServer()`, and almost all of them have three
causes.

### What already matches

`whyOutOfDate()` (`src/freshness.c`) is ADR-0032's one test: no knowledge, the CU
changed, an input changed, an input left behind, each reason named and logged. `Inputs`
asks for each change time once per request. The one missing input is the config (#36),
so `markAllCompilationUnitsStale()` and the zero sentinel are still there.

### Cause 1: removal still strips first (ADR-0033 is not in)

Most of the special cases are here. Every place that builds has to decide when to strip:

- A header is stripped in three places, each with its own idea of "every includer":
  `stripChangedHeldHeaders()`, inside `collectSiblingCUs()`, and
  `stripHeadersWhoseIncludersAreAllBuilt()`.
- A CU's own file is stripped in `buildKnowledgeOfCU()` and in `stripRequestCU()`. That
  needs the `stripRequest` flag, which split the request collection in two:
  `collectRequestCUForStripping()` and `collectRequestCUToBuild()`.
- *Left behind* makes up for the all-or-nothing rule:
  - `markHeadersLeftBehind()` marks a header.
  - `closeEditorBuffersNoLongerHeld()` also sets `isLeftBehind`.
  - `forgetKnowledgeOfHeadersLeftBehind()` exists because the mark is not in the
    snapshot.

  The amendment of 2026-10-26 added this chain. **Each interim rule produced a rule to
  make up for it, and that is the smell.**

### Cause 2: two freshness tests coexist

`stripChangedHeldHeaders()` still asks `fileNumberIsStale()`, which compares
`lastParsedMtime` for equality. It also strips without checking that the includers fit
under the cap (#39), and it writes `lastParsedMtime`. Everything else asks
`knowledgeIsOutOfDate()`. A held header is judged by the old rule and everything else by
the new one. `lastParsedMtime` is still written in four places mostly to keep that alive.

### Cause 3: the parse that answers is also the knowledge build

The operation parses the request CU itself, in `singlePass()`, which also sets
`knowledgeTime`. That is why `callServer()` chooses from three flags what to collect:

- `requiresProcessingInputFile`
- `operationParsesRequestFile`
- `answeredWithoutReferences`

The macro-body expansion (ADR-0027) is a third kind of parse. It has its own cap and
sets no knowledge time. Doing the answering parse separately from the knowledge build is
#1, and #54 lives on the same path.

### Smaller departures

- **The goal is computed in two phases with two build lists.** The siblings are built
  first. Then, for an operation on a name, `analyseAllOutOfDateCUs()` builds the rest. In
  the model it is one goal and one build.
- **Four limits with no measured reason:**
  - `MAX_CUS_TO_REPARSE`, 128
  - `MAX_INCLUDE_WALK_FILES`, 256, where the walk stops silently (#37)
  - `MAX_INCLUDE_CLOSURE`, 1000
  - `MAX_FILES`

### The shape it could converge to

```
goal  = reach(request file) or whole project   (policy)
build = out-of-date CUs in goal                (one predicate, config included)
build them; remove what no CU still emits      (ADR-0033, or an alternative, part 2)
answer (cursor parse only where it is needed)
```

Each step in today's `callServer()` either belongs to one of these lines or is a removal
workaround.

### Suggested order, before the measurement

The backlog has `request-through-includers` (#54) next. It would *add* a special case:
includers that are current get parsed only for the cursor, with a bound of their own, on
the path where cause 3 already tangles.

The suggested order instead:

1. Include edges (#42) first, since a missing edge makes any removal rule lose live
   references silently.
2. The removal mechanism (#43 and #44, or the alternative below).
3. Fold #39 and `fileNumberIsStale()` into that work.

Then #54 and #1 land on a simpler request path. The config as an input (#36) is
independent and small, and it removes the zero sentinel.

## Part 2: Ghosts against losses

### What ADR-0033 rests on

"Ghosts are visible and a refactoring can refuse them, a lost reference is neither." That
was true of what it was compared with: strip, rebuild some of the includers, and record
nothing. The loss was silent because nothing said a header was incomplete.

### An option the ADR does not list: strip and mark

- When a header is stripped, record a time on it.
- The header is *complete* again when every CU that reaches it has knowledge newer than
  that time.

That is one time per file, compared with knowledge times that already exist. It needs no
number on each reference and no sweep. Partial work adds up here too: each rebuilt
includer puts its part back, and the header stays marked until the last one. The Variant-B
gap, a CU that stops emitting into a header, is closed as well, since the strip removes
the reference and nothing puts it back.

Where the two models are the same:

- **Refactorings:** an operation that edits code brings its goal up to date before it
  runs (ADR-0032), so after that both models are exact. In the ghost model that holds only
  if the sweep runs before the answer, in the same request, which should be stated either
  way. The choice therefore matters only for browsing while knowledge is partly out of
  date, and after a "no".
- **A missing include edge:** both are exactly as exposed. One sweeps too early, the other
  declares the header complete too early. Either way it is silent.

What generations do better: they can tell that a single reference is suspect. A CU that
reaches its file has been parsed again and did not refresh it. A refactoring could refuse
exactly those references, instead of checking whether the text at the position still
reads the name.

The claim to test was: *nearly all of a header's references are emitted by every
includer*. If so, parsing one includer again restores almost everything, and strip and
mark loses little while ghosts duplicate a lot.

### Measurement

How it was done:

- A temporary hook in `addToReferenceList()`'s caller in `src/cxref.c` logged every
  reference a CU parse emitted at a header position: the CU, link name, line, column,
  usage, type and header.
- One cold build of `src/` (its own `.c-xrefrc`: `.`, `-I3pp`), with no snapshot and
  `MAX_CUS_TO_REPARSE` raised so that one `-browse-push` in `options.c` built every CU
  that reaches it.
- It ran in a scratch worktree that has since been removed, and nothing was committed.
- Every emission was counted, not the table after deduplication.
- A reference is identified by (link name, line, column, usage).
- One emission with an empty `inputFileName` (`%%%i` at 1:0, probably the scan) is left
  out.

**`options.h`, 62 reaching CUs:**

| | |
|---|---|
| References in the union | 161 |
| Emitted by every includer | 160 (99.4%) |
| What any single includer restores | 160 or 161 |

The one exception is the include guard, `OPTIONS_H_INCLUDED` at line 1, which only 26
CUs emit. **For a header of declarations the claim holds.**

**All headers with at least two includers:**

| | Headers | References | Emitted by every includer |
|---|---|---|---|
| The project's own | 91 | 4998 | 88.8% |
| System | 175 | 21674 | 89.4% |

The headers where the share is lowest:

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

**Where the references that not every includer emits are:**

| Header | Not emitted by every includer | Inside a `#define` body |
|---|---|---|
| `list.h` | 258 | 258 |
| `misc.h` | 35 | 35 |
| `log.h` | 25 | 24 |
| `head.h` | 30 | 29 |

A macro expansion puts its references at the macro body's positions, and only the CUs
that expand the macro emit them. Some of them depend on the CU's own types, e.g. the
`next` field of whichever struct a CU hands to `LIST_APPEND`. There are also locals
declared inside macro bodies (`13fb-17-e!tmp`, `8c4e-10-c!mop_index`). `#ifdef` barely
shows in `src/`. **What varies between includers is macro bodies.**

### What the numbers say

- **Declarations: strip and mark wins clearly.** After an edit, ghosts duplicate every
  reference below the changed line until every reaching CU has been parsed again. That is
  what the 2026-10-26 amendment saw in `options.h`. Strip and mark loses close to nothing.
- **Macro bodies: both models are wrong in a way users will see.**
  - Strip and mark: after one includer is parsed again, about half of `list.h`'s
    references are missing. That includes the uses of a struct's `next` field through
    `LIST_APPEND`, until the CUs that expand the macro are parsed again. The loss is
    marked, and it shrinks as more includers are parsed.
  - Ghosts: the same references stay, at old positions if the body moved.
- **So ADR-0033's preference has more going for it than part 1 credited.** The references
  that differ between includers, the only ones a loss can affect, are in macro bodies.
- **Strip and mark is still much cheaper:** one time per header, against a generation on
  every reference (about 10 MB on ffmpeg), the refresh, the sweep and its cost question
  (#45), ghosts that survive a restart, and the refusal contract for ghosts.
- **A mixed rule is possible,** strip and mark except at positions inside a `#define`
  body. But it is the kind of special case this note started out worried about.

### Not measured

- **One project only,** `src/`. Its `_tests.c` files make cgreen's macro headers
  prominent. A project with real `#ifdef` variance, e.g. ffmpeg, could look different.
- **How often a real edit hits a macro body** rather than a declaration.
- **How the references in a macro body are used when browsing.** Whether the missing ones
  are ones a user browses to (field uses through a macro) or mostly locals inside the
  body.

## Questions for the decision

1. Does strip and mark go into ADR-0033's considered options, with these numbers, before
   #43 and #44 are built?
2. If it does, is the macro-body loss acceptable for browsing, given that refactorings
   complete their goal first and the loss is marked and narrows?
3. Either way, does #42 (include edges) move ahead of #54, since both removal models
   depend on it?

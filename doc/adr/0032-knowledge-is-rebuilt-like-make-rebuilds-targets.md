# Knowledge is rebuilt like `make` rebuilds targets

Date: 2026-10-03

## Status

Accepted (2026-10-04)

Amended (2026-10-05): terms sharpened, the decision unchanged.

Builds on [ADR-0014](0014-adopt-on-demand-parsing-architecture.md) and
[ADR-0020](0020-separate-buffer-sync-from-operation-dispatch.md)
Goes with [ADR-0033](0033-references-are-removed-when-every-cu-reaching-their-file-is-parsed-again.md)

## Deciders

Thomas Nilefalk (maintainer)

## Terms

Used in this decision, and also in Terminology in `doc/docs/06-principles.adoc`, where
*Out of date* will replace *Staleness*.

This decision is about knowledge, not files. The reference table holds referenceable
items, each with its references, and a reference's position names the file it is in.
Nothing in the table is kept per file or per CU.

- **Knowledge**: the references that parsing one CU adds to referenceable items, wherever
  they are positioned, i.e. in the CU and in the headers it includes. They are not kept as
  a unit. Only the knowledge time is.
- **Input**: a file or setting that knowledge depends on, i.e. the CU, every file in its
  include closure, and the project config.
- **Change time**: an input's modification time, from the held buffer if the client
  holds one, otherwise from disk.
- **Knowledge time**: when the knowledge of a CU was recorded.
- **Out of date**: a CU's knowledge, when there is none, or when an input changed after
  the knowledge time.
- **Reach** (of a symbol): the CUs whose knowledge can reference it. For a `static` that is
  its own CU, for a global the CUs that include its declaring header, directly or through
  other headers. Reach walks through headers but ends in CUs, since only a CU is parsed.
- **Goal** (of a request): the knowledge that has to be up to date for the request. For an
  operation on a position that of the CUs in the symbol's reach, for an operation on a
  name that of every CU. (`make`'s word for what is asked for. A target is what a rule
  builds.)
- **Operation on a position / on a name**: whether the request starts from a cursor
  position or from a symbol name. `needsWholeProjectParsed()` (`src/server.c`) lists
  the operations on a name.
- **Time budget**: how long parsing may take before the user is asked.
- **Completeness question**: the question asked when bringing the goal up to date
  would take longer than the time budget.

## Where the `make` analogy stops

The `make` analogy describes when a CU is parsed. It does not describe what a parse
leaves in the table. Two things connect them. Reach goes from a symbol to the CUs whose
knowledge the request needs. Parsing goes back, and is the only thing that adds
references.

```
the request's symbol ── reach (via headers) ──▶ CUs whose knowledge must be current
                                                         │ out of date?
references at positions ◀──────── parse ──────── the CUs to build
```

A header takes part in three ways. Reach walks through it, it is an input to every CU
that includes it, and references are positioned in it. It is never parsed on its own,
since without an includer it has no preprocessor context, so it has no knowledge of its
own.

## Problem Statement and Context

_In the context of_
- the in-memory reference table being the truth and the `.cx` file a snapshot
  (ADR-0014),
- entry refresh bringing the table up to date before each operation (ADR-0020),
- `lastParsedMtime` recording the modification time of the content that was parsed,
  disk file or held buffer, and staleness being an equality test against it
  (`doc/docs/08-algorithms.adoc`, "Dual Semantics"),

_facing the fact that_
- what is parsed when is decided by several mechanisms, each with its own test: Pass 1
  reparses edited CUs, Pass 2 the includers of edited headers, Pass 3 unparsed siblings
  (`fileNumberIsStale()` and `fileNeedsParsing()`), and the completeness parse counts CUs
  with no `lastParsedMtime`. It is hard to say which test runs when, and what it means,
- each mechanism is a *trigger*: it adds files to parse when it believes something
  changed. A missing trigger gives a wrong answer. A header changed on disk triggers
  nothing, so its other includers keep references to the old content
  (`tests/test_shared_header_changed_on_disk`),
- a zero `lastParsedMtime` means both "never parsed" and "forced stale"
  (`markPreloadedFilesAsAncient()`, `markAllCompilationUnitsStale()`), and some readers
  test for zero while others rely on zero being older than any real time,
- the file table records which version of a file was seen, but never when the knowledge
  was recorded,
- Pass 2 strips a stale header's references and then reparses at most
  `MAX_CUS_TO_REPARSE` (128) of its includers, so a header with more includers loses the
  references at header positions that only the remaining includers contribute, e.g.
  behind an `#ifdef` only they take,
- 128 is a judgement between waiting and correctness, not a measured limit,

_we propose to_
- treat a CU's knowledge as the **output of a rule** whose inputs are the CU, every file
  in its include closure, and the project config (the Makefile, in `make` terms),
- record a **knowledge time** per CU, when the knowledge was recorded, separate from the
  **change time** of an input, which is its modification time on disk or, when the
  client holds a buffer for it, the buffer's. A held buffer wins, as today,
- let **one predicate** decide what needs parsing: a CU has no knowledge, or one of its
  inputs changed after its knowledge time. "No knowledge" is the only special value. A
  config change makes every CU out of date because the config is an input,
- let **policy** decide what is parsed now: the request's goal. An operation on a
  position needs the symbol's reach. An operation on a name (search, unused globals,
  push by name) needs the whole project, since the name can be anywhere,
- replace the CU count with a **time budget**. What fits in it is parsed without asking.
  Above it, browsing asks with an estimate, e.g. "Parsed 128 of 400 CUs. Completing
  would take about 45 seconds more?", and remembers a "no" for the session,
- let an operation that edits code, like a rename, or that works on a name, bring its
  goal up to date before it runs: silently within the time budget, and above it only
  after the completeness question. A "no" cancels the operation and is not remembered.
  The question counts out-of-date knowledge of every kind, not only CUs never parsed,
- treat **the references positioned in a header** as all or nothing until ADR-0033 is in
  place: either all its includers are parsed again, or none is and their knowledge is
  left out of date. The references are not stripped before that is certain,

_disregarding the fact that_
- a file restored with an older modification time (`cp -p`, `tar`, `rsync -t`) is not
  noticed, as in `make`. Today's equality test notices it. This is a known deficiency,
- knowledge recorded from a held buffer whose edits are then thrown away is newer than
  the disk file, and would look current. Such knowledge is never written to the snapshot,
  as `markPreloadedFilesAsAncient()` arranges today, and is forgotten when the buffer is
  dropped,
- the filesystem stamps modification times from a coarse clock that lags the system
  clock, so a write just after a parse can get a time before it. The knowledge time is
  rounded down a tick, accepting an occasional extra parse,
- a "no" leaves out-of-date knowledge for the rest of the session, until an operation
  that goes through the completeness question completes it. When the "no" should be forgotten is left for later,

_because_
- `make` is a model every C programmer already knows, and it says in one sentence what
  is parsed and why,
- skipping what is provably current errs towards an extra parse, where a missing trigger
  errs towards a wrong answer,
- the project config fits the model as the Makefile does, which removes the forced-stale
  marker and with it the double meaning of zero,
- the time budget is the unit the judgement is about. A project of this repository's
  size never sees the question, and on a large one the user chooses between waiting and
  a partial answer, and is told which one they got.

## Decision Outcome

A CU's knowledge is out of date when it has none, or when the CU, a file it includes or
the project config changed after the knowledge was recorded. That is the only freshness
test. What is brought up to date is the request's goal, the symbol's reach or for a
name the whole project, within a time budget. Above the budget browsing asks and
remembers a "no" for the session, while an operation that edits code or works on a name
asks every time and is cancelled by a "no".

## Consequences and Risks

**Benefits:**
- One freshness test replaces Pass 1, 2 and 3, `fileNumberIsStale()`, `fileNeedsParsing()`
  and the completeness count. The passes become an order of work, not separate rules.
- A header changed on disk makes its includers out of date like an edited one, which
  fixes `test_shared_header_changed_on_disk`.
- The cursor parse can reuse the request CU's references when its knowledge is current
  (backlog §0.1).
- The zero `lastParsedMtime` loses its second meaning.

**Risks:**
- *A new field.* The knowledge time is persisted in the snapshot beside, or instead of,
  today's content modification time.
- *Include closures must be known before parsing.* The include references in the table
  give them, and the lightweight scan gives them for CUs never parsed. A CU whose
  closure is incomplete may be judged current when it is not.
- *Checking inputs costs a `stat` per file*, per request, for the files the goal
  reaches. That should be measured on a large project before it is optimized.
- *Docs describe the old model.* The "Dual Semantics" and "Change Detection" parts of
  `doc/docs/08-algorithms.adoc` and the Staleness entry in `doc/docs/06-principles.adoc`
  are rewritten.

## Considered Options

- **Rules like `make`, knowledge time per CU** — proposed.
- **Keep the triggers and add one for headers changed on disk.** Smallest change, but
  adds a fourth mechanism to the three that are already hard to follow, and the next
  missing trigger is a wrong answer again.
- **Header-side modification time**, set when the header is read. Pass 1 reads an edited
  header before Pass 2 looks at it, so the header would look current and its other
  includers would be skipped. Setting it only when it is zero collides with zero meaning
  "forced stale".
- **Per-input modification times, compared for equality.** Catches the restored older
  file, but needs state for every CU-header pair. Possible later, if the deficiency
  matters.

## Origin

Discussed on the Linux machine, session `76735541`, 2026-10-03. A spike for backlog item
`behind-the-back` found that the failing case is a header shared by two CUs and changed
on disk (`tests/test_shared_header_changed_on_disk`). Trying to make Pass 2 compare
headers with their disk time ran into the double meaning of a zero `lastParsedMtime`,
and the question of what a header's time could mean led to the `make` model.

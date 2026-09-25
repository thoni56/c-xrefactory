# Project identity is the root path, not the name

Date: 2026-09-25

## Status

Proposed

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

_In the context of_
- one server serving one project, bound once and immutable afterwards
  (link:0021-single-project-server.md[ADR-0021]),
- auto-discovery finding the project from the request file, so the root is always
  known (link:0005-automatically-find-config-files.md[ADR-0005]),
- the snapshot already living at `<project root>/.c-xref/db` — keyed by path, not by
  name (link:0028-remove-snapshot-location-and-partition-options.md[ADR-0028]),
- the `[section]` name in `.c-xrefrc` having been chosen precisely *because* a path
  does not travel between machines,

_facing the fact that_
- the question the server actually asks is not "what is this project called?" but
  "is this file inside the tree I am bound to?" — a question about location, which a
  name cannot answer,
- `handleProject()` (`src/cxref.c:1805`) already holds both comparisons side by side:
  a root prefix test against `lockedProjectRoot` when the project was auto-detected,
  and a `strcmp` of config section names when it was not,
- two checkouts of one repository carry identical `.c-xrefrc` files and are therefore
  indistinguishable by name and trivially distinguishable by path,
- `handleProject()` never reaches either comparison in practice: its first statement is
  `if (options.project != NULL)`, so the `-p` the client sends on *every* request makes
  the server echo the name back as `PPC_SET_INFO` without looking at the file,
- `PPC_PROJECT_MISMATCH` and its client prompt — "Switch to this file's project?
  (Server will restart)", `editors/emacs/c-xref.el:2366` — are fully implemented and
  effectively unreachable,
- coverage says the name branch is nearly dead already:
  `applyConventionBasedDatabasePath()` is entered 679 times, leaves by the
  not-auto-detected path 9 times, and reaches the convention body 670 times, so
  auto-detection — and with it the root comparison — is the live path,
- `options.xrefrc` is SESSION tier (`src/options.h:73`), consumed at startup to locate
  the config (`src/options.c:1028`) and cleared before
  `applyConventionBasedDatabasePath()` runs. Its guard there has **zero** coverage
  although `tests/Makefile.boilerplate:17` puts `-xrefrc .c-xrefrc` on every single
  system test. The option is supplied universally and has no effect at the point that
  tests for it,

_we decided to_
- make the running server's binding a **root path**, and treat the `[section]` name as
  what it is: a label for humans and a key for looking options up, never an identity,
- **remove `-xrefrc`**, which deletes the name-comparison branch rather than repairing
  it,
- **stop sending `-p` on every request** (the Setup Ladder's first rung), so
  `handleProject()` reaches the comparison at all,
- have `PPC_PROJECT_MISMATCH` compare roots by prefix, always, with no second branch,

_disregarding the fact that_
- a `.c-xrefrc` may carry several sections describing overlapping trees, and a path
  cannot tell those apart. The name still selects *which options apply*; it simply
  stops deciding *which server owns this file*,
- the root path must never be written anywhere that travels — a config, or a snapshot
  copied between machines,

_because_
- an identity used for binding has to be unique on this machine, and a name that must
  travel cannot be unique; they are answers to different questions and were only ever
  in conflict because one was asked to do both jobs,
- the constraint about travelling is already satisfied and costs nothing new: the
  snapshot stores absolute paths today (observed in `c-xref-replay/src/.c-xref/db`,
  2026-09-25), so it already does not travel.

## Decision Outcome

The server binds to a root path. A request whose file lies outside that root gets
`PPC_PROJECT_MISMATCH`, and the client asks whether to restart and switch — the
behaviour that was designed, implemented, and has been unreachable. `-xrefrc` and the
per-request `-p` both go. The `[section]` name remains in `.c-xrefrc` and keeps
travelling.

## Consequences and Risks

**Benefits:**
- Deletes a branch that cannot be made correct, instead of maintaining it.
- Re-enables `PPC_PROJECT_MISMATCH` and the "restart and switch" prompt.
- Removes the last reason for `applyConventionBasedDatabasePath()` to be conditional,
  which link:0028-remove-snapshot-location-and-partition-options.md[ADR-0028] also
  wants.
- Two checkouts of one repository — increasingly normal, and the case that exposed
  this — become distinguishable by construction.

**Risks:**
- *Removing `-p` has a measured blast radius.* `doc/backlog.md` records `-push` as ok
  with `-p` and FATAL without, and `-browse-next` as FATAL either way (Mac, session
  `7c731c79`, 2026-09-24). `-p` is one of three hatches past the gate in
  `answerEditorAction` (`src/cxref.c:1887`); this decision removes one, so the gate has
  to be settled at the same time.
- *`FATAL_ERROR` is the wrong severity at that gate.* A missing `-getproject` is a
  client protocol mistake, not a broken invariant, and killing the server turns it into
  a restart loop. `errorMessage` is what the convention asks for.
- *Fixtures assume `-xrefrc`.* Every system test passes it via
  `tests/Makefile.boilerplate:17`, together with `-p $(CURDIR)`.
- *Symlinked or differently-spelled roots* compare unequal as plain strings. A prefix
  test needs a normalised path, which `getRealFileName_static` already provides
  elsewhere.

## Considered Options

- **Path binds, name describes** — chosen.
- **Repair the name comparison**, by also storing the root for config-driven projects
  and comparing both. Keeps two branches that must agree, to preserve a path the
  coverage says is taken 9 times in 679, and that removing `-xrefrc` deletes anyway.
- **Make the name unique**, by requiring a distinct `[section]` per checkout. Pushes the
  problem onto the user, who must notice it, and breaks the moment a config is copied
  along with the tree it describes — which is how the identical pair arose.
- **Fix it in the client**, by having a manual project lock consult the server before
  deciding. Attempted 2026-09-25 (session `3af59d00`) and reverted: it papers over a
  server-side check that is switched off and comparing the wrong thing.

## Origin

Found on the Mac (session `3af59d00`, 2026-09-25), client and server both at
`f2718ee6`, while replaying a refactoring in a second checkout of this repository.
Locked to project `c-xrefactory`, opening `src/main.c` in the other worktree and
pushing gave `Parsing 102 sibling compilation units... 0 remaining`, `Project:
c-xrefactory`, and then a client crash — rather than the prompt that exists for exactly
this situation.

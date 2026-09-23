# Batch prime as a request

Date: 2026-09-23 (first proposed 2026-06-14, as CreateMode)

## Status

Proposed

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

_In the context of_
- the on-demand server architecture (ADR-0014, ADR-0022) having removed the
  *mandatory* `-create` step — the first session builds references on demand and
  the `.cx` snapshot warms later starts,
- a deliberate, whole-project parse still being wanted: priming a large project
  before a long session, and giving a test or a measurement a fully built state,
- the legacy `XrefMode` (`-create`/`-update`) having been the only thing that
  offered "parse everything up front",

_facing the fact that_
- the server can already parse everything — `callServer` runs a completeness
  parse over every unparsed compilation unit once the user answers the
  "N of M compilation units need reparsing" question — but there is no way to
  ask for it, only to provoke it,
- `test_ffmpeg` and `test_systemd` do provoke it, by searching for
  `non_existant_to_force_parse_all` and answering `-continue`; a search designed
  to match nothing is what a missing request looks like when a caller needs its
  side effect,
- the original proposal bundled a third need, validating a project config, with
  this one, and they pull in opposite directions (see below),

_we decided to_
- add **`-parse-all`**, a server operation that parses every compilation unit the
  project scan found and has not parsed yet, then returns like any other request,
- let whatever wants a whole-project parse ask for it: a test fixture, a
  measurement, a CI step through the driver, the client from a menu entry, or a
  one-shot invocation that sends the request to itself,
- introduce **no new parse logic**: the operation dispatches into the
  completeness parse that already exists,
- keep a standalone batch command as an option we are *not* taking now, and
  track config validation as the separate thing it is,

_disregarding the fact that_
- a standalone `c-xref -create`-style command is friendlier in a shell than a
  protocol conversation, and CI lives in a shell,

_because_
- the server has one parse path now, and the way to keep it one is to add
  sentences it understands rather than callers that drive it their own way,
- a mode accrues its own initialisation and drifts from the server's — the
  original proposal named that risk itself — while a request cannot drift from a
  path it *is*,
- a command, if we later want one, is a shell around this request: start,
  `-getproject`, `-parse-all`, save, exit. Choosing the request first means that
  command has nothing of its own to get wrong.

## Decision Outcome

`-parse-all` parses every compilation unit that the project scan discovered and
that has not been parsed, writes nothing new of its own, and leaves the snapshot
to be saved as usual. It asks no question: it *is* the answer to the question the
server asks when an operation needs the whole project.

It is a request like any other, so it carries no project state, needs no separate
initialisation, and reaches the same code as an ordinary "parse all before
searching?" confirmation.

## Consequences and Risks

**Benefits:**
- The two large fixtures stop faking a prime with a search that cannot match.
- Priming becomes available to anything that speaks the protocol, including the
  client, without a second entry point into the program.
- Nothing new can diverge from the server's parsing, because nothing new parses.

**Risks:**
- *A request is not a shell command.* CI and a user at a prompt want to type
  something. Until a command exists they need the driver, which is one more moving
  part in a pipeline. The mitigation is that the command is small and can be added
  later without changing this decision.

## Considered Options

- **`-parse-all` as a server operation** — chosen, above.
- **CreateMode: a standalone `c-xref -create` batch mode** — the original
  proposal here, now judged the worse fit.
- **Leave it alone** — keep provoking the completeness parse with a search that
  matches nothing.

### CreateMode

A batch mode that starts as `-server` does, parses everything discoverable,
writes the snapshot and exits, taking no options because the discovered
`.c-xrefrc` is authoritative.

Its merit is real and is the reason it is kept on the table: no client required,
so CI, a config check, and a test `prepare` step can all be a single command.

Its cost is that it puts the primitive inside a mode. We are in the middle of
deleting a mode precisely because a second way into the parser is what lets
behaviour drift; a mode that "reuses server internals" is one commit away from
not reusing them, and the original proposal listed that drift as its first risk.
The request is the same capability with nowhere to drift to, and if the shell
command is wanted afterwards it is a wrapper over it.

It was also argued for on the strength of unblocking XrefMode removal. That
turned out to be untrue: removing the `-create` primes from the system tests
showed the server builds the same state on demand, and 21 of 23 primes went
without any batch mode.

### Leave it alone

`-search=non_existant_to_force_parse_all` followed by `-continue` does work, and
has been working. It fails as documentation: nothing about it says "index this
project", the name has to carry the intent, and it breaks the day a symbol by
that name exists. It also cannot be offered to a user.

## Config validation is a different need

The original proposal listed "validating a project config" beside priming, and
they do not belong together. Priming wants to parse *everything*. Validation
wants to parse *as little as it can get away with* to answer "does this
configuration resolve?" — which includes are not found, which defines leave a
branch unparsed. That it may end up parsing much of the project is an
implementation consequence, not the need.

The reporting half is tracked as its own work: re-keying the 19
`options.mode != ServerMode` guards in `yylex.c` onto a report-errors flag, so
the diagnostics server mode currently swallows can be asked for by an operation
and rendered by the client. See the roadmap, "Report what did not parse".

## Related Decisions

- **ADR-0014**: On-demand parsing architecture — removed the *mandatory*
  `-create`; this adds an *optional* way to ask for the same work.
- **ADR-0021**: Single-project server — the request inherits the one-project lock.
- **ADR-0022**: Lightweight file structure scanning — the discovery that decides
  what "everything" means.

## History

- **2026-06-14** — proposed as CreateMode, a standalone batch mode, justified by
  priming, config validation, and unblocking XrefMode removal.
- **2026-09-19** — the unblock justification was withdrawn: the system tests
  showed the server builds that state on demand, so XrefMode removal does not
  wait for a batch mode. What was left was priming, a CI entry point, and the
  observation that server mode stays quiet about parse errors. The update asked
  whether an operation would do instead of a mode.
- **2026-09-23** — rewritten around that question's answer. The request is the
  primitive, the mode becomes an option not taken, and config validation is
  separated out as a different need.

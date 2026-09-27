# Opening a project and asking about a file are separate requests

Date: 2026-09-27

## Status

Proposed

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

_In the context of_
- one server serving one project, bound once and immutable afterwards
  (link:0021-single-project-server.md[ADR-0021]),
- the project being identified by its root path
  (link:0029-project-identity-is-the-root-path.md[ADR-0029]),
- the Setup Ladder in `doc/docs/10-roadmap.adoc`, whose rule is "nothing before it's
  known",

_facing the fact that_
- `-getproject` is a command on an unbound server and a query on a bound one. The first
  call discovers the project, locks it, loads the snapshot and marks the context
  initialised. Later calls only answer whether the file is in the locked project
  (`handleProject()`, `src/cxref.c`). The request does not say which of the two it is,
- the client sends `-getproject` before every operation, so every operation carries a
  hidden "and bind, if not already",
- the answer folds two kinds of file into one `<set-info>`: a file the project owns, and
  a file it only reaches through an include path (`fileIsUnderIncludePaths()`), which
  `tests/test_getproject_unknown_cu_under_include_path` records as too generous,
- process start guesses a project from the process's working directory and reads its
  config with errors switched off (`mainTaskEntryInitialisations()`, `src/startup.c`),
  before any request has named a file. Nothing said at that point reaches the client,
  since the answer file is only opened per request,

_we decided to_
- split `-getproject` into two requests:
  **open project** (`-openproject <file>` or similar), a command that is valid once,
  binds to the project a given file belongs to, and reports in its answer what went
  wrong: no project, a different project already open, a config problem;
  and **is this file in the project**, a query without side effects,
- let the server answer the query, from the root, the declared source directories and
  the include paths, since it is the only party that knows all three and the client
  should stay thin,
- let the query tell three kinds of file apart: *own* (under the root or a declared
  source directory), *visible* (reached through an include path) and *outside*,
- remove the guess at process start, which also leaked the defines of the project the
  server happened to be started in into the one it was opened on,
- open the project with a request, not on the command line, so the Setup Ladder keeps
  its three steps: process start, open project, every request after,
- answer any other request before the project is open with an error in that request's
  answer, not a fatal error,

_disregarding the fact that_
- a client that is not updated still sends `-getproject` before every operation, so the
  old request has to keep working, or be refused with a message, for a while,

_because_
- a request that is a command in one state and a query in another cannot be understood
  from the request alone, and the gate in `answerEditorAction()` exists only because
  binding can happen as a side effect of any `-getproject`,
- the binding is the one place where config errors have to be reported, and a command
  with an answer is where that works, as the `-refs`/`-refnum` warning showed
  (`811c9010`, reported when the project is locked, not when the config is parsed),
- the protocol only has answers to requests. The answer file is opened per request, so
  whatever the server finds out at process start has nowhere to go.

## Decision Outcome

Opening a project is a request of its own, `-openproject`, made once, with an answer.
Asking whether a file belongs to the project is another request, answered by the server
as own, visible or outside. The client starts a server with no project, opens the
project with the first request, asks about each new file, and on *outside* asks the
user whether to switch, which restarts the server. A request before the project is open
gets an error in its answer.

## Consequences and Risks

**Benefits:**
- Each request means one thing, whatever state the server is in.
- The Setup Ladder's second step gets a single entry point, and the gate becomes
  "nothing before open project".
- *Visible* files can be told apart from *own* ones, which is what
  `doc/docs/11-planned-features.adoc` (Semantic Read-only Files) needs.
- Process start no longer does project work.

**Risks:**
- *Protocol change on both sides.* The Emacs client and the tests that use `-getproject`
  change together. The old request needs a transition.
- *The guess at process start was not always overridden.* A server started in one
  project and opened on another parsed the second with the first one's `-D` defines,
  since process start had already made them macros
  (`tests/test_server_started_in_another_project`). The guess is removed.

## Considered Options

- **Two requests** — chosen.
- **Keep `-getproject` as both**, and document the two meanings. Keeps the hidden bind
  in every operation and the gate that guards it.
- **Let the client decide membership**, from the root that open project returns. Needs
  the client to know the declared source directories and the include paths, i.e. to
  duplicate the meaning of the config.
- **Bind on the command line only**, with no open request. Has no answer to report
  problems in, and the "no project, create one?" flow would need a server to ask.
- **Give the file on the command line, and report in the first request.** Lets the
  server start its work before the first request, but the client sends that request
  right after starting the server, so little is gained, and the result still has to wait
  for a request to be reported in. Opening in a request says the same thing with one
  mechanism instead of two.

## Origin

Discussed on WSL, session `76735541`, 2026-09-27, after a warning about ignored config
options turned out to have no request to go with when raised at process start.

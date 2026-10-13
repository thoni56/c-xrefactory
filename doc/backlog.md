# Backlog

What to work on next, and in what order. As of 2026-10-09.

The *descriptions* live in the guidebook (`doc/docs/`), in the `.suspended` notes and in
the ADRs. This file carries only the **order** and the **dependencies**, which are
recorded nowhere else. Keep it thin: one line per item plus a pointer. If an item needs a
paragraph, the paragraph belongs in its chapter, not here.

A few items have no repo home yet — they came out of a session and were never written
down. Those lines say where to start instead of pointing, and should shrink to a pointer
once the work gets a chapter, a test or a `.suspended` note.

Ordering principle: first the one item that gates the most rows, then the chains that unblock, then bugs and
features. Relative effort only — no dates (#noestimates).

Each item has a name, the `<a id>` before its title. A `Waits for:` line names what it
waits for, and `Goes with:` what it belongs with. The docs build draws the dependency
graph from them on every push, linked at the end of this file, and `utils/backlog_graph
--check`, run with the tests, fails if a name is unknown.

## 1. The foundational one

- (#3) <a id="partition-options"></a>**Partition `options` by lifetime** — Session / Project / Request, one owner each, and
   a pure `parseCommandLine()`. `doc/docs/17-major-codebase-improvements.adoc` §17.4. The
   roadmap's Setup Ladder says it gates the rows below it. Largest single item here.
   Begin with step 2 of that section, extracting side effects, in the order it gives.
   Steps 2.a and 2.b take `test_cold_start_interrogates_compiler_once` off suspension.
   When it moves `ProjectConfig` off being a file-static singleton (`src/startup.c:42`,
   `static ProjectConfig projectConfig = {0};`), give it a `makeProjectConfig()` that
   returns `{.sourceDirs = NULL, .optionSets = makeOptionSets()}`. Today both fields are
   emptied by code that frees first — `freeStringList(projectConfig.sourceDirs)` and
   `resetOptionSets()` — which is only safe because static storage zeroes them; an
   automatic `ProjectConfig` declared without `= {0}` would free garbage. A constructor
   makes "born empty" one expression instead of every declaration site remembering, and
   gives `makeOptionSets()` its first production caller.
   A small first step: the global `inputFileName` is two things. It is the request file
   for the cursor parse (`processFile`), for `-get` and for the refactorings
   (`refactory.c`), and the file to parse when a CU is built (`parseFileWithFullInit`) or
   a macro expanded (`singlePass`). The second becomes a parameter to
   `initializeFileProcessing()`, the first goes to the `Request` (§17.4), or to the
   `RefactoringRequest` for the refactorings.
   Today a CU built during entry refresh resets the request's options: the fast path of
   `initializeFileProcessing()` copies `savedOptions`, taken at server start, over
   `options`. `parseFileWithFullInit()` keeps only `cursorOffset`, `noErrors`,
   `serverOperation` and, as a stopgap, `dumpSelection` (`test_dump_after_a_rebuild`).
   Every other REQUEST option is lost when a rebuild comes before the dispatch.
- (#4) <a id="setup-ladder"></a>**The Setup Ladder, in this sequence**, each small once the partition lands
   (`doc/docs/10-roadmap.adoc`, Convergence: The Setup Ladder → Remaining):
   a. <a id="root-path-id"></a>**the server's project id is its root path** — ADR-0029. `lockedProject` becomes the
      detected project root and `lockedProjectRoot` goes, so `SET_INFO` and
      `PROJECT_MISMATCH` carry the path, not the `[section]` name. Test first:
      `test_project_lock`'s `expected` gets the path, then the server, then the other
      `expected` files with named sections (about 15). The client's half is in `e476e396`,
      its project is only what `-getproject` answers;
      Waits for: adr-0029
   b. <a id="client-stops-p"></a>**the client stops sending `-p` on every request** — ADR-0029: this is not tidying,
      it is what re-enables `PPC_PROJECT_MISMATCH`, since `handleProject()`
      (`src/cxref.c:1805`) returns on its first line when `options.project` is set and
      never looks at the request's file
      (`c-xref-send-data-to-process-and-dispatch`, `editors/emacs/c-xref.el:1922` — the
      roadmap says 1925). Since `4717f231` a tripwire in `answerEditorAction()` warns
      (log and bottom-warning) whenever `-p` would change the answer; in the server tests
      it fired only where `-p` stood in for `-getproject`, and those now send
      `-getproject`. What it still has to show is the client in daily use, which needs
      the tripwire on `stable` or Emacs pointed at the repo's build. Goes with ADR-0030's split: `-openproject` parses nothing, and the
      membership query may reparse a changed preload of a file in the project, but not
      of one outside it. Once `-p` stops, membership is answered for real, and today's rule (the root as a prefix, or under `-I`) is
      wrong both ways, so it waits for the project's extent;
      Waits for: adr-0029, root-path-id, project-extent
      Goes with: startup-command-line, adr-0030
   c. <a id="setup-into-openproject"></a>**project setup moves into the future `-openproject`** — discovery, config read, compiler
      interrogation, snapshot load;
      Waits for: partition-options, adr-0030
   d. <a id="openproject-tripwire"></a>**the assert that nothing before the future `-openproject` reads a project-scoped option** — the
      tripwire, as the disk-read assert was for Memory as Truth;
      Waits for: setup-into-openproject
   e. <a id="project-extent"></a>**what a project consists of** — ADR-0031 (accepted 2026-10-02). One definition of
      *own* for scan and membership, *visible* through the include graph, *outside* saying
      whether a project was found.
      Pinned by three suspended tests: `test_server_refuse_project_switch`,
      `test_getproject_listed_dir_outside_root`, `test_getproject_pruned_file`. The
      membership query must stop registering its file first, which also unsuspends
      `test_getproject_unknown_cu_under_include_path`. While there: a config listing `.`
      walks the root twice, since `.` normalizes with a trailing slash and the skip in
      `scanProjectStructure()` compares unnormalized strings; and the client's mismatch
      switch kills the server without `-exit`, losing everything since the last snapshot.
      Waits for: adr-0031

- (#5) <a id="drop-cxfi-refnum"></a>**Drop the `CXFI_REFNUM` record from the snapshot** — it is always written as 1. A safe
   format change: a snapshot of another format version is ignored like no snapshot and
   rewritten on exit, so it costs one cold start, and a bump of its own is fine. `m`
   (`lastFullUpdateMtime`) is only written and read back, and goes the same way. So may
   `i` (`isArgument`), if it means nothing for a scanned project, and the usage
   numbering, if `UsageMacroExpandingCU`, which nothing produces, goes.

## 2. After XrefMode

- (#6) <a id="report-errors-flag"></a>**Re-key the 19 `options.mode != ServerMode` guards in `src/yylex.c`** onto a
   report-errors flag, and stop `formatMessage()` (`src/commons.c`) dropping the position
   in server mode. Roadmap, "Report what did not parse". Doubles as the server half of
   the Indexing Log Buffer item. Scope reporting to the operation's own parse, not the
   session-wide `-errors`. Restore `tests/test_multipass`'s error-position assertion in
   the same change: it was dropped when that test moved to the server, because the
   position is what server mode throws away. With XrefMode gone the guards never
   fire, so today the server reports nothing they guard.
- (#7) <a id="startup-command-line"></a>**Decide, and enforce, what the startup command line may carry** — *no repo home yet.*
   Bigger than `c-xref file.c`, and it needs code, not just a decision. Once XrefMode is
   gone the server takes no file at startup: every request names its own, and the same
   parser serves both phases, so the question becomes which options are process-scoped at
   all. The Setup Ladder's step 1 answers it in principle; this makes it concrete.
   - Genuinely startup, on the evidence of what the client sends and what `main()` reads
     before anything else: the mode; the transport (`-crlfconversion`, `-crconversion`,
     `-o <answerfile>` — `editors/emacs/c-xref.el:1615`); logging
     (`-log=`, `-debug`, `-trace`, `-info`, `-errors`, `-warnings`, `-infos`, scanned in
     `main()` before the mode is even known); and `-statistics`.
   - Everything project-scoped — `-p`, `-I`, `-D`, `-optinclude` — belongs to
     the config and binds at the future `-openproject`, not here.
   - **`-exactpositionresolve` has no strategy.** It is real: it changes link names
     (`src/semact.c`) and goes into the snapshot's check number (`src/cxfile.c`). Since it
     changes what a symbol is, it is probably project-scoped, although `options.h` marks it
     REQUEST. Decide whether it belongs in the config or goes. Item 14 waits on the same
     question.
   - **`-lsp` is a third mode and should be one.** `want_lsp_server()` (`src/lsp.c`) scans
     argv in `main()` and returns before `mainTaskEntryInitialisations()`, so it is a mode
     in behaviour but not in `options.mode`. If "state a mode" is the rule, it should say
     so the same way `-server` does.
   - The code: `startup.c:807` processes the command line with
     `PROCESS_FILE_ARGUMENTS_YES`, which is why `c-xref -server file.c` silently schedules
     that file. Flip it to `NO`, now that XrefMode is gone. But `NO` only *ignores*: `matched =
     true` sits outside the `if` in `options.c`, so a bare word vanishes without a word.
     Rejecting it means reporting there. `c-xref file.c` already answers "No mode given",
     pinned by `tests/test_options_mode_required`.
   - `.c-xrefrc` is also read with `YES` (`startup.c:446`), for the source directories
     listed in it. Do not assume that list is legacy. What dies with `-p` is the
     *registry*: several `[project]` sections, each covering directories, one selected by
     name. Saying "these directories are also part of my project" is a different need and
     it survives — a project using a library you have the source for, which you want to
     navigate into, is not covered by `-I`, which only tells the preprocessor where to
     look. Auto-discovery gives you the tree under the config; anything outside it still
     has to be named. Likely home: the config, or the machine-specific sibling, since an
     external source tree is usually a local path.
- (#8) <a id="finish-after-xrefmode"></a>**Finish after XrefMode**, in these pieces:
   a. <a id="mode-checks"></a>**fold the `options.mode` checks outside `src/yylex.c`** — about 33,
      in semact, options, cxfile, commons, startup, cxref, refactory, parsing and main.
      They are constant wherever the mode is known, since LSP sets `ServerMode` too
      (`src/parsing.c`). The ones that report a missing mode stay. The 30 in `yylex.c`
      are item 6's. Among them `commons.c`'s `options.cxrefProtocol && options.mode !=
      ServerMode`, which is never true: only `-server` sets `cxrefProtocol`, together
      with the mode;
      Goes with: report-errors-flag
- (#9) <a id="remove-bridge"></a>**Remove the `parseBufferUsingServer` bridge** — §17.3; 9 refactoring call sites
   re-entering `callServer`. Independent of the rest of this section (it asserts
   `ServerMode`), but it is the last divergent parse path.

## 3. Correctness, by (quiet × cheap)

- (#10) <a id="refactoring-refusals"></a>**What a refactoring refuses, spelled out and tested** —
    *no repo home yet.* ADR-0033 lets ghosts stay visible until the sweep, and relies on
    refactorings refusing them. Today rename and the parameter refactorings compare only
    the name at each stored position (`precheckThatSymbolRefsCorresponds`,
    `validateReferencePointsToFunction`, `src/refactory.c`). A ghost whose position still
    reads the name passes: a declaration in a branch no longer taken, or the same name
    belonging to another symbol after a change on disk, which rename then edits. Rename
    of an included file has no precheck, and a marker not on an `#include` is skipped
    silently. Only rename and delete parameter have refusal tests
    (`test_rename_aborts_on_name_mismatch`, `test_parameter_delete_aborts_on_name_mismatch`),
    and both fake the ghost by editing the snapshot; add and move parameter have none.
    Write the contract down in the docs — per refactoring, what is refused, what is
    asked, what is edited anyway — and pin it with tests that make their ghosts for real,
    by changing a shared header on disk. Ghosts can be made that way today, so this does
    not wait for `sweep`. The message "try re-saving the file" no longer fits. Found by
    reading the code (Mac, session `05984816`, 2026-10-06), not tested.
    Goes with: behind-the-back, sweep
- (#11) <a id="behind-the-back"></a>**A header changed on disk, which the editor does not hold, keeps its old references** —
    it is stripped only when the request file is it or includes it directly, and all its
    includers are built. An includer built later adds the new references beside the old
    ones (ghosts). From reading the code (WSL, session `d25255b8`, 2026-10-11 and
    2026-10-12), not tested.
    Waits for: request-target
    Goes with: sweep
- (#13) <a id="token-pasting-trio"></a>**Token pasting trio**, in this order — the later two assume the first
    (`doc/docs/18-known-bugs.adoc`): `tests/test_token_pasting_numbers` (changes the
    passing `test_collate_const_suffix_pasting`) → `tests/test_token_pasting_float` →
    `tests/test_collate_hex_prefix_pasting` (touches how every number is lexed).
- (#14) <a id="header-static-link-names"></a>**Header static: prototype and definition get different link names** —
    `setStaticFunctionLinkName` (`src/semact.c`); renames break the build today.
    `tests/test_static_declared_and_defined_in_header/.suspended` has the
    `exactPositionResolve` question to settle first, so it starts as a decision.
    Waits for: startup-command-line
- (#15) <a id="correctness-by-cost"></a>**The other correctness bugs**, Latin-1 first; the order of
    the rest is not known. Each has a `.suspended` note or a `18-known-bugs.adoc` entry,
    or says it has no repo home yet:
   a. <a id="latin1-swallows"></a>**A Latin-1 character swallows the characters after it** —
      `tests/test_latin1_character_does_not_swallow_what_follows/.suspended`. Quiet and
      cheap: legacy C with Latin-1 comments loses a `*/` and the rest of the file
      silently. Fixed by checking UTF-8 continuation bytes, not by bringing `-encoding`
      back.
   b. <a id="relative-include-path"></a>**A relative `-I` in `.c-xrefrc` resolves against the working directory of the server process** —
      *no repo home yet* (`doc/c-xrefrc.man`), while a directory line resolves against
      the config's directory and `-prune` against the root. With the config in
      `tests/test_ffmpeg/ffmpeg`, `-Iffmpeg` found nothing and the absolute path did (WSL,
      session `d25255b8`, 2026-10-09). Probably the config's general path rule that
      ADR-0031 proposes for `-prune` should cover `-I` too. The same config had
      `-prune ffmpeg/compat/atomics`, which with `ffmpeg/` as root names a directory that
      does not exist, silently.
      Goes with: adr-0031
   c. <a id="unknown-cu-under-include"></a>**Any file under an `-I` directory counts as project** —
      `tests/test_getproject_unknown_cu_under_include_path`.
      Goes with: project-extent
   d. <a id="no-project-twice"></a>**`-getproject` for a file outside any project answers `<no-project-found>` twice** —
      *no repo home yet*. `tests/test_no_project_writes_no_snapshot` shows it, and its
      `expected` keeps both for now, so that test is only about the snapshot. Seen on
      WSL, 2026-09-27.
   e. <a id="warm-start-fatal"></a>**A warm start trips `FATAL cxref.c:1494: 'browsingStack.top' is not true`** —
      *no repo home yet*. Running `tests/test_browsing_pop_needs_refresh` a second time
      from the first run's `.c-xref` snapshot dies on the assert, so the same requests
      crash a server in the editor. Seen on WSL, 2026-09-25. Tests start cold now, so the
      warm start has to be built inside a test; see that test's `.suspended`.
   g. <a id="unused-yacc-statics"></a>**GlobalUnused gives a false positive for statics in `.y` files** —
      `18-known-bugs.adoc`.
   h. <a id="push-by-name"></a>**Push by name when a name has several bindings** —
      `tests/test_browsing_push_by_name`. Needs the behaviour decided first.
   i. <a id="truncated-reference-set"></a>**A refactoring can see a truncated reference set** —
      `18-known-bugs.adoc`. Severe where it bites, rewriting 13 of 25 occurrences on
      ffmpeg, but it needs a header included by more than ~130 CUs, and c-xrefactory has
      79 with a worst fan-in of 39, so it cannot happen here. That, and not its severity,
      is why it sits here.
      Goes with: time-budget, refactoring-refusals
   j. <a id="lsp-position-encoding"></a>**The LSP server announces `positionEncoding: "utf-8"` but counts code points** —
      *no repo home yet* (`src/lsp_handler.c`). In LSP 3.17 "utf-8" means columns in
      bytes, while the encoding collapse makes every server column a character column, so
      on a line with non-ASCII the two disagree. The honest value is "utf-32"; the
      protocol also lets a server pick only an encoding the client offered, with
      "utf-16" as the default, which agrees with code points outside the astral planes.
      From reading the code (Mac, session `36694da1`, 2026-10-02), not tested.
      It is also in the wrong place: at the top level of the `initialize` response, where
      the protocol has it in `result.capabilities`, so a client ignores it and assumes
      "utf-16". `test_lsp_simple_lifecycle` pins it there (WSL, session `d25255b8`,
      2026-10-10).
   k. <a id="lsp-input-ends"></a>**The LSP server exits as a JSON parse error when its input ends** —
      *no repo home yet* (`src/lsp.c`). At end of file the header reader returns a length
      of 0, the empty content does not parse, and the exit code is
      `LSP_RETURN_ERROR_JSON_PARSE_ERROR`. The protocol does not say what an ending input
      means, but a client that died without `exit` should not look like a parse error.
      From reading the code (WSL, session `d25255b8`, 2026-10-10), not tested.

## 4. Performance, in strict dependency order

Roadmap → Optimization has the baseline: a cold-start PUSH on ffmpeg, measured in March
2026 and not since.

- (#16) <a id="header-filtered-siblings"></a>**Header-filtered sibling parsing** — for an operation on a symbol, build
    only the CUs that reach a file declaring it: for `AudioFIRContext` from `af_afir.c` on
    ffmpeg that is 2, not the 1567 collected today. What to build is collected before the
    operation, and the symbol is known only from the cursor parse. Looking the cursor up
    in the table (`cursor-parse-reuse`) gives the symbol first, when the request CU is
    current. It narrows what `request-target` widens: the reach of the symbol, not of the
    request file's closure.
    Goes with: cursor-parse-reuse, request-target
- (#17) <a id="scan-angle-includes"></a>**Extend the lightweight scan to `<...>` includes** — blocked on header-filtered
    sibling parsing, or ubiquitous system headers drag in nearly every CU. Also removes
    the cold-start double progress bar.
    Waits for: header-filtered-siblings
    Goes with: project-extent
- (#18) <a id="lexing-cache"></a>**Lexing cache re-introduction** — present in the original codebase, lost in
    restructuring.
    Waits for: scan-angle-includes
- (#19) <a id="parallel-parsing"></a>**Parallel parsing** — last, and only if the three above leave `avcodec.h` (614 CUs) or
    `internal.h` (1171 CUs) unacceptable. Global mutable parser state, a single CX arena
    and a shared file table are the obstacles.
    Waits for: lexing-cache

## 5. Features, by readiness

- (#20) <a id="remove-references-stopgap"></a>**Take the client's "Remove References and Restart Server" back out** — it landed
    2026-09-24 as a deliberate stopgap (`c-xref-project-remove-references-and-restart`,
    `editors/emacs/c-xref.el`), because discarding the database is the standing remedy for
    two unrelated symptoms: shadow occurrences after behind-the-back disk changes, and a
    stale snapshot outliving a visibility fix (both in `doc/docs/18-known-bugs.adoc`). The
    real work is making the database not need discarding — behind-the-back detection and
    entry refresh are probably most of it,
    and item 22 dissolves another part. Remove the entry when they land, and check the
    symptoms are gone rather than assuming it. Check a third case too: with `sweep`,
    ghosts in a header are removed only when every CU reaching it has been parsed again,
    and generations are not in the snapshot (ADR-0033). After a "no" to the completeness
    question, ghosts in a header like ffmpeg's `avcodec.h` (614 CUs) are written at exit
    and come back with the snapshot, session after session.
    Waits for: behind-the-back
    Goes with: sweep, time-budget
    Goes with: index-based-sessions

- (#21) <a id="move-function-comment-prompt"></a>**Move Function comment y/n prompt** — replaces the `c-xref-comments-moving-level`
    customization; collapse `CommentMovingMode` to a bool and stop the backward walk at a
    blank line (`src/options.h`, `src/move_function.c`). The TDD scaffolding already
    landed: four `tests/test_move_function_*comment*` tests; sweep
    `-commentmovinglevel=6` → `=1` in the three "with…" `commands.input` files and add
    `test_move_function_stops_at_blank_line` in the same change. Most shovel-ready item
    here.
- (#22) <a id="index-based-sessions"></a>**Index-based sessions** — dissolves stale-POP *and* lets an answer grow while you
    work. Roadmap → Memory as Truth → Remaining. Depends only on entry refresh, which is
    done.
    Waits for: entry-refresh
- (#23) <a id="indexing-log-buffer"></a>**Indexing Log Buffer** — Option A (`PPC_LOG` + a silent `*c-xref-log*` buffer),
    `doc/docs/11-planned-features.adoc`. Follows the report-errors item. The client half
    does not have to be invented: `c-xref-tags-dispatch` and its two helpers in
    `editors/emacs/c-xref.el` rendered exactly this for the `-create` log — a stream of
    PPC records into `*c-xref-log*`, severity faces, `file://` links made clickable — and
    are kept, uncalled, for that reason. The viewer commands and keymap below them are
    still bound; only the producer is gone. Whether they stay is decided here.
    Waits for: report-errors-flag
- (#24) <a id="retry-creating-request"></a>**Retry the request that created the project** — small, and it becomes first contact
    with every new project once auto-discovery is the only way in.
- (#25) <a id="lsp-tiers"></a>**LSP tiers 1–2** — code actions and `workspace/executeCommand` for extract, move
    function and the parameter refactorings. Stubs exist: `handle_code_action`,
    `handle_execute_command`, with `codeActionProvider` commented out in
    `src/lsp_handler.c`. Keep tier 3 (custom methods + per-editor extension code) small.
- (#26) <a id="move-function-next"></a>**Move Function next steps**, then **Delete Function**. Also **`-parse-all`, a request that parses every compilation
    unit the scan found and has not parsed** — what ADR-0024's CreateMode reduces to
    once the priming is a request rather than a mode; `test_ffmpeg` and
    `test_systemd` ask for it today by searching a name nobody defines and answering
    `-continue`. The concrete cost of not having it: a Delete Parameter on
    `saveReferencesToStore` rewrote `cxfile.c`, `cxfile.h` and `cxref.c` but left
    `src/cxfile.mock`, which declares the same function for the unit tests, so the
    unit build would not compile. Observed on the Mac, session `3af59d00`, 2026-09-25.
    Browsing now reaches a test CU through its mock
    (`tests/test_push_reaches_a_test_that_includes_only_the_mock`). Whether a refactoring
    does is not tested: a Delete Parameter on `knowledgeIsOutOfDate` said the parameter
    was used in `freshness.mock` after that use had been removed and saved, so it read
    older knowledge of the mock (WSL, session `d25255b8`, 2026-10-09). No test pins
    mock-reaching refactorings today. If it becomes something a user asks for, it needs to say how long
    it will take: `src/progress.c` already keeps `timeZero` and a monotonic clock,
    and the parse loops already count down, so the missing part is the division and
    a format that says "4 min left" rather than a remaining count. Decide the unit
    first — CUs are uneven enough (`avcodec.h`'s consumers against a 40-line `.c`)
    that a mean over them swings early and settles late, where lines or bytes are
    steadier and both are known before parsing starts.
    Also **unused-detection exclude patterns**, **browsing includes**,
    **semantic read-only files**, **rename handles `expect`**, **project-local
    config**, **Inline Function and Inline Macro**, **Make Static**, **Extract an Expression as a
    Function**, **Unify Parameter Names**, **Introduce Parameter Object**, **Inline Output
    Parameter**, **Finish Extract Variable**, **Inline Variable**, **Name and Define
    Constant**, **C11 `_Generic`** (`tests/test_parsing_generics`). All in
    `11-planned-features.adoc`.
- (#27) <a id="local-config-fragments"></a>**Local config fragments: the config names, the fragment binds** —
    ADR-0034, proposed. Decide it, with where the fragment lives and the syntax of a
    default. Then: expand defaults, report an unbound name, read the fragment before the
    environment, and ignore the personal options in a config, with a warning.
    Goes with: project-extent
- (#28) <a id="chapter-17-hygiene"></a>**Chapter 17 hygiene, opportunistically** — incremental `cxfile.c` cleanup, extract
    the macro expansion module, hashtab → hashlist, split the editor module, rename server
    operations, elisp recompiled and deleted on every build.
- (#29) <a id="dump-reference-database"></a>**Dump Reference Database** — `-dump=knowledge` answers from the
    in-memory table (`src/dump.c`), one record per fact. The selection names which facts,
    so a new kind of fact never changes the answer for an old one. Left are selections for
    references and includes. With those, the about 45 tests that throw the protocol away
    and read the snapshot written at exit with `cxref_reader`, mostly parsing and token
    pasting, can compare the answer like the others, and a parser test stops also testing
    the snapshot writer and a clean exit. Several selections in one request
    (`-dump=knowledge,references`) when a test asks for two. A client command, so the
    answer can be read in `*Messages*` without stopping the server. What it does not solve
    is realising you need the dump before the state is gone. A rename missed
    `clearPreloadedThisRequestFlags` in `src/editor.mock` (WSL, 2026-09-25), and the dump
    taken afterwards had the reference, so it could not say whether the server knew it at
    the time of the rename.

- (#30) <a id="compile-commands"></a>**Read `-I` and `-D` from `compile_commands.json`** —
    *no repo home yet.* Nothing reads it today. CMake and Meson write one, and `bear`
    makes one for Make. It decides whether a stranger's first project works without
    learning `.c-xrefrc` (section 8).
    Goes with: project-extent, local-config-fragments, partition-options

## 6. Developer tooling

- (#31) <a id="macos-watchers"></a>**Two causes of watchers not firing on macOS, both found** — *no repo home yet.*
    Kept because the symptoms are absence and noise, which is what gets
    re-investigated from scratch. Diagnosed on the Mac, session `4e8903dc`,
    2026-09-30.

    a. **`fs watcher error`, once per scan, forever.** Emacs writes a `.#<name>`
       lock symlink beside an unsaved buffer, and its target is
       `user@host.pid:boot`, not a path. watchexec follows symlinks by default, so
       the walk fails with `ENOENT` and reports the error every scan for as long as
       the buffer is dirty. Deterministic: 8 errors in 8s, 0 with
       `--no-follow-symlinks`, which is now passed (`src/Makefile`). `-i '.#*'` does
       **not** help — ignore rules filter events, not the directory walk. This is
       almost certainly also behind the cov-mode non-trigger reported years ago.

    b. **Changes silently missed, no error.** `fseventsd` had been pinned at 96% CPU
       and had leaked to 3.8 GB over 18 days, degrading notification machine-wide —
       which is why it hit watchexec and Emacs alike, erratically, for years.
       `sudo killall fseventsd` restores it (launchd respawns; no reboot). Before:
       native fired 0 of 2. After: 5 of 5, and `--poll` 5 of 5, so the polling
       workaround was dropped again. **If watchers go quiet, check `fseventsd`'s CPU
       and RSS before anything else.** Spotlight is the likely driver — `mds_stores`
       was at 202% while indexing a tree that rewrites hundreds of files per build.

       **Sample it before killing it.** Why it wedged is unknown, because this session
       restarted it first and the restart destroys the evidence. Capture together, while
       it is high, and before `killall`:
       `top -l 2 -pid $(pgrep -x fseventsd) -stats pid,cpu,time` (is it sustained?),
       `sample fseventsd 10` (user-space stacks), and
       `sudo fs_usage -w -f pathname -t 5` system-wide (which paths generate the events,
       i.e. the driver — the sample cannot show that).

       **Seen again, not wedged** (Mac, session `36694da1`, 2026-10-02). The user saw
       100% in Activity Monitor a few times over some minutes. A sample at 10:26 found
       it nearly idle: about 3% of one CPU, ordinary dispatch work, 5.6 MB footprint (8.2
       MB peak) against 3.8 GB last time — the process respawned on 2026-09-30 had not
       leaked. Watchers fired normally the same morning. So: spikes, likely during builds
       and test runs (7% with mds at 16% during one), not the sustained wedge. Two
       limits of that sample: it only sees user space, so time spent in the kernel shows
       as the `dev.fsevents` thread sitting in `read`; and the watchers did not appear
       among fseventsd's client threads although they work, so client threads are no
       measure of who is watching. `mdutil -s` cannot report a single directory, so
       whether `.metadata_never_index` (repo root, `2d1fefad`) is honoured is unverified.

    Item (b) recurs unless the indexer stops seeing the build. `OBJDIR = .objects`
    (`src/sources.mk:17`) and the 132 regenerated `*.gcov` sit in `src/`: 748 of its
    1074 files are build output. Since `2d1fefad` a `.metadata_never_index` in the repo
    root asks Spotlight to skip the whole tree, build output included, so the `*.gcov`
    can stay beside the sources where Emacs cov-mode reads them. Whether it works is
    the open question above; if it does not, moving the build output is what is left.

## 7. Freshness as make (ADR-0032, ADR-0033)

The order is roughly the numbers. `header-not-half` can be done any time. Next is
`request-through-includers`.

Performance, from reading the code (Mac, session `05984816`, 2026-10-08): at equal
correctness the design costs the same or less. Parsing dominates, and what is parsed
follows the request rather than triggers: an edited header no longer parses up to the cap's
worth of includers at once, only the goal's. More is parsed only where the old answer was
incomplete (deeper reach, disk changes). The bookkeeping added — a `stat` per input, a
sweep per request — is small beside what goes. The risks are in the implementation, and
are noted in `cu-inputs`, `out-of-date`, `time-budget` and `sweep-cost`.

- (#35) <a id="cu-inputs"></a>**A CU's inputs** — the CU, its include closure and the project
    config, put together where `out-of-date` uses them, the CU first. `Inputs`
    (`src/freshness.c`) holds them for one request: the include graph and each file's
    change time, asked for once. Left: the config's change time (`out-of-date`). The
    closure can hold the scan's includes beside the parse's, which for the predicate only
    costs a parse.
    Goes with: out-of-date
- (#36) <a id="out-of-date"></a>**The config as an input of `knowledgeIsOutOfDate()`** — it
    replaces `markAllCompilationUnitsStale()`, and the zero sentinel goes. The config is the
    `.c-xrefrc` and its local fragment (ADR-0034), and its change time is the later of the
    two.
    Left until real use shows the cost (see `sweep-cost`): `MAX_INCLUDE_CLOSURE` (1000)
    has no measured reason, and evaluating change first, i.e. `stat` each known file once
    and walk the reverse include graph to the CUs whose knowledge is older, costs in
    proportion to what changed.
    Goes with: cu-inputs, local-config-fragments
- (#37) <a id="request-target"></a>**What a request needs** — the CUs that reach the request
    file's closure, and the whole project for an operation on a name. Left: when its array
    of 256 files is full, `collectCUsIncluding()` walks no further up, silently, which on
    ffmpeg cuts a widely included header short. The scan does not record `<...>`
    includes (`scan-angle-includes`), so for a project that includes its own headers that
    way, reach misses CUs never parsed.
    Waits for: adr-0032
    Goes with: header-filtered-siblings
- (#54) <a id="request-through-includers"></a>**A request in a header is parsed through its includers** —
    never on its own (ADR-0032, amended 2026-10-11), as for a macro body. No includer
    gives a message saying that no compilation unit includes the file. Includers that
    are current are parsed only for the cursor, and need a bound.
    `test_push_in_header_no_cu_includes` comes off suspension.
- (#1) <a id="cursor-parse-reuse"></a>**The request CU is parsed for the cursor, also when its knowledge is current** —
    the lexer finds what the cursor is on during the parse (`lexer.c:448`), so every
    operation with a cursor parses the request file, and in a macro body every CU that
    expands it. ADR-0032 says to reuse the references when the knowledge is current,
    which needs a lookup of the reference at a position in the table. Completion, extract
    and the macro body need the parse.
    Goes with: request-through-includers
- (#39) <a id="header-not-half"></a>**A changed header the editor holds is stripped also when not all its includers fit** —
    `stripChangedHeldHeaders()` (`src/rebuild.c`) strips it and adds its includers to the
    build list only up to the cap. A reference in the header that only the others emit,
    e.g. under an `#ifdef` they set, is missing until they are built. Strip only when
    `everyIncluderIsRebuilt()`, as the sibling collection does (ADR-0032). It still asks
    `fileNumberIsStale()`, which has no coarse-clock gap, and records the header as parsed. Not needed once `sweep` is in.
- (#56) <a id="closed-buffer-strip"></a>**A header whose buffer the client stops sending is stripped, and none of its includers built** —
    `tests/test_reverted_held_header_keeps_its_references/.suspended`.
    Goes with: header-not-half
- (#57) <a id="request-strip-gate"></a>**Nothing tests that a search leaves its request file's references alone** —
    `callServer()` calls `collectRequestCU()`, which strips the request CU, only when
    `operationParsesRequestFile()` is true. Within the cap,
    `test_search_keeps_references_of_changed_request_file` analyses a.c again anyway, so
    it would pass even if the search stripped a.c. A unit test of the gate.
- (#55) <a id="parse-all-out-of-date"></a>**An operation on a name analyses only the files never analysed** —
    parse-all counts and parses the CUs without knowledge
    (`countUnparsedCompilationUnits()`, `parseAllUnparsedCompilationUnits()` in
    `src/server.c`), not those out of date. A CU whose header changed on disk keeps its
    old references through a search. From reading the code (WSL, session `d25255b8`,
    2026-10-12), not tested.
    Goes with: time-budget, behind-the-back
- (#40) <a id="time-budget"></a>**A time budget instead of the cap** — the cap (`MAX_CUS_TO_REPARSE`,
    128 today) becomes a time budget, and above it the question
    with an estimate. Browsing remembers a "no" for the session, an operation that edits
    or works on a name is cancelled by it. Touches the client's question. Open: does a remembered "no" stop parsing, or only the asking, so
    that each request still parses a budget's worth? With `sweep` the second is safe, and
    is what lets every reaching CU finally have parsed.
    Before `sweep`, ADR-0032 rebuilds a changed header with all its includers or none,
    which can parse more than today's cap. Keep the cap until this item is in, or make
    the window between them short.
    The estimate needs how many CUs are out of date in the whole project. That can be
    found change first (`out-of-date`): `stat` every known file once and walk the reverse
    include graph to the CUs. Then project-wide goals use that and the rest ask
    `knowledgeIsOutOfDate()` per CU, with a test that the two agree for every CU.
    The ask-or-not decision (`needsConfirmationToAnalyse()`) takes the budget and is
    unit-tested on both sides of it. The budget is a user setting, so system tests use 0
    (always asks) and unlimited (never asks), and none sit near the boundary.
- (#41) <a id="freshness-docs"></a>**The docs follow** — *Out of date* replaces *Staleness* in
    `06-principles`, and `08-algorithms` loses "Dual Semantics". In `06-principles`, the
    part on why reparsing only a subset is safe also describes strip-then-reparse.
    `08-algorithms` still describes the entry refresh as Pass 1, 2 and 3; "Freshness Is
    Make, for the Goal" in `20-insights` is a start for its rewrite.
- (#42) <a id="include-edges"></a>**Who writes include edges** — the parse and the lightweight
    scan both do. The parse records the `#include`s the preprocessor took, quoted and
    `<...>`, at their real lines. The scan records every quoted `#include`, `#ifdef` or
    not, no `<...>`, all at line 1, column 0 of the includer. A "defined" reference at
    line 1 of a file marks the file itself and is not an edge. Today the scan wins after a
    restart: it adds its edges to CUs that already have knowledge, so an include is listed
    twice, once on line 1 (`tests/test_restart_keeps_includes_as_parsed`). Decide who may
    write an edge and which writer wins. The sweep needs it: it removes a file's
    references once every CU reaching it has been parsed again, so an edge the graph
    lacks makes it remove live references, silently (ADR-0033).
- (#43) <a id="generations"></a>**Generations** — a counter per parse, a mark on every
    reference, refreshed in `addToReferenceList()`, and a test that every reference
    enters the table there.
    Waits for: adr-0033
- (#44) <a id="sweep"></a>**Remove references no CU still emits** — after each request that
    parsed something, remove a reference older than every reaching CU's knowledge
    generation, and drop every `removeReferenceableItemsForFile()` before a build. A capped reverse walk removes nothing in that file.
    `test_preprocess_edit_removes_ifdef_define` comes off suspension.
    Waits for: generations, include-edges
- (#45) <a id="sweep-cost"></a>**What the sweep costs** — memory and walk time on ffmpeg,
    whether the reverse walk can go uncapped, and how the mark is stored. The baseline
    to time against: `removeReferenceableItemsForFile()` walks the whole table, and today
    runs before every CU reparse and for every changed header the editor holds, so a full
    cap, 128 CUs, on ffmpeg is 128 walks over 1.24 million references. The sweep is one walk per
    request that parsed, none for navigation. A stripped reference is also only
    unlinked, never freed (`cxAlloc`), so every reparse leaves its CU's references behind
    as dead memory; refreshing in place stops that. For the sweep itself, compute the
    oldest knowledge generation of a file's reaching CUs once per file, not per
    reference.
    For information: the table only answers exact identity, so other lookups walk all
    of it. By file, the strip above. By kind, `buildIncludeGraph()` looking for the
    include items, about 38 ms on a fully parsed ffmpeg, twice per request. By name
    only, `navigation.c` once per menu item. Per request on ffmpeg, 1376 `stat`s took
    about 5 ms and 2434 include closures about 50 ms (WSL, session `d25255b8`,
    2026-10-11).
    Goes with: sweep

## 8. Real users: installing and being found

Prepare the installation side whenever it suits; it depends on nothing above. Hold the
announcement until branch switching is safe, since a wrong rename after a `git checkout`
loses a first user. *No repo home yet* for any of these — discussed on WSL and on the
Mac (session `05984816`), 2026-10-07.

- (#46) <a id="release-build"></a>**A lean release build** — no coverage, no unit tests, only
    `c-xref`. The generated parsers are committed, so `cc` and `make` are enough.
- (#47) <a id="protocol-handshake"></a>**The client and the server check a protocol version** —
    once the elisp and the binary are installed apart they drift (MELPA builds HEAD, the
    binary is whatever was built). On a mismatch the client offers to rebuild, rather
    than failing in some later request.
- (#48) <a id="install-server"></a>**`M-x c-xref-install-server`** — builds from source by
    default, since every user of a C tool has a compiler; downloads a release binary as
    the shortcut. The pattern of `pdf-tools-install` and `irony-install-server`. A binary
    fetched by Emacs with `curl` is not quarantined by macOS Gatekeeper.
    Waits for: release-build, protocol-handshake
- (#49) <a id="releases"></a>**Tagged releases** — semver tags from `stable`, GitHub Releases
    with binaries, and through the tags MELPA Stable. A Homebrew tap and an AUR package
    are cheap once releases exist.
    Waits for: release-build
- (#50) <a id="melpa"></a>**The elisp on MELPA** — the package without binaries, which MELPA
    forbids; the server comes from `install-server`. `el-get` and `load.el` stay.
    Waits for: install-server, releases
- (#51) <a id="readme-first-screen"></a>**The README sells on its first screen** — a GIF or
    asciinema of a parameter move across files; the Java section goes.
- (#52) <a id="announce"></a>**Announce** — positioned beside clangd, not against it: the
    refactorings clangd does not have (parameters, move function, macros, Yacc). LSP
    tier 2 is the shape of that, with clangd as the main server; lsp-mode takes add-on
    servers, eglot needs a look. EmacsConf, Sacha Chua's Emacs News, Show HN. A stranger's
    first project working without configuration matters here, see `compile-commands`.
    Waits for: sweep, refactoring-refusals, install-server, readme-first-screen
    Goes with: lsp-tiers, retry-creating-request, compile-commands
- (#53) <a id="windows-build"></a>**A Windows build** — most of the code is plain C. About 28
    `__WIN32__` remnants from the original Xrefactory are there, unmaintained and untested
    (`misc.c`, `commons.c`, `options.c`). The work is probably: file names (drive letters,
    backslashes, case-insensitive names, i.e. `normalizeFileName()` and file identity, the
    risky part), modification times with nanoseconds (`st_mtim` in `fileio.c`), the
    compiler interrogation (`popen`, and MSVC's `cl` does not answer `-v -E`), and the
    tests, which need make, bash and sed, and a CI job. A MinGW build under MSYS2 is
    probably the shortest route. From reading the code (WSL, session `d25255b8`,
    2026-10-11). Until then WSL works, but on files under `/mnt/c` each `stat` goes
    through 9P to the Windows side, at about a millisecond each.

## Foundation

What the open items rest on, so that a `Waits for:` can name it. Not a record of what
was done.

- <a id="entry-refresh"></a>**Entry refresh before every request (ADR-0020)**
- <a id="adr-0029"></a>**ADR-0029: the project's identity is its root path**
- <a id="adr-0030"></a>**ADR-0030: opening a project and asking about a file are separate**
- <a id="adr-0031"></a>**ADR-0031: what files a project consists of**
- <a id="adr-0032"></a>**ADR-0032: knowledge is rebuilt like `make` rebuilds targets**
- <a id="adr-0033"></a>**ADR-0033: references are removed when every reaching CU is parsed again**

## Open questions that would reorder this

* **Who establishes the project — the client, every request, or the request's own file?**
  *ADR-0029 settles the identity half: the server binds to a root path, the `[section]`
  name stops being an identity, and `-p` goes so `handleProject()` reaches the
  comparison. What stays open is the gate below — which of the three hatches survives,
  and at what severity. Ties to item 7, which puts `-p` in the config rather than on the
  command line.* `callServer` (`src/server.c`) already derives everything from the
  request's input file — `initializeProjectContext`, snapshot load, scan, sibling parse —
  and only then does `answerEditorAction` (`src/cxref.c:1887`) refuse with
  `FATAL_ERROR` unless one of `options.xrefrc`, `lockedProject` or `options.project` is
  set. The gate sits downstream of all the work it might have saved: observed parsing 79
  sibling CUs and then failing. It is also inconsistent — measured on an isolated copy of
  `src/`, fresh server each time (Mac, session `7c731c79`, 2026-09-24):

  | | with `-p` | without `-p` |
  |---|---|---|
  | `-push` | ok | FATAL |
  | `-browse-next` | FATAL | FATAL |

  The client sends `-p` on every request, so the third hatch should always hold; for
  `-browse-next` `options.project` is gone by dispatch time, and where it is lost was not
  traced. Three ways out: the client precedes every request with `-getproject` (ceremony,
  ~91ms warm); the client sends it once per server (fragile — the client restarts servers
  silently on crash and on Remove References, and cannot tell that it did); or the server
  locks implicitly from the first request's file and `-getproject` becomes only the
  "which project is this / switch project" query, with `PPC_PROJECT_MISMATCH`
  (`src/cxref.c:1824`) still enforcing one project per server. Removing `-p` (item 7)
  deletes one of the three hatches, so this has to be settled first or alongside.
  Separately: `FATAL_ERROR` is the wrong severity — a missing `-getproject` is a client
  protocol mistake, not a broken invariant, and killing the server turns it into a
  restart loop. `errorMessage` is what the convention asks for.

  What ADR-0029 adds: the third hatch does not merely "always hold", it *suppresses* the
  check. `handleProject()` returns on `options.project != NULL` before comparing
  anything, so `PPC_PROJECT_MISMATCH` and the client's "Switch to this file's project?
  (Server will restart)" prompt (`editors/emacs/c-xref.el:2366`) have been unreachable
  for as long as the client has sent `-p`.

* **ADR-0027 stays `Accepted`, not `Implemented`:** rename does not yet refuse from inside
  a macro body, and the menu is complete only up to the collection caps.
* **`18-known-bugs.adoc` cites `test_browsing_push_name_parses_whole_project`**, which
  does not exist; the suspended directory is `tests/test_browsing_push_by_name`. Fix the
  reference when touching the remaining-bugs item.
* **Outside this repo:** the external regression scripts still lack the orphan-test-dir
  filter that `utils/failing` got in `db064a2a` — a `tests/test_*/` with an `output` but
  no `Makefile` reads as a failure.

[Open the dependency graph](https://thoni56.github.io/c-xrefactory/backlog-graph.html),
drawn from the `Waits for:` lines above when a push reaches the docs site.
For changes not pushed yet, `make -C doc dist/backlog-graph.html` draws it locally.

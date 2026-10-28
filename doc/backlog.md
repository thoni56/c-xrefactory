# Backlog

What to work on next, and in what order. As of 2026-10-27.

The *descriptions* live in the guidebook (`doc/docs/`), in the `.suspended` notes and in
the ADRs. This file carries only the **order** and the **dependencies**, which are
recorded nowhere else. Keep it thin: one line per item plus a pointer. If an item needs a
paragraph, the paragraph belongs in its chapter, not here.

A few items have no repo home yet — they came out of a session and were never written
down. Those lines say where to start instead of pointing, and should shrink to a pointer
once the work gets a chapter, a test or a `.suspended` note.

A planned feature gets an item only once it has a place in the order. Until then
`11-planned-features.adoc` is the list. A suspected bug is an investigation, and its title
says so. A red test makes it a bug, and a disproved one is deleted.

Ordering principle: first the one item that gates the most rows, then the chains that unblock, then bugs and
features. Relative effort only — no dates (#noestimates).

Each item has a name, the `<a id>` before its title, and the title is on one line. A
`Waits for:` line names what it waits for. An `After:` line names the one item it should
preferably come after, an order that can be skipped, and these make the themes trees.
`Goes with:` names what it belongs with. The docs build draws the dependency graph on
every push, `After` dashed and `Goes with` not at all, linked at the end of this file, and
`utils/backlog_graph --check`, run with the tests, fails if a name is unknown.

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
   `options`. `parseFileWithFullInit()` keeps only `cursorOffset`,
   `serverOperation` and, as stopgaps, `dumpSelection` (`test_dump_after_a_rebuild`) and
   `index` (`test_completion_select_after_a_rebuild`).
   Every other REQUEST option is lost when a rebuild comes before the dispatch.
   The other way round, the lock that `-getproject` takes copies all of `options` into
   `savedOptions` (`src/cxref.c:1829`), so the next request starts as `OP_GET_PROJECT`
   unless it names an operation. A request with no operation of its own then gets a
   `-getproject` answer, "No source file to identify project" (WSL, session `d25255b8`,
   2026-10-14).
- (#4) <a id="setup-ladder"></a>**The Setup Ladder, in this sequence**, each small once the partition lands
   (`doc/docs/10-roadmap.adoc`, Convergence: The Setup Ladder → Remaining):
   a. <a id="root-path-id"></a>**the server's project id is its root path** — ADR-0029. `lockedProject` becomes the
      detected project root and `lockedProjectRoot` goes, so `SET_INFO` and
      `PROJECT_MISMATCH` carry the path, not the `[section]` name. Test first:
      `test_project_lock`'s `expected` gets the path, then the server, then the other
      `expected` files with named sections (about 15). The client's half is in `e476e396`,
      its project is only what `-getproject` answers. Where the client shows the project,
      Project > Show Active (`c-xref-project-active`) and the mismatch dialog
      (`c-xref-server-dispatch-project-mismatch`), it shows the path through
      `abbreviate-file-name`. Project > New (`c-xref-project-new`) asks only for the root.
      Then `foundProjectName` in `searchForProjectConfigFileAndProjectForFile()` goes, with
      the section buffers of its callers in `src/startup.c` and `src/cxref.c`.
      In `src/startup.c` the section name also decides a full reload in
      `initializeFileProcessing()`, and `reInitCwd()` takes it as the directory when it is
      an absolute path. Both go.
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
      Then `initializeFileProcessing()` discovers nothing. Today it must skip the config
      search once locked, since the search sets `autoDetectedProjectRoot` (`src/options.c`)
      and so `cxFileLocation`. `initializeProjectContext()` has a copy of that discovery.
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
- (#79) <a id="no-section-headers"></a>**A `.c-xrefrc` has no section header** — *no repo home yet.*
    A config is directories and options, nothing else. Once the root path is the id, the
    header's last job, the project's name, is gone. A `[` is then reported with its
    position, as an unterminated quote is, and the rest of the file is still read.
    Project > New (`c-xref-project-new`, `editors/emacs/c-xref.el`) stops asking for a name
    and writing `[name]`. The test configs lose their header, the common template too (55
    of 60). `getProjectNameFromConfigFile()` goes, and with it a fourth read of the config,
    and so does `sectionHeaderSeen` in `readOptionsIntoArgs()`. Supersedes ADR-0031's "a
    second header ends what is read": no config with several sections exists outside the
    author's own, so nothing merges (WSL, session `d25255b8`, 2026-10-25).
    After: root-path-id

## 2. After XrefMode

- (#23) <a id="indexing-log-buffer"></a>**The diagnostics buffer reports unresolved includes, with a suggested `-I`** —
    `doc/docs/11-planned-features.adoc` (Diagnostics Buffer). What did not parse reaches
    `*c-xref-diagnostics*` with its position, grouped by unit, and `next-error` visits it.
    Left: the report of unresolved includes.
- (#68) <a id="lsp-mode"></a>**An `LspMode`, and a sink for the messages** — *no repo home yet.*
    LSP sets `ServerMode` (`src/parsing.c:140`), a stop-gap for the `assert(options.mode)`
    that are gone now, and nothing reads the mode any more. `Mode` becomes `ServerMode`
    or `LspMode`, set once from argv, and it replaces `cxrefProtocol`. A
    check of the mode deep in the code is where packing is mixed into collecting. `src/commons.c` then sends messages to a sink
    chosen at startup: `<log>` and `<analysing>` for `-server`,
    `textDocument/publishDiagnostics` for `-lsp`, which replaces a document's diagnostics
    as a reparse replaces a unit's group. The first case of what #25 needs.
    Goes with: startup-command-line, partition-options
- (#7) <a id="startup-command-line"></a>**Decide, and enforce, what the startup command line may carry** — *no repo home yet.*
   Bigger than `c-xref file.c`, and it needs code, not just a decision. Once XrefMode is
   gone the server takes no file at startup: every request names its own, and the same
   parser serves both phases, so the question becomes which options are process-scoped at
   all. The Setup Ladder's step 1 answers it in principle; this makes it concrete.
   - Genuinely startup, on the evidence of what the client sends and what `main()` reads
     before anything else: the mode; the transport (`-o <answerfile>` —
     `editors/emacs/c-xref.el:1615`); logging
     (`-log=`, `-debug`, `-trace`, `-info`, `-errors`, `-warnings`, `-infos`, scanned in
     `main()` before the mode is even known); and `-statistics`. `-errors` means two
     things: the client wants parse messages (`src/commons.c`), and `main()` makes it the
     console log level. One of them should get a name of its own.
   - Everything project-scoped — `-p`, `-I`, `-D`, `-optinclude` — belongs to
     the config and binds at the future `-openproject`, not here.
   - **`-exactpositionresolve` has no strategy.** It is real: it changes link names
     (`src/semact.c`) and goes into the snapshot's check number (`src/cxfile.c`). Since it
     changes what a symbol is, it is probably project-scoped, although `options.h` marks it
     REQUEST. Decide whether it belongs in the config or goes. Item 14 waits on the same
     question.
   - **The mode is chosen from argv alone.** `main()` picks `-help`, `-lsp`, `-server` or
     `-about` by scanning argv, before any option is processed, so only `-server` runs the
     initialisations. That scan is where the check belongs: `-lsp`, `-about` and `-help`
     take nothing else, and `-server` takes the startup options above.
   - Done: `c-xref -server file.c` is refused, `main()` reports the operand and exits
     (`tests/test_server_refuses_startup_file`), as `c-xref file.c` answers "No mode
     given" (`tests/test_options_mode_required`).
   - **File arguments: none at startup, exactly one per request.** A request's only bare
     argument is the current buffer's file (`c-xref-server-call-on-current-buffer`,
     `editors/emacs/c-xref.el`), which may not exist on disk yet; other buffers travel as
     `-preload`. The server only enters it in the file table: the structure scan and the
     snapshot know the rest of the project, and goal and reach (ADR-0032) decide what to
     parse. Nothing walks a file argument any more: a request enters its file directly
     (`addRequestFileToFileTable()`), and the walk, with its backquoted commands and
     `dirInputFile()`, is gone, and an `-I` is one directory. Left: two
     operands in a request are not reported, which the operands `processOptions()`
     hands back make easy.
   - `.c-xrefrc` also lists source directories, as its operands
     (`loadProjectSettings()`). Do not assume that list is legacy. What dies with `-p` is the
     *registry*: several `[project]` sections, each covering directories, one selected by
     name. Saying "these directories are also part of my project" is a different need and
     it survives — a project using a library you have the source for, which you want to
     navigate into, is not covered by `-I`, which only tells the preprocessor where to
     look. Auto-discovery gives you the tree under the config; anything outside it still
     has to be named. Likely home: the config, or the machine-specific sibling, since an
     external source tree is usually a local path.
- (#78) <a id="options-carry-scope"></a>**Each option carries its scope, and `processOptions` refuses one outside it** — *no repo home yet.*
    Every source goes through the same `processOptions()` and every option is accepted
    from all of them. The sources are three contexts: startup (`mainTaskEntryInitialisations()`),
    project (the config read in `getAndProcessProjectConfig()`, `applyOptionSet()`, and an
    `-optinclude`, which takes the context it appears in) and request (`initServer()`, and
    the bridge in `refactory.c` until #9). These are #3's lifetimes, which `options.h`
    already marks per field as SESSION, PROJECT and REQUEST.
    `processOptions()` takes the context, and an option's entry states where it is allowed, e.g. a `-server` in
    `.c-xrefrc` is reported with its position, as an unterminated quote is. #7 decides the
    startup column. The replays in `loadProjectSettings()` and
    `initializeFileProcessing()` are not a context, they go with #3.
    Test first: an `options_tests.c` case where a request-only option in the project
    context is refused (WSL, session `d25255b8`, 2026-10-25).
    Goes with: partition-options, startup-command-line
- (#9) <a id="remove-bridge"></a>**Remove the `parseBufferUsingServer` bridge** — §17.3; 9 refactoring call sites
   re-entering `callServer`. Independent of the rest of this section, but it is the last
   divergent parse path. `-olexmacro` and `-olexvariable` only exist for it and go with it.
- (#62) <a id="docs-audit"></a>**Audit the docs, chapter by chapter** — much of `doc/docs/` was
    written while the batch modes and the disk database were the truth. Each chapter is
    read against the code as it is, and what is stale is fixed or becomes a line here.
    `06-principles` first, e.g. the snapshot paragraph in "Reference Database and Parsing".
    After: freshness-docs

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
    Goes with: behind-the-back
    After: sweep
- (#11) <a id="behind-the-back"></a>**A header changed on disk, which the editor does not hold, keeps its old references when it has more includers than the cap** —
    a header that some includers are built into, but not all, is left behind, and its
    other includers are then out of date until all are built in one request and it is
    stripped (`tests/test_ghosts_stay_when_one_includer_is_parsed_alone`). Above the cap
    that never happens. Ghosts from before that are not seen as left behind, and stay.
    Waits for: request-target
    Goes with: sweep
- (#93) <a id="settle-left-behind"></a>**Every request settles each header left behind** —
    puts all includers of a header marked `isLeftBehind` on the build list, not only those
    the request reaches (siblings, the request CU, a name operation's
    `collectOutOfDateCUs()`). Until then a request elsewhere still reads the header's old
    references. Above the cap it meets #11.
    Goes with: behind-the-back
- (#94) <a id="inserted-text-loses-non-ascii"></a>**Investigate: which refactorings insert text copied from the server's buffer** —
    *no repo home yet.* A loaded buffer has each multibyte character replaced by a space
    (`convertUtf8()`, `src/encoding.c`), so that byte offsets are character offsets. Text the
    server copies from its buffer into an insertion loses its non-ASCII characters. Move
    Function's `extern` declaration is one, a block it moves is not (the client moves that).
    Extract Function is not checked. Seen in a Move Function's comment (WSL, session
    `d25255b8`, 2026-10-28).
- (#13) <a id="token-pasting-trio"></a>**Token pasting trio**, in this order — the later two assume the first
    (`doc/docs/18-known-bugs.adoc`): `tests/test_token_pasting_numbers` (changes the
    passing `test_collate_const_suffix_pasting`) → `tests/test_token_pasting_float` →
    `tests/test_collate_hex_prefix_pasting` (touches how every number is lexed).
- (#14) <a id="header-static-link-names"></a>**Header static: prototype and definition get different link names** —
    `setStaticFunctionLinkName` (`src/semact.c`); renames break the build today.
    `tests/test_static_declared_and_defined_in_header/.suspended` has the
    `exactPositionResolve` question to settle first, so it starts as a decision.
    Waits for: startup-command-line
- (#15) <a id="correctness-by-cost"></a>**The other correctness bugs**; their order is not
    known. Each has a `.suspended` note or a `18-known-bugs.adoc` entry,
    or says it has no repo home yet:
   b. <a id="relative-include-path"></a>**A relative `-I` in `.c-xrefrc` resolves against the working directory of the server process** —
      *no repo home yet* (`doc/c-xrefrc.man`), while a directory line resolves against
      the config's directory and `-prune` against the root. With the config in
      `tests/test_ffmpeg/ffmpeg`, `-Iffmpeg` found nothing and the absolute path did (WSL,
      session `d25255b8`, 2026-10-09). Probably the config's general path rule that
      ADR-0031 proposes for `-prune` should cover `-I` too. The same config had
      `-prune ffmpeg/compat/atomics`, which with `ffmpeg/` as root names a directory that
      does not exist, silently.
      The cause is that the global `cwd` has two meanings in one request: `initCwd()` sets
      it to the process's working directory, and only after `processOptions()` does
      `reInitCwd()` (`src/startup.c`) make it the config's directory. The `-I` is
      resolved in between. If `getcwd()` fails, `cwd` is `/` in that window, so relative
      names resolve under the root. Resolving the config's relative paths against the
      config's directory, not `cwd`, fixes both (Mac, session `05984816`, 2026-10-14).
      After: adr-0031
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
      Goes with: refactoring-refusals
      After: time-budget
   j. <a id="lsp-position-encoding"></a>**Unconfirmed: the LSP server announces `positionEncoding: "utf-8"` but counts code points** —
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
      After: lsp-mode
   k. <a id="lsp-input-ends"></a>**Unconfirmed: the LSP server exits as a JSON parse error when its input ends** —
      *no repo home yet* (`src/lsp.c`). At end of file the header reader returns a length
      of 0, the empty content does not parse, and the exit code is
      `LSP_RETURN_ERROR_JSON_PARSE_ERROR`. The protocol does not say what an ending input
      means, but a client that died without `exit` should not look like a parse error.
      From reading the code (WSL, session `d25255b8`, 2026-10-10), not tested.
      After: lsp-position-encoding

- (#58) <a id="search-result-line-zero"></a>**Investigate: the client asks for search result number 0** —
    typing a search string gave `-search-goto=0` at once, twice, each time
    after the question "N of M files need analysing" (WSL, 2026-10-13, the client on
    `ac3f52e2`). After a push had analysed the files, the same search asked nothing and
    listed one line. `c-xref-call-task-on-line` (`editors/emacs/c-xref.el`) sends the
    line number from `count-lines`, which is 0 only in an empty buffer. Suspects: the
    answer to the question dispatches the symbol list nested inside the first answer
    (`c-xref-server-dispatch-wait-confirmation`), and the dialog's Enter or space is also
    "go to" in the search results. The ERT harness (`check-elisp`) can drive it. The
    server now answers 0 with an error naming the number
    (`test_search_result_number_zero_is_answered`).
    Needs designing first: the client has met questions only as answers, never in the
    middle of a request it is still dispatching.
- (#60) <a id="include-bracket-from-macro"></a>**`#include MACRO` where the macro is `<...>`** — the
    header is not opened, so everything it declares is missing, e.g. FreeType's
    `#include FT_FREETYPE_H`. The lexer makes one `STRING_LITERAL` of a `<...>` only right
    after `#include` (`putIncludeString()`, `src/lexembuffer.c`); a macro body is ordinary
    tokens. Join the tokens up to `>` into a header name (C11 6.10.2p4 leaves how to the
    implementation; gcc joins their spellings). The tokens after `<` are not cut either,
    as the `"..."` branch does, so they reach the parser as syntax errors
    (`processIncludeDirective()`, `src/yylex.c`). `test_parsing_include_using_macro` pins
    both; its two syntax errors go when this is fixed.
- (#69) <a id="latin-1-lookalikes"></a>**Unconfirmed: Latin-1 that looks like UTF-8 is read as one character** — *no repo home yet.*
    `convertUtf8()` (`src/encoding.c`) takes a lead byte with its continuation bytes as
    one character, so `å°` and one more byte in 0x80–0xBF, in Latin-1, become one space
    and every position after it on the line is two characters early. Read a sequence that
    would be an overlong or invalid code point as single bytes. A file that is not UTF-8
    is not reported, it parses.
- (#70) <a id="completion-select-out-of-range"></a>**Investigate: selecting a completion says "selection out of range"** — *no repo home yet.*
    In `yylex.c`, half-way through an Add Parameter, `pre` and F8 listed 8 completions
    over 16 lines. Enter on the last one, `previousLexemP`, sent `-completion-select=16`, which
    is the right line. The answer was `selection out of range.`
    `getMatchOnNthLine()` (`src/match.c`) counts wrapped lines and returns NULL only for an
    empty list, so the session entry `selectCompletion()` found probably had no matches.
    The completion request also got a syntax error at the cursor (858:45).
- (#71) <a id="newline-position"></a>**A `'\n'` lexem has the next line's number** — *no repo home yet.*
    `handleNewline()` (`src/lexer.c`) puts the `'\n'` after the newline, and the blanks and
    comments after it, have been read. Its column is where the line ended, its line is that
    of the next lexem. A message at the end of a line lands on the line below, e.g.
    `#if __has_include(` with nothing after it.
- (#72) <a id="if-syntax-error-twice"></a>**A malformed `#if` is reported twice** — *no repo home yet.*
    When `handleHasIncludeOperator()` (`src/yylex.c`) has reported what is wrong and
    returns 0, `cppexp_yyparse()` adds "Syntax error on: `__has_include`"
    through `styyerror()` (`src/semact.c`). The second message says nothing more.

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
    Goes with: cursor-parse-reuse
    After: request-target
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

- (#85) <a id="move-shared-static"></a>**Move Function asks about a `static` shared with what stays** —
    make it extern where it is, or move it and make it extern from the target.
    11-planned-features → Move Function → A `static` shared with what stays.
- (#87) <a id="move-to-new-module"></a>**Move Function to New Module** — create the target `.c` and `.h`,
    with an include guard and the includes, then make the ordinary move, so the refusal and the
    helpers come with it. Adding the module to the build, and a mock, may be the user's.
    11-planned-features → Move Function → Next steps.
- (#86) <a id="extract-array-used-after"></a>**Extract Function passes an array declared in the region and used after it** —
    today it refuses (`refuseToReturnAnArray()`, `src/extract.c`), since a function can't return
    an array. Move the declaration out of the region, before the call, and pass the array, as
    when it is declared before the region (`char message[]` as a parameter). The refusal goes.
    Test: `tests/test_extract_array_declared_in_region_used_after`.
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
    1.10.0 announces the entry in `CHANGES.md`, so its removal needs a line there too.
    Waits for: behind-the-back
    Goes with: sweep, time-budget
    Goes with: index-based-sessions

- (#22) <a id="index-based-sessions"></a>**Index-based sessions** — dissolves stale-POP *and* lets an answer grow while you
    work. Roadmap → Memory as Truth → Remaining. Depends only on entry refresh, which is
    done.
    Waits for: entry-refresh
- (#24) <a id="retry-creating-request"></a>**Retry the request that created the project** — small, and it becomes first contact
    with every new project once auto-discovery is the only way in.
- (#25) <a id="lsp-tiers"></a>**LSP tiers 1–2** — code actions and `workspace/executeCommand` for extract, move
    function and the parameter refactorings. Stubs exist: `handle_code_action`,
    `handle_execute_command`, with `codeActionProvider` commented out in
    `src/lsp_handler.c`. Keep tier 3 (custom methods + per-editor extension code) small.
    First, the refactorings have to return their edits, so that a protocol can write
    them. Today they write native protocol records as they go (`ppc…()` calls in
    `refactory.c`, `extract.c` and `cxref.c`). The `assert(options.cxrefProtocol)` lines
    roughly map where an operation depends on the native protocol, so keep them until then.
    After: lsp-mode
- (#26) <a id="move-function-next"></a>**Move Function next steps** — the extern and forward declarations,
    include management, header placement, preview, and what is shown afterwards.
    11-planned-features → Move Function → Next steps. Two cases: a target without a header
    (`tests/test_move_function_creates_the_targets_header`, suspended), and moving
    `waitForUserConfirmation()` into `src/ppc.c` needed `log.h` added by hand (2026-10-27).
- (#88) <a id="parse-all"></a>**`-parse-all`, a request that parses every compilation unit the scan found and has not parsed** —
    what ADR-0024's CreateMode reduces to once the priming is a request rather than a mode.
    `test_ffmpeg` and `test_systemd` ask for it today by searching a name nobody defines and
    answering `-continue`. A refactoring only changes what has been parsed: a Delete
    Parameter on `saveReferencesToStore` left `src/cxfile.mock` behind, and the unit build
    did not compile (Mac, session `3af59d00`, 2026-09-25). No test pins that a refactoring
    reaches a mock.
    If a user asks for it, it should say how long it will take. `src/progress.c` already
    keeps `timeZero` and a monotonic clock, and the parse loops count down, so what is
    missing is the division and a format like "4 min left". Decide the unit first: CUs are
    uneven (`avcodec.h`'s consumers against a 40-line `.c`), lines or bytes are steadier,
    and both are known before parsing starts.
- (#92) <a id="refactoring-help"></a>**The refactoring dialog's help describes what the dialog offers** — *no repo home yet.*
    `c-xref-refactoring-documentation` (`editors/emacs/c-xrefdoc.el`) has no Rename Module or
    Organize Includes, describes a Make Static Function Extern the dialog does not offer, and
    Extract Function's example is Java. A test that every dialog entry (`c-xref.el`, the
    `PPC_AVR_` list) has a help entry, and every help entry a dialog entry, finds the next one.
- (#82) <a id="browser-for-c"></a>**The browser's Symbol Pane says which symbol each line is** — a line is
    `+  1/1  LIMIT`, so a macro `LIMIT` and a `static int LIMIT` in another file look the
    same (`tests/test_menu_filter_levels`). In the Java version the pane was a class tree
    and the class name told them apart (`c-xref-server-dispatch-classh-lines` in
    `3b932e0e`). For C, give each line its kind and where it is defined, e.g.
    `macro LIMIT a.h:1` and `static variable LIMIT c.c:1`. The server already sends
    `type=`, the client drops it. The file of the definition is a new attribute.
    Then the user manual's "where they're defined" is true.
    Ideas for making the rest fit C:
    - A macro defined in two headers is one entry with `drefn=2`, so the line could list
      both places, or the entry could be split by definition.
    - Filter 1 differs from 2 only for an `#include`, so two levels, the symbol and every
      symbol with the name, are probably enough.
    - Unused Symbols uses the browser as a plain list. A buffer like
      `*c-xref-diagnostics*`, with `M-g n`, is what it wants.
    - It need not take the whole frame without a tree to show.
- (#27) <a id="local-config-fragments"></a>**Local config fragments: the config names, the fragment binds** —
    ADR-0034, proposed. Decide it, with where the fragment lives and the syntax of a
    default. Then: expand defaults, report an unbound name, read the fragment before the
    environment, and ignore the personal options in a config, with a warning.
    After: project-extent
- (#28) <a id="chapter-17-hygiene"></a>**Chapter 17 hygiene, opportunistically** — incremental `cxfile.c` cleanup,
    hashtab → hashlist, split the editor module. Also "tags" in the elisp, the last of
    the old wording: two docstrings in `c-xref.el` say the search "inspects the tags".
- (#76) <a id="macro-expansion-module"></a>**Extract the macro expansion module from `src/yylex.c`** — §17.2.
    `yylex.c` is 2794 lines. The macro stack, the macro arguments memory and the
    expansions of the last lexem (`lastLexemExpansions`, `noteMacroExpansionsOfLastLexem()`)
    now sit together at its top, under section comments. The plan in §17.2 is older than
    that, and says 2353 lines, so check it against the code first (WSL, session
    `d25255b8`, 2026-10-22).
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
    Goes with: project-extent, partition-options
    After: local-config-fragments

## 6. Developer tooling

- (#61) <a id="yydebug-to-log"></a>**The `-yydebug` parse trace goes to the log** — byacc's skeleton
    writes it with `printf` (`byacc-1.9/skeleton.c`), and in a server stdout is the sync
    stream, so a client stops with `SYNCHRO PROBLEM`. It works only with the server driver
    or from a shell. The trace gives states, not rule names; byacc's `-v` listing maps them.

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
- (#36) <a id="out-of-date"></a>**The config as an input of `knowledgeIsOutOfDate()`** — it
    replaces `markAllCompilationUnitsStale()`, and the zero sentinel goes. The config is the
    `.c-xrefrc` and its local fragment (ADR-0034), and its change time is the later of the
    two.
    Left until real use shows the cost (see `sweep-cost`): `MAX_INCLUDE_CLOSURE` (1000)
    has no measured reason, and evaluating change first, i.e. `stat` each known file once
    and walk the reverse include graph to the CUs whose knowledge is older, costs in
    proportion to what changed.
    Goes with: local-config-fragments
    After: cu-inputs
- (#37) <a id="request-target"></a>**What a request needs** — the CUs that reach the request
    file's closure, and the whole project for an operation on a name. Left: when its array
    of 256 files is full, `collectCUsIncluding()` walks no further up, silently, which on
    ffmpeg cuts a widely included header short. The scan does not record `<...>`
    includes (`scan-angle-includes`), so for a project that includes its own headers that
    way, reach misses CUs never parsed.
    Waits for: adr-0032
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
    After: request-through-includers
- (#39) <a id="header-not-half"></a>**Unconfirmed: a changed header the editor holds is stripped also when not all its includers fit** —
    `stripChangedHeldHeaders()` (`src/rebuild.c`) strips it and adds its includers to the
    build list only up to the cap. A reference in the header that only the others emit,
    e.g. under an `#ifdef` they set, is missing until they are built. Strip only when
    `everyIncluderIsRebuilt()`, as the sibling collection does (ADR-0032). It still asks
    `fileNumberIsStale()`, which has no coarse-clock gap, and records the header as parsed. Not needed once `sweep` is in.
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
    After: sweep

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
    than failing in some later request. Until this is in, every upgrade path (el-get,
    `load.el`) replaces the client and the server together, and the only user is the
    maintainer, so a protocol or option change is made in one step: no transition, no
    deprecation, an option the client stops sending is simply unknown afterwards.
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
    forbids; the server comes from `install-server`. `el-get` and `load.el` stay. This is
    where the client and the server start to be installed apart.
    Waits for: install-server, releases, protocol-handshake
- (#51) <a id="readme-first-screen"></a>**The README sells on its first screen** — a GIF or
    asciinema of a parameter move across files; the Java section goes.
- (#81) <a id="customize-names"></a>**The customizable options get names of today** — Customize shows
    "C Xref ..." and "C Xrefactory ...", since it makes its labels from the symbol names
    (`c-xref-...` in `editors/emacs/c-xrefactory.el`). A group can get a `:tag`, but an
    option is relabelled only by a rename, with `define-obsolete-variable-alias` so that
    old settings still work. MELPA's `package-lint` probably wants one prefix, the
    package's name, so the two go together.
    Goes with: melpa
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
- (#73) <a id="refactoring-catalogue"></a>**Every known refactoring, with its status** — a list,
    one row per refactoring, done, planned or left out, linking to its design in
    `doc/docs/11-planned-features.adoc` and its recipe in `19-refactoring-recipes.adoc`.
    It shows how many of them c-xrefactory can do, so probably in the user manual or the
    README. *No repo home yet.*

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
  | `-browse-push` | ok | FATAL |
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
* **Outside this repo:** the external regression scripts still lack the orphan-test-dir
  filter that `utils/failing` got in `db064a2a` — a `tests/test_*/` with an `output` but
  no `Makefile` reads as a failure.

[Open the dependency graph](https://thoni56.github.io/c-xrefactory/backlog-graph.html),
drawn from the `Waits for:` lines above when a push reaches the docs site.
For changes not pushed yet, `make -C doc dist/backlog-graph.html` draws it locally.

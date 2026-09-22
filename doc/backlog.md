# Backlog

What to work on next, and in what order. As of 2026-09-22.

The *descriptions* live in the guidebook (`doc/docs/`), in the `.suspended` notes and in
the ADRs. This file carries only the **order** and the **dependencies**, which are
recorded nowhere else. Keep it thin: one line per item plus a pointer. If an item needs a
paragraph, the paragraph belongs in its chapter, not here.

A few items have no repo home yet — they came out of a session and were never written
down. Those lines say where to start instead of pointing, and should shrink to a pointer
once the work gets a chapter, a test or a `.suspended` note.

Ordering principle: first the cheap things that change what everything else costs, then
the one item that gates the most rows, then the chains that unblock, then bugs and
features. Relative effort only — no dates (#noestimates).

## 0. Verify first — cheap, and changes the cost of the rest

1. **The same CUs are reparsed on every request — cause unknown** — *no repo home.*
   Observed 2026-09-18 while tracing `tests/test_browsing_push_in_unexpanded_macro`:
   `source.c` parsed during `-getproject` and again during the following `-push`, both
   logging "0 skipped (already parsed)". If it still reproduces, every request pays to
   reparse the same neighbourhood — the cold-start cost Pass 3 exists to bound — and the
   parse-all check for project-wide operations counts never-parsed CUs the same way, so it
   would keep asking too.

   **Not the obvious cause.** Checked 2026-09-22: `parseUnparsedSiblingCUs`
   (`src/server.c`) *does* set `fi->lastParsedMtime = editorFileModificationTime(...)`
   after each `reparseStaleFile`, and `git log -S` dates that line to `a6020c82`
   (2026-02-27, the commit that added Pass 3) — so it was already there when the double
   parse was seen. Do not re-walk that path.

   **Where to look instead**, both being the other end — who *clears* the field:
   `markAllCompilationUnitsStale()` (`src/referencerefresh.c`), called at `src/server.c`
   when `isProjectConfigChanged()` says the config changed, deliberately as a cold
   restart; and `markPreloadedFilesAsAncient()` (`src/cxref.c`), which looked like the
   culprit for a preloading driver but is reached only from the `-exit` handler in
   `src/options.c`. The first is the live suspect: a config-change test that keeps
   answering yes would mark *every* CU stale on *every* request, which is a bigger cost
   than the one originally suspected. Reproduce with `make trace` in that test directory
   and see which branch fires, before changing anything. Upstream of everything in §4.
2. **`optionSetsLoaded` guards less than it looks like it does** — *no repo home.* The
   file-static in `src/startup.c` is set once and never reset; its job was to stop
   `loadProjectSettings` re-reading the config, but `initializeProjectContext` now resets
   and reads for itself, so what the flag actually prevents is worth re-deciding. Small,
   and it wants the lifetime partition's answer to "who owns this" more than a patch.
3. **Expanding-CU rename** — down to one identifier: `UsageMacroBaseFileUsage` →
   `UsageMacroExpandingCU`. `addMacroBaseUsageRef` and `findMacroExpansionFile` were
   deleted with the marker (`3a17bf78`), so only the enum member survives, kept so an
   existing `.cx` still loads. No snapshot format bump — the usage is stored numerically,
   and the name is not. Terminology in `doc/docs/06-principles.adoc` already uses the new
   words and cites the old identifiers; update the parenthesised names afterwards.

## 1. The foundational one

4. **Partition `options` by lifetime** — Session / Project / Request, one owner each, and
   a pure `parseCommandLine()`. `doc/docs/17-major-codebase-improvements.adoc` §17.4. The
   roadmap's Setup Ladder says it gates the rows below it. Largest single item here.
   When it moves `ProjectConfig` off being a file-static singleton (`src/startup.c:42`,
   `static ProjectConfig projectConfig = {0};`), give it a `makeProjectConfig()` that
   returns `{.sourceDirs = NULL, .optionSets = makeOptionSets()}`. Today both fields are
   emptied by code that frees first — `freeStringList(projectConfig.sourceDirs)` and
   `resetOptionSets()` — which is only safe because static storage zeroes them; an
   automatic `ProjectConfig` declared without `= {0}` would free garbage. A constructor
   makes "born empty" one expression instead of every declaration site remembering, and
   gives `makeOptionSets()` its first production caller.
5. **The Setup Ladder, in this sequence**, each small once the partition lands
   (`doc/docs/10-roadmap.adoc`, Convergence: The Setup Ladder → Remaining):
   a. the client stops sending `-p` on every request
      (`c-xref-send-data-to-process-and-dispatch`, `editors/emacs/c-xref.el:1922` — the
      roadmap says 1925), and the server tests that pass `-p`/`-xrefrc` convert to
      `-getproject`;
   b. project setup moves into `-getproject` — discovery, config read, compiler
      interrogation, snapshot load;
   c. process start strips to parsing setup (`mainTaskEntryInitialisations()`,
      `src/startup.c`);
   d. the assert that nothing before `-getproject` reads a project-scoped option — the
      tripwire, as the disk-read assert was for Memory as Truth.

## 2. The XrefMode chain

6. **Re-key the 19 `options.mode != ServerMode` guards in `src/yylex.c` onto a
   report-errors flag**, and stop `formatMessage()` (`src/commons.c`) dropping the position
   in server mode. Roadmap, "Report what did not parse". Doubles as the server half of
   the Indexing Log Buffer item. Scope reporting to the operation's own parse, not the
   session-wide `-errors`.
7. **Give the remaining XrefMode-only tests a server-mode home.** Standard *defines* is
   already done (`9e357408`); three are left, and only one is a straight conversion:
   - `tests/test_discover_standard_includes` — asserts the compiler's include directories
     were found, negatively: it greps the run's log for "can't open file stdbool.h" and
     requires it absent. A server version can assert it positively, from the dump.
   - `tests/test_multipass` — DONE for the symbol half. Multi-pass does fire in server
     mode: `maxPasses` starts at 1 and the per-pass config read raises it, and the
     converted test gets `pass1` and `pass2` from mutually exclusive `#ifdef` branches in
     one table, byte-identical to the XrefMode output. What is still missing is its
     *other* half, that the syntax error planted in the PASS2 branch is reported with
     file and line: `formatMessage()` drops the position in server mode. Restore that
     assertion when item 6 lands.
   - `tests/test_options_optinclude` — not convertible, rewrite it. It asserts
     `-optinclude` indirectly through `-refnum=1` and `ls CXrefs | wc -l`, and both the
     option and that layout die with XrefMode. Put something in `more_options` that
     changes parsing — a `-D` that flips an `#ifdef`, or an `-I` that resolves an
     include — and assert it from the dump.
8. **Decide what a bare `c-xref file.c` does** once XrefMode is no longer the default
   mode. A decision, not code.
9. **Remove XrefMode** — deletes the `-create`/`-update` legacy engine. Needs the three
   items above. ADR-0027's expanding-CU dependency is discharged: the include-graph walk
   replaced the marker rather than recording it (`3a17bf78`), so nothing writes or reads
   it any more.
10. **Remove the `parseBufferUsingServer` bridge** — §17.3; 9 refactoring call sites
    re-entering `callServer`. Independent of the rest of this chain (it asserts
    `ServerMode`), but it is the last divergent parse path.

## 3. Correctness, by (quiet × cheap)

11. **Extract passes statics by value** — *no test pins it.* Since the gate fix this
    compiles and silently does the wrong thing, where before it failed to compile.
    Liveness-after-the-region is the wrong question for static storage: treat
    static/thread-local as live after the region in `classifyVariableUsingDataFlow`
    (`src/extract.c`), which pushes it to `CLASSIFIED_AS_IN_OUT_ARGUMENT`. Failing system
    test first. Quietest bug on this list.
12. **Token pasting trio**, in this order — the later two assume the first
    (`doc/docs/18-known-bugs.adoc`): `tests/test_token_pasting_numbers` (changes the
    passing `test_collate_const_suffix_pasting`) → `tests/test_token_pasting_float` →
    `tests/test_collate_hex_prefix_pasting` (touches how every number is lexed).
13. **Header static: prototype and definition get different link names** —
    `setStaticFunctionLinkName` (`src/semact.c`); renames break the build today.
    `tests/test_static_declared_and_defined_in_header/.suspended` has the
    `exactPositionResolve` question to settle first, so it starts as a decision.
14. **`prepareInputFileForRequest`** (`src/server.c`) — resume from the clue, not the
    symptom: the extra scheduled file appears *only* when a header is the request file, so
    the question is what schedules it, not why the `fileNumber` tie-break loses. Pinned by
    `tests/test_browsing_push_in_unexpanded_macro/.suspended` and a suspended Cgreen test.
    A naive `options.inputFiles` fix broke ~50 tests once (`00a8ded3`).
15. Then, roughly by cost — each has a `.suspended` note or a `18-known-bugs.adoc` entry:
    `tests/test_getproject_unknown_cu_under_include_path` (any file under an `-I`
    directory counts as project) · `tests/test_preprocess_edit_removes_ifdef_define` (CU
    reparse leaves a header declaration it no longer emits — ADR-0025 variant B) ·
    `tests/test_browsing_push_by_name` (needs decided behaviour when a name has several
    bindings) · GlobalUnused false positive for statics in `.y` files ·
    `tests/test_parsing_generics` (`_Generic` not parsed) · **a refactoring can see a
    truncated reference set** (`18-known-bugs.adoc`) — severe where it bites, rewriting 13
    of 25 occurrences on ffmpeg, but it needs a header included by more than ~130 CUs, and
    c-xrefactory has 79 with a worst fan-in of 39, so it cannot happen here. That, and
    not its severity, is why it sits in this bucket.

## 4. Performance, in strict dependency order

Roadmap → Optimization. Do the Pass 3 item first; it may change the measured baseline (cold-start
PUSH on ffmpeg `af_afir.c`: scan 2.7s, two Pass 3 rounds 19s each, 42s total).

16. **Header-filtered sibling parsing** — 2481 sibling CUs → 2 for `AudioFIRContext`. The
    design problem is that Pass 3 runs during sync and the symbol is only known during
    dispatch.
17. **Extend the lightweight scan to `<...>` includes** — blocked on header-filtered
    sibling parsing, or ubiquitous system headers drag in nearly every CU. Also removes
    the cold-start double progress bar.
18. **Lexing cache re-introduction** — present in the original codebase, lost in
    restructuring.
19. **Parallel parsing** — last, and only if the three above leave `avcodec.h` (614 CUs) or
    `internal.h` (1171 CUs) unacceptable. Global mutable parser state, a single CX arena
    and a shared file table are the obstacles.

## 5. Features, by readiness

20. **Move Function comment y/n prompt** — replaces the `c-xref-comments-moving-level`
    customization; collapse `CommentMovingMode` to a bool and stop the backward walk at a
    blank line (`src/options.h`, `src/move_function.c`). The TDD scaffolding already
    landed: four `tests/test_move_function_*comment*` tests; sweep
    `-commentmovinglevel=6` → `=1` in the three "with…" `commands.input` files and add
    `test_move_function_stops_at_blank_line` in the same change. Most shovel-ready item
    here.
21. **Index-based sessions** — dissolves stale-POP *and* lets an answer grow while you
    work. Roadmap → Memory as Truth → Remaining. Depends only on entry refresh, which is
    done.
22. **Indexing Log Buffer** — Option A (`PPC_LOG` + a silent `*c-xref-log*` buffer),
    `doc/docs/11-planned-features.adoc`. Follows the report-errors item.
23. **Retry the request that created the project** — small, and it becomes first contact
    with every new project once auto-discovery is the only way in.
24. **LSP tiers 1–2** — code actions and `workspace/executeCommand` for extract, move
    function and the parameter refactorings. Stubs exist: `handle_code_action`,
    `handle_execute_command`, with `codeActionProvider` commented out in
    `src/lsp_handler.c`. Keep tier 3 (custom methods + per-editor extension code) small.
25. **Move Function next steps** — remove the source header's extern declaration, include
    management, helper-function detection, smarter header placement, preview — then
    **Delete Function**. Also **CreateMode** (ADR-0024), **unused-detection exclude
    patterns**, **browsing includes**, **semantic read-only files**, **rename handles
    `expect`**, **project-local config**. All in `11-planned-features.adoc`.
26. **Chapter 17 hygiene, opportunistically** — incremental `cxfile.c` cleanup, extract
    the macro expansion module, hashtab → hashlist, split the editor module, rename server
    operations, elisp recompiled and deleted on every build.

## Open questions that would reorder this

* **ADR-0027's expanding-CU dependency is discharged** (answered 2026-09-22). The walk
  replaced the marker: `addMacroBaseUsageRef` and its call site are gone, so nothing
  writes or reads it. §2 has lost its hardest precondition and could move ahead of §1 —
  that reordering is still a choice, not a conclusion. What ADR-0027 still lacks is rename
  refusing from inside a macro body, and completeness above the collection caps; it stays
  `Accepted`, not `Implemented`.
* **The Pass 3 item is an observation, not a confirmed bug.** If it is false, §4's ordering stands
  but its baseline does not.
* **`18-known-bugs.adoc` cites `test_browsing_push_name_parses_whole_project`**, which
  does not exist; the suspended directory is `tests/test_browsing_push_by_name`. Fix the
  reference when touching the remaining-bugs item.
* **Outside this repo:** the external regression scripts still lack the orphan-test-dir
  filter that `utils/failing` got in `db064a2a` — a `tests/test_*/` with an `output` but
  no `Makefile` reads as a failure.

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

1. **Pass 3 sets no `lastParsedMtime`** — if real, every request reparses the neighbourhood
   Pass 3 exists to bound, and the parse-all check for project-wide operations miscounts
   the same way. *No repo home.* Observed while tracing
   `tests/test_browsing_push_in_unexpanded_macro`: `source.c` parsed during `-getproject`
   and again during the following `-push`, both logging "0 skipped (already parsed)".
   Start at `parseUnparsedSiblingCUs` (`src/server.c`) and compare with the completeness
   parse in `callServer`, which sets `fi->lastParsedMtime` explicitly. Verify with a
   two-request test before calling it a bug. Upstream of everything in §4.
2. **Two `-pass` parsers that disagree** — *no repo home.* `readOptionsIntoArgs`
   (`src/options.c:900`) matches `strncmp(optionText, "-pass", 5)` with no digit check,
   then `sscanf`s the number; on a malformed marker like `-passfoo` the scan fails and the
   pass number silently keeps the **previous section's** value, so the options that follow
   are attributed to the wrong pass. It also raises `maxPasses` to whatever it reads, with
   no bound, and that drives the pass loop. `readOptionSets` (`src/optionsets.c`) got both
   checks in `700ef8fc`. The fix is one shared predicate used by both, but it changes how
   `-passfoo` behaves in the hot config-reading path, so it wants its own commit and a full
   system-test run. Report it per the errorMessage convention, not with an assert.
   *(The sibling item — the three reads of `projectConfig.optionSets` clearing
   inconsistently — was fixed by `resetOptionSets`; `optionSetsLoaded` in `src/startup.c`
   is the leftover worth a second look.)*
3. **Expanding-CU rename** — `UsageMacroBaseFileUsage` → `UsageMacroExpandingCU`,
   `addMacroBaseUsageRef` → `addExpandingCUMarker`, `findMacroExpansionFile` →
   `findExpandingCU`. Terminology in `doc/docs/06-principles.adoc` already uses the new
   words and cites the old identifiers; update the parenthesised names afterwards. No
   snapshot format bump — the usage is stored numerically.

## 1. The foundational one

4. **Partition `options` by lifetime** — Session / Project / Request, one owner each, and
   a pure `parseCommandLine()`. `doc/docs/17-major-codebase-improvements.adoc` §17.4. The
   roadmap's Setup Ladder says it gates the rows below it. Largest single item here.
5. **The Setup Ladder, in this sequence**, each small once 4 lands
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
   item 22. Scope reporting to the operation's own parse, not the session-wide `-errors`.
7. **Give the three XrefMode-only tests a server-mode home** — standard defines, per-pass
   `-D`, `-optinclude`.
8. **Decide what a bare `c-xref file.c` does** once XrefMode is no longer the default
   mode. A decision, not code.
9. **Remove XrefMode** — deletes the `-create`/`-update` legacy engine. Needs 6, 7, 8 and
   ADR-0027's expanding-CU marker.
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
    `tests/test_parsing_generics` (`_Generic` not parsed).

## 4. Performance, in strict dependency order

Roadmap → Optimization. Do item 1 first; it may change the measured baseline (cold-start
PUSH on ffmpeg `af_afir.c`: scan 2.7s, two Pass 3 rounds 19s each, 42s total).

16. **Header-filtered sibling parsing** — 2481 sibling CUs → 2 for `AudioFIRContext`. The
    design problem is that Pass 3 runs during sync and the symbol is only known during
    dispatch.
17. **Extend the lightweight scan to `<...>` includes** — blocked on 16, or ubiquitous
    system headers drag in nearly every CU. Also removes the cold-start double progress
    bar.
18. **Lexing cache re-introduction** — present in the original codebase, lost in
    restructuring.
19. **Parallel parsing** — last, and only if 16–18 leave `avcodec.h` (614 CUs) or
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
    `doc/docs/11-planned-features.adoc`. Follows item 6.
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

* **Is ADR-0027's expanding-CU marker dependency discharged?** `0b029fc2`, `3a17bf78` and
  `092db313` landed exactly that work. If the include-graph walk *replaces* the marker
  rather than still recording it, §2 loses its hardest precondition and could move ahead
  of §1.
* **Item 1 is an observation, not a confirmed bug.** If it is false, §4's ordering stands
  but its baseline does not.
* **`18-known-bugs.adoc` cites `test_browsing_push_name_parses_whole_project`**, which
  does not exist; the suspended directory is `tests/test_browsing_push_by_name`. Fix the
  reference when touching item 15.
* **Outside this repo:** the external regression scripts still lack the orphan-test-dir
  filter that `utils/failing` got in `cadb4cb5` — a `tests/test_*/` with an `output` but
  no `Makefile` reads as a failure.

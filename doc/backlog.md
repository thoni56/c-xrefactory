# Backlog

What to work on next, and in what order. As of 2026-09-24.

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

## 0. Verify first — cheap, and might change the cost of the rest

1. **An operation's cursor parse re-parses a CU that Pass 3 already parsed** — *no repo
   home.* Traced 2026-09-23 on `7ba5650f` in `tests/test_browsing_push_in_unexpanded_macro`,
   cold: `-getproject` parses `source.c` in Pass 3, `-push` skips it as already parsed — so
   entry refresh is doing its job — and then the macro-body path parses it again to resolve
   the cursor, as `-push` always does with the request file. The open question is whether an
   operation's cursor parse should reuse a CU's references instead of re-parsing it.

2. **A cold start interrogates the compiler twice** —
   `tests/test_cold_start_interrogates_compiler_once/.suspended`. Waits for item 3,
   steps 2a and 2b.

## 1. The foundational one

3. **Partition `options` by lifetime** — Session / Project / Request, one owner each, and
   a pure `parseCommandLine()`. `doc/docs/17-major-codebase-improvements.adoc` §17.4. The
   roadmap's Setup Ladder says it gates the rows below it. Largest single item here.
   Begin with step 2 of that section, extracting side effects, in the order it gives.
   When it moves `ProjectConfig` off being a file-static singleton (`src/startup.c:42`,
   `static ProjectConfig projectConfig = {0};`), give it a `makeProjectConfig()` that
   returns `{.sourceDirs = NULL, .optionSets = makeOptionSets()}`. Today both fields are
   emptied by code that frees first — `freeStringList(projectConfig.sourceDirs)` and
   `resetOptionSets()` — which is only safe because static storage zeroes them; an
   automatic `ProjectConfig` declared without `= {0}` would free garbage. A constructor
   makes "born empty" one expression instead of every declaration site remembering, and
   gives `makeOptionSets()` its first production caller.
4. **The Setup Ladder, in this sequence**, each small once the partition lands
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

5. **Remove `-refs` and `-refnum`** — ADR-0028. The snapshot is always
   `<project root>/.c-xref/db` and always one file. Deletes the partition handling in
   `src/cxfile.c` (the hash modulo, the per-file loops, the single-versus-many branches)
   and makes `applyConventionBasedDatabasePath()` unconditional. Decide what happens to
   the `CXFI_REFNUM` record in the snapshot — keeping it and always writing 1 costs
   nothing, changing the format involves `test_snapshot_version_migration`. Fixtures
   assume partitions: the common `tests/c-xrefrc.tpl` sets both options, ten per-test
   templates set `-refnum`, and `tests/test_autodetect_creates_cxref_dir` asserts the
   `X0000…X0009` layout because of it. A config that still says either option gets an
   `errorMessage` and is otherwise honoured.
6. **Remove `-xrefrc`** — ADR-0005 called for this in 2022, when discovery was still a
    proposal: "when this functionality is implemented you'd just remove those options and
    add a `.c-xrefrc` in the root of the test directory instead". Discovery landed in
    `d5a7182f`, and every test does have a generated `.c-xrefrc` in its directory — while
    `tests/Makefile.boilerplate:17` still passes `-xrefrc .c-xrefrc -p $(CURDIR)` as well,
    and six `commands.input` files pass it too. Removing it also retires the "legacy path"
    that `doc/docs/14-code.adoc` documents as retained for explicit configurations. The
    case it leaves unanswered — a tree you cannot write a config into — is recorded in
    ADR-0028 as unsupported for now. `-stdop` and `-no-stdop` are named in the same ADR
    sentence; check whether they still exist before assuming they need removing too.
## 2. The XrefMode chain

7. **Re-key the 19 `options.mode != ServerMode` guards in `src/yylex.c` onto a
   report-errors flag**, and stop `formatMessage()` (`src/commons.c`) dropping the position
   in server mode. Roadmap, "Report what did not parse". Doubles as the server half of
   the Indexing Log Buffer item. Scope reporting to the operation's own parse, not the
   session-wide `-errors`. Restore `tests/test_multipass`'s error-position assertion in
   the same change: it was dropped when that test moved to the server, because the
   position is what server mode throws away.
8. **Decide, and enforce, what the startup command line may carry** — *no repo home yet.*
   Bigger than `c-xref file.c`, and it needs code, not just a decision. Once XrefMode is
   gone the server takes no file at startup: every request names its own, and the same
   parser serves both phases, so the question becomes which options are process-scoped at
   all. The Setup Ladder's step 1 answers it in principle; this makes it concrete.
   - Genuinely startup, on the evidence of what the client sends and what `main()` reads
     before anything else: the mode; the transport (`-crlfconversion`, `-crconversion`,
     `-o <answerfile>` — `editors/emacs/c-xref.el:1615`); logging
     (`-log=`, `-debug`, `-trace`, `-info`, `-errors`, `-warnings`, `-infos`, scanned in
     `main()` before the mode is even known); and `-statistics`.
   - Everything project-scoped — `-p`, `-xrefrc`, `-I`, `-D`, `-optinclude` — belongs to
     the config and binds at `-getproject`, not here. `-refs` and `-refnum` are not on
     that list any more: they are removed outright, see the items below.
   - **`-exactpositionresolve` has no strategy.** It is real: it changes link names
     (`src/semact.c`) and goes into the snapshot's check number (`src/cxfile.c`). Since it
     changes what a symbol is, it is probably project-scoped, although `options.h` marks it
     REQUEST. Decide whether it belongs in the config or goes. Item 13 waits on the same
     question.
   - **`-lsp` is a third mode and should be one.** `want_lsp_server()` (`src/lsp.c`) scans
     argv in `main()` and returns before `mainTaskEntryInitialisations()`, so it is a mode
     in behaviour but not in `options.mode`. If "state a mode" is the rule, it should say
     so the same way `-server` does.
   - The code: `startup.c:841` processes the command line with
     `PROCESS_FILE_ARGUMENTS_YES`, which is why `c-xref -server file.c` silently schedules
     that file. Flip it to `NO` when XrefMode goes. But `NO` only *ignores*: `matched =
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
9. **Remove XrefMode** — deletes the `-create`/`-update` legacy engine. Needs items 6 and
   7; nothing else holds it up. `-xrefactory-II` goes with it: it selects the protocol
   output over XrefMode's plain text, and with only the server left there is nothing to
   select. Make `options.xref2` the default, then drop the flag and the client's use of
   it (`editors/emacs/c-xref.el:1615`).
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
14. Then, roughly by cost — each has a `.suspended` note or a `18-known-bugs.adoc` entry:
    `tests/test_getproject_unknown_cu_under_include_path` (any file under an `-I`
    directory counts as project) · `tests/test_preprocess_edit_removes_ifdef_define` (CU
    reparse leaves a header declaration it no longer emits — ADR-0025 variant B) ·
    `tests/test_browsing_push_by_name` (needs decided behaviour when a name has several
    bindings) · GlobalUnused false positive for statics in `.y` files ·
    `tests/test_parsing_generics` (`_Generic` not parsed) · a compilation unit deleted
    outside the editor is never let go (`18-known-bugs.adoc`) · **a refactoring can see a
    truncated reference set** (`18-known-bugs.adoc`) — severe where it bites, rewriting 13
    of 25 occurrences on ffmpeg, but it needs a header included by more than ~130 CUs, and
    c-xrefactory has 79 with a worst fan-in of 39, so it cannot happen here. That, and
    not its severity, is why it sits in this bucket.

## 4. Performance, in strict dependency order

Roadmap → Optimization. Baseline: cold-start PUSH on ffmpeg `af_afir.c` — scan 2.7s, two
Pass 3 rounds 19s each, 42s total.

15. **Header-filtered sibling parsing** — 2481 sibling CUs → 2 for `AudioFIRContext`. The
    design problem is that Pass 3 runs during sync and the symbol is only known during
    dispatch.
16. **Extend the lightweight scan to `<...>` includes** — blocked on header-filtered
    sibling parsing, or ubiquitous system headers drag in nearly every CU. Also removes
    the cold-start double progress bar.
17. **Lexing cache re-introduction** — present in the original codebase, lost in
    restructuring.
18. **Parallel parsing** — last, and only if the three above leave `avcodec.h` (614 CUs) or
    `internal.h` (1171 CUs) unacceptable. Global mutable parser state, a single CX arena
    and a shared file table are the obstacles.

## 5. Features, by readiness

19. **Take the client's "Remove References and Restart Server" back out** — it landed
    2026-09-24 as a deliberate stopgap (`c-xref-project-remove-references-and-restart`,
    `editors/emacs/c-xref.el`), because discarding the database is the standing remedy for
    three unrelated symptoms: shadow occurrences after behind-the-back disk changes, the
    deleted-compilation-unit reparse loop, and a stale snapshot outliving a visibility fix
    (all three in `doc/docs/18-known-bugs.adoc`). The real work is making the database not
    need discarding — behind-the-back detection and entry refresh are probably most of it,
    and item 21 dissolves another part. Remove the entry when they land, and check the
    three symptoms are gone rather than assuming it.

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
    `doc/docs/11-planned-features.adoc`. Follows the report-errors item. The client half
    does not have to be invented: `c-xref-tags-dispatch` and its two helpers in
    `editors/emacs/c-xref.el` rendered exactly this for the `-create` log — a stream of
    PPC records into `*c-xref-log*`, severity faces, `file://` links made clickable — and
    are kept, uncalled, for that reason. The viewer commands and keymap below them are
    still bound; only the producer is gone.
23. **Retry the request that created the project** — small, and it becomes first contact
    with every new project once auto-discovery is the only way in.
24. **LSP tiers 1–2** — code actions and `workspace/executeCommand` for extract, move
    function and the parameter refactorings. Stubs exist: `handle_code_action`,
    `handle_execute_command`, with `codeActionProvider` commented out in
    `src/lsp_handler.c`. Keep tier 3 (custom methods + per-editor extension code) small.
25. **Move Function next steps** — remove the source header's extern declaration, include
    management, helper-function detection, smarter header placement, preview — then
    **Delete Function**. Also **`-parse-all`, a request that parses every compilation
    unit the scan found and has not parsed** — what ADR-0024's CreateMode reduces to
    once the priming is a request rather than a mode; `test_ffmpeg` and
    `test_systemd` ask for it today by searching a name nobody defines and answering
    `-continue`. If it becomes something a user asks for, it needs to say how long
    it will take: `src/progress.c` already keeps `timeZero` and a monotonic clock,
    and the parse loops already count down, so the missing part is the division and
    a format that says "4 min left" rather than a remaining count. Decide the unit
    first — CUs are uneven enough (`avcodec.h`'s consumers against a 40-line `.c`)
    that a mean over them swings early and settles late, where lines or bytes are
    steadier and both are known before parsing starts.
    Also **unused-detection exclude patterns**, **browsing includes**,
    **semantic read-only files**, **rename handles `expect`**, **project-local
    config**, **Inline Function and Inline Macro**. All in `11-planned-features.adoc`.
26. **Local config fragments — the need, not a solution** — *no repo home yet.*
    `.c-xrefrc` travels with the project and is checked in, which is why
    `11-planned-features.adoc` argues for it: "it will not contain absolute file paths".
    Some things are machine-specific and still have to be said somewhere — where the
    source of a library you want to navigate into actually lives, an `-I` into a local
    toolchain — and today there is nowhere to say them.
    Two directions, neither chosen, and the point of this entry is the need rather than
    either of them. `-optinclude` already reads another options file, but it processes it
    with `PROCESS_FILE_ARGUMENTS_NO` (`handleIncludeOption`, `src/options.c`), so an
    included file cannot contribute source directories — the main thing this need wants —
    and `readOptionsFromFile` makes a missing file fatal, where absent is the normal case
    for a machine-specific fragment. The other direction is a sibling file picked up
    automatically, where absent is normal by construction and nothing checked in refers
    to it. Either way, decide whether a fragment extends or replaces the project config.
27. **Chapter 17 hygiene, opportunistically** — incremental `cxfile.c` cleanup, extract
    the macro expansion module, hashtab → hashlist, split the editor module, rename server
    operations, elisp recompiled and deleted on every build.

## Open questions that would reorder this

* **Should §2 move ahead of §1?** Its hardest precondition is gone — the include-graph
  walk replaced ADR-0027's expanding-CU marker — so the XrefMode chain no longer waits on
  anything but its own items. Whether it goes before the lifetime partition is a choice
  nobody has made.
* **ADR-0027 stays `Accepted`, not `Implemented`:** rename does not yet refuse from inside
  a macro body, and the menu is complete only up to the collection caps.
* **`18-known-bugs.adoc` cites `test_browsing_push_name_parses_whole_project`**, which
  does not exist; the suspended directory is `tests/test_browsing_push_by_name`. Fix the
  reference when touching the remaining-bugs item.
* **Outside this repo:** the external regression scripts still lack the orphan-test-dir
  filter that `utils/failing` got in `db064a2a` — a `tests/test_*/` with an `output` but
  no `Makefile` reads as a failure.

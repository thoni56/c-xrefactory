# Backlog

What to work on next, and in what order.

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

An item is a `###` heading with its number, its title and its name in backticks, e.g.
`` ### (#22) Index-based sessions `index-based-sessions` ``. A part of an item is a `####`
heading with a letter instead of the number. A Foundation item is a bullet, also with its
name last. A `Waits for:` line names what an
item waits for. An `After:` line names the one item it should preferably come after, an
order that can be skipped, and these make the themes trees. An order may be invented,
e.g. for a group of small bugs. An item with neither stands alone in the graph.
The docs build draws the dependency graph on every push, `Waits for` thick and dark and
`After` dashed and grey, linked at the end of this file. `utils/backlog_graph --check`,
run with the tests, fails if a name is unknown.

## 1. The foundational one

### (#3) Partition `options` by lifetime, in this sequence `partition-options`

Session / Project / Request,
one owner each, and a pure `parseCommandLine()`. `doc/docs/18-major-codebase-improvements.adoc` §17.3.
The roadmap's Setup Ladder says it gates the rows below it.

#### c. A change of pass does not reload the config `config-without-pass`

Both reads in
`loadProjectSettings()` are already without a pass, and a pass's options come through
`applyOptionSet()`, so the full reload on a change of pass (`initializeFileProcessing()`)
has nothing left to reload. Only the first load needs it. Takes
`test_cold_start_interrogates_compiler_once` off suspension. A first attempt also changed
three move function tests, for a reason not understood. §17.3 step 2.b.

#### d. `-D` is applied when a parse starts `defines-at-parse-start`

From `definitionStrings`, which takes
the config's defines out of the memory checkpoint. §17.3 step 2.c.

After: config-without-pass

#### e. Opening the output file and initialising file processing move into the dispatcher `side-effects-to-dispatcher`

The last side effects of option parsing. §17.3 step 2.d.

After: defines-at-parse-start

#### f. A rebuild keeps the request's options `rebuild-keeps-request-options`

Today the fast path of
`initializeFileProcessing()` copies `savedOptions` over `options`, and `parseFileWithFullInit()`
keeps only `cursorOffset`, `serverOperation` and, as stopgaps, `dumpSelection`
(`test_dump_after_a_rebuild`) and `index` (`test_completion_select_after_a_rebuild`). The
other way round, the `-getproject` lock copies all of `options` into `savedOptions`
(`src/cxref.c:1829`), so a request with no operation gets a `-getproject` answer (WSL, session
`d25255b8`, 2026-10-14). Solved once `savedOptions` holds only Session and Project.

After: side-effects-to-dispatcher

#### g. `ProjectConfig` grows, and gets `makeProjectConfig()` `project-config-constructor`

§17.3 step 1, one
field at a time. When it stops being a file-static singleton (`src/startup.c:42`), the
constructor returns `{.sourceDirs = NULL, .optionSets = makeOptionSets()}`, since both
are emptied today by code that frees first and is safe only because static storage is zeroed.

#### h. A typed `Request` for one operation `typed-request-pilot`

`OP_SEARCH` or `OP_ABOUT`, parsed and
dispatched end to end. §17.3 step 3. `inputFileName`'s request half goes here.

After: side-effects-to-dispatcher

#### i. Refactoring owns a `RefactoringRequest` `refactoring-request`

`refactoringOptions`,
`deepCopyOptionsFromTo(&options, &refactoringOptions)` and `beInteractive`'s
`localSavedOptions` go together. §17.3 step 4.

After: typed-request-pilot

### (#4) The Setup Ladder, in this sequence `setup-ladder`

Each small once the partition lands
(`doc/docs/10-roadmap.adoc`, Convergence: The Setup Ladder → Remaining):

#### b. The client stops sending `-p` on every request `client-stops-p`

ADR-0029: this is not tidying,
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
With `-p` gone, `initializeFileProcessing()` (`src/startup.c`) can drop its "or a
different project" condition and its discovery branch, keeping only the locked one.
The lock already makes the config names the same, so today they guard only a request
with `-p` and no `-getproject` (WSL, session `d25255b8`, 2026-11-18).
`previousProjectConfigurationFile` is then nothing previous, only the project's config
file, set at `-getproject` and read by the config's change check and reload. It can
move into `ProjectConfig` (`project-config-constructor`) as a field, beside what
`lockedProject` says about the same project, where its name does not clash with the
locals called `projectConfigFileName`.

Waits for: adr-0029, project-extent

#### c. Project setup moves into the future `-openproject` `setup-into-openproject`

Discovery, config read, compiler
interrogation, snapshot load;
Then `initializeFileProcessing()` discovers nothing. Today it must skip the config
search once locked, since the search sets `autoDetectedProjectRoot` (`src/options.c`)
and so `cxFileLocation`. `initializeProjectContext()` has a copy of that discovery.

Waits for: side-effects-to-dispatcher, adr-0030

#### d. The assert that nothing before the future `-openproject` reads a project-scoped option `openproject-tripwire`

The
tripwire, as the disk-read assert was for Memory as Truth;

Waits for: setup-into-openproject

#### e. What a project consists of `project-extent`

ADR-0031 (accepted 2026-10-02). One definition of
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

## 2. After XrefMode

### (#23) The diagnostics buffer reports unresolved includes, with a suggested `-I` `indexing-log-buffer`

`doc/docs/11-planned-features.adoc` (Diagnostics Buffer). What did not parse reaches
`*c-xref-diagnostics*` with its position, grouped by unit, and `next-error` visits it.
Left: the report of unresolved includes.

After: project-config-constructor

### (#68) An `LspMode`, and a sink for the messages `lsp-mode`

*no repo home yet.*
LSP sets `ServerMode` (`src/parsing.c:140`), a stop-gap for the `assert(options.mode)`
that are gone now, and nothing reads the mode any more. `Mode` becomes `ServerMode`
or `LspMode`, set once from argv (`startup-command-line`), and it replaces `cxrefProtocol`. A
check of the mode deep in the code is where packing is mixed into collecting. `src/commons.c` then sends messages to a sink
chosen at startup: `<log>` and `<analysing>` for `-server`,
`textDocument/publishDiagnostics` for `-lsp`, which replaces a document's diagnostics
as a reparse replaces a unit's group. The first case of what #25 needs.

### (#7) Decide, and enforce, what the startup command line may carry `startup-command-line`

*no repo home yet.*
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

### (#78) Each option carries its scope, and `processOptions` refuses one outside it `options-carry-scope`

*no repo home yet.*
Every source goes through the same `processOptions()` and every option is accepted
from all of them. The sources are three contexts: startup (`mainTaskEntryInitialisations()`),
project (the config read in `getAndProcessProjectConfig()`, `applyOptionSet()`, and an
`-optinclude`, which takes the context it appears in) and request (`initServer()`). These are #3's lifetimes, which `options.h`
already marks per field as SESSION, PROJECT and REQUEST.
`processOptions()` takes the context, and an option's entry states where it is allowed, e.g. a `-server` in
`.c-xrefrc` is reported with its position, as an unterminated quote is. #7 decides the
startup column. The replays in `loadProjectSettings()` and
`initializeFileProcessing()` are not a context, they go with #3.
Test first: an `options_tests.c` case where a request-only option in the project
context is refused (WSL, session `d25255b8`, 2026-10-25).

After: indexing-log-buffer

### (#62) Audit the docs, chapter by chapter `docs-audit`

Much of `doc/docs/` was
written while the batch modes and the disk database were the truth. Each chapter is
read against the code as it is and against the shape, "The Shape It Converges To" in
`08-algorithms`, and what is stale is fixed or becomes a line here.
`06-principles` first, e.g. the snapshot paragraph in "Reference Database and Parsing".
`14-code`'s server request flow, the dispatch tree with its line numbers, the Pass
1/2/3 entry refresh and `firstPass`, describes code that is gone.
`12-components`' Content Buffers, "Staleness Detection", compares `lastParsedMtime`
with the buffer's modification time; that field is gone, and a change is found by
its content token since ADR-0035/0036.
`bringHeldFilesUpToDate()` is named in `08-algorithms` ("Buffer-driven refresh" and the
call tree below it) and in `11-planned-features`' LSP Implementation Path, step 3; it
went in `86d9e9851`, when held CUs and siblings became one build list.

After: freshness-docs

## 3. Correctness, by (quiet × cheap)

### (#10) What a refactoring refuses, spelled out and tested `refactoring-refusals`

*no repo home yet.* Under ADR-0035 ghosts remain from the Variant-B gap
(`variant-b`). Missing references are
known, through out-of-date CUs, and a refactoring asks about them. Today rename and the parameter refactorings compare only
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
by changing a shared header on disk. The message "try re-saving the file" no longer fits. Found by
reading the code (Mac, session `05984816`, 2026-10-06), not tested.

Waits for: adr-0035

### (#111) Files that appear or vanish between requests are not seen `files-appear-vanish`

An accepted gap, `doc/docs/12-components.adoc` ("Design boundary"), to be looked at
again. Files are found only by the walk on the first request and on a config change,
by a request naming one, and by an include a scan or parse resolves. So a new CU that
nothing includes and no request names is missing until a restart, also from an
operation on a name, and nothing says so. Deletion is seen for CUs only:
`markMissingFilesAsDeleted()` at the walk, or when a CU is about to be built. A
deleted header loses its references (ADR-0036), but is not marked
deleted, so it stays in the file table and is saved in the snapshot, a file that does
not exist and has no references. Its includers' scan edges to it stay. Once setup moves into
`-openproject` the request loop sees only changes to files it knows, so "until a
restart" becomes "until the next open". One way to see both: a directory's
modification time moves when an entry is added, removed or renamed, so stamp the
directories the walk visits and list again those whose time moved; a vanished file is
a change. `06-principles` (New files and deleted files) and `08-algorithms` (Deletion
Detection) describe more than the code does (Mac, session `4839a129`, 2026-11-06).

### (#94) Investigate: which refactorings insert text copied from the server's buffer `inserted-text-loses-non-ascii`

*no repo home yet.* A loaded buffer has each multibyte character replaced by a space
(`convertUtf8()`, `src/encoding.c`), so that byte offsets are character offsets. Text the
server copies from its buffer into an insertion loses its non-ASCII characters. Move
Function's `extern` declaration is one, a block it moves is not (the client moves that).
Extract Function is not checked. Seen in a Move Function's comment (WSL, session
`d25255b8`, 2026-10-28).

### (#13) Token pasting trio `token-pasting-trio`

In this order — the later two assume the first
(`doc/docs/19-known-bugs.adoc`): `tests/test_token_pasting_numbers` (changes the
passing `test_collate_const_suffix_pasting`) → `tests/test_token_pasting_float` →
`tests/test_collate_hex_prefix_pasting` (touches how every number is lexed).

After: files-appear-vanish

### (#14) Header static: prototype and definition get different link names `header-static-link-names`

`setStaticFunctionLinkName` (`src/semact.c`); renames break the build today.
`tests/test_static_declared_and_defined_in_header/.suspended` has the
`exactPositionResolve` question to settle first, so it starts as a decision.

Waits for: startup-command-line

### (#15) The other correctness bugs `correctness-by-cost`

Their order is not
known. Each has a `.suspended` note or a `19-known-bugs.adoc` entry,
or says it has no repo home yet:

#### b. A relative `-I` in `.c-xrefrc` resolves against the working directory of the server process `relative-include-path`

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

#### g. GlobalUnused gives a false positive for statics in `.y` files `unused-yacc-statics`

`19-known-bugs.adoc`.

After: token-pasting-trio

#### h. Push by name when a name has several bindings `push-by-name`

`tests/test_browsing_push_by_name`. Needs the behaviour decided first.

After: unused-yacc-statics

#### j. The LSP server counts columns in bytes in opened documents and in code points in files from disk `lsp-position-encoding`

*no repo home yet* (`src/lsp_handler.c`, `src/editor.c`). A file read from disk
goes through `performEncodingAdjustments()`, which makes every UTF-8 sequence one
space, so its columns are code points. A `didOpen` text goes through
`loadTextIntoEditorBuffer()`, which copies it without that collapse, so its columns
are bytes (and CR LF stays). Observed: `/*å*/ int myFunction…` in a `didOpen` gives
the definition at character 11, which is bytes; code points would be 10 (Mac,
session `f5b5c93a`, 2026-11-19). The disk half cannot be observed yet, since a
definition is not found outside the opened document.
So the announced "utf-8" (columns in bytes, `243477d33`, which meant the file's
encoding) is true for every answer today, but it is not negotiated, and it turns
false once an answer comes from a file on disk. The announcement used to be outside
`capabilities`, where no client read it; fixed in `3cd114c26`.
Fix: `didOpen` collapses as a file read does, so every column is a code point; then
announce "utf-32" when the client offers it in `general.positionEncodings`, and
nothing otherwise, which means "utf-16" and agrees outside the astral planes.

After: lsp-mode

#### l. A UTF-8 identifier is split into pieces `utf8-identifiers`

*no repo home yet* (`src/encoding.c`). The encoding collapse makes each UTF-8
sequence one space, which keeps columns equal to the editor's character columns, but
in an identifier it ends the identifier: `int på;` is lexed as `p` and nothing, so a
wrong symbol `p` gets the references. gcc and clang accept UTF-8 identifiers. Inside
strings and comments the collapse is harmless. A fix needs a placeholder that the
lexer reads as an identifier character, still one byte per character, and a symbol
name that is not the placeholder for every non-ASCII letter. From reading the code
(Mac, session `f5b5c93a`, 2026-11-19), not tested.

After: lsp-position-encoding

### (#58) Investigate: the client asks for search result number 0 `search-result-line-zero`

Typing a search string gave `-search-goto=0` at once, twice, each time
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

After: inserted-text-loses-non-ascii

### (#60) `#include MACRO` where the macro is `<...>` `include-bracket-from-macro`

The
header is not opened, so everything it declares is missing, e.g. FreeType's
`#include FT_FREETYPE_H`. The lexer makes one `STRING_LITERAL` of a `<...>` only right
after `#include` (`putIncludeString()`, `src/lexembuffer.c`); a macro body is ordinary
tokens. Join the tokens up to `>` into a header name (C11 6.10.2p4 leaves how to the
implementation; gcc joins their spellings). The tokens after `<` are not cut either,
as the `"..."` branch does, so they reach the parser as syntax errors
(`processIncludeDirective()`, `src/yylex.c`). `test_parsing_include_using_macro` pins
both; its two syntax errors go when this is fixed.

After: push-by-name

### (#69) Unconfirmed: Latin-1 that looks like UTF-8 is read as one character `latin-1-lookalikes`

*no repo home yet.*
`convertUtf8()` (`src/encoding.c`) takes a lead byte with its continuation bytes as
one character, so `å°` and one more byte in 0x80–0xBF, in Latin-1, become one space
and every position after it on the line is two characters early. Read a sequence that
would be an overlong or invalid code point as single bytes. A file that is not UTF-8
is not reported, it parses.

After: search-result-line-zero

### (#70) Investigate: selecting a completion says "selection out of range" `completion-select-out-of-range`

*no repo home yet.*
In `yylex.c`, half-way through an Add Parameter, `pre` and F8 listed 8 completions
over 16 lines. Enter on the last one, `previousLexemP`, sent `-completion-select=16`, which
is the right line. The answer was `selection out of range.`
`getMatchOnNthLine()` (`src/match.c`) counts wrapped lines and returns NULL only for an
empty list, so the session entry `selectCompletion()` found probably had no matches.
The completion request also got a syntax error at the cursor (858:45).

After: latin-1-lookalikes

### (#105) Investigate: a `.c` file that another file includes `included-c-file`

*no repo home yet.*
`isCompilationUnit()` decides by the file name, so an included `.c` is a CU. Building it
on its own removes the references its includer put there
(`removeReferenceableItemsForFile()` in `buildKnowledgeOfCU()`, `src/rebuild.c`). Find
out what the scan, the freshness check and a rebuild do with one, and pin it in a test.

After: completion-select-out-of-range

### (#71) A `'\n'` lexem has the next line's number `newline-position`

*no repo home yet.*
`handleNewline()` (`src/lexer.c`) puts the `'\n'` after the newline, and the blanks and
comments after it, have been read. Its column is where the line ended, its line is that
of the next lexem. A message at the end of a line lands on the line below, e.g.
`#if __has_include(` with nothing after it.

After: include-bracket-from-macro

### (#72) A malformed `#if` is reported twice `if-syntax-error-twice`

*no repo home yet.*
When `handleHasIncludeOperator()` (`src/yylex.c`) has reported what is wrong and
returns 0, `cppexp_yyparse()` adds "Syntax error on: `__has_include`"
through `styyerror()` (`src/semact.c`). The second message says nothing more.

After: newline-position

### (#122) A server waiting for a confirmation after the client failed `server-waits-after-client-error`

*no repo home yet.*
`ppcWaitConfirmation()` (`src/ppc.c`) reads stdin until it gets `-continue` or `-cancel`.
If the client signals an error while it shows the question, nothing answers, and the
next request's options arrive where the server expects the answer. Find out what the
server does then, and pin it in a test. Found on WSL, session `d25255b8`, when the
question could open beside the minibuffer (fixed in the client).
A case: `test_multipass` sends `-continue` after a search that asks nothing, so it
arrives as a request ("unknown option -continue"). It passes today, but with `OP_SEARCH`
out of `requiresProcessingInputFile()` the server stops on `log_fatal` in
`answerEditorAction()` for `OP_NONE`. Probably the request inherited `OP_SEARCH` before
(WSL, session `d25255b8`, 2026-11-22).

After: if-syntax-error-twice

## 4. Performance, in strict dependency order

Roadmap → Optimization has the baseline: a cold-start PUSH on ffmpeg, measured in March
2026 and not since.

### (#16) Header-filtered sibling parsing `header-filtered-siblings`

For an operation on a symbol, build
only the CUs that reach a file declaring it: for `AudioFIRContext` from `af_afir.c` on
ffmpeg that is 2, not the 1567 collected today. What to build is collected before the
operation, and the symbol is known only from the cursor parse. Looking the cursor up
in the table (`cursor-parse-reuse`) gives the symbol first, when the request CU is
current. It narrows what `request-target` widens: the reach of the symbol, not of the
request file's closure. A CU that includes the declaring file only with `<...>` is
then missed, also when another header brings it in today (`scan-angle-includes`).

After: request-target

### (#17) Extend the lightweight scan to `<...>` includes `scan-angle-includes`

Blocked on header-filtered
sibling parsing, or ubiquitous system headers drag in nearly every CU. Also removes
the cold-start double progress bar.
Measured on WSL, session `d25255b8`, 2026-11-06: in `tests/test_systemd`, 29 of about
7000 include lines name the project's own headers with `<...>`, 20 of them
`<libudev.h>`. `tests/test_ffmpeg` includes its own headers quoted, except `doc/examples`,
which includes `<libavcodec/avcodec.h>`: 16 references to `avcodec_open2` are missed there
(WSL, session `d25255b8`, 2026-11-22). cgreen's own
sources are the large case, `<cgreen/...>` throughout. Resolving `<...>` against the
`-I` directories only, not the compiler's, might leave the system headers out and
lift the block. Which headers are the project's own is `project-extent`'s question.

Waits for: header-filtered-siblings

### (#18) Lexing cache re-introduction `lexing-cache`

Present in the original codebase, lost in
restructuring.

Waits for: scan-angle-includes

### (#19) Parallel parsing `parallel-parsing`

Last, and only if the three above leave `avcodec.h` (614 CUs) or
`internal.h` (1171 CUs) unacceptable. Global mutable parser state, a single CX arena
and a shared file table are the obstacles.

Waits for: lexing-cache

## 5. Features, by readiness

### (#85) Move Function asks about a `static` shared with what stays `move-shared-static`

Make it extern where it is, or move it and make it extern from the target.
11-planned-features → Move Function → A `static` shared with what stays.

### (#87) Move Function to New Module `move-to-new-module`

Create the target `.c` and the includes
the function needs, then make the ordinary move, which creates the header, so the refusal and the helpers come with it. Adding the module to the build, and a mock, may be the user's.
11-planned-features → Move Function → Next steps.

After: move-includes

### (#86) Extract Function passes an array declared in the region and used after it `extract-array-used-after`

Today it refuses (`refuseToReturnAnArray()`, `src/extract.c`), since a function can't return
an array. Move the declaration out of the region, before the call, and pass the array, as
when it is declared before the region (`char message[]` as a parameter). The refusal goes.
Test: `tests/test_extract_array_declared_in_region_used_after`.

### (#95) Extract gets its types right, in this sequence `extract-typing-ladder`

*no repo home yet.*
Each step starts from a red test.
1. Put Extract Variable's declaration before the statement: see "Extract Variable puts
   the declaration in the wrong place for some statements" in `19-known-bugs.adoc`.
2. Keep `volatile`, `restrict` and `_Atomic`, and the qualifiers on a pointer (`char *const`).
   The grammar maps them to QualifierNone (`src/c_parser.y`).
3. Extract Predicate, a `bool`. It needs a marker for an expression, since
   `putRegionMarker()` (`src/lexer.c`) wraps the region in braces and a marked
   expression no longer parses.
   A region of statements that assigns a `bool` used after it already gives a `bool`
   function, e.g. `okToAnalyse()` in `src/server.c` (WSL session d25255b8, 2026-11-22).
4. Deduce the type of the expression. `prettyPrintType()` (`src/misc.c`) can print it.
5. Extract an Expression as a Function. Today that gives a void function and a
   statement call, and `isInExpression` (`src/extract.c`) is read but never set.

After: extract-array-used-after

### (#96) Extract Function declares the locals it returns through out parameters `extract-out-locals-declared`

When the region declares two or more locals used after it, the call site passes them by
address but no longer declares them. GitHub issue #79.
Test: `tests/test_extract_locals_declared_in_region_used_after` (suspended).

After: extract-typing-ladder

### (#118) Inline Output Parameter `inline-output-parameter`

Removes the copies Extract Function
makes of a local it changes, `size_t length = *_length;` and `*_length = length;`, and uses the
pointer instead. 11-planned-features → Inline Output Parameter.
A real case: `readFileContent()` in `src/projectstructure.c`, done by hand (WSL session d25255b8).

After: extract-out-locals-declared

### (#119) Turn an output parameter into the return value `output-parameter-to-return`

For a function
with a single output parameter and no return value. 11-planned-features → Inline Output Parameter,
last paragraph.

After: inline-output-parameter

### (#121) Extract Function lets a region out by `return <constant>;` `extract-guard-return`

The first
case in 11-planned-features → Extract Function with Jumps out of the Region: a region that
is only left by `return <constant>;`, or `return;`, and has no result of its own. The new
function returns a `bool` and the call site does the `return`. A real case:
`editorbuffer.c:128`, the else-branch of `findOrCreateAndLoadEditorBufferForFile()`,
which `loadAllOpenedEditorBuffers()` (`editor.c:290`) duplicates (WSL session d25255b8).
The same holds for a single `break` or `continue`, as long as it is the only kind of
jump: the call site does that jump instead. A real case: the empty check in
`case OP_BROWSE_NEXT` (`cxref.c:1755`), which ends with a `break`.
And for a `goto` to a label outside the region. A real case: `setUpProjectIfNew()`
in `src/server.c`, extracted from `callServer()`, whose failure did `goto done`
(WSL, session `d25255b8`, 2026-11-22).

After: output-parameter-to-return

### (#123) Inline Variable `inline-variable`

Replaces each use of a local assigned once with
its initializer and removes the declaration. 11-planned-features → Inline Variable.
A real case: `hasBeenScanned()` in `src/projectstructure.c`, where a variable extracted from an
`if` and a `FileItem *` from `getFileItemWithFileNumber()` were both inlined by hand
(WSL session d25255b8, 2026-11-22).
And `okToAnalyse()` in `src/server.c`, where the extracted `bool` was returned
directly instead of through its local.

After: extract-guard-return

### (#126) Rename Symbol leaves the lines aligned to the old name `rename-keeps-alignment`

Renaming a function whose signature goes over several lines leaves the continuation
lines aligned to the old name. Extract Function ends with a Rename from its generated
name, so every extracted function with a long parameter list gets it, e.g.
`parseForTheAnswer()` in `src/server.c` (WSL session d25255b8, 2026-11-22).

After: inline-variable

### (#97) Rename in code that does not parse leaves occurrences behind `rename-partial-in-broken-code`

With the cursor on a declaration the parser still saw, only that occurrence is renamed,
with no warning. On a use it refuses, see `tests/test_rename_in_code_that_does_not_parse`,
whose `source.c` also shows this with the cursor on offset 66 (WSL session d25255b8).

### (#20) Take the client's "Remove References and Restart Server" back out `remove-references-stopgap`

It landed
2026-09-24 as a deliberate stopgap (`c-xref-project-remove-references-and-restart`,
`editors/emacs/c-xref.el`), because discarding the database is the standing remedy for
two unrelated symptoms: shadow occurrences after behind-the-back disk changes, and a
stale snapshot outliving a visibility fix (both in `doc/docs/19-known-bugs.adoc`). The
real work is making the database not need discarding — behind-the-back detection and
entry refresh are probably most of it,
and `index-based-sessions` dissolves another part. Remove the entry when they land, and check the
symptoms are gone rather than assuming it. One ghost stays under ADR-0035: that of
the Variant-B gap (`variant-b`).
1.10.0 announces the entry in `CHANGES.md`, so its removal needs a line there too.

Waits for: adr-0035

### (#22) Index-based sessions `index-based-sessions`

Dissolves stale-POP *and* lets an answer grow while you work. Roadmap → Memory as
Truth → Remaining. The design and its steps are in 11-planned-features → Index-based
Sessions, today's stack in 14-datastructures → Browser Stack. Open: the format of a
rename's conflicts. Before the request chain
(`symbol-at-cursor-from-table`, `cursor-parse-reuse`, `header-filtered-siblings`),
since it decides what a push sees once those change what is built. It makes
`session-refresh-time` unnecessary, and answers `held-cus-on-first-request` for Next,
Previous, Pop and Re-push.

After: request-target

### (#133) Selecting several symbols in the browser menu is hard to discover `browser-menu-selection`

The browser menu lists the symbols under the cursor and every other symbol with the same
name. Toggling more of them on makes Next walk them all. A user who does not know this
does not use it, and closes the menu when it appears. The rename's conflict dialog uses
the same browser, so this goes with the format of the conflicts in
`index-based-sessions` (WSL, session `d25255b8`, 2026-11-24).

After: index-based-sessions

### (#130) Other refactorings may leave sessions on the browsing stack `refactorings-leave-sessions`

The `OP_REFACTORY` case in `src/cxref.c` deletes the session of the request parse for every
refactoring. A rename deletes the sessions of its safety check in `makeSafetyCheckAndUndo()`.
The other refactorings push sessions too, with `pushReferences()` and `pushEmptySession()`.
Nobody has checked that a Pop after them goes back to the push. A test like
`test_pop_after_a_rename_returns_to_the_push` for each refactoring would show it
(WSL, session `d25255b8`, 2026-11-24).

After: rename-partial-in-broken-code

### (#117) After a refused or undone refactoring, a later one edited the wrong places `refused-rename-references`

Seen in daily use: a rename of `popReferenceOnly()` to the existing `popSession`, refused or
undone, then an Add Parameter on the wrong function, undone too. A later Add Parameter on
`popSession()` then edited the calls of `popReferenceOnly()`, 11 characters in, the length
of `popSession(`. So the table had references to `popSession` at those calls. One of the
edited files was a `.mock` (WSL, session `d25255b8`, 2026-11-23).
The safety check builds the edited CUs into the table before it undoes the edits, which
is the suspect. But three paths do not show it: a refused rename
(`test_refused_rename_keeps_the_references`), a rename undone and saved
(`test_undone_rename_keeps_the_references`), and a rename and an Add Parameter undone
without saving, followed by another Add Parameter
(`test_undone_refactorings_keep_the_references`). Not tried: a `.mock` among the edited
files, and the request between the undo and the Add Parameter that found no symbol.
The order of the steps is not known, so the next case needs a record of the requests.

After: rename-partial-in-broken-code

### (#128) A refactoring edits a buffer whose file changed on disk `refactoring-stale-buffer`

The client preloads only modified buffers, so for an unmodified open file the server
reads the disk and computes the edits for the disk's text. The client applies them to
the buffer. If the file changed on disk, the buffer is stale, and two things guard it
by accident. The precheck (`c-xref-server-dispatch-precheck`) fails if the offsets have
moved. Otherwise Emacs asks "changed on disk; really edit the buffer?" at the first
replacement. There `r` is right, `n` stops the refactoring halfway, and `y` edits the
stale text, so a save loses the change on disk. The client should check every buffer a
refactoring will edit before the request (`verify-visited-file-modtime`), and revert it
or refuse (WSL, session `d25255b8`, 2026-11-23).

After: refused-rename-references

### (#129) A request trace for daily use `daily-request-trace`

After a refactoring went wrong in daily use, the order of the requests was not known, so
it could not be reproduced (`refused-rename-references`). `-commandlog=<file>` writes each
request's arguments, and `-info` the builds and an "answered" line per request. Two
things are missing. The "answered" line could say what the answer did: the edits per
file, and the question if one was asked. And what only the client sees, an Undo, a save
or a No to a question, has to be logged by the client, with a time, so that it can be
read beside the server's log (WSL, session `d25255b8`, 2026-11-23).

After: refactoring-stale-buffer

### (#24) Retry the request that created the project `retry-creating-request`

Small, and it becomes first contact
with every new project once auto-discovery is the only way in.

### (#120) LSP follows an edited document, and answers across files `lsp-implementation-path`

11-planned-features → LSP Integration → Implementation Path, steps 2–4: the project
set up on `initialize`, `didChange`, `didSave` and `didClose`, and `definition` from
every file. Today only `didOpen` is handled, so an edited document is answered from
its opened text, and a definition outside it is not found.

After: lsp-mode

### (#25) LSP tiers 1–2 `lsp-tiers`

Code actions and `workspace/executeCommand` for extract, move
function and the parameter refactorings. Stubs exist: `handle_code_action`,
`handle_execute_command`, with `codeActionProvider` commented out in
`src/lsp_handler.c`. Keep tier 3 (custom methods + per-editor extension code) small.
First, the refactorings have to return their edits, so that a protocol can write
them. Today they write native protocol records as they go (`ppc…()` calls in
`refactory.c`, `extract.c` and `cxref.c`). The `assert(options.cxrefProtocol)` lines
roughly map where an operation depends on the native protocol, so keep them until then.

After: lsp-implementation-path

### (#26) Move Function next steps `move-function-next`

Smarter header placement and a preview.
11-planned-features → Move Function → Next steps.

After: move-shared-static

### (#99) Move Function removes the forward declaration in the source `move-removes-forward-declaration`

A `static` one left behind breaks the build. 11-planned-features → Move Function → Next steps.

After: move-function-next

### (#100) Move Function adds the includes the moved function needs to the target `move-includes`

Moving `waitForUserConfirmation()` into `src/ppc.c` needed `log.h` added by hand (2026-10-27).
11-planned-features → Move Function → Include management.

### (#101) Move Function declares the function for every caller, and only for them `move-header-for-callers`

A header is created also when the only caller is in the target, and only the source file
gets the include, not other files that call a function that wasn't static.

After: move-includes

### (#102) Move Function removes the extern from the source's header `move-removes-extern`

When the function was not static. 11-planned-features → Move Function → Remove extern declaration.

After: move-removes-forward-declaration

### (#103) Move Function takes a type that only moved code uses `move-takes-types`

A static
table moves along, but its `typedef` does not (`initTypeModifications()` into `src/type.c`).

After: move-removes-extern

### (#104) Where an API comment goes when a function moves `move-api-comment`

Today the comment goes
with the definition, and the extern in the header gets none. Open whether it belongs in the header.

After: move-takes-types

### (#88) `-parse-all`, a request that parses every compilation unit the scan found and has not parsed `parse-all`

What ADR-0024's CreateMode reduces to once the priming is a request rather than a mode.
`test_ffmpeg` and `test_systemd` ask for it today by searching a name nobody defines and
answering `-continue`. A refactoring only changes what has been parsed: a Delete
Parameter on `saveReferencesToStore` left `src/cxfile.mock` behind, and the unit build
did not compile (Mac, session `3af59d00`, 2026-09-25). No test pins that a refactoring
reaches a mock. A Move Function of a `static` function to another module makes it extern,
and that needs a new mock in the target's `.mock`, but only in a project with Cgreen
mocks (`popSession()`, WSL, session `d25255b8`, 2026-11-23).
If a user asks for it, it should say how long it will take. `src/progress.c` already
keeps `timeZero` and a monotonic clock, and the parse loops count down, so what is
missing is the division and a format like "4 min left". Decide the unit first: CUs are
uneven (`avcodec.h`'s consumers against a 40-line `.c`), lines or bytes are steadier,
and both are known before parsing starts.

### (#134) Make Static `make-static`

Makes a function `static` when nothing outside its file refers to it, and removes its
declaration from the header. 11-planned-features → Make Static. It needs a full parse
first, since a refactoring can miss references today, e.g. from a `.mock`.

After: parse-all

### (#92) The refactoring dialog's help describes what the dialog offers `refactoring-help`

*no repo home yet.*
`c-xref-refactoring-documentation` (`editors/emacs/c-xrefdoc.el`) has no Rename Module or
Organize Includes, describes a Make Static Function Extern the dialog does not offer, and
Extract Function's example is Java. A test that every dialog entry (`c-xref.el`, the
`PPC_AVR_` list) has a help entry, and every help entry a dialog entry, finds the next one.

After: retry-creating-request

### (#82) The browser's Symbol Pane says which symbol each line is `browser-for-c`

A line is
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

After: refactoring-help

### (#27) Local config fragments: the config names, the fragment binds `local-config-fragments`

ADR-0034, proposed. Decide it, with where the fragment lives and the syntax of a
default. Then: expand defaults, report an unbound name, read the fragment before the
environment, and ignore the personal options in a config, with a warning.

After: project-extent

### (#28) Chapter 18 hygiene, opportunistically `chapter-18-hygiene`

Incremental `cxfile.c` cleanup,
hashtab → hashlist, split the editor module. Also "tags" in the elisp, the last of
the old wording: two docstrings in `c-xref.el` say the search "inspects the tags".

After: parse-all

### (#76) Extract the macro expansion module from `src/yylex.c` `macro-expansion-module`

§17.2.
`yylex.c` is 2794 lines. The macro stack, the macro arguments memory and the
expansions of the last lexem (`lastLexemExpansions`, `noteMacroExpansionsOfLastLexem()`)
now sit together at its top, under section comments. The plan in §17.2 is older than
that, and says 2353 lines, so check it against the code first (WSL, session
`d25255b8`, 2026-10-22).

After: chapter-18-hygiene

### (#29) Dump Reference Database `dump-reference-database`

`-dump=knowledge` answers from the
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

After: macro-expansion-module

### (#30) Read `-I` and `-D` from `compile_commands.json` `compile-commands`

*no repo home yet.* Nothing reads it today. CMake and Meson write one, and `bear`
makes one for Make. It decides whether a stranger's first project works without
learning `.c-xrefrc` (section 8). It also lists the CUs, so it could answer part of
`project-extent`.

After: local-config-fragments

### (#131) Unify Parameter Names `unify-parameter-names`

Gives a parameter one name in every declaration and definition of its function: the
header, a forward declaration, the definition and a mock. Today each is a symbol of its
own, and Rename changes only one. 11-planned-features → Unify Parameter Names.

After: rename-keeps-alignment

### (#132) Completion inside Cgreen's `when(` offers the parameters of the mocked function `cgreen-when-completion`

`expect(helper, when(count, ...))` is now a reference to `helper()`, so the server knows
the function. `when()` must name a parameter of the mock, and by convention the mock and
the real function have the same parameter names. Unify Parameter Names makes that
convention hold. The same knowledge lets a parameter rename also edit `when()` and
`mock()`. 11-planned-features → Names in stringized macro arguments.

After: unify-parameter-names

## 6. Developer tooling

### (#61) The `-yydebug` parse trace goes to the log `yydebug-to-log`

Byacc's skeleton
writes it with `printf` (`byacc-1.9/skeleton.c`), and in a server stdout is the sync
stream, so a client stops with `SYNCHRO PROBLEM`. It works only with the server driver
or from a shell. The trace gives states, not rule names; byacc's `-v` listing maps them.

After: dump-reference-database

## 7. Freshness as make (ADR-0032, ADR-0035, ADR-0036)

The order is roughly as listed. "Freshness Is Make, for the
Goal" in `08-algorithms` has the shape the request converges to.

Performance, from reading the code (Mac, session `05984816`, 2026-10-08): at equal
correctness the design costs the same or less. Parsing dominates, and what is parsed
follows the request rather than triggers: an edited header no longer parses up to the cap's
worth of includers at once, only the goal's. More is parsed only where the old answer was
incomplete (deeper reach, disk changes). The bookkeeping added — a `stat` per prerequisite, a
walk of the table per changed file — is small beside what goes. The risks are in the
implementation, and are noted in `cu-prerequisites`, `out-of-date`, `time-budget` and
`removal-cost`.

### (#115) Every parse of a CU parses it once for each pass `every-parse-all-passes`

A CU is started by whoever parses it, before `startParsingBuffer()`, from the project
baseline and that pass's options (`startCompilationUnitForPass()`). The refresh, the
request's own parse and `pushReferences()` loop over the passes. Extract, the
parameters, the bounds and the move target parse once, without a pass. Each needs a
test where its answer depends on a pass first, and with the second loop, "parse this
buffer in every pass" can become one function (WSL, session `d25255b8`, 2026-11-17).

After: index-based-sessions

### (#113) The symbol at the cursor is looked up in the table, not parsed for `symbol-at-cursor-from-table`

A push, browsing or for a refactoring, parses the request file only to find what is at
the cursor (`cxref.c:328`), although the refresh has just built its references. The
LSP looks it up by position (`lsp_adapter.c:43`), and so do `##` symbols
(`refactory.c:482`). The lookup has to give the set of referenceables at the position,
visible usages only, with the macro default position (`cxref.c:351`) and the caller
position. Needs tests for macros, `##` and several symbols at one position first
(WSL, session `d25255b8`, 2026-11-17).
This is the target line of the shape: the request CU is made current, then the symbol
is looked up, so that its reach is the goal. Locals are in the table only after a parse
in this server run, since the snapshot skips `VisibilityLocal` (`cxfile.c:355`). So a
lookup that finds nothing parses the request CU; a test for a local after a restart
(WSL, session `d25255b8`, 2026-11-24).
A parse that needs only a point, e.g. the bounds, also selects the symbol at the
cursor, into the refactoring's browsing entry, since `needsReferenceAtCursor()`
decides both.

After: every-parse-all-passes

### (#116) The refactorings' push operations are session kinds `refactoring-session-kinds`

`OP_INTERNAL_PUSH_FOR_RENAME`, `_ARGUMENT_MANIPULATION`, `_USAGE_CHECK` and
`OP_INTERNAL_SAFETY_CHECK` are no longer requests. `pushReferences()` stores them as the
session's operation, and they decide the selection menu (`showSelectionMenu()`,
`isRenameMenuSelection()`). Rename and argument manipulation behave the same everywhere
and can be one. The `OP_INTERNAL_` prefix, and their place in `ServerOperation`, say
something else.

After: symbol-at-cursor-from-table

### (#109) A session refreshes a file by a time, not a flag `session-refresh-time`

A file's references get a time they last changed, never cleared, and a session
refreshes its copy of a file when that is later than the copy. Replaces
`needsBrowsingStackRefresh`, which is set in only some of the places that change the
table and is cleared by the first session that reads it (stale-POP).
`index-based-sessions` removes it again.

After: refactoring-session-kinds

### (#107) The macro path parses CUs that are current `cap-is-a-question`

For a cursor in a macro body, `collectCompilationUnitsExpandingTheMacro()` parses up to
`MAX_CUS_TO_REPARSE` of the CUs that can expand it on every push. They are the request
file's includers, so the build has made them current and the answer comes from the
table: `test_every_cu_expanding_a_macro_is_parsed` gets 130 referents from 128 parses,
786 ms on a warm push (WSL, session `d25255b8`). Skipping the loop for current CUs is
probably enough (`cursor-parse-reuse`).
A datapoint: a cold avcodec push builds 2515 CUs and the first next 34 more, all `.c` files and
many included by another `.c`, with the same answer (WSL, session `d25255b8`, f32fbc69).

Waits for: adr-0035

### (#35) A CU's prerequisites `cu-prerequisites`

The CU, its include closure and the project
config, put together where `out-of-date` uses them, the CU first. `Prerequisites`
(`src/freshness.c`) holds them for one request: the include graph and each file's
change time, asked for once. Left: the config's change time (`out-of-date`).

### (#36) The config as an input of `knowledgeIsOutOfDate()` `out-of-date`

It
replaces `markAllCompilationUnitsStale()`, and the zero sentinel goes. A config change
already removes every reference, and it is found by its content token, also across a
restart. The config is the
`.c-xrefrc` and its local fragment (ADR-0034, `local-config-fragments`), and its change time is the later of the
two.
Left until real use shows the cost (see `removal-cost`): `MAX_INCLUDE_CLOSURE` (1000)
has no measured reason, and evaluating change first, i.e. `stat` each known file once
and walk the reverse include graph to the CUs whose knowledge is older, costs in
proportion to what changed.

After: cu-prerequisites

### (#37) What a request needs `request-target`

The CUs that reach the request
file's closure, and the whole project for an operation on a name. Left: the scan does
not record `<...>`
includes (`scan-angle-includes`), so for a project that includes its own headers that
way, reach misses CUs never parsed.

Waits for: adr-0032

### (#124) The held CUs in the goal are there for the sessions `held-cus-on-first-request`

`collectGoal()` collects the out-of-date held CUs for every operation, but only if
`projectWasOpen`. What reads them outside the request's reach is the refresh of the
session on top of the browsing stack, since a session walks a copy of its references.
A first request has no session, so there is nothing to keep fresh, and no test fails
on the guard. Once a session reads the table, the held CUs either belong to a goal
for the sessions, or go, and `projectWasOpen` with them (WSL, session `d25255b8`,
2026-11-22).

Waits for: index-based-sessions

### (#125) An operation on a name asks about its whole goal `whole-project-goal`

The goal is on two lists. `buildList` is built without a question,
and has the held CUs that are out of date. `wholeProject` has the other
CUs that are out of date (`collectOutOfDateCUsNotIn()`), and the question counts only those.
With one list, the question counts the held CUs too, and a "no" leaves
them unbuilt. Which is right depends on why the held CUs are built (WSL, session
`d25255b8`, 2026-11-22).
A search from a file builds its request CU on the second list, since `OP_SEARCH`
is in `requiresProcessingInputFile()` (`processFile()` opens it, but does not parse it).
Taken out, the request is built with the reach instead, before the question: `test_a_search_from_a_file_asks_before_analysing_its_siblings`
asks about 130 of 131, `test_a_config_change_is_reloaded_once` shows two progress runs.
`time-budget` changes what the question counts.

Waits for: held-cus-on-first-request

### (#127) Modules follow the lines of the shape `modules-follow-the-shape`

`callServer()` calls one function for each line ("A Request, Step by Step" in
`08-algorithms`), and what each of them calls shows the modules it uses. `rebuild.c`
holds three lines, the removal, the goal and the build. `projectstructure.c` is discover and
scan, and `freshness.c` the prerequisites that the rest read. Look at it once the goal
is one step (WSL, session `d25255b8`, 2026-11-22).

After: whole-project-goal

### (#54) A request in a header is parsed through its includers `request-through-includers`

Never on its own (ADR-0032, amended 2026-10-11), as for a macro body. No includer
gives a message saying that no compilation unit includes the file. Includers that
are current are parsed only for the cursor, and need a bound.
`test_push_in_header_no_cu_includes` comes off suspension.

### (#1) The request CU is parsed for the cursor, also when its knowledge is current `cursor-parse-reuse`

The lexer finds what the cursor is on during the parse (`lexer.c:448`), so every
operation with a cursor parses the request file, and in a macro body every CU that
expands it. ADR-0032 says to reuse the references when the knowledge is current,
which needs a lookup of the reference at a position in the table. Completion, extract
and the macro body need the parse.
A refactoring parses the request file at the cursor too (`processFile()`), into the session
`parseForTheAnswer()` pushes for `OP_REFACTORY`. A rename does not read that session. It
parses the buffer again in `pushReferences()` (WSL, session `d25255b8`, 2026-11-23).

After: request-through-includers

### (#40) A time budget instead of the count `time-budget`

The count where
`needsConfirmationToAnalyse()` asks becomes a time budget, and the question gets an estimate. Browsing remembers a "no" for the session, an operation that edits
or works on a name is cancelled by it. Touches the client's question. Open: does a remembered "no" stop parsing, or only the asking, so
that each request still parses a budget's worth? Under ADR-0035 partial work adds up,
so the second is safe.
The estimate needs how many CUs are out of date in the whole project. That can be
found change first (`out-of-date`): `stat` every known file once and walk the reverse
include graph to the CUs. Then project-wide goals use that and the rest ask
`knowledgeIsOutOfDate()` per CU, with a test that the two agree for every CU.
The ask-or-not decision (`needsConfirmationToAnalyse()`) takes the budget and is
unit-tested on both sides of it. The budget is a user setting, so system tests use 0
(always asks) and unlimited (never asks), and none sit near the boundary.

After: out-of-date

### (#108) A CU that stops emitting into an unchanged header leaves its references there `variant-b`

What a CU emits into a header is the union over its passes, and it shrinks when the
preprocessor state on the way in changes: the CU itself, or a header included before
it but not by it, e.g. one that stops defining `FEATURE` (ADR-0035). Nothing about the header
changed, so nothing removes them. In `src/` what differs between includers is mostly
macro expansions (`doc/freshness-after-adr-0035.md`). Generations are one
option, and would need an ADR. `test_preprocess_edit_removes_ifdef_define` stays
suspended, and its `.suspended` still names ADR-0033's sweep.

Waits for: adr-0035

### (#45) What removal costs `removal-cost`

`removeReferenceableItemsForFile()`
walks the whole table, and runs before every CU build and once per changed file, so a
request that sees many files change at once, e.g. after a `git checkout`, walks it
many times: 128 CUs on ffmpeg is 128 walks over 1.24 million references. One walk
for a set of files would do. A removed reference is also only unlinked, never freed
(`cxAlloc`), so every build leaves its CU's old references behind as dead memory.
For information: the table only answers exact identity, so other lookups walk all
of it. By file, the strip above. By kind, `buildIncludeGraph()` looking for the
include items, about 38 ms on a fully parsed ffmpeg, once per request. By name
only, `navigation.c` once per menu item. Per request on ffmpeg, 1376 `stat`s took
about 5 ms and 2434 include closures about 50 ms (WSL, session `d25255b8`,
2026-10-11).

Waits for: adr-0035

### (#41) The docs follow `freshness-docs`

*Out of date* replaces *Staleness* in
`06-principles`, and `08-algorithms` loses "Dual Semantics". In `06-principles`, the
part on why reparsing only a subset is safe also describes strip-then-reparse.
`08-algorithms` still describes the entry refresh as Pass 1, 2 and 3; "Freshness Is
Make, for the Goal" in `08-algorithms` is a start for its rewrite.
ADR-0036 (accepted 2026-11-11): in `06-principles` *Change time* goes, and *Saving /
persisting* says what the snapshot keeps of a held file (WSL, session `d25255b8`,
2026-11-10).
The include graph that a request takes now answers both directions, from the scan's
edges and the parse's include references. That makes
`20-insights` ("The include edges are inputs, not knowledge"), Pass 2's
Discovery in `08-algorithms` and `12-components.adoc:129` stale, since they describe a walk over `TypeCppInclude`
references in the table.

Waits for: adr-0035

## 8. Real users: installing and being found

Prepare the installation side whenever it suits; it depends on nothing above. Hold the
announcement until branch switching is safe, since a wrong rename after a `git checkout`
loses a first user. *No repo home yet* for any of these — discussed on WSL and on the
Mac (session `05984816`), 2026-10-07.

### (#46) A lean release build `release-build`

No coverage, no unit tests, only
`c-xref`. The generated parsers are committed, so `cc` and `make` are enough.

### (#47) The client and the server check a protocol version `protocol-handshake`

Once the elisp and the binary are installed apart they drift (MELPA builds HEAD, the
binary is whatever was built). On a mismatch the client offers to rebuild, rather
than failing in some later request. Until this is in, every upgrade path (el-get,
`load.el`) replaces the client and the server together, and the only user is the
maintainer, so a protocol or option change is made in one step: no transition, no
deprecation, an option the client stops sending is simply unknown afterwards.

### (#48) `M-x c-xref-install-server` `install-server`

Builds from source by
default, since every user of a C tool has a compiler; downloads a release binary as
the shortcut. The pattern of `pdf-tools-install` and `irony-install-server`. A binary
fetched by Emacs with `curl` is not quarantined by macOS Gatekeeper.

Waits for: release-build, protocol-handshake

### (#49) Tagged releases `releases`

Semver tags from `stable`, GitHub Releases
with binaries, and through the tags MELPA Stable. A Homebrew tap and an AUR package
are cheap once releases exist.

Waits for: release-build

### (#50) The elisp on MELPA `melpa`

The package without binaries, which MELPA
forbids; the server comes from `install-server`. `el-get` and `load.el` stay. This is
where the client and the server start to be installed apart.

Waits for: install-server, releases, protocol-handshake

### (#51) The README sells on its first screen `readme-first-screen`

A GIF or
asciinema of a parameter move across files; the Java section goes.

### (#81) The customizable options get names of today `customize-names`

Customize shows
"C Xref ..." and "C Xrefactory ...", since it makes its labels from the symbol names
(`c-xref-...` in `editors/emacs/c-xrefactory.el`). A group can get a `:tag`, but an
option is relabelled only by a rename, with `define-obsolete-variable-alias` so that
old settings still work. MELPA's `package-lint` probably wants one prefix, the
package's name, so the two go together (`melpa`).

### (#52) Announce `announce`

Positioned beside clangd, not against it: the
refactorings clangd does not have (parameters, move function, macros, Yacc). LSP
tier 2 (`lsp-tiers`) is the shape of that, with clangd as the main server; lsp-mode takes add-on
servers, eglot needs a look. EmacsConf, Sacha Chua's Emacs News, Show HN. A stranger's
first project working without configuration matters here, see `compile-commands` and
`retry-creating-request`.

Waits for: refactoring-refusals, install-server, readme-first-screen

### (#53) A Windows build `windows-build`

Most of the code is plain C. About 28
`__WIN32__` remnants from the original Xrefactory are there, unmaintained and untested
(`misc.c`, `commons.c`, `options.c`). The work is probably: file names (drive letters,
backslashes, case-insensitive names, i.e. `normalizeFileName()` and file identity, the
risky part), modification times with nanoseconds (`st_mtim` in `fileio.c`), the
compiler interrogation (`popen`, and MSVC's `cl` does not answer `-v -E`), and the
tests, which need make, bash and sed, and a CI job. A MinGW build under MSYS2 is
probably the shortest route. From reading the code (WSL, session `d25255b8`,
2026-10-11). Until then WSL works, but on files under `/mnt/c` each `stat` goes
through 9P to the Windows side, at about a millisecond each.

After: refactoring-catalogue

### (#73) Every known refactoring, with its status `refactoring-catalogue`

A list,
one row per refactoring, done, planned or left out, linking to its design in
`doc/docs/11-planned-features.adoc` and its recipe in `20-refactoring-recipes.adoc`.
It shows how many of them c-xrefactory can do, so probably in the user manual or the
README. *No repo home yet.*

After: customize-names

## Foundation

What the open items rest on, so that a `Waits for:` can name it. Not a record of what
was done.

- **Entry refresh before every request (ADR-0020)** `entry-refresh`
- **ADR-0029: the project's identity is its root path** `adr-0029`
- **ADR-0030: opening a project and asking about a file are separate** `adr-0030`
- **ADR-0031: what files a project consists of** `adr-0031`
- **ADR-0032: knowledge is rebuilt like `make` rebuilds targets** `adr-0032`
- **ADR-0035: a file's references are removed when its content changes** `adr-0035`
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

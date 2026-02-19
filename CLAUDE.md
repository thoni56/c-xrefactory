# CLAUDE.md

## How We Work Together

**STOP before acting.** This is a collaborative project. When you see something that could be done:
1. Discuss it first
2. Ask "Should I do X?" before doing X
3. Answer questions without acting on them
4. If unsure what to do, ask

**The user often prefers to make changes themselves**, especially refactoring, since they can use c-xrefactory itself. Propose changes, don't just make them.

**Never:**
- Make code changes without explicit agreement
- "Clean up" debug logging, comments, or code style unprompted
- Run tests unless asked (watch commands are running in background)
- Give time estimates

**The directory is "Utveckling" (Swedish), not anything else. You should always verify the path by looking at the working directorys path.**

## Project Principles

### No Time Estimates (#noestimates)

This is a hobby/OSS project. Do not use calendar time estimates.

Instead emphasize: **Value**, **Scope**, **Dependencies**, **Risk**

Use relative effort only: "smaller", "larger", "foundational"

### Test-Driven Development

- Create a failing test FIRST before fixing bugs
- Unittests are using Cgreen and are always run on build
- Systemtests are in `tests/test_<name>/` directories

### Unittests

- Using Cgreen
- Mock functions are defined in <module>.mock
- Mock files are included in the test
- Only a few, simple, module files are linked as is, defined in the Makefile

### System Tests

- Live in `tests/test_<testname>` directories
- Each directory needs a `.c-xrefrc` that is created automatically on `make` either from a local `c-xrefrc.tpl` or a common template if it doesn't exist
- `make clean` removes all generated files, such as the `.c-xrefrc` and the diskdb, so that the directory will be populated from afresh
- The existance of a `.suspended` file means the test will not be run when all system tests are run, it can still be run using `make` in the test directory
- Output is usually collected in an `output.tmp` that can be normalized to not fail on paths, lengths, dates etc.
- `output` is removed when it matches `expected`, so a remaining `output` means the test failed
- the test diff is designed to say "remove (`<`) this and insert (`>`) that to get the expected output"

## Build & Test Commands

```bash
make -C src                 # Development build with coverage
make -C src test            # Run quick tests
cd tests/test_<name> && make  # Run single test
```

Watch commands (usually already running):
```bash
make -C src watch-for-unittests
make -C src watch-for-systemtests
```

## Architecture Overview

**Client-Server Model:**
- Emacs plugin (`editors/emacs/`) communicates with C backend (`src/`)
- Backend maintains symbol database in `.cx` files

**Key Subsystems:**
- Parsing: `c_parser.y`, `yacc_parser.y`, `yylex.c` (uses custom `byacc-1.9`)
- Symbols: `symbol.c`, `cxfile.c`, `reference.c`
- Refactoring: `refactorings.c`, `extract.c`
- Server: `server.c` handles 60+ operations

**Generated files (don't edit):** `*.tab.[ch]`, `lexem.[ch]`, `options_config.h`

## Commit Messages

```
[topic]: Brief summary

Optional detailed explanation.
```

Topics: `[fix]`, `[feat]`, `[refactor]`, `[test]`, `[docs]`, `[build]`

## Documentation

The term **'docs'** refers to Structurizr-based Asciidoc in `doc/docs/`, not markdown files.

# Follow extern declarations in C files via scanner hints

Date: 2026-09-17

## Status

Proposed

If accepted, this supersedes ADR-0013.

## Deciders

Thomas Nilefalk

## Problem Statement and Context

ADR-0013 accepts that on-demand parsing does not find references reachable only
through a declaration written directly in a `.c` file:

```c
/* other.c: no #include "globals.h" */
extern int counter;
void bump(void) { counter++; }
```

Pass 3 of entry refresh (see Algorithms) parses sibling CUs found through shared
headers. `other.c` shares no header with the file where `counter` is declared, so
it is never parsed, and a rename of `counter` misses it. The code then fails to link.

ADR-0013 considered two ways to find such files and rejected both:

- **Grep for the name** was rejected because grep results would be used as
  references: matches in strings, comments and other scopes would be wrong
  results.
- **Parsing the whole project** on every operation was rejected as far too slow.

Since then, lightweight file structure scanning (ADR-0022) reads every project
file on startup to find `#include` lines. It already visits the files that hold
these declarations.

The same pattern is more common for functions than for variables. A prototype
needs no `extern`, and older code often declares the functions it calls at the top
of the `.c` file instead of including a header:

```c
int helper(void);
```

## Decision Outcome

Extend the lightweight scan to record, for each `.c` file, the names in file-scope
declarations that are not definitions: `extern` variable declarations and function
prototypes. Keep this as an index from name to files.

When an operation needs the references of a global symbol, add the files the index
lists for its name to the CUs Pass 3 parses.

The index only chooses **which files to parse**. It is never a source of
references: the parser still decides what is a reference to which symbol. So a
false match (a name in a comment, a string, a local declaration, an unrelated
symbol with the same name) only costs an extra parse, never a wrong result. That is
what separates this from the grep option ADR-0013 rejected.

## Consequences and Risks

**Benefits:**

- Rename, push and other reference queries on a global also find files that declare
  it without a header, closing most of the gap ADR-0013 accepted.
- The cost stays proportional to the symbol: only files that mention its name in a
  declaration are added.
- No new source of truth: references still come only from parsing.

**Costs and risks:**

- **Scanner complexity.** Recognising prototypes needs more than finding a keyword:
  the scanner has to tell a file-scope declaration from a definition and from a
  call, and skip comments, strings and preprocessor lines. It stays a heuristic
  and can err in both directions.
- **Misses that remain:**
  - declarations produced by macros
  - pre-C99 implicit function declarations, where a function is called with no
    declaration at all
  - uses in a file that neither includes the header nor declares the name
    (these do not compile in modern C, but some old code relies on implicit int)
- **Common names.** A name like `init` declared in many files adds all of them. The
  worst case approaches parsing every file that declares the name, which is still
  far less than the whole project.
- **Staleness.** The index must be refreshed when a file changes, the same way the
  include structure is, keyed on file modification time.

## Considered Options

- Option 1: Accept the limitation (ADR-0013, current)
- Option 2: Scanner hints choose extra files to parse (chosen)
- Option 3: Parse the remaining project gradually between requests
- Option 4: Parse the whole project on every operation

### Option 1: Accept the limitation

Keep ADR-0013. Simple, but a rename can silently leave code that does not link, and
users of older code bases are the ones least likely to notice before building.

### Option 2: Scanner hints choose extra files to parse

As described above. Removes most of the gap at a cost proportional to the symbol,
at the price of a heuristic in the scanner.

### Option 3: Parse the remaining project gradually between requests

After a cold start, the server parses a few never-parsed CUs at a time whenever no
request is waiting, and saves the snapshot as it goes. Once every CU has been parsed
once, the snapshot is complete and every gap caused by on-demand parsing closes,
including this one, search and push by name.

This is not an alternative to Option 2 so much as a complement. It closes the gap
only eventually: the first rename after a cold start can still miss references.
It also changes the single-threaded server loop, which must check for a waiting
request between CUs. It is closer to ADR-0024 (CreateMode, an upfront batch prime)
than to this decision, and deserves its own ADR if pursued.

### Option 4: Parse the whole project on every operation

Rejected in ADR-0013 for the same reasons that still hold: it defeats the on-demand
architecture (ADR-0014) and makes interactive operations slow on large projects.

## References

- ADR-0013: Limited detection of archaic extern declarations in C files
- ADR-0014: Adopt on-demand parsing architecture
- ADR-0022: Lightweight file structure scanning
- ADR-0024: Batch prime via CreateMode

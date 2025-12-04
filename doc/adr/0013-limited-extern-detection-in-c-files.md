# Limited detection of archaic extern declarations in C source files

Date: 2025-12-04

## Status

Proposed

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

C allows declaring external variables directly in `.c` files without corresponding header declarations:

```c
// file1.c - defines variable
int secret_global_var = 42;

// file2.c - uses it via extern (no #include dependency)
extern int secret_global_var;
void use_it() { printf("%d\n", secret_global_var); }
```

This pattern creates a dependency that is **invisible to include-based dependency tracking**. When implementing on-demand parsing (see ADR-0014), the question arises: How should c-xrefactory handle such references?

### The Problem with On-Demand Parsing

With on-demand parsing:
1. User requests "rename `secret_global_var` to `global_var`"
2. c-xrefactory parses current file + files in include closure
3. `file2.c` is **not** in the include closure (no `#include` connects them)
4. Rename misses the reference in `file2.c`
5. Code fails to compile after refactoring

### Why This Matters

This pattern appears in:
- **Legacy codebases** from 1980s-1990s before header file conventions solidified
- **Embedded systems** with minimal includes and global state
- **Poorly maintained projects** without proper modularity
- **Quick hacks** where developers skip creating header files

Modern C code uses headers to declare external symbols:

```c
// common.h - proper modern C
extern int global_var;

// file1.c
#include "common.h"
int global_var = 42;

// file2.c
#include "common.h"  // Now there's a dependency!
void use_it() { printf("%d\n", global_var); }
```

## Decision Outcome

**c-xrefactory will NOT attempt to detect cross-file references for variables declared `extern` in `.c` files without corresponding header declarations when operating in on-demand mode.**

Users relying on this archaic pattern must:
1. **Run full project scan** (`-create`) to ensure complete analysis, OR
2. **Modernize their code** by adding proper header files (recommended), OR
3. **Accept linker errors** as validation after refactorings

This is an explicitly accepted limitation, not a bug.

## Consequences and Risks

### Benefits

**Architectural simplicity**:
- On-demand parsing can rely on `#include` relationships for dependency closure
- No need for expensive whole-project scanning on every operation
- Clear mental model: "dependencies are what you `#include`"

**Performance**:
- Operations remain fast (parse only relevant files)
- No need to scan entire project for every refactoring

**Maintainability**:
- Simpler codebase without special-case detection logic
- Easier to explain and document behavior

### Risks and Mitigations

**Risk 1: Users discover incomplete refactorings**

*Scenario*: User renames a global variable, misses hidden `extern` references, code doesn't compile.

*Mitigations*:
- **Documentation**: Clearly document this limitation in user guide
- **Linker catches it**: Compilation will fail with "undefined reference" error
- **Testing**: Unit tests should catch issues before deployment
- **Workaround**: Run full `-create` for projects using this pattern
- **Best practice**: Documentation should encourage proper header usage

**Risk 2: Tool appears "broken" to legacy code users**

*Scenario*: Users with legacy codebases expect complete refactoring, get surprised.

*Mitigations*:
- **Clear messaging**: When refactoring global symbols, warn "Consider running full project scan for complete coverage"
- **Detection heuristic**: If renaming a non-static global, suggest full scan
- **Documentation**: FAQ section on handling legacy code patterns
- **Option flag**: Provide `-thorough` or similar flag to force full project analysis

**Risk 3: Competing tools handle this better**

*Scenario*: Other C refactoring tools detect these references, c-xrefactory doesn't.

*Reality check*:
- **clangd**: Requires compile_commands.json, parses only compiled files
- **ccls**: Same approach as clangd
- **ctags/cscope**: Don't track symbol types, can't safely rename
- None of these tools magically find undeclared `extern` references either

*Mitigation*:
- **Unique strengths**: c-xrefactory's macro understanding and Yacc support differentiate it
- **Target audience**: Projects caring about refactoring quality likely already use headers
- **Migration path**: Provide tools to detect and fix archaic patterns (see Future Work)

### Impact Assessment

**High-impact projects**: <5% of C codebases use this pattern extensively (estimate based on modern C practices)

**Medium-impact projects**: ~20% might have isolated cases (a few global state variables)

**Low-impact projects**: ~75% use proper header files exclusively

For most users, this limitation will never be encountered.

## Considered Options

### Option 1: Grep-based detection

**Approach**: After parsing, grep all `.c` files for potential `extern` references to the symbol.

**Pros**:
- Catches some cases
- Relatively simple to implement

**Cons**:
- **False positives**: Matches in strings, comments, different scopes
  ```c
  char *msg = "extern int foo;";  // Not a reference!
  /* extern int foo; */            // Not a reference!
  struct S { int foo; };           // Different scope!
  ```
- **Name collisions**: C has weak namespacing
- **Performance cost**: Grep entire codebase on every refactoring
- **Macro-hidden references**: Won't find symbols constructed by macros
- **Incomplete anyway**: Can't distinguish `extern int foo` (reference) from `int foo` (definition)

**Verdict**: Rejected - too many false positives, incomplete solution

### Option 2: Always parse entire project

**Approach**: Every operation triggers full project parse, maintaining complete symbol information.

**Pros**:
- Catches all references
- Simple mental model: "always complete"

**Cons**:
- **Defeats on-demand architecture** - one of the main architectural goals
- **Terrible performance** for interactive operations (seconds of delay)
- **Memory issues** for long-running LSP servers
- **Inconsistent with modern IDE expectations** (instant response)

**Verdict**: Rejected - incompatible with LSP goals

### Option 3: Accept limitation, document clearly (CHOSEN)

**Approach**: Document this as an accepted limitation, provide workarounds for affected users.

**Pros**:
- **Enables on-demand architecture** - critical for LSP
- **Good performance** for vast majority of use cases
- **Simple implementation** - no special-case logic
- **Encourages good practices** - incentivizes proper header usage
- **Pragmatic** - affects <5% of modern codebases

**Cons**:
- Some legacy codebases need extra steps (full scan)
- Might appear less "complete" than batch-only tools

**Verdict**: **ACCEPTED** - Best trade-off for modern architecture

### Option 4: Hybrid approach

**Approach**: On-demand by default, but detect "risky" globals and offer full scan.

**Example**:
```
Renaming global variable 'state' to 'app_state'...
Warning: This is a non-static global. Consider running full project scan
to ensure all references are found, including archaic extern declarations.

Run with -thorough? [y/N]
```

**Pros**:
- Best of both worlds for experienced users
- Educational (teaches users about the issue)
- Provides escape hatch

**Cons**:
- More complex UX
- Still incomplete (only warns, doesn't solve)

**Verdict**: Potential future enhancement, but not required initially

## Future Work

### Detection Tool

Provide a linter/analyzer to detect archaic patterns:

```bash
c-xref-lint --check-extern-abuse
# Reports:
#   file2.c:5: Archaic 'extern' declaration without header
#   Suggestion: Declare in header file
```

### Automated Migration

Tool to extract `extern` declarations into proper headers:

```bash
c-xref --modernize-externs
# Creates headers with proper declarations
# Updates .c files to include them
```

### Warning System

When renaming non-static globals:
```
Warning: Renaming non-static global 'foo' in on-demand mode.
For complete coverage of archaic extern declarations, run:
  c-xref -create && c-xref -rename foo bar
```

## References

- ADR-0014: Adopt on-demand parsing architecture
- [C99 Standard §6.2.2](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf) - Linkage of identifiers
- [Modern C: §8.2 "Declarations and Definitions"](https://gustedt.gitlabpages.inria.fr/modern-c/)

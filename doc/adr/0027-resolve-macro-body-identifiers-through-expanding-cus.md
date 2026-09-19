# Resolve an identifier in a macro body through every expanding CU

Date: 2026-09-18

## Status

Proposed

## Deciders

Thomas Nilefalk

## Problem Statement and Context

A macro body is stored as tokens. The parser does not treat it as code, so the
identifiers in it get no references. They become code when a compilation unit
expands the macro, and only then do they resolve to anything.

That turns a question that looks local into one that is not:

```c
/* macro.h */
int calculate_area(int width, int height);

#define GET_AREA(w, h) calculate_area(w, h)
//                     ^ cursor here, "goto definition"
```

`calculate_area` is declared three lines up, in the same file. The server still
cannot answer until it has parsed a compilation unit that expands `GET_AREA`.
This is the suspended `test_browsing_push_in_unexpanded_macro_without_create`.

Such a compilation unit is an *expanding CU* (see Principles). Today
`findMacroExpansionFile()` finds one by looking for an expanding-CU marker, a
`UsageMacroBaseFileUsage` reference at line 0, column 0. `expandMacroCall()`
records that marker only in XrefMode, which is what `-create` runs, so a server
that has never run a batch prime has none.

Completion needs an expanding CU for the same reason.
`test_completion_in_macro_definition` covers it: inside `#define FIELD(q) q->`
the parameter `q` has no type of its own, and the completion list comes from how
the macro is used.

### Different CUs can expand the same macro differently

```c
/* node.h */
#define NEXT(p)  ((p)->next)
//                    ^ cursor here
```

```c
/* list.c */                              /* tree.c */
#include "node.h"                         #include "node.h"

struct list_node {                        struct tree_node {
    int value;                                struct tree_node *child;
    struct list_node *next;                   struct tree_node *next;
};                                        };

void walk_list(struct list_node *n) {     void walk_tree(struct tree_node *n) {
    n = NEXT(n);                              n = NEXT(n);
}                                         }
```

Standing on `next` in `node.h`, `list.c` answers `struct list_node.next` and
`tree.c` answers `struct tree_node.next`. Both are correct. Only the user knows
which one they meant.

`findMacroExpansionFile()` returns a single file number, so the user gets
whichever expanding CU was found, with no sign that the other exists. Which one
that is depends on what has been parsed. Under on-demand parsing (ADR-0014) the
parsed set grows as the user works, so the same keypress on the same character
can answer differently in two sessions.

## Decision Outcome

An identifier inside a macro body resolves to every meaning its expanding CUs
give it, and the browser menu should show them all.

**Distinct referents, not distinct CUs.** Two hundred CUs that expand `NEXT(p)`
to the same `struct list_node.next` are one entry. A macro is often expanded in
many places and means the same thing in nearly all of them, so the menu stays
short while the search is long.

**Scope.** This covers the identifiers inside a macro body, for navigation and
completion. Browsing the macro name itself is unaffected, since a macro is a
symbol with references like any other.

**Rename is out of scope, and should refuse from inside a macro body.**
Renaming `next` in the body above would rewrite two unrelated struct fields.
Rename from one of the expansions instead.

**The search is bounded by the include structure.** A macro can only be expanded
by a CU that includes its definition, so the includers of the defining file
bound the set. Lightweight file structure scanning (ADR-0022) already builds
that graph, and `collectIncludersOfStaleHeader()` already walks it.

**The menu is complete over that bound.** The scan runs to the end of it before
the menu is shown.

## Consequences and Risks

**Benefits:**

- The answer stops depending on browsing history. A set of distinct referents
  only grows, so an answer never changes, it only gains entries.
- The user is told that the identifier has more than one meaning. The tool
  cannot pick for them, and today it picks silently.
- The expanding-CU marker is no longer needed. It indexes one file per macro,
  which is the wrong shape for a set, and the references inside an expansion
  already record which CU was being parsed.

**Costs and risks:**

- The search cannot stop at the first hit. Finding every meaning means parsing
  every includer of the defining file. (The earlier implementation stopped
  at the first found, which one was random.)
- A macro that is included widely and expanded nowhere costs the full bounded
  scan and returns nothing.
- A macro defined by `-D` has no defining file, so the bound degenerates to the
  whole project. Treat it like search and push by name, which ask before parsing
  everything.
- Nothing can be shown until the bounded scan finishes, so a widely included
  macro makes the user wait. Ask first, the way search does.

## Considered Options

- Option 1: Pick the first expanding CU found (current behaviour)
- Option 2: Record the expanding-CU marker in server mode as well
- Option 3: Show every distinct referent in the browser menu (chosen)
- Option 4: Refuse to resolve inside a macro body

### Option 1: Pick the first expanding CU found

What the code does today. It gives one answer quickly and needs no menu. It is
wrong whenever the macro means more than one thing, and it is wrong invisibly:
the user sees a destination, not a choice. The answer also depends on what has
been parsed, so it is not reproducible.

`findMacroExpansionFile()` keeps the file of the last matching marker it walks
past, so which expanding CU comes back is arbitrary even among the ones the
snapshot recorded.

### Option 2: Record the expanding-CU marker in server mode as well

Remove the XrefMode guard in `expandMacroCall()` so a cold server records
markers too. Verified to make the suspended test pass. It costs a reference at
line 0, column 0 for every macro, which appears in the database dumps and breaks
23 tests in the collation and token-pasting family.

It also completes an index whose shape is wrong. The marker answers "one file
that expands this macro", and the decision above needs all of them.

### Option 3: Show every distinct referent in the browser menu

As described above. The user chooses, the answer is reproducible, and the menu
is the mechanism the tool already uses when a name has several bindings. It
costs a bounded search where the current code stops at the first hit.

### Option 4: Refuse to resolve inside a macro body

Answer "not available here" and let the user navigate from an expansion instead.
Honest and cheap, and it throws away a case that works today whenever the macro
has only one meaning, which is the common one.

## References

- ADR-0013: Limited detection of archaic extern declarations in C files
- ADR-0014: Adopt on-demand parsing architecture
- ADR-0019: Unify browse terminology
- ADR-0022: Lightweight file structure scanning
- ADR-0024: Batch prime via CreateMode
- ADR-0026: Follow extern declarations in C files via scanner hints

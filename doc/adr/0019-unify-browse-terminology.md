# Unify "Browse" terminology across user commands and server operations

Date: 2026-02-09

## Status

Proposed

## Problem Statement and Context

"Goto" is currently used ambiguously at multiple levels:

- User-facing commands (menus, Emacs functions, documentation)
- Server operations (OP_* enum values)
- Protocol commands (ppcGotoPosition)
- Internal function names (e.g., functions using "goto" to mean "issue positional response" or "navigate to")

This creates confusion as "goto" means different things: user intent to explore code, server computation of which reference, and low-level cursor positioning.

Additionally, "Browsing" as a concept already exists in documentation and the exercise, but isn't reflected consistently in naming.

## Proposed Decision

Adopt "Browse" as the umbrella term for reference navigation. Unify user command names with server operation names.

**Naming pattern:**
- `BROWSE_*` = navigation within browsing mode
- `*_BROWSE` = entry point that starts browsing

**Protocol layer:**

- `ppcGotoPosition` or rename to `ppcPositionCursor` - low-level editor instruction, distinct from user-level "browse"

## Architectural Insight

Browsing, Completion, and Search share the same navigation sub-operations but operate on different stacks:

| Domain | Stack | Entry Point | Navigation |
|--------|-------|-------------|------------|
| Browsing | browsingStack | PUSH | NEXT, PREVIOUS, POP, FILTER |
| Completion | completionStack | COMPLETION | NEXT, PREVIOUS, GOTO_N |
| Search | searchingStack | SEARCH | NEXT, PREVIOUS, GOTO_N |

This suggests a common "exploration mode" abstraction with domain-specific entry points but shared navigation semantics. The terminology unification should reflect this pattern - each domain has its entry operation, then uses common navigation verbs.

## Consequences and Risks

**Benefits:**

- User command name = server operation name (one concept instead of two)
- "Browse" clearly indicates "exploration mode with back-stack"
- Distinguishes user intent from low-level cursor positioning
- Consistent with existing "Browser" terminology in docs

**Risks:**

- Emacs function names may need updating for consistency
- Internal function names using "goto" ambiguously need careful review to determine actual intent

## Considered Options

TBD

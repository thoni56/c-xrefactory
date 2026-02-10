# Unify "Browse" terminology across user commands and server operations

Date: 2026-02-09

## Status

Proposed

## Problem Statement and Context

"Goto" is currently used ambiguously at multiple levels:

- User-facing commands (menus, Emacs functions, documentation)
- Server operations (OLO_* enum values)
- Protocol commands (ppcGotoPosition)

This creates confusion as "goto" means different things: user intent to explore code, server computation of which reference, and low-level cursor positioning.

Additionally, "Browsing" as a concept already exists in documentation and the exercise, but isn't reflected consistently in naming.

## Proposed Decision

Adopt "Browse" as the umbrella term for reference navigation. Unify user command names with server operation names.

**Entry points into browsing:**

| Current | Proposed |
|---------|----------|
| `OLO_PUSH` | `BROWSE_PUSH` or `PUSH_AND_BROWSE` |
| `OLO_COMPLETION_GOTO` | `COMPLETION_BROWSE` |
| `OLO_TAG_SEARCH` | `SEARCH_BROWSE` |

**Navigation within browsing (shared across all contexts):**

| Current | Proposed |
|---------|----------|
| `OLO_NEXT` | `BROWSE_NEXT_REFERENCE` |
| `OLO_PREVIOUS` | `BROWSE_PREVIOUS_REFERENCE` |
| `OLO_GOTO_DEF` | `BROWSE_GOTO_DEFINITION` |
| `OLO_POP` | `BROWSE_POP` |
| `OLO_REF_FILTER_SET` | `BROWSE_SET_FILTER` |

**Protocol layer (unchanged):**

- `ppcGotoPosition` or rename to `ppcPositionCursor` - low-level editor instruction

**Naming pattern:**
- `BROWSE_*` = navigation within browsing mode
- `*_BROWSE` = entry point that starts browsing

=======
## Architectural Insight

Browsing, Completion, and Search share the same navigation sub-operations but operate on different stacks:

| Domain | Stack | Entry Point | Navigation |
|--------|-------|-------------|------------|
| Browsing | browsingStack | PUSH | NEXT, PREVIOUS, POP, FILTER |
| Completion | completionStack | COMPLETION | FORWARD, BACK, GOTO |
| Search | searchingStack | TAG_SEARCH | FORWARD, BACK, GOTO |

This suggests a common "exploration mode" abstraction with domain-specific entry points but shared navigation semantics. The terminology unification should reflect this pattern - each domain has its entry operation, then uses common navigation verbs.

## Consequences

**Benefits:**

- User command name = server operation name (one concept instead of two)
- "Browse" clearly indicates "exploration mode with back-stack"
- Distinguishes user intent from low-level cursor positioning
- Consistent with existing "Browser" terminology in docs
- Completion, search, and push-based browsing share the same sub-commands

**Risks:**

- Renaming OLO_* values affects multiple files
- Emacs function names may need updating for consistency

## Alternatives Considered

TBD

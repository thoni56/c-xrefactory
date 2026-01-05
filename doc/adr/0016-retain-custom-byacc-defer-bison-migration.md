# Retain custom byacc parser

Date: 2026-01-01

## Status

Accepted

## Deciders

Thomas Nilefalk

## Problem Statement and Context

_In the context of_
- `c-xrefactory` using a custom fork of byacc-1.9 for parser generation,
- modern Bison (3.x+) being actively maintained with better features and tooling,
- the original rationale for custom byacc being Java support with recursive parsing,

_facing the fact that_
- Java support has been removed (ADR-0011), eliminating the primary reason for custom byacc,
- custom byacc-1.9 is unmaintained legacy code from the 1990s,
- modern Bison has equivalent features for most byacc capabilities (symbol prefixes via `%define api.prefix`, better error messages, named semantic values),
- the custom byacc provides a non-standard `lastyystate` variable used extensively in code completion logic (~30+ references),

_we decided to_
- retain the custom byacc-1.9 for parser generation,
- defer migration to modern Bison indefinitely,

_disregarding the fact that_
- modern Bison has better tooling, documentation, and active maintenance,
- continuing to use custom byacc-1.9 incurs maintenance burden (e.g., ensuring it builds on modern systems),
- future compiler or toolchain changes might break byacc-1.9 compatibility,

_because_
- the `lastyystate` variable is deeply integrated into code completion heuristics and would require 2-3 weeks of dedicated work to replace,
- the grammars themselves are standard LALR(1) and don't benefit from Bison-specific features,
- the current byacc-1.9 works reliably on modern systems (Linux, macOS, WSL),
- migration effort (~3-4 weeks total) doesn't provide proportional value given stable current state,
- if migration becomes necessary in the future, the path is well-understood and documented.

## Decision Outcome

Continue using custom byacc-1.9 for C and Yacc grammar parsing. Migration to modern Bison is deferred until:
- byacc-1.9 becomes incompatible with modern toolchains and cannot be easily fixed, OR
- a compelling feature in modern Bison justifies the migration effort, OR
- a major parser rewrite is undertaken for other reasons

If migration becomes necessary, the primary work item will be replacing `lastyystate`-based completion logic with an alternative state tracking mechanism.

## Consequences and Risks

**Benefits:**
- Avoid 3-4 weeks of migration work with uncertain value
- Code completion logic remains unchanged and stable
- No risk of introducing parser behavior regressions

**Risks:**
- Custom byacc-1.9 may require occasional fixes for new compiler warnings or system changes
- Missing out on potential Bison improvements (better error messages, diagnostics, performance)
- Technical debt from using unmaintained parser generator
- Future migration becomes harder as codebase evolves

**Mitigation:**
- Document byacc-1.9 dependencies and custom features (this ADR)
- Monitor byacc-1.9 build health on CI/modern platforms
- Preserve knowledge of migration path and blockers for future decision-making
- Consider gradual migration approach: start with non-completion grammars (cppexp_parser.y) as proof-of-concept

## Technical Notes

Custom byacc-1.9 features currently in use:
- Symbol prefix support (`-p c_yy`, `-p yacc_yy`) for multiple parsers in single executable
- File prefix support (`-b parser_name`) for custom output naming
- **`lastyystate` variable** - tracks last parser state for completion heuristics (CRITICAL BLOCKER)
- `yyErrorRecovery()` hook for custom error handling
- YYSTYPE union handling with `#include "yystype.h"`

Modern Bison equivalents exist for all features except `lastyystate`, which requires custom implementation via `%parse-param` or global state tracking.

# Pass structs by value in extracted functions

Date: 2026-01-04

## Status

Implemented

## Deciders

Thomas Nilefalk

## Problem Statement and Context

`c-xrefactory` originally forced structs/unions to be passed by pointer in extracted functions — a design choice from the late 1990s when C90 was dominant. Modern C standards (C99+) efficiently support struct-by-value semantics, and the extraction logic's special-case handling for structs had zero test coverage.

The tool already supports multiple C standards as input, so output doesn't need to be restricted to C90. Users targeting strict C90 compliance can manually post-process extracted functions or add `*` to parameters if needed.

## Decision

Remove struct/union special-case handling from extraction logic. Pass structs and unions by value in extracted functions, treating them like all other types.

This removes:
- The `isStructOrUnion()` function (only used in extraction)
- The struct check in variable assignment detection
- Struct/union conversion logic in classification

## Consequences

**Benefits:**
- Extracted code follows modern C semantics
- Clearer intent in extracted code : by-value passing signals "read-only"
- Simpler logic without special cases
- Reduces 1990s-era technical debt

**Risks:**
- Users on strict C90 may need to manually convert some pointers
- Potential minor efficiency loss if compiler doesn't optimize struct copying (rare in practice)

**Mitigation:**
- Document in release notes
- Test case (`test_extract_struct_by_value`) validates the behavior
- Users can post-process functions if pointer semantics are needed

# Remove -strict / -strictAnsi option

Date: 2026-01-05

## Status

Proposed

## Context

The `-strict` option was intended to enforce ANSI C compliance by:

1. **Rejecting non-ANSI string literals**: Multi-line strings without escaped newlines are rejected, as they violate C89/C90 standard.
2. **Excluding non-ANSI tokens**: Keywords like `__inline__`, `__const__`, `__int64`, and calling conventions (`__cdecl`, `__stdcall`) are not recognized.

### Historical Rationale
In the 1990s when c-xrefactory was created, compiler implementations varied significantly. The `-strict` option provided portability checking across different C compilers (MSVC, Borland, various Unix compilers).

### Current State of the Art

**String literals**: Modern C compilers universally accept either:

- Proper concatenated string literals (standard, portable): `"line1" "line2"`
- Escaped newlines (ANSI-compliant): `"line1\\\nline2"`
- Multi-line strings as GCC extension (non-standard but accepted)

None of these require the `-strict` check. Code that follows best practices doesn't encounter this warning.

**Non-ANSI tokens**: The non-ANSI tokens in `tokenNameInitTableNonAnsi` are:

- GCC extensions (`__inline__`, `__typeof__`, `__asm__`) - now standard in C99+
- MSVC/Windows-specific (`__cdecl`, `__fastcall`, `__int64`) - irrelevant on modern platforms
- Old 16-bit modifiers (`_near`, `_far`, `_pascal`) - completely obsolete

These tokens are **never** encountered in modern, portable code.

### Contrast with Fallback Definitions

The `fallback_defines` mechanism (which allows parsing C11/C17/C23 features via macro stubs) is complementary and remains valuable. It pragmatically handles modern language features by mapping them to parseable equivalents.

The `-strict` option, by contrast, is a **restriction** that prevents parsing valid modern code unnecessarily.

## Decision

Remove the `-strict` / `-strictAnsi` option entirely, including:

- The option parsing code
- The `tokenNameInitTableNonAnsi` table
- String literal line-ending checks in `lexembuffer.c` (lines 290, 297)
- Any documentation or man pages referencing `-strict`

## Consequences

### Positive

- Simplifies the parser by removing legacy restrictions
- Removes code that implements an obsolete compatibility check
- Aligns with modern C practices where these extensions are expected
- Reduces maintenance burden

### Negative

- Any scripts or build systems using `-strict` will fail with "unknown option"
- These are assumed to be rare or non-existent in modern usage

### Neutral

- The `fallback_defines` mechanism will continue to handle modern language features
- System headers with compiler-specific extensions will continue to parse correctly

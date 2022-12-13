# Remove enum and struct fill macro generation

Date: 2020-04-13

## Status

Accepted

## Problem Statement and Context

`c-xrefactory` has an option to generate macros for filling structs
and turning enums into strings. This option requires a pre-pass of the
c-xrefactory sources themselves to generate those macros before
compilation of said sources. This creates a Catch-22 situation in that
it requires a bootstrap step.

It also makes the build scripts much more complicated and difficult to
maintain.

## Decision

Replace the macros that are actually used and remove the generation
feature from `c-xrefactory`. There is actually no real alternative.

## Consequences and Risks

Hopefully the sources will be easier to maintain and build scripts
easier to modify.

There is a slight risk that some functionality might be broken during
the fairly simple, but not trivial, replacement.

## Considered Options

- Remove the functionality
- Keep the functionality

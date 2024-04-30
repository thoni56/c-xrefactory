# Remove alignment

Date: 2022-12-18

## Status

Accepted

And implemented.

## Deciders

Thomas Nilefalk

## Problem Statement and Context

The alignment adjustment code in the memory handling causes complexity and makes memory management harder to refactor.

## Decision Outcome

Decided to remove the alignment adjustment code

## Consequences and Risks

On some architectures, unknown which, alignment might be an issue. It
is unknown if there actually exists architectures that can not handle
un-aligned data, such as a pointer value stored across "long word"
boundaries.

In any case the possible performace impact (if any) can be disregarded.

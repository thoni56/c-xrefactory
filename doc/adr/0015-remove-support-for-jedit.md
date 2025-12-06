# Remove support for Jedit

Date: 2024-05-02

## Status

Implemented (2024-05-02 in 70e45307)

## Deciders

Thomas Nilefalk

## Problem Statement and Context

_In the context of_
- `c-xrefactory` currently supporting Emacs and Jedit as clients with custom protocol,

_facing the fact that_
- `jedit` is a less and less popular editor
- supporting multiple client with custom protocol implmentation is costly

_we decided to_
- remove all support for Jedit as a client

_disregarding the fact that_
- the Jedit client seems to have more support for Java refactorings

_because_
- future direction is in the LSP protocol, rather than a custom one.

## Decision Outcome

Remove all support for Jedit as a client.

## Consequences and Risks

Any users with Jedit as the preferred client will lose support.



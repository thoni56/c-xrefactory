# Remove support for Java

Date: 2024-04-12

## Status

Accepted

Superceeds [Remove support for JavaDoc](0010-remove-support-for-javadoc.md).

## Deciders

Thomas Nilefalk

## Problem Statement and Context

_In the context of_
- `c-xrefactory` currently supporting C, Yacc and Java
- `c-xrefactory` is very complex legacy code,

_facing the fact that_
- there are currently many alternatives with better support for Java refactoring,
- Java having evolved emmensly since 1.4,
- including the fact that the Java run-time no longer is available in .jar files,
- and the Java parts complicates the source and structure of `c-xrefactory`,

_we decided to_
- remove all support for Java,

_disregarding the fact that_
- there are many interesting Java refactorings implemented, `c-xrefactory` being the first software to cross the "Refactoring Rubicon"
- and some of those might be used as patterns for C refactorings

_because_
- the source is still available in the repo.

## Decision Outcome

Remove all support for Java, including parsing, browsing symbols and refactoring. This also applies to the editor adaptor, which should not retain any remnants of support for Java.

## Consequences and Risks

A lot of code can be removed which is good. A risk might be that we want to keep supporting Java but that is unlikely since there are better support for more modern versions of Java available.

Using the Java version of some refactorings, like "Move Static Method", as a model for a C version, is still possible since all code is in the repo.
The last commit with "complete" Java support is also tagged with "LAST_WITH_JAVA_SUPPORT" for easy identificatio.


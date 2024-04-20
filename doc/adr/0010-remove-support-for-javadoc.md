# Remove support for JavaDoc

Date: 2023-01-16

## Status

Superseded by [Remove support for Java](0011-remove-support-for-java.md)

## Deciders

Thomas Nilefalk

## Problem Statement and Context

The sources contains a lot of cruft and Java will very likely be abandoned.

## Decision Outcome

Remove support for browsing symbols in the JavaDoc along with all options pertaining to configuring that functionality.

## Consequences and Risks

A lot of code can be removed which is good. A risk might be that we want to keep supporting Java but that is unlikely since there are better support for more modern versions of Java available.

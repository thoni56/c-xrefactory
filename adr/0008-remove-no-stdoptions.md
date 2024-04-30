# Remove no-stdoptions

Date: 2022-12-12

## Status

Accepted

## Context

The "-no-stdoptions" seems superflous. The option seems to be intended
to prevent reading the options/settings for the project in the
standard settings file, usually in the users home directory.

## Decision

Remove the option.

## Consequences

Assuming that the options/settings for a project is only defined in a
single place, there are no downsides or risks.

Any command line script or Makefile using this option will be rejected
with a "unknown option" message.

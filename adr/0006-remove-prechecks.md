# Remove pre-check functions

Date: 2022-04-14

## Status

Accepted and implemented.

### Context and Problem Statement

There are a number of options that selects "trivial pre-check" functions.
These are not used anywhere, so it seems they are remnants of something old.

### Decision Drivers

The primary driver for this decision is to minimize "cruft", extra code that is not useful.

### Considered Options

* Keep as is (for posterity and possible reuse)
* Remove all "tpc":s except the one that is used
* Remove the concept, and all code refering to, "tpc":s, meaning the one used "getlastimportstatement" need to be renamed

### Decision Outcome

Chosen option "remove the concept of tpc:s", because it simplifies code, clarifies purpose of the single _tpc_ that is actually used.



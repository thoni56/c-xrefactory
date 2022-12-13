## Functional Overview

The _c-xref_ program is actually a mish-mash of a multitude of
features baked into one program. This is the major cause of the mess
that it is source-wise.

It was

- a generator for persistent cross-reference data
- a reference server for editors, serving cross-reference, navigational and completion data over a protocol
- a refactoring server (the worlds first to cross the Refactoring Rubicon)
- ~~an HTML cross-reference generator (probably the root of the project)~~ (REMOVED)
- ~~a C macro generator for structure fill (and other) functions~~ (REMOVED)

It is the first three that are unique and constitutes the great value
of this project. The last two have been removed from the source, the
last one because it was a hack and prevented modern, tidy, building,
coding and refactoring. The HTML cross-reference generator has been
superseeded by modern alternatives like Doxygen and is not at the core
of the goal of this project.

One might surmise that it was the HTML-crossreference generator that
was the initial purpose of what the original `Xrefactory` was based
upon. Once that was in place the other followed, and were basically
only bolted on top without much re-architecting the C sources.

What we'd like to do is partition the project into separate parts,
each having a clear usage.


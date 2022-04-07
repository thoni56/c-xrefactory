---
title: "3. Automatically find configuration files
---

* Status: Current
* Deciders: Thomas Nilefalk
* Date: 2022-04-06

# How to find configuration files?

## Y-Statement

_In the context of_ finding and reading .c-xrefrc files with configuration options
_facing the fact_ that it should be possible to store in a repo and avoid sending various command line options to c-xref
_we decided for_ searching the directory tree upwards from the indicated file or current working directory for a .c-xrefrc
_and neglected_ -
_to achieve_ ease of configuration and possibility to share settings when working on different machines with a shared repo
_accepting_ the incompatibility that this might introduce for some users
_because_ this is similar to what many other tools do (gdb/.gdbinit, git/.git, lsp/.lspconfig, ...)
    
## Decision Drivers

The current options, `-stdop`, `-no-stdop` `-xrefrc` creates a complicated set of combinations for which I don't really see the need.
In tests you probably want to say `-nostdop` and `-xrefrc ...`, but when this functionality is implemented you'd just remove those options and add a `.c-xrefrc` in the root of the test directory instead.

Also, the location of the `.c-xrefrc` implicitly indicates the directory tree that will be analyzed.
Probably a way to ignore particular subtrees has to be introduced.

## Considered Options

None.

## Decision Outcome

To be implemented.

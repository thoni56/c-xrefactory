# Automatically find configuration files

Date: 2022-04-06

## Status:

Implemented - discovery in `d5a7182f` (2026-01-23); the last of the option removals it
calls for, `-xrefrc`, in `6f83b04a` (2026-09-25)

Outcome settled by [ADR-0031](0031-what-files-a-project-consists-of.md)

## Deciders

Thomas Nilefalk

## How to find configuration files?

### Y-Statement

_In the context of_ finding and reading .c-xrefrc files with configuration options  
_facing the fact_ that it should be possible to store configurations in a repo and avoid sending various command line options to c-xref  
_we decided for_ searching the directory tree upwards from the indicated file starting with the current working directory for a .c-xrefrc  
_and neglected_ problems with keeping total backwards compatibility  
_to achieve_ ease of configuration and possibility to share settings when working on different machines with a shared repo  
_accepting_ the incompatibility that this might introduce for some users  
_because_ this is similar to what many other tools do (gdb/.gdbinit, git/.git, lsp/.lspconfig, ...)  
    
### Decision Drivers

The current options, `-stdop`, `-no-stdop` `-xrefrc` creates a complicated set of combinations for which I don't really see the need.
In tests you probably want to say `-nostdop` and `-xrefrc ...`, but when this functionality is implemented you'd just remove those options and add a `.c-xrefrc` in the root of the test directory instead.

Also, the location of the `.c-xrefrc` implicitly indicates the directory tree that will be analyzed.
Probably a way to ignore particular subtrees has to be introduced, unless it already exists (`-prune`?).

### Considered Options

None.

### Decision Outcome

Yet to be decided.

*Settled by ADR-0031 (2026-10-02).* A config holds one project, and its section header
means nothing. The project is the tree under the config's directory, plus the
directories the config lists, minus what `-prune` removes; a nested `.c-xrefrc` is
another project. The root path is the project's identity (ADR-0029). The implications
below were answered along those lines: no multiple projects per file, and no required
project heading.

### Decision Implications

It has to be decided if such an "automatic" configuration file 

- may allow multiple projects - probably not, this will be confusing
- must have a project heading (initial line with "[projectname]") - again, probably not, it would be better to infere the projectname, if needed, from the last part of the path

or if the "project concept" should be scrapped.

`c-xref` will still be "project-based", it's just that the project is

- contained in a directory tree defined by where the `.c-xrefrc` file exists
- the `.c-xrefrc` file only contains one project and defines settings for that one project

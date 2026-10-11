# The config names what moves, and a local fragment binds it

Date: 2026-10-11

## Status

Proposed

Settles the machine-local fragment that
[ADR-0031](0031-what-files-a-project-consists-of.md) leaves open
Builds on [ADR-0029](0029-project-identity-is-the-root-path.md) and
[ADR-0005](0005-automatically-find-config-files.md)

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

_In the context of_
- `.c-xrefrc` being found from the file and checked in with the project, so that
  settings are shared between machines with a shared repository (ADR-0005),
- the config holding no absolute paths, since it travels (`11-planned-features.adoc`,
  ADR-0029), and the home config being gone, so relative paths are the norm,
- ADR-0031 dividing a project's files into *own*, *visible* and *outside*, and making
  directories outside the root, i.e. listed in the config, more common,
- `${NAME}` (or `%NAME%`) in the config expanding a `-set` variable, then an environment
  variable (`expandEnvironmentVariables`, `src/options.c`),

_facing the fact that_
- an own path is always relative to the project, but an external one, e.g. the source
  of a library to navigate into or an `-I` into a local toolchain, lives in a different
  place on each machine, and today there is nowhere to say where,
- the compiler may differ between machines too, and it decides the system includes and
  the predefined macros (`__GNUC__`, `__x86_64__` ...),
- a name that is not bound is left as written, and becomes a path that does not exist,
  silently,
- a GUI Emacs often lacks the shell's environment, so the environment is an unreliable
  place for a binding,
- `-optinclude` reads its file with `PROCESS_FILE_ARGUMENTS_NO` (`handleIncludeOption`),
  so an included file cannot add source directories, and a missing file is fatal,
  where absent is the normal case for a machine-local file,
- some options a config accepts are personal, i.e. the user's and not the project's or
  the machine's: `-maxcompls=`, `-completioncasesensitive`, `-olinelen=`,
  `-commentmovinglevel=`, `-errors` and its family, `-log=`, `-debug`, `-trace`,
  `-commandlog=`, `-statistics`,

_we propose to_
- let the **checked-in config name every movable part** by a variable, e.g.
  `-I ${FFMPEG}`, a listed directory `${FFMPEG}/libavcodec`, or `-compiler=${CC}`,
- let a **machine-local fragment only bind names**, `-set FFMPEG /home/...`, and nothing
  else. A fragment says where something is, never what the project is,
- let a name have a **default** in the config, so that a fragment is needed only on a
  machine that differs,
- **report an unbound name** when the project is opened, so the client can ask for it
  and write the fragment,
- let a fragment's bindings **come before the environment**,
- keep the **personal options out of both files**. They are editor settings, sent by the
  client. In a config they are ignored, with a warning,

_disregarding the fact that_
- a binding cannot change what the project is, so a project that really differs
  between machines, e.g. by a `-D`, cannot be expressed. Such a `-D` usually comes from
  a local `configure`, i.e. from the build, and belongs with `compile-commands` in
  `doc/backlog.md`,
- a config that sets a personal option today loses it,

_because_
- the checked-in config stays the complete declaration of the project, and two machines
  index the same project, only found in different places,
- restricting a fragment by *kind*, bindings only, needs no list of allowed options, and
  so no guessing what future projects will need,
- the split matches the paths that actually occur: own paths are relative, and only the
  external ones need a binding,
- it is how make does it: a Makefile names `$(CC)`, and `make CC=clang` binds it from
  outside,
- half of it works today, since `${NAME}` already expands `-set` variables.

## Decision Outcome

Proposed. The config names what moves, a local fragment binds the names and does nothing
else, an unbound name is reported, and personal options are editor settings only.

## Consequences and Risks

- *The syntax of a default is not chosen.* `${NAME:-default}` is the shell's. It needs
  the expansion to learn it.
- *Where the fragment lives is open.* A sibling `.c-xrefrc.local` is simple, but it is
  inside the repository and relies on `.gitignore`. A per-user file keyed by the root
  path, which ADR-0029 makes the project's identity, can never be checked in by
  mistake. That is not the home config coming back: the home config defined projects,
  this only binds names.
- *Reporting an unbound name needs a place in the protocol*, an answer to
  `-getproject` or `-openproject` (ADR-0030) that lists the names.
- *Required or optional.* An unbound library to navigate into could be optional, i.e.
  not navigable, while an unbound toolchain `-I` makes parsing go wrong. A name with a
  default is never unbound, so perhaps a name without one is required.
- *Two more options look local but are not bindings.* `-filescasesensitive` and
  `-filescaseunsensitive` are a property of the filesystem, and `-crlfconversion` and
  `-crconversion` of the checkout. They are better detected than configured. This
  decision leaves them as they are.
- *`-optinclude` has nothing left to do* and could go.

## Considered Options

- **A fragment that only binds names** — proposed.
- **A free-form fragment**, like `-optinclude` but able to add source directories. The
  simplest, but the project can then differ between machines, and two machines index
  different projects under the same config.
- **A fragment limited to a list of allowed options.** It needs to know in advance what
  future projects will need, and still lets a fragment change the project through the
  options on the list.

## Origin

The need was noted on WSL and the Mac in `doc/backlog.md` ("Local config fragments"),
and bindings only became the leading direction on the Mac, session `36694da1`,
2026-10-02. Thomas came to the same three options and the same preference on WSL,
session `d25255b8`, 2026-10-11, and a sweep over the options in `src/options.c` found
`-compiler=` the only other local one, and the personal options belonging in neither
file.

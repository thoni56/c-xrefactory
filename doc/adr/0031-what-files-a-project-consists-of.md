# What files a project consists of

Date: 2026-09-30

## Status

Accepted (2026-10-02)

Settles the open outcome of [ADR-0005](0005-automatically-find-config-files.md)
Corrects and builds on [ADR-0029](0029-project-identity-is-the-root-path.md)
Defines own, visible and outside for [ADR-0030](0030-opening-a-project-and-asking-about-a-file-are-separate.md)

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

_In the context of_
- the project being identified by its root path, the directory holding its `.c-xrefrc`
  (link:0005-automatically-find-config-files.md[ADR-0005],
  link:0029-project-identity-is-the-root-path.md[ADR-0029]),
- opening a project and asking about a file being separate requests, the query answering
  *own*, *visible* or *outside*
  (link:0030-opening-a-project-and-asking-about-a-file-are-separate.md[ADR-0030]),
- ADR-0030 defining *own* as "under the root or a declared source directory" and
  *visible* as "reached through an include path", without saying what a declared source
  directory is, or what it did before,

_facing the fact that_
- **the meaning of a bare directory name in `.c-xrefrc` changed, and
  nothing recorded it.** In the original model (`doc/c-xrefrc.man`)
  one config held several sections, `-p` chose one, and the section's
  directory names were the *complete* set of input files — "`c-xref -p
  solver` will make c-xref read (recursively) all source files from
  `/home/marian/solver` and `/home/marian/commons`" given that those
  two directories were listed. A listed directory did not have to be
  under anything. The directory names *were* the project,
- ADR-0005 (2022) replaced that with "the location of the `.c-xrefrc` implicitly
  indicates the directory tree that will be analyzed", with `-prune` for subtrees. It
  left its own outcome "Yet to be decided" and never mentioned directory names.
  Discovery landed in `d5a7182f` (2026-01-23), the directory names came back as
  `ProjectConfig.sourceDirs` in `98bffa57` (2026-03-25), and `scanProjectStructure()`
  (`src/server.c`) now scans the root *and then also* each listed directory. The names
  went from "these are the project" to "also scan these" without a decision saying so,
- scanning and membership use **different definitions**. Scanning is root plus listed
  directories minus `-prune`. Membership in `handleProject()` (`src/cxref.c`) is root
  prefix or `-I` prefix. So a CU in a listed directory outside the root is scanned and
  indexed, yet a `-getproject` for it answers `PROJECT_MISMATCH`, and a file under an
  `-I` directory counts as the project although nothing scans it,
- the `-I` rule was added in `d1e73170` (2026-03-24) so that browsing into a header
  outside the root does not trigger the "switch project" prompt. The same commit
  suspended `tests/test_getproject_unknown_cu_under_include_path`, because the rule also
  accepts a CU the project never includes,
- in practice only the **first** section of a `.c-xrefrc` is read:
  `getProjectNameFromConfigFile()` (`src/options.c`) takes the first header as the
  project's name, lines before any header apply too, and a later section is read only
  if its header is a path prefix of the chosen section's name
  (`processSingleProjectMarker()`), a leftover of the path-keyed sections. ADR-0029's
  "the name still selects *which options apply*" among several sections does not
  describe the code. Further sections are dead text in practice —
  `tests/test_server_project_switch` had one,
- once the root path is the identity, the section name has one job left: it is what
  `set-info` returns and the client displays. It is also why the server keeps two
  globals, `lockedProject` (the name) and `lockedProjectRoot`,
- `-prune` has changed meaning once already. Originally (`doc/c-xref.man`) it took "a
  simple name, an absolute path, or a ':' separated list of both", a name matching that
  directory anywhere. The user manual now documents a path relative to the root, matched
  exactly, and says the name matching was dropped on purpose. But
  `resolvePrunePaths()` (`src/options.c`) concatenates the root and the entry without
  normalizing it, which also dropped absolute paths and `:` lists without saying so. A
  prune can therefore only name something under the root, not inside a listed directory
  elsewhere — although the user manual says paths in `.c-xrefrc` are resolved against
  the config file's directory,
- a nested `.c-xrefrc` below the root — every `tests/test_*` directory in this repository
  gets one when its test runs — is scanned as part of the enclosing project, although
  discovery from a file in it would find a different project,

_we propose to_
- define **own** as: a file under the root for which discovery finds the open root, or a
  file under a directory listed in the config — in both cases not removed by `-prune`. A
  listed directory adds to the project and never narrows it; narrowing is what `-prune`
  is for,
- let **`-I` mean what it means to the compiler**, a place to look for included files,
  and nothing about membership,
- let **`-prune` follow the config's general path rule**: each entry is normalized
  against the config's directory, as the listed directories already are. That keeps
  root-relative entries as they are, lets `../lib/tests` prune inside a listed
  directory outside the root, brings absolute paths back (for a machine-local fragment,
  not the checked-in config, see below), and makes the comparison with
  the walker's paths reliable because both sides are normalized. Name matching stays
  dropped, as documented, and so do `:` lists — one `-prune` per entry,
- make scanning and the *own* answer **one definition**, in one place, so the two cannot
  drift again,
- define **visible** as a file transitively included by an *own* file, according to the
  include graph the server already keeps, not every file under an `-I` directory,
- let *own* and *visible* files both be **editable**. A file the project marks read-only
  (Semantic Read-only Files in `doc/docs/11-planned-features.adoc`) is the explicit
  exception, and a refactoring that would touch one is refused before any edit, rather
  than failing when the file is saved,
- let the **membership query look the file up by name and never register it** in the
  file table, as `-get` already does. A file the table does not know has no includer, so
  it is not visible, and nothing a query names ends up in the snapshot,
- let a **nested `.c-xrefrc` below the root act as an implicit `-prune`** of its subtree:
  that tree is another project. This is what "discovery finds the open root" in the
  definition of *own* amounts to, and it keeps membership and discovery in agreement, so
  which project a file belongs to does not depend on which file was opened first. A
  directory listed explicitly stays *own* even if it has a config of its own, since
  listing it is the point,
- let an **outside** answer say whether discovery from the file finds another project.
  It does not need to name it, since the open after a switch finds it anyway. Finding
  nothing and finding the open project's own root both count as "no project found",
  since switching to either gains nothing, and the client offers a switch only when a
  project was found,
- let the **root path be the project's id everywhere**: `lockedProject` holds it,
  `lockedProjectRoot` goes, and `set-info` and the mismatch answer return it. A long path
  is a display problem, and the client's to solve,
- let the **section header mean nothing**: a config is a flat list of options. A first
  header is tolerated and ignored, so existing configs keep working. A second header
  ends what is read, as in practice it does today, and `-openproject` warns about it —
  ignoring it instead would merge the sections and silently change the project. The
  path-prefix matching of later sections goes, and so does `__BASE`, the variable that
  held the section name. Nothing in the repository or in the configs in use refers to it,
- let **`-openproject` accept a directory as well as a file**, with the same upward
  search from it (`searchUpwardForProjectLocalConfig()` already starts at a directory
  when given one). A directory holding the config opens that root; requiring the config
  to be in the directory itself would be a second discovery rule for one case. This
  gives the client an explicit "open project at…", and fits LSP's `rootUri`,

_disregarding the fact that_
- a file that is only under a listed directory cannot open its project. Discovery walks
  upward, and nothing in the listed directory points back at the configs that list it —
  the system's configs cannot all be searched. The client has to open the project from a
  file under the root, or from the root itself,
- a directory listed in two projects' configs is *own* in both. That is harmless with one
  server per project,
- the include graph only knows what has been scanned or parsed, and the lightweight scan
  resolves only `#include "..."`. A header reached only through `<...>` and `-I` becomes
  visible once a CU including it has been parsed; opened before that, it is outside with
  no project found, which gives a message and no switch. Teaching the scan angle-bracket
  includes would remove that, and is in the backlog already ("Extend the lightweight scan
  to `<...>` includes"),
- a `.c-xrefrc` that comes and goes changes the enclosing project's extent. The configs
  in this repository's test directories are generated when a test runs, so the
  repository's project would differ between clones that have run different tests. That
  is uncommon, and an explicit `-prune` makes it stable. A nested config is noticed when
  the project is scanned, i.e. when it is opened and after a config change, so adding one
  during a session prunes nothing until then,
- a directory outside the root, or an `-I` into a local toolchain, often lives in a
  different place on each machine, while the config travels with the project and must
  not contain absolute paths (`doc/docs/11-planned-features.adoc`, ADR-0029). This
  decision makes such directories more common without saying where their locations
  go. That needs a machine-local fragment, `doc/backlog.md`'s "Local config fragments",
  which this decision does not settle. `${NAME}` in the config already expands `-set`
  variables and then environment variables, so a fragment that only binds names the
  checked-in config uses is one direction. A name that is not bound is left as written
  today and becomes a path that does not exist; `-openproject` should report it,

_because_
- the root, the listed directories and `-prune` together describe almost any layout — a
  project tree, libraries outside it to scan, generated or test trees inside it to skip —
  without the several-sections-per-file machinery the root-path identity replaced,
- `-I` is already understood by every C programmer through the compiler, and giving it a
  second meaning is what produced the suspended bug,
- *visible* exists so that browsing into an included header does not prompt a project
  switch. Every `#include` is recorded as a reference on a per-file include item, the
  graph is kept in the snapshot, and the stale-header pass already walks it backwards
  from a file to its includers. What the project includes is the question, not where a
  file lies,
- switching projects is itself a discovery: the client restarts the server and opens it
  from the file. For a pruned file without a config of its own, that discovery finds the
  same root again, so a switch offer would restart into the same project and ask again,
- the recent decisions all key on the path — the snapshot location (ADR-0028), the
  identity (ADR-0029) — and a name beside it only adds a second thing that must agree.

## Decision Outcome

A project consists of its root tree and the directories its config lists, minus what
`-prune` removes; these are its *own* files, and they are what is scanned. `-I` is only
the header search path. A file an own file includes, directly or through other headers,
is *visible*. Anything else is *outside*, and the answer says whether a project was
found for it, which is what decides if a switch is offered. A nested `.c-xrefrc` is an
implicit `-prune`. The root path is the project's id everywhere; a config's section
header means nothing, and a second one ends what is read, with a warning.
`-openproject` takes a file or a directory under the root.

For this repository that means: the root is the repository, `tests` is pruned
explicitly (the generated configs in the test directories would do it only after the
tests have run), and a third-party tree outside the root would be listed as a directory.

## Consequences and Risks

**Benefits:**
- One definition for scanning and membership; the listed-directory mismatch goes away.
- `test_getproject_unknown_cu_under_include_path` gets a defined fix.
- The config's meaning is written down, after two changes that were not.
- The server's two lock globals become one, and the name/root duality disappears.

**Risks:**
- *The query must stop registering its file.* Every request's file argument is added to
  the file table and scheduled today, which is what
  `test_getproject_unknown_cu_under_include_path` is suspended on and how a queried file
  ends up in the snapshot. The membership query has to take the name only.
- *Existing configs with several sections* keep reading only their first section, as
  they do today; what is new is the warning.
- *Answers change from name to path.* `set-info` and the mismatch answer return the root
  path, so tests with a named section change their expected output —
  `test_project_lock` answers `CURDIR/project_a` instead of `project_a`. Tests generated
  from the common template already have `[CURDIR]` as their header and see no change.
- *Prune resolution changes* only for entries that reach outside the root or are
  absolute, which match nothing today. Entries under the root resolve as before.
- *Two manual pages describe the original model.* `doc/c-xrefrc.man` (several sections,
  `-p`, directory names as the whole project) and `doc/c-xref.man` (`-prune` by name)
  are rewritten from this decision; `doc/user-manual.adoc` needs only the extension.
- *Three suspended tests pin parts of this decision* and come off suspension as it is
  implemented: `test_server_refuse_project_switch` (a nested config is another project),
  `test_getproject_listed_dir_outside_root` and `test_getproject_pruned_file` (membership
  disagrees with the scan). The pruned test expects today's `project-mismatch` until the
  protocol has a form for "outside, no project found".
- *The client's mismatch handling changes.* It offers a switch on every mismatch today
  (`c-xref-server-dispatch-project-mismatch`); it has to say "not part of the project"
  instead when no project was found. The same function kills the server without `-exit`,
  so a switch loses everything since the last snapshot. That is a defect of its own, best
  fixed in the same change.

## Considered Options

- **Root plus listed directories minus prune, `-I` for headers only** — proposed.
- **Listed directories only**, the original meaning: the root contributes nothing unless
  listed. Faithful to the man page, but contradicts the root-path identity, since the
  directory that identifies the project would not have to belong to it.
- **Root minus prune, listed directories ignored.** The simplest, but a library outside
  the root could not be scanned at all.
- **Keep `-I` as membership**, the current rule. Needs no include walk, but keeps
  accepting CUs the project never includes.
- **Visible as "under an `-I` directory"**, ADR-0030's wording. The same too-wide rule
  under another name.
- **Visible as "directly included by an own file".** Simpler, but a header included only
  through another header would fall out, and the transitive walk exists already.

## Origin

Discussed on the Mac, session `36694da1`, 2026-09-30. An experiment ignoring `-p` in the
server, and the tripwire that followed it, left `tests/test_server_project_switch`
(since rewritten as `test_server_refuse_project_switch`) refusing to become a "refuse
project switch" test: locked on its first section, a file under the second section's
directory answered `set-info` for the first. Tracing the config back to
`doc/c-xrefrc.man` showed that the directory names had changed meaning without a
decision. The same session settled the open points on 2026-10-01: *outside* says
whether a project was found, *visible* is transitive, a nested config is an implicit
`-prune`, the path is the id and the section header means nothing, and `-prune`
follows the config's path rule.

# What files a project consists of

Date: 2026-09-30

## Status

Proposed

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
- **the meaning of a bare directory name in `.c-xrefrc` changed, and nothing recorded
  it.** In the original model (`doc/c-xrefrc.man`) one config held several sections, `-p`
  chose one, and the section's directory names were the *complete* set of input files —
  "`c-xref -p solver` will make c-xref read (recursively) all source files from
  `/home/marian/solver` and `/home/marian/commons`". A listed directory did not have to
  be under anything. The directory names *were* the project,
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
- only the **first** section of a `.c-xrefrc` is ever read:
  `getProjectNameFromConfigFile()` (`src/options.c`) takes the first header, and only
  that section's options are processed. ADR-0029's "the name still selects *which
  options apply*" among several sections does not describe the code. Further sections
  are silently dead text — `tests/test_server_project_switch` had one,
- `resolvePrunePaths()` (`src/options.c`) concatenates the root and the prune entry, so a
  prune can only name something under the root, not inside a listed directory elsewhere,
- a nested `.c-xrefrc` below the root — every `tests/test_*` directory in this repository
  has one — is scanned as part of the enclosing project, although discovery from a file
  in it would find a different project,

_we propose to_
- define **own** as: every file under the root, plus every file under a directory listed
  in the config, minus what `-prune` removes. A listed directory adds to the project and
  never narrows it; narrowing is what `-prune` is for,
- let **`-I` mean what it means to the compiler**, a place to look for included files,
  and nothing about membership,
- let **`-prune` apply to the root and to the listed directories alike**, resolved
  against the config's directory, so it can name a subtree of a listed directory outside
  the root,
- make scanning and the *own* answer **one definition**, in one place, so the two cannot
  drift again,
- define **visible** as a file the project's parses have actually included, not every
  file under an `-I` directory,
- let a **nested `.c-xrefrc` below the root prune its subtree** implicitly: that tree is
  another project. A directory listed explicitly stays *own* even if it has a config of
  its own, since listing it is the point,
- have **one section per config**, and have `-openproject` report further sections as a
  config problem rather than ignore them silently,
- let **`-openproject` accept a directory as well as a file**. Discovery searches upward
  from it either way (`searchUpwardForProjectLocalConfig()` already handles a
  directory),

_disregarding the fact that_
- a file that is only under a listed directory cannot open its project. Discovery walks
  upward, and nothing in the listed directory points back at the configs that list it —
  the system's configs cannot all be searched. The client has to open the project from a
  file under the root, or from the root itself,
- a directory listed in two projects' configs is *own* in both. That is harmless with one
  server per project,

_because_
- the root, the listed directories and `-prune` together describe almost any layout — a
  project tree, libraries outside it to scan, generated or test trees inside it to skip —
  without the several-sections-per-file machinery the root-path identity replaced,
- `-I` is already understood by every C programmer through the compiler, and giving it a
  second meaning is what produced the suspended bug,
- *visible* exists so that browsing into an included header does not prompt a project
  switch. The file table already knows what the project's parses included, and that is
  the question, not where a file lies.

## Decision Outcome

A project consists of its root tree and the directories its config lists, minus what
`-prune` removes; these are its *own* files, and they are what is scanned. `-I` is only
the header search path. A file the project's parses include without owning it is
*visible*. A nested `.c-xrefrc` below the root is another project. A config has one
section. `-openproject` takes a file or a directory under the root.

For this repository that means: the root is the repository, `tests` is pruned (or pruned
already by the configs in each test directory), and a third-party tree outside the root
would be listed as a directory.

## Consequences and Risks

**Benefits:**
- One definition for scanning and membership; the listed-directory mismatch goes away.
- `test_getproject_unknown_cu_under_include_path` gets a defined fix.
- The config's meaning is written down, and `doc/c-xrefrc.man` can be rewritten from it.

**Risks:**
- *Visible needs the file table to tell included files from merely named ones.* The
  suspended test's note says `-getproject` registers the request file in the file table,
  so "known to the file table" is not yet "included by the project". It needs either no
  registration from the query or transient entries.
- *Existing configs with several sections change meaning.* They already do today,
  silently; reporting it makes the change visible instead of making it new.
- *Prune resolution changes* for entries that reach outside the root. Entries under the
  root resolve as before.
- *Three suspended tests pin parts of this decision* and come off suspension as it is
  implemented: `test_server_refuse_project_switch` (a nested config is another project),
  `test_getproject_listed_dir_outside_root` and `test_getproject_pruned_file` (membership
  disagrees with the scan). The pruned case also needs an answer here first: a pruned
  file with no config of its own belongs to no project, so "outside, switch?" would
  restart into the same project.

## Considered Options

- **Root plus listed directories minus prune, `-I` for headers only** — proposed.
- **Listed directories only**, the original meaning: the root contributes nothing unless
  listed. Faithful to the man page, but contradicts the root-path identity, since the
  directory that identifies the project would not have to belong to it.
- **Root minus prune, listed directories ignored.** The simplest, but a library outside
  the root could not be scanned at all.
- **Keep `-I` as membership**, the current rule. Avoids the file-table work for *visible*,
  but keeps accepting CUs the project never includes.
- **Visible as "under an `-I` directory"**, ADR-0030's wording. Needs no file-table
  change, but is the same too-wide rule under another name.

## Origin

Discussed on the Mac, session `36694da1`, 2026-09-30. An experiment ignoring `-p` in the
server, and the tripwire that followed it, left `tests/test_server_project_switch`
(since rewritten as `test_server_refuse_project_switch`) refusing to become a "refuse project switch" test: locked on its first section, a file
under the second section's directory answered `set-info` for the first. Tracing the
config back to `doc/c-xrefrc.man` showed that the directory names had changed meaning
without a decision.

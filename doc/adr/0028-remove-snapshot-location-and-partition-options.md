# Remove the snapshot's location and partition options

Date: 2026-09-23

## Status

Accepted

## Deciders

Thomas Nilefalk (maintainer)

## Problem Statement and Context

_In the context of_
- the `.cx` file being a disposable startup snapshot of an in-memory table — loaded
  whole, mtime-validated, rebuilt on demand if it is missing or stale (ADR-0014,
  and "Memory as Truth" in the roadmap),
- auto-discovery finding the project from the request file, so the project root is
  always known (link:0005-automatically-find-config-files.md[ADR-0005]),
- `applyConventionBasedDatabasePath()` already overriding whatever `-refs` said with
  `<project root>/.c-xref/db` for a detected project,

_facing the fact that_
- `-refnum` partitions the database by symbol hash, which was a **read** optimisation:
  the old engine answered a lookup by reading the one partition a symbol hashed to.
  Nothing reads the file that way any more. It is read once, in full, at startup,
- `-refs` lets the snapshot live anywhere, but there is one right place, the code
  already goes there, and the option therefore says something that is not true,
- both are written into every config the tests generate and into every config a user
  made before discovery existed,

_we decided to_
- **remove `-refs` and `-refnum`**: the snapshot is always `<project root>/.c-xref/db`,
  and always a single file,
- make `applyConventionBasedDatabasePath()` unconditional and delete the partition
  handling in `cxfile.c`,
- report either option, when found in a config, through `errorMessage` and carry on —
  a stale config is user input, not a reason to fail,

_disregarding the fact that_
- a user can no longer put the snapshot somewhere else: a faster disk, a scratch
  directory, or beside a checkout they cannot write into,

_because_
- a disposable cache is not worth configuring. If it cannot be written, the project
  still works; the next start is slower and nothing else changes,
- every surviving option has to be placed on the Setup Ladder — session, project or
  request — and defended there. These two cannot be defended: one configures a cache,
  the other tunes a read path that no longer exists.

## Decision Outcome

The snapshot lives at `<project root>/.c-xref/db`, as one file, always. `-refs` and
`-refnum` are gone from the option table; a config containing either gets a message
naming the option and saying where the snapshot now lives, and the rest of the config
is honoured.

## Consequences and Risks

**Benefits:**
- Deletes the partition handling in `cxfile.c` — the hash modulo, the per-file loops,
  and the single-versus-many branches.
- Removes the last reason for `applyConventionBasedDatabasePath()` to be conditional.
- One less thing a generated config has to say, and one less thing a user can get wrong.

**Risks:**
- *The snapshot format carries the count.* `CXFI_REFNUM` is written and read back
  (`currentCxFileCountMatches`). Either the record stays and is always 1, or the format
  changes and `test_snapshot_version_migration` has an opinion about it. Decide when
  implementing; keeping the record costs nothing.
- *Fixtures assume partitions.* The common `tests/c-xrefrc.tpl` sets both options, ten
  per-test templates set `-refnum`, and `test_autodetect_creates_cxref_dir` asserts the
  `X0000…X0009` plus `XFiles` layout precisely because the template asks for ten.

## Considered Options

- **Remove both** — chosen.
- **Keep them, bound to the project tier.** This is what the backlog assumed before this
  decision: they are project-scoped, so they move into the config and bind at
  `-getproject`. It preserves the ability to place the snapshot elsewhere, at the cost of
  keeping a read-path tuning knob for a read path that was deleted.
- **Keep `-refs`, drop `-refnum`.** Splits the difference and keeps the weaker of the two:
  the location, which the code already overrides for every detected project.

## The tree you cannot write into

Removing these options raises a case worth naming rather than discovering later: source
you want to browse but cannot write a `.c-xrefrc` into — a vendored dependency, a
read-only checkout, a system source tree. Upward discovery stops before `$HOME`, so such
a tree has no discoverable config, and `-xrefrc` is today the only way to point at one.

Two different needs hide there:

- *An **external source tree** that is part of your project* — a library whose source you
  want to navigate into. That one is covered, and survives: the config names extra source
  directories, which is a different mechanism from the `[project]` registry that dies with
  `-p`.
- *A separate project on a tree you cannot write to.* **Unsupported for now.** It needs an
  answer eventually — a config elsewhere that names its tree, most likely — but not one
  invented in passing while deleting a cache option.

## Related Decisions

- **ADR-0005**: Automatically find configuration files — makes the project root always
  known, which is what lets the snapshot's location be a convention rather than an option.
  It also called for removing `-xrefrc`, which is the option this decision leans on.
- **ADR-0014**: On-demand parsing — made the `.cx` file a cache rather than the truth.
- **ADR-0021**: Single project per server.

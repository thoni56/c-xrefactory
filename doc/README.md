# Documentation

We continually try to document architecture and design using
Structurizr (cd to this directory and run `../utils/structurizr`, then
open http://localhost:8080).

The documentation consists of three elements

- documentation
- diagrams/model
- decisions

## Documentation

In the subdirectory `docs` each section in the documentation
is stored in a separate Markdown or AsciiDoc file. In it diagrams
generated from the system/architecture model is included.

At this point the user manual is still a separate AsciiDoc file.

## Diagrams/Model

All diagrams are generated by Structurizr from the model in `workspace.dsl`.

## Decisions

We document significant, including architecture-relevant, decisions
using classic ADRs stored as separate files in the `adr`
subdirectory. There are some good documentation and ideas about ADRs
[here|https://github.com/joelparkerhenderson/architecture_decision_record]
and [here|https://adr.github.io/].

We have not picked a template yet (and might never do) so write what
seems relevant.

The ADRs should be Markdown files given a name after the following
pattern:

    <nnn>--<name>.md

- <nnn> - sequential number
- <name> should be a a present tense imperative verb phrase,
  e.g. "choose_database" or "manage_password" because this will match
  what the reader want to know when coming here

The ADR should at least contain something like a
[Y-statement|https://www.infoq.com/articles/sustainable-architectural-design-decisions/]
following this pattern:

    In the context of <use case/user story u>,
    facing <concern c>
    we decided for <option o> and neglected <other options>,
    to achieve <system qualities/desired consequences>,
    accepting <downside/undesired consequences>,
    because <reason for accepting downside>.

NOTE: As the whole c-xrefactory project is a hobby project where I'm
constantly trying out and learning things, this is also a WIP.

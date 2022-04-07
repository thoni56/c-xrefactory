# Architecture Decision Records

In this directory we will store ADRs to document "architecture"-like
decisions. There are some good documentation and ideas about ADRs
[here|https://github.com/joelparkerhenderson/architecture_decision_record]
and [here|https://adr.github.io/].

We have not picked a template yet (and might never do) so write what
seems relevant.

The ADRs should be Markdown files given a name after the following
pattern:

    <nnn>-<state>-<name>.md

- <nnn> - sequential number
- <state> - One letter describing Pending, Accepted, Implemented, Revoked, Overriden
- <name> should be a a present tense imperative verb phrase,
  e.g. "choose_database" or "manage_password" because this will match
  what the reader want to know when coming here

The ADR should at least contain a
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

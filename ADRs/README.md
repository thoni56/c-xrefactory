# Architecture Decision Records

In this directory we will store ADRs to document "architecture"-like
decisions. There are some good documentation and ideas about ADRs
[here|https://github.com/joelparkerhenderson/architecture_decision_record]
and [here|https://adr.github.io/].

We have not picked a template yet (and might never do) so write what
seems relevant.

The ADRs should be Markdown files given a name after the following
pattern:

    <nnn>-<decision-date>-<state>-<name>.md

- <nnn> - sequential number
- <decision-date> - date the decision was taken, i.e the date from
  which that decision should be followed
- <state> - One letter describing Pending, Current, Revoked, Overriden
- <name> should be a a present tense imperative verb phrase,
  e.g. "choose_database" or "manage_password" because this will match
  what the reader want to do when coming here
  
The ADR should at least contain a
[Y-statement|https://www.infoq.com/articles/sustainable-architectural-design-decisions/]
following this pattern:

    In the context of <use case/user story u>,
    facing <concern c>
    we decided for <option o> and neglected <other options>, 
    to achieve <system qualities/desired consequences>, 
    accepting <downside d/undesired consequences>, 
    because <additional rationale>.
  
NOTE: As the whole c-xrefactory project is a hobby project where I'm
constantly learning things, this is also a WIP.
    

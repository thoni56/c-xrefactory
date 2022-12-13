# Remove HTML generation

Date: 2021-09-05

## Status:

Accepted

## Deciders

Thomas Nilefalk

## Should we keep the HTML generation?

The HTML generation is one of multiple functions of C-xrefactory. It
is not at the core of what we want to support and develop going
forward.

    
### Decision Drivers

Primary driver is simplicity. Removing the functionality will remove a
radical amount of code as well as some complications in the code.

### Considered Options

- Keep, and improve the code
- Remove

### Decision Outcome

Remove the functionality completely.

### Decision Implications

C-xrefactory will no longer be able to generate HTML presentation, but
there are other tools to do that.

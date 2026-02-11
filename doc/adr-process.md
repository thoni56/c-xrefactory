# Architecture Decision Records (ADRs) - Process Guide

The `adr/` directory contains Architecture Decision Records (ADRs) for c-xrefactory. ADRs document significant architectural and design decisions made during the project's evolution.

**Note**: This guide is kept outside the `adr/` directory because Structurizr parses all `.md` files in that directory as ADRs.

## What is an ADR?

An Architecture Decision Record captures an important architectural decision made along with its context and consequences. Each ADR describes:

- **Context**: The issue motivating the decision
- **Decision**: The change we're proposing or have agreed to implement
- **Consequences**: What becomes easier or more difficult, and any risks
- **Alternatives**: Other options considered and why they were rejected

## When to Write an ADR

Create an ADR for decisions that:

- Affect the overall architecture or system structure
- Have significant consequences or trade-offs
- Remove or change major functionality
- Adopt new patterns, conventions, or technologies
- Accept limitations or constraints
- Are difficult to reverse later

Examples: removing features, changing parsing architecture, accepting performance trade-offs, adopting new design patterns.

**Don't** create ADRs for:
- Minor implementation details
- Routine bug fixes
- Small refactorings without architectural impact
- Decisions that can be easily reversed

## ADR Status Lifecycle

Each ADR has a status indicating its current state:

| Status | Meaning | Usage |
|--------|---------|-------|
| **Draft** | Exploratory, capturing ideas and direction | Early thinking, not yet a concrete proposal |
| **Proposed** | Under discussion, not yet decided | Decision documented, seeking feedback/approval |
| **Accepted** | Decision approved, implementation in progress or planned | Commitment made but work not complete |
| **Implemented** | Fully completed and deployed | Decision executed, feature/change live in codebase |
| **Rejected** | Proposed but decided against | Documents why something was NOT done (useful history) |
| **Superseded** | Replaced by a later ADR | Include reference to superseding ADR |
| **Deprecated** | No longer recommended but not formally replaced | Discouraged but not forbidden |

### Status Format

Use this format in the Status section:

```markdown
## Status

Implemented (completed YYYY-MM-DD)
```

Or for proposed/accepted:

```markdown
## Status

Proposed
```

The "Date:" field at the top of the ADR always reflects when the **decision** was made, not when implementation completed.

## Creating a New ADR

1. **Number it sequentially**: Find the highest numbered ADR and increment by 1
2. **Use the template**: Copy `templates/template.md`
3. **Name the file**: `NNNN-brief-description.md` (e.g., `0015-use-static-analysis.md`)
4. **Fill in all sections**: Don't leave sections empty - explain your thinking
5. **Document alternatives**: Show what options were considered and why rejected
6. **Start with "Proposed"**: Begin with status "Proposed" unless immediately implementing

Example:
```bash
cd doc/adr
cp templates/template.md 0015-my-decision.md
# Edit the file...
cd ..
git add adr/0015-my-decision.md
git commit -m "[adr] Add ADR-0015: My Decision"
```

## ADR References

- [Michael Nygard's original blog post](https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions) introducing ADRs
- [adr-tools](https://github.com/npryce/adr-tools) - Command-line tools for working with ADRs
- [ADR GitHub organization](https://adr.github.io/) - Collection of ADR resources

## Integration with Documentation

ADRs are referenced from:
- **Roadmap** (`docs/15-roadmap.adoc`) - Links to key architectural decisions
- **Proposed Refactorings** (`docs/16-proposed-refactorings.adoc`) - Detailed technical proposals
- **Structurizr** (`workspace.dsl`) - C4 model architectural views

When referencing an ADR from other documentation, use:
```markdown
See [ADR-0012: Remove Lexem Stream Caching](../adr/0012-remove-lexem-stream-caching.md)
```

Or in AsciiDoc:
```asciidoc
See link:../adr/0012-remove-lexem-stream-caching.md[ADR-0012: Remove Lexem Stream Caching]
```

---

**Note:** This directory is a living record of architectural decisions. As the project evolves, new ADRs will be added, and existing ones may be superseded or deprecated. The numbering scheme ensures chronological order and prevents conflicts.

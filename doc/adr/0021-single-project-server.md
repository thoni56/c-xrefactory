# Single-project server policy

Date: 2026-01-23

## Status

Implemented

## Problem Statement and Context

The original c-xrefactory architecture supported multiple projects in a single server session. A global `$HOME/.c-xrefrc` contained sections for all projects, and the server would search through them to find which project a given file belonged to. This multi-project design caused:

- **Table corruption**: The in-memory reference table could contain symbols from multiple projects, leading to incorrect cross-references and stale data.
- **Complex checkpoint system**: A 2-level memory checkpoint system was needed to efficiently switch between projects within a single session, caching expensive compiler discovery results per project.
- **Session confusion**: The browsing stack could hold references from different projects, making navigation unreliable when switching between files from different projects.

## Decision

The server handles one project at a time. Project identity is determined on the first `-getproject` request and the server locks to that project for its lifetime.

### How it works

1. Client sends `-getproject <file>` before any operation.
2. On first call, the server discovers the project by searching upward from the file's directory for a `.c-xrefrc` file (ADR-0005). The discovered project name and root directory become the locked project.
3. On subsequent `-getproject` calls, the server checks if the file belongs to the locked project. If not, it signals a project mismatch to the client.
4. Operations cannot run without a locked project.

The client's existing pattern of sending `-getproject` before each operation required no change. The server simply added locking behavior to the existing project discovery mechanism.

## Consequences and Risks

**Benefits:**

- Eliminates table corruption from project mixing
- Enables the "memory as truth" architecture where in-memory references are the single source of truth
- Makes index-based sessions safe (no cross-project symbol collisions)
- Simplifies the mental model: one server = one project
- Natural fit for LSP (one workspace = one language server)

**Risks:**

- Users with complex multi-project workflows need separate server instances per project. In practice, the Emacs client already manages this naturally since each project has its own `.c-xrefrc`.

## Related Decisions

- **ADR-0005**: Automatically find config files — enables project discovery by upward search
- **ADR-0014**: Adopt on-demand parsing architecture
- **Roadmap**: This decision is the foundation of the "Memory as Truth" dependency chain

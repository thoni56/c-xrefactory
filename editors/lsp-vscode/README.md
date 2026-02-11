# c-xrefactory LSP client for VS Code

Experimental LSP client for VS Code. Currently supports:

- `textDocument/definition` (Go to Definition)

## Prerequisites

- A built `c-xref` executable in `src/` (run `make -C src` first)
- A `.c-xrefrc` configuration file for your project, or you will be prompted to create one on first use
- Node.js (for installing the extension)

## Install

From the repository root:

```
cd editors/lsp-vscode
npm install
code --install-extension .
```

Or to test without installing, open VS Code and use "Run Extension" from the debug panel (F5) with this directory as workspace.

## Configuration

The extension looks for the `c-xref` executable relative to the extension install path. If you've installed c-xrefactory elsewhere, set the path in VS Code settings:

```json
{
    "c-xrefactory.executablePath": "/path/to/c-xref"
}
```

## Notes

- This is experimental and untested - please report issues
- Only a single LSP feature (Go to Definition) is implemented so far
- The server is started automatically when you open a C file

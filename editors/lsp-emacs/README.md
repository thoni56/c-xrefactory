# c-xrefactory LSP client for Emacs

Experimental LSP client using Emacs `lsp-mode`. Currently supports:

- `textDocument/definition` (Go to Definition)

## Quick test

```
emacs -q -l /path/to/c-xrefactory/editors/lsp-emacs/c-xrefactory-lsp.el myfile.c
```

This starts Emacs without your init file and loads only the c-xrefactory LSP configuration. The `lsp-mode` package will be installed automatically if missing.

## Adding to your Emacs configuration

Add to your `~/.emacs` or `~/.emacs.d/init.el`:

```elisp
(load "/path/to/c-xrefactory/editors/lsp-emacs/c-xrefactory-lsp.el")
```

## Prerequisites

- A built `c-xref` executable in `src/` (run `make -C src` first)
- A `.c-xrefrc` configuration file for your project, or you will be prompted to create one on first use

## Notes

- This is experimental and only a single LSP feature is implemented so far
- The server is started automatically when you open a C file
- `lsp-mode` is installed from MELPA if not already present

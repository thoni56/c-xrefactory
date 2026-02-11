;; c-xrefactory LSP client configuration for Emacs
;;
;; Requires: lsp-mode (installed automatically if missing)
;;
;; Usage: Add to your ~/.emacs or ~/.emacs.d/init.el:
;;   (load "/path/to/c-xrefactory/editors/lsp-emacs/c-xrefactory-lsp.el")
;;
;; Or for a quick test run:
;;   emacs -q -l /path/to/c-xrefactory/editors/lsp-emacs/c-xrefactory-lsp.el

(require 'package)
(setq package-archives '(("melpa" . "https://melpa.org/packages/")
                         ("gnu" . "https://elpa.gnu.org/packages/")))
(package-initialize)

(unless (package-installed-p 'lsp-mode)
  (package-refresh-contents)
  (package-install 'lsp-mode))

(require 'lsp-mode)

;; Find the c-xref executable relative to this file
(defvar c-xrefactory-lsp-exec
  (expand-file-name "../../src/c-xref" (file-name-directory load-file-name))
  "Path to the c-xref executable.")

(defun c-xrefactory-lsp-setup ()
  "Register c-xrefactory as an LSP server for C files."
  (add-to-list 'lsp-language-id-configuration '(c-mode . "c"))

  (lsp-register-client
   (make-lsp-client
    :new-connection (lsp-stdio-connection (list c-xrefactory-lsp-exec "-lsp"))
    :activation-fn (lsp-activate-on "c")
    :major-modes '(c-mode)
    :server-id 'c-xrefactory-lsp
    :priority 1))

  (add-hook 'c-mode-hook #'lsp))

(c-xrefactory-lsp-setup)

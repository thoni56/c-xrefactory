;; As we run this with --no-init-file we need to set up a rudimentary
;; package handling

(require 'package)
(setq package-user-dir "~/.emacs.d/elpa/")
(setq package-archives '(("melpa" . "https://melpa.org/packages/")
                         ("gnu" . "https://elpa.gnu.org/packages/")))
(package-initialize)

(unless (package-installed-p 'lsp-mode)
  (package-refresh-contents)
  (package-install 'lsp-mode))

(require 'lsp-mode)


(defun setup-lsp-server ()
  ;; Associate the major mode with the LSP language ID
  (add-to-list 'lsp-language-id-configuration '(c-mode . "c"))

  ;; Register the LSP server
  (lsp-register-client
   (make-lsp-client
    :new-connection (lsp-stdio-connection '("/home/thoni/Utveckling/c-xrefactory/src/c-xref" "-lsp" "-log=log" "-trace"))
    :major-modes '(c-mode)
    :server-id 'c-xrefactory-lsp))

  ;; Automatically enable lsp-mode for c-mode buffers
  (add-hook 'c-mode-hook #'lsp))


(setup-lsp-server)
(setq lsp-log-io t)

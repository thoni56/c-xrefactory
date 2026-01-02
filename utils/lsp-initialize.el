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

(setq c-xref-exec (expand-file-name "../src/c-xref" (file-name-directory load-file-name)))

(require 'lsp-mode)

(defun setup-lsp-server ()
  ;; Associate the major mode with the LSP language ID
  (add-to-list 'lsp-language-id-configuration '(c-mode . "c"))

  ;; Register the LSP server
  (lsp-register-client
   (make-lsp-client
    :new-connection (lsp-stdio-connection (list c-xref-exec "-lsp" "-log=log" "-trace"))
    :activation-fn (lsp-activate-on "c")
    :major-modes '(c-mode)
    :server-id 'c-xrefactory-lsp
    :priority 1))

  ;; Automatically enable lsp-mode for c-mode buffers
  (add-hook 'c-mode-hook #'lsp))


(setup-lsp-server)
(setq lsp-log-io t)

(defun my-lsp-mode-keybindings ()
  "Custom key bindings for LSP mode."
  (local-set-key (kbd "<f2>") 'lsp-rename)
  (local-set-key (kbd "<f3>") 'lsp-ui-find-previous-reference)
  (local-set-key (kbd "<f4>") 'lsp-ui-find-next-reference)
  (local-set-key (kbd "<f6>") 'lsp-find-definition)
  (local-set-key (kbd "<f8>") 'lsp-completion-at-point)
  (local-set-key (kbd "<f11>") 'lsp-execute-code-action))

(add-hook 'lsp-mode-hook 'my-lsp-mode-keybindings)

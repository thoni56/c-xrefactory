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
  ;; Define the major mode for c-xrefactory (if not already defined)
  (define-derived-mode c-xrefactory-mode fundamental-mode "C-Xrefactory"
    "A major mode for using the c-xrefactory LSP server.")

  ;; Associate the major mode with the LSP language ID
  (add-to-list 'lsp-language-id-configuration '(c-xrefactory-mode . "c-xrefactory"))

  ;; Register the LSP server
  (lsp-register-client
   (make-lsp-client
    :new-connection (lsp-stdio-connection '("/home/thoni/Utveckling/c-xrefactory/src/c-xref" "-lsp" "-log=log" "-trace"))
    :major-modes '(c-xrefactory-mode)
    :server-id 'c-xrefactory-lsp))

  ;; Automatically enable lsp-mode for c-xrefactory-mode buffers
  (add-hook 'c-xrefactory-mode-hook #'lsp))



(defun test-lsp-server-initialize ()
  (let ((log-buffer (get-buffer-create "*lsp-log*"))) ;; Create or get the log buffer
    ;; Enable LSP logging
    (setq lsp-log-io t)
    (with-current-buffer log-buffer
      (erase-buffer)) ;; Clear log buffer at the start
    (setq lsp--log-io-buffer log-buffer) ;; Explicitly assign the log buffer

    (message "Log buffer: %s" log-buffer)
    (message "Log buffer contents: %s" (with-current-buffer log-buffer (buffer-string)))

    ;; Create a temporary buffer and switch to fundamental-mode
    (with-temp-buffer
      (c-xrefactory-mode)  ;; Ensure the mode matches the client
      ;; Start `lsp-mode`
      (let ((default-directory "/tmp/"))  ;; **Set root directory, optional**
        (lsp))  ;; **Start the LSP client**
      (setq lsp-log-io t)

      ;; **Wait for server response**
      (with-current-buffer log-buffer
        (while (not (re-search-forward "initialize" nil t))
          (message "Polling log buffer... Current buffer contents:\n%s" (buffer-string))
          (sit-for 0.1)))  ;; **Polling for response**

      ;; **Check log for successful initialization**
      (if (re-search-forward "initialize" nil t)
          (progn
            (message "Test passed")
            (kill-emacs 0))
        (progn
          (message "Test failed: %s" (buffer-string))
          (kill-emacs 1))))))

(setup-lsp-server)
(setq lsp-log-io t)
;;(test-lsp-server-initialize)

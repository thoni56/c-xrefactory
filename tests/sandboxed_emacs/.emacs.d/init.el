;; Added by Package.el.  This must come before configurations of
;; installed packages.  Don't delete this line.  If you don't want it,
;; just comment it out by adding a semicolon to the start of the line.
;; You may delete these explanatory comments.
(package-initialize)

(when (eq system-type 'cygwin)
  ;; For native Cygwin Emacs-w32 we want to use login shells
  (setenv "PATH" (shell-command-to-string "source ~/.bash_profile; echo -n $PATH"))
  (setenv "PATH" (concat "/usr/bin" path-separator
                       (getenv "PATH")))
  (setq shell-file-name "bash")
  (setenv "SHELL" shell-file-name)
  (setq explicit-shell-file-name shell-file-name)

  (setq explicit-bash-args '("--login" "-i"))
  (setq exec-path (cons "/bin" exec-path))
  )

;;; El-get
(add-to-list 'load-path "~/.emacs.d/el-get/el-get")
(unless (require 'el-get nil 'noerror)
  (with-current-buffer
      (url-retrieve-synchronously
       "https://raw.githubusercontent.com/dimitri/el-get/master/el-get-install.el")
    (goto-char (point-max))
    (eval-print-last-sexp)))
(add-to-list 'el-get-recipe-path "~/.emacs.d/el-get-user/recipes")
(el-get 'sync)

;;; Add some Melpa archive
(require 'package)
(add-to-list 'package-archives '("melpa" . "http://melpa.org/packages/"))

;;; Formatting and indentation
(setq-default tab-width 4)
(setq c-basic-offset 4)
(setq c-tab-always-indent t)
(setq-default indent-tabs-mode nil)

(add-hook 'before-save-hook 'whitespace-cleanup)

;; Compilation keys
(global-set-key [f10] 'compile)
(global-set-key [f9] 'next-error)
(setq compilation-scroll-output t)


(global-set-key [kp-delete] 'delete-char)
(if (eq system-type 'darwin)
    (setq mac-option-modifier nil
      mac-command-modifier 'meta
      x-select-enable-clipboard t))

;; Colorize compilation buffer
(ignore-errors
  (require 'ansi-color)
  (defun my-colorize-compilation-buffer ()
    (when (eq major-mode 'compilation-mode)
      (ansi-color-apply-on-region compilation-filter-start (point-max))))
  (add-hook 'compilation-filter-hook 'my-colorize-compilation-buffer))

;; Extra Makefile-mode pattern
(setq auto-mode-alist
        (cons '("Makefile\\.*" . makefile-mode) auto-mode-alist))

;; Magit
(global-set-key (kbd "C-x g") 'magit-status)

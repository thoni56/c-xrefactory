;;; check-elisp.el --- byte-compile the elisp as a check  -*- lexical-binding: t; -*-
;; Byte-compile the elisp as a check.
;;
;; The compiled files are the point of `make compile' (used when installing),
;; not of this check: here we only want to know that everything still compiles,
;; so the .elc files are written to a temporary directory and thrown away.
;; Exits non-zero if any file fails to compile. Warnings do not fail the check.

(require 'bytecomp)  ; so byte-compile-dest-file-function is special before we bind it

(let* ((tmp (make-temp-file "cxref-elc" t))
       (byte-compile-dest-file-function
        (lambda (file)
          (expand-file-name (concat (file-name-nondirectory file) "c") tmp)))
       (ok t))
  (dolist (file '("c-xrefprotocol.el" "c-xrefdoc.el" "c-xrefactory.el" "c-xref.el"))
    ;; Load each file after compiling it, as compileCxrefactory.el does:
    ;; later files refer to definitions from earlier ones.
    (unless (byte-compile-file file t)
      (setq ok nil)))
  (delete-directory tmp t)
  (kill-emacs (if ok 0 1)))

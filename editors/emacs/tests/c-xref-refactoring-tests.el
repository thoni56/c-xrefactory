;;; c-xref-refactoring-tests.el --- tests of what a refactoring asks the server  -*- lexical-binding: t; -*-

;; Move Function lets the server ask whether to move the comments, so the
;; client does not say. The server is not called, the options it would get are
;; kept.

(require 'ert)
(require 'cl-lib)

(ert-deftest c-xref-move-function-leaves-the-comments-to-the-server ()
  (let ((target (make-temp-file "c-xref-test-target" nil ".c"))
        (options nil))
    (unwind-protect
        (with-current-buffer (find-file-noselect target)
          (c-xref-set-moving-target-position nil)
          (cl-letf (((symbol-function 'c-xref-do-refactoring)
                     (lambda (opts) (setq options opts))))
            (c-xref-moving '("-rfct-move-function")))
          (kill-buffer))
      (delete-file target))
    (should (equal (car options) "-rfct-move-function"))
    (should-not (cl-some (lambda (option) (string-prefix-p "-commentmovinglevel" option))
                         options))))

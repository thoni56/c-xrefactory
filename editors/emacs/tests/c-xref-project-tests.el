;;; c-xref-project-tests.el --- tests of the Project menu  -*- lexical-binding: t; -*-

(require 'ert)

(ert-deftest c-xref-project-new-in-a-buffer-without-a-file-starts-from-its-directory ()
  (let ((directory (file-name-as-directory (make-temp-file "c-xref-project" t)))
        (offered nil))
    (unwind-protect
        (with-temp-buffer
          (setq default-directory directory)
          (cl-letf (((symbol-function 'read-directory-name)
                     (lambda (_prompt dir &rest _) (setq offered dir) dir))
                    ((symbol-function 'read-string)
                     (lambda (_prompt initial &rest _) initial)))
            (c-xref-project-new))
          (should (equal offered directory)))
      (delete-directory directory t))))

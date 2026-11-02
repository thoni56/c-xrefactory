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

(ert-deftest c-xref-project-show-active-shows-the-root-path-short ()
  (let ((c-xref-active-project (expand-file-name "~/project"))
        (shown nil))
    (cl-letf (((symbol-function 'c-xref-entry-point-make-initialisations) #'ignore)
              ((symbol-function 'message)
               (lambda (format &rest arguments) (setq shown (apply #'format format arguments)))))
      (c-xref-project-active))
    (should (equal shown "Active project: ~/project"))))

(ert-deftest c-xref-project-mismatch-shows-the-locked-root-path-short ()
  (let ((asked nil))
    (cl-letf (((symbol-function 'c-xref-server-dispatch-read-record)
               (lambda (_ss i _len _tag) (cons (expand-file-name "~/project") i)))
              ((symbol-function 'c-xref-yes-or-no-window)
               (lambda (text &rest _) (setq asked text) nil)))
      (should-error (c-xref-server-dispatch-project-mismatch nil 0 0 nil)))
    (should (string-match-p "locked to project '~/project'" asked))))

(ert-deftest c-xref-project-new-asks-only-for-the-root ()
  (let* ((directory (file-name-as-directory (make-temp-file "c-xref-project" t)))
         (name (file-name-nondirectory (directory-file-name directory))))
    (unwind-protect
        (with-temp-buffer
          (setq default-directory directory)
          (cl-letf (((symbol-function 'read-directory-name)
                     (lambda (_prompt dir &rest _) dir))
                    ((symbol-function 'read-string)
                     (lambda (&rest _) (ert-fail "Project > New must not ask for a name"))))
            (c-xref-project-new))
          (with-temp-buffer
            (insert-file-contents (expand-file-name ".c-xrefrc" directory))
            (should (equal (buffer-string) (format "[%s]\n  .\n" name)))))
      (delete-directory directory t))))

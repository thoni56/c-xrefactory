;;; c-xref-upgrade-tests.el --- tests of upgrading the installation  -*- lexical-binding: t; -*-

;; After an upgrade the echo area says which version was installed. That is
;; the revision the build wrote into src/options_config.h, the same one the
;; server's About reports.

(require 'ert)

(ert-deftest c-xref-installed-version-is-the-built-revision ()
  (let ((c-xref-install-directory (make-temp-file "c-xref-install" t)))
    (unwind-protect
        (progn
          (make-directory (expand-file-name "src" c-xref-install-directory))
          (with-temp-file (expand-file-name "src/options_config.h" c-xref-install-directory)
            (insert "#define GIT_HASH \"1.10.0-3-g46498\"\n#define VERSION \"1.10.0\"\n"))
          (should (equal (c-xref-installed-version) "1.10.0-3-g46498")))
      (delete-directory c-xref-install-directory t))))

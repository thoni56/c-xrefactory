;;; c-xref-find-file-tests.el --- the server can send us to a file that doesn't exist yet  -*- lexical-binding: t; -*-

(require 'ert)

(ert-deftest c-xref-find-file-opens-a-file-that-does-not-exist-yet ()
  (let* ((directory (make-temp-file "c-xref-find-file" t))
         (file (expand-file-name "target.h" directory))
         (c-xref-run-find-file-hooks nil))
    (unwind-protect
        (save-window-excursion
          (c-xref-find-file file)
          (should (equal (buffer-file-name) file))
          (should (= (buffer-size) 0)))
      (when (get-file-buffer file) (kill-buffer (get-file-buffer file)))
      (delete-directory directory t))))

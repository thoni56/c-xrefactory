;;; c-xref-help-tests.el --- tests of the help buffer  -*- lexical-binding: t; -*-

(require 'ert)

(ert-deftest c-xref-help-links-to-the-user-manual ()
  (let ((browsed nil))
    (save-window-excursion
      (cl-letf (((symbol-function 'browse-url) (lambda (url &rest _) (setq browsed url))))
        (c-xref-help)
        (goto-char (point-min))
        (search-forward "user-manual.html")
        (push-button (1- (point)))))
    (should (equal browsed "https://thoni56.github.io/c-xrefactory/user-manual.html"))))

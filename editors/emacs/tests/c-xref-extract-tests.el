;;; c-xref-extract-tests.el --- tests of the extraction dialog  -*- lexical-binding: t; -*-

(require 'ert)

(ert-deftest c-xref-extract-variable-asks-for-the-declaration ()
  (let ((offered nil)
        (refactoring nil)
        (c-xref-renaming-default-name nil))
    (with-temp-buffer
      (insert "void f(void) {\n    int x, y, z;\n    int a = x+y+z;\n}\n")
      (goto-char (point-min))
      (search-forward "x+y+z")
      (set-mark (match-beginning 0))
      (cl-letf (((symbol-function 'read-from-minibuffer)
                 (lambda (_prompt &optional initial &rest _)
                   (push initial offered)
                   "long sum"))
                ((symbol-function 'c-xref-do-refactoring)
                 (lambda (arguments) (setq refactoring arguments))))
        (c-xref-extraction-dialog "newVariable_" "int newVariable_ = " ";" 3 "newVariable_"))
      (should (equal offered '("int newVariable_")))
      (should (string-match-p "long newVariable_ = x\\+y\\+z;" (buffer-string)))
      (should (equal refactoring '("-rfct-rename" "-renameto=sum"))))))

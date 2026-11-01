;;; c-xref-failed-extraction-tests.el --- an extraction the rename can't finish  -*- lexical-binding: t; -*-

(require 'ert)

(defconst c-xref-test-refusal
  (let ((text "No symbol found at the cursor.\n"))
    (format "<error len=%d>%s</error>" (length text) text)))

(defun c-xref-test-refuse-the-rename (_arguments)
  (c-xref-server-dispatch c-xref-test-refusal 0 (length c-xref-test-refusal)
                          (c-xref-get-basic-server-dispatch-data 'c-xref-server-process)))

(defmacro c-xref-test-with-marked-source (&rest body)
  `(let* ((file (make-temp-file "c-xref-extraction" nil ".c"
                                "void f(void) {\n    int x, y, z;\n    int a = x+y+z;\n}\n"))
          (source (find-file-noselect file))
          (c-xref-save-files-before-refactoring nil)
          (c-xref-multifile-undo-state nil)
          (c-xref-renaming-default-name "sum"))
     (unwind-protect
         (progn
           (delete-other-windows)
           (switch-to-buffer source)
           (goto-char (point-min))
           (search-forward "x+y+z")
           (set-mark (match-beginning 0))
           (c-xref-refactoring-init-actions "extraction")
           ,@body)
       (with-current-buffer source (set-buffer-modified-p nil))
       (kill-buffer source)
       (delete-file file))))

(defmacro c-xref-test-refused-extraction (&rest arguments)
  `(cl-letf (((symbol-function 'c-xref-do-refactoring) #'c-xref-test-refuse-the-rename)
             ((symbol-function 'c-xref-read-key-sequence)
              (lambda (&rest _) (ert-fail "the error dialog must not show"))))
     (should-error (c-xref-extraction-dialog ,@arguments))))

(ert-deftest c-xref-refused-extraction-leaves-the-code-as-it-was ()
  (c-xref-test-with-marked-source
   (let ((before (buffer-string))
         (position (point)))
     (c-xref-test-refused-extraction "newVariable_" "int newVariable_ = " ";" 3 "newVariable_")
     (with-current-buffer source
       (should (equal (buffer-string) before))
       (should (= (point) position)))
     (should (null c-xref-multifile-undo-state)))))

(ert-deftest c-xref-refused-variable-extraction-shows-what-to-do ()
  (c-xref-test-with-marked-source
   (c-xref-test-refused-extraction "newVariable_" "int newVariable_ = " ";" 3 "newVariable_")
   (should (eq (window-buffer) (get-buffer c-xref-info-buffer)))
   (let ((text (buffer-string)))
     (should (string-match-p "Extract Variable failed" text))
     (should (string-match-p "wrong place" text))
     (should (string-match-p "int sum = x\\+y\\+z;" text))
     (should (string-match-p "line 3" text))
     (should (string-match-p "int a = x\\+y\\+z;" text)))
   (goto-char (point-min))
   (search-forward "#_known_bugs")
   (should (button-at (1- (point))))
   (search-forward "/issues")
   (should (button-at (1- (point))))))

(ert-deftest c-xref-refused-function-extraction-does-not-blame-the-declaration ()
  (c-xref-test-with-marked-source
   (c-xref-test-refused-extraction "newFunction_();\n" "void newFunction_(void) {\n" "}\n" 1 "newFunction_")
   (let ((text (buffer-string)))
     (should (string-match-p "Extract Function failed" text))
     (should-not (string-match-p "wrong place" text)))))

(ert-deftest c-xref-failed-extraction-window-closes-on-q ()
  (c-xref-test-with-marked-source
   (c-xref-test-refused-extraction "newVariable_" "int newVariable_ = " ";" 3 "newVariable_")
   (execute-kbd-macro "q")
   (should (eq (window-buffer) source))
   (should (= (length (window-list)) 1))))

(ert-deftest c-xref-extraction-is-rolled-back-whatever-stopped-it ()
  (c-xref-test-with-marked-source
   (let ((before (buffer-string)))
     (cl-letf (((symbol-function 'c-xref-do-refactoring)
                (lambda (_arguments) (error "exiting"))))
       (should-error (c-xref-extraction-dialog "newVariable_" "int newVariable_ = " ";" 3 "newVariable_")))
     (with-current-buffer source
       (should (equal (buffer-string) before)))
     (should-not (equal (window-buffer) (get-buffer c-xref-info-buffer))))))

;;; c-xref-move-function-tests.el --- which windows a move leaves showing what  -*- lexical-binding: t; -*-

(require 'ert)

(defmacro c-xref-test-with-source-and-target (&rest body)
  `(let* ((source-file (make-temp-file "c-xref-source" nil ".c" "int moved(void) {\n}\n"))
          (target-file (make-temp-file "c-xref-target" nil ".c" "int zero(void) {\n}\n"))
          (source (find-file-noselect source-file))
          (target (find-file-noselect target-file))
          (c-xref-save-files-before-refactoring nil)
          (c-xref-save-files-after-refactoring nil)
          (c-xref-multifile-undo-state nil))
     (unwind-protect
         (save-window-excursion
           (delete-other-windows)
           (switch-to-buffer source)
           (goto-char (+ (point-min) 5))
           ,@body)
       (kill-buffer source)
       (kill-buffer target)
       (delete-file source-file)
       (delete-file target-file))))

(defmacro c-xref-test-move-to-target (&rest body)
  ;; The server's last goto takes the calling window to the moved function
  `(cl-letf (((symbol-function 'c-xref-moving)
              (lambda (_options)
                (switch-to-buffer target)
                (goto-char (point-max)))))
     (c-xref-move-function nil)
     ,@body))

(ert-deftest c-xref-move-keeps-the-source-beside-a-target-already-shown ()
  (c-xref-test-with-source-and-target
   (let* ((caller (selected-window))
          (other (split-window)))
     (set-window-buffer other target)
     (c-xref-test-move-to-target
      (should (eq (window-buffer caller) source))
      (should (eq (window-buffer other) target))
      (should (eq (selected-window) other))
      (should (= (point) (with-current-buffer target (point-max))))))))

(ert-deftest c-xref-move-without-the-target-shown-goes-to-it ()
  (c-xref-test-with-source-and-target
   (c-xref-test-move-to-target
    (should (= (length (window-list)) 1))
    (should (eq (window-buffer) target)))))

;;; c-xref-move-function-tests.el --- which windows a move leaves showing what  -*- lexical-binding: t; -*-

(require 'ert)
(require 'ert-x)

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

(defmacro c-xref-test-move-creating-header (&rest body)
  ;; The server sends us to the header, which isn't on disk, and fills it
  `(let ((header (concat (file-name-sans-extension target-file) ".h")))
     (unwind-protect
         (cl-letf (((symbol-function 'c-xref-moving)
                    (lambda (_options)
                      (c-xref-find-file header)
                      (insert "#ifndef TARGET_H\n")
                      (switch-to-buffer target))))
           (ert-with-message-capture messages
             (c-xref-move-function nil)
             ,@body))
       (when (get-file-buffer header)
         (with-current-buffer (get-file-buffer header) (set-buffer-modified-p nil))
         (kill-buffer (get-file-buffer header)))
       (when (file-exists-p header) (delete-file header)))))

(ert-deftest c-xref-move-saves-a-new-file-and-says-so ()
  (c-xref-test-with-source-and-target
   (c-xref-test-move-creating-header
    (should (file-exists-p header))
    (should-not (buffer-modified-p (get-file-buffer header)))
    (should (string-match-p (format "Created and saved %s"
                                    (regexp-quote (file-name-nondirectory header)))
                            messages)))))

(ert-deftest c-xref-move-leaves-a-new-file-of-your-own-unsaved ()
  (c-xref-test-with-source-and-target
   (let* ((own-file (concat source-file ".mine"))
          (own (find-file-noselect own-file)))
     (with-current-buffer own (insert "mine"))
     (unwind-protect
         (c-xref-test-move-creating-header
          (should-not (file-exists-p own-file)))
       (with-current-buffer own (set-buffer-modified-p nil))
       (kill-buffer own)
       (when (file-exists-p own-file) (delete-file own-file))))))

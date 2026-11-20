;;; c-xref-preload-tests.el --- tests of what a request tells the server  -*- lexical-binding: t; -*-

;; The server holds the files whose buffers are modified only as long as
;; every request sends them as -preload. A request that leaves one out tells
;; the server that the buffer was closed, and a header the server had held is
;; then a change, for every CU that includes it.

(require 'ert)
(require 'cl-lib)

(defvar c-xref-test-sent nil)

(defun c-xref-test-capture-the-request (data _proc)
  (setq c-xref-test-sent data))

(defmacro c-xref-test-with-a-modified-header (&rest body)
  "Run BODY in a.c, with a buffer of h.h modified, and keep what is sent in `c-xref-test-sent'."
  (declare (indent 0))
  `(let* ((dir (make-temp-file "c-xref-preload-test" t))
          (c-file (expand-file-name "a.c" dir))
          (header (expand-file-name "h.h" dir))
          (c-xref-tmp-dir dir)
          (c-xref-test-sent nil))
     (write-region "#include \"h.h\"\n" nil c-file)
     (write-region "extern int shared;\n" nil header)
     (let ((header-buffer (find-file-noselect header))
           (c-buffer (find-file-noselect c-file)))
       (unwind-protect
           (progn
             (with-current-buffer header-buffer
               (goto-char (point-max))
               (insert "extern int other;\n"))
             (with-current-buffer c-buffer
               (cl-letf (((symbol-function 'c-xref-send-data-to-running-process)
                          #'c-xref-test-capture-the-request)
                         ((symbol-function 'c-xref-wait-until-task-sync) #'ignore)
                         ((symbol-function 'c-xref-server-read-answer-file-and-dispatch) #'ignore)
                         ((symbol-function 'c-xref-entry-point-make-initialisations-no-project-required)
                          #'ignore)
                         ((symbol-function 'c-xref-update-browser-if-displayed) #'ignore))
                 ,@body)))
         (dolist (buffer (list header-buffer c-buffer))
           (with-current-buffer buffer (set-buffer-modified-p nil))
           (kill-buffer buffer))
         (delete-directory dir t)))))

(ert-deftest c-xref-a-pop-sends-the-modified-buffers ()
  (c-xref-test-with-a-modified-header
    (c-xref-pop-and-return)
    (should (string-match-p "\"-preload\" \"[^\"]*/h\\.h\"" c-xref-test-sent))))

;; -about goes straight to the server, past the call that collects the buffers
(ert-deftest c-xref-about-sends-the-modified-buffers ()
  (c-xref-test-with-a-modified-header
    (c-xref-about nil)
    (should (string-match-p "\"-preload\" \"[^\"]*/h\\.h\"" c-xref-test-sent))))

;; Every request that needs a project asks for it first, in a request of its own
(ert-deftest c-xref-asking-for-the-project-sends-the-modified-buffers ()
  (c-xref-test-with-a-modified-header
    (c-xref-lock-project-for-file (buffer-file-name))
    (should (string-match-p "\"-preload\" \"[^\"]*/h\\.h\"" c-xref-test-sent))))

;;; c-xref-preload-tests.el ends here

;;; c-xref-diagnostics-tests.el --- tests of the diagnostics buffer, what did not parse  -*- lexical-binding: t; -*-

(require 'ert)

(ert-deftest c-xref-a-log-record-goes-to-the-diagnostics-buffer-without-a-window ()
  (let* ((text "Can't open file somefile\n")
         (answer (format "<log len=%d>%s</log>" (length text) text)))
    (when (get-buffer c-xref-diagnostics-buffer)
      (kill-buffer c-xref-diagnostics-buffer))
    (delete-other-windows)
    (switch-to-buffer (get-buffer-create "source.c"))
    (c-xref-server-dispatch answer 0 (length answer)
                            (list (cons 'caller-window (selected-window))))
    (should (equal (mapcar (lambda (w) (buffer-name (window-buffer w))) (window-list))
                   '("source.c")))
    (should (equal (with-current-buffer c-xref-diagnostics-buffer (buffer-string))
                   (concat c-xref-diagnostics-header text)))))

(defun c-xref-test-selected-file-and-line ()
  "The file and line shown at point in the selected window."
  (with-current-buffer (window-buffer (selected-window))
    (list buffer-file-name (line-number-at-pos (window-point)))))

(defun c-xref-test-log-records (&rest messages)
  "The server's answer carrying MESSAGES as log records."
  (mapconcat (lambda (message) (format "<log len=%d>%s</log>" (length message) message))
             messages ""))

(ert-deftest c-xref-next-error-visits-the-diagnostics-in-order ()
  (let* ((file (file-truename (make-temp-file "c-xref-diagnostics" nil ".c"
                                              "int a;\nint b c;\n/* open\n")))
         (answer (c-xref-test-log-records (format "%s:2: Syntax error on: c\n" file)
                                          (format "%s:3: unterminated comment\n" file))))
    (unwind-protect
        (progn
          (when (get-buffer c-xref-diagnostics-buffer)
            (kill-buffer c-xref-diagnostics-buffer))
          (delete-other-windows)
          (switch-to-buffer (get-buffer-create "source.c"))
          (c-xref-server-dispatch answer 0 (length answer)
                                  (list (cons 'caller-window (selected-window))))
          (next-error)
          (should (equal (c-xref-test-selected-file-and-line) (list file 2)))
          (next-error)
          (should (equal (c-xref-test-selected-file-and-line) (list file 3))))
      (when (get-file-buffer file)
        (kill-buffer (get-file-buffer file)))
      (delete-file file))))

(defun c-xref-test-last-message ()
  "The last line in *Messages*."
  (with-current-buffer "*Messages*"
    (save-excursion
      (goto-char (point-max))
      (skip-chars-backward "\n")
      (buffer-substring (line-beginning-position) (point)))))

(ert-deftest c-xref-an-answer-with-diagnostics-ends-with-a-notice ()
  (let ((answer (c-xref-test-log-records "source.c:2: Syntax error on: c\n"
                                         "source.c:3: unterminated comment\n")))
    (when (get-buffer c-xref-diagnostics-buffer)
      (kill-buffer c-xref-diagnostics-buffer))
    (message "before")
    (c-xref-server-dispatch answer 0 (length answer)
                            (list (cons 'caller-window (selected-window))))
    (should (equal (c-xref-test-last-message)
                   "2 new messages in *c-xref-diagnostics*"))))

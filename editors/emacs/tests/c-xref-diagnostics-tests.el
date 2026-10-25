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

(ert-deftest c-xref-the-mode-line-shows-the-total-of-the-diagnostics ()
  (let ((answer (c-xref-test-log-records "source.c:2: Syntax error on: c\n"
                                         "macro.h:1:15: note: expanded from macro 'M'\n"
                                         "source.c:3: unterminated comment\n")))
    (when (get-buffer c-xref-diagnostics-buffer)
      (kill-buffer c-xref-diagnostics-buffer))
    (message "before")
    (c-xref-server-dispatch answer 0 (length answer)
                            (list (cons 'caller-window (selected-window))))
    ;; format-mode-line gives "" in batch, there is no mode line to format
    (should (member '(:eval (c-xref-diagnostics-mode-line)) global-mode-string))
    (should (equal (substring-no-properties (c-xref-diagnostics-mode-line)) " C-xref:2"))
    (should (equal (c-xref-test-last-message) "before"))))

(defun c-xref-test-analysing-record (file)
  "The server's announcement that FILE is being analysed."
  (format "<analysing len=%d>%s</analysing>" (length file) file))

(ert-deftest c-xref-the-messages-of-a-unit-come-under-its-heading ()
  (let ((answer (concat (c-xref-test-analysing-record "/project/a.c")
                        (c-xref-test-log-records "/project/a.c:2:9: Syntax error on: c\n"))))
    (when (get-buffer c-xref-diagnostics-buffer)
      (kill-buffer c-xref-diagnostics-buffer))
    (c-xref-server-dispatch answer 0 (length answer)
                            (list (cons 'caller-window (selected-window))))
    (should (equal (with-current-buffer c-xref-diagnostics-buffer (buffer-string))
                   (concat c-xref-diagnostics-header
                           "Analysing /project/a.c\n"
                           "/project/a.c:2:9: Syntax error on: c\n")))))

(ert-deftest c-xref-a-unit-analysed-again-replaces-its-group ()
  (let ((answer (concat (c-xref-test-analysing-record "/project/a.c")
                        (c-xref-test-log-records "/project/a.c:2:9: Syntax error on: c\n")
                        (c-xref-test-analysing-record "/project/b.c")
                        (c-xref-test-log-records "/project/b.c:1:1: unterminated comment\n")
                        (c-xref-test-analysing-record "/project/a.c")
                        (c-xref-test-log-records "/project/a.c:3:1: Syntax error on: d\n"))))
    (when (get-buffer c-xref-diagnostics-buffer)
      (kill-buffer c-xref-diagnostics-buffer))
    (c-xref-server-dispatch answer 0 (length answer)
                            (list (cons 'caller-window (selected-window))))
    (should (equal (with-current-buffer c-xref-diagnostics-buffer (buffer-string))
                   (concat c-xref-diagnostics-header
                           "Analysing /project/b.c\n"
                           "/project/b.c:1:1: unterminated comment\n"
                           "Analysing /project/a.c\n"
                           "/project/a.c:3:1: Syntax error on: d\n")))))

(ert-deftest c-xref-a-unit-analysed-clean-leaves-no-group ()
  (let ((answer (concat (c-xref-test-analysing-record "/project/a.c")
                        (c-xref-test-log-records "/project/a.c:2:9: Syntax error on: c\n")
                        (c-xref-test-analysing-record "/project/a.c"))))
    (when (get-buffer c-xref-diagnostics-buffer)
      (kill-buffer c-xref-diagnostics-buffer))
    (c-xref-server-dispatch answer 0 (length answer)
                            (list (cons 'caller-window (selected-window))))
    (should (equal (with-current-buffer c-xref-diagnostics-buffer (buffer-string))
                   c-xref-diagnostics-header))))

(ert-deftest c-xref-the-mode-line-shows-nothing-without-diagnostics ()
  (let ((answer (concat (c-xref-test-analysing-record "/project/a.c")
                        (c-xref-test-log-records "/project/a.c:2:9: Syntax error on: c\n")
                        (c-xref-test-analysing-record "/project/a.c"))))
    (when (get-buffer c-xref-diagnostics-buffer)
      (kill-buffer c-xref-diagnostics-buffer))
    (c-xref-server-dispatch answer 0 (length answer)
                            (list (cons 'caller-window (selected-window))))
    (should-not (c-xref-diagnostics-mode-line))))

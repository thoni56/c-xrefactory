;;; c-xref-log-tests.el --- tests of the log of what did not parse  -*- lexical-binding: t; -*-

(require 'ert)

(ert-deftest c-xref-a-log-record-goes-to-the-log-buffer-without-a-window ()
  (let* ((text "Can't open file somefile\n")
         (answer (format "<log len=%d>%s</log>" (length text) text)))
    (when (get-buffer c-xref-log-view-buffer)
      (kill-buffer c-xref-log-view-buffer))
    (delete-other-windows)
    (switch-to-buffer (get-buffer-create "source.c"))
    (c-xref-server-dispatch answer 0 (length answer)
                            (list (cons 'caller-window (selected-window))))
    (should (equal (mapcar (lambda (w) (buffer-name (window-buffer w))) (window-list))
                   '("source.c")))
    (should (equal (with-current-buffer c-xref-log-view-buffer (buffer-string))
                   text))))

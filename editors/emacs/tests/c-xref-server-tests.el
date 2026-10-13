;;; c-xref-server-tests.el --- tests of starting the server  -*- lexical-binding: t; -*-

;; A server that has stopped is started again on the next request. That is
;; said in *Messages*, with how the old one ended, so a crash does not pass
;; unnoticed. The old server is a real process that has exited or was killed
;; by a signal; the new one is a cat, so the request has somewhere to go.

(require 'ert)
(require 'cl-lib)

(defvar c-xref-test-server nil)

(defun c-xref-test-killed-process (signal)
  "A process that was killed by SIGNAL."
  (let ((process (start-process "c-xref-test-killed" nil "sleep" "10")))
    (signal-process process signal)
    (while (process-live-p process)
      (accept-process-output process 0.05))
    process))

(defun c-xref-test-exited-process (status)
  "A process that has exited with STATUS."
  (let ((process (start-process "c-xref-test-exited" nil "sh" "-c" (format "exit %d" status))))
    (while (process-live-p process)
      (accept-process-output process 0.05))
    process))

(defun c-xref-test-restart-message (old-server)
  "What *Messages* says when a request finds OLD-SERVER stopped."
  (let ((c-xref-test-server (cons old-server (cons "" nil))))
    (cl-letf (((symbol-function 'c-xref-start-server-process)
               (lambda (_initopts _ofile proc _filter)
                 (set proc (cons (start-process "c-xref-test-cat" nil "cat") (cons "" nil))))))
      (with-current-buffer (messages-buffer)
        (let ((inhibit-read-only t)) (erase-buffer)))
      (unwind-protect
          (c-xref-send-data-to-running-process "-olcxsearchgoto1" 'c-xref-test-server)
        (delete-process (car c-xref-test-server)))
      (with-current-buffer (messages-buffer) (buffer-string)))))

(ert-deftest c-xref-restarting-a-killed-server-says-so ()
  (should (string-match-p "server had stopped (signal 15), starting a new one"
                          (c-xref-test-restart-message (c-xref-test-killed-process 15)))))

(ert-deftest c-xref-restarting-a-stopped-server-says-so ()
  (should (string-match-p "server had stopped (exit status 3), starting a new one"
                          (c-xref-test-restart-message (c-xref-test-exited-process 3)))))

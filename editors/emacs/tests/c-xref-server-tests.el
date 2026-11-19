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
          (c-xref-send-data-to-running-process "-search-goto=1" 'c-xref-test-server)
        (delete-process (car c-xref-test-server)))
      (with-current-buffer (messages-buffer) (buffer-string)))))

(ert-deftest c-xref-restarting-a-killed-server-says-so ()
  (should (string-match-p "server had stopped (signal 15), starting a new one"
                          (c-xref-test-restart-message (c-xref-test-killed-process 15)))))

(ert-deftest c-xref-restarting-a-stopped-server-says-so ()
  (should (string-match-p "server had stopped (exit status 3), starting a new one"
                          (c-xref-test-restart-message (c-xref-test-exited-process 3)))))

;; A server started the way the client starts one, but a sleep, so it has
;; something to kill
(defun c-xref-test-start-a-server ()
  "Start a server in `c-xref-test-server'."
  (let ((start-a-process (symbol-function 'start-process))
        (kill-emacs-hook nil))
    (cl-letf (((symbol-function 'start-process)
               (lambda (&rest _args)
                 (funcall start-a-process "c-xref-test-sleep" nil "sleep" "10"))))
      (c-xref-start-server-process '("-server") "ofile" 'c-xref-test-server #'ignore))))

(defun c-xref-test-kill-the-server ()
  "Kill the server in `c-xref-test-server', and wait until its sentinel has run."
  (let ((server (car c-xref-test-server)))
    (signal-process server 9)
    (while (process-live-p server)
      (accept-process-output server 0.05))
    (accept-process-output nil 0.05)))

;; The server can be gone before it is asked to stop, e.g. killed from a
;; shell. Stopping it then only forgets it, so an upgrade goes on to build.
(ert-deftest c-xref-stopping-a-killed-server-forgets-it ()
  (let ((c-xref-server-process (cons (c-xref-test-killed-process 9) (cons "" nil))))
    (c-xref-stop-server)
    (should (null c-xref-server-process))))

;; The search results live in the server. When it dies they are gone, so the
;; window showing them is closed then, not when a key in it is answered by a
;; new server that never searched.
(ert-deftest c-xref-a-server-that-dies-closes-the-search-results ()
  (let ((c-xref-test-server nil))
    (c-xref-test-start-a-server)
    (unwind-protect
        (progn
          (display-buffer (get-buffer-create c-xref-search-results-buffer))
          (c-xref-test-kill-the-server)
          (should-not (get-buffer c-xref-search-results-buffer)))
      (when (get-buffer c-xref-search-results-buffer)
        (kill-buffer c-xref-search-results-buffer)))))

;; The browser lives in the server too, so its windows also go when it dies
(ert-deftest c-xref-a-server-that-dies-closes-the-browser ()
  (let ((c-xref-test-server nil))
    (c-xref-test-start-a-server)
    (c-xref-set-this-frame-dispatch-data nil)
    (delete-other-windows)
    (switch-to-buffer (get-buffer-create "source.c"))
    (c-xref-create-browser-windows nil (list (cons 'caller-window (selected-window))))
    (c-xref-test-kill-the-server)
    (should (equal (mapcar (lambda (w) (buffer-name (window-buffer w))) (window-list))
                   '("source.c")))))

;; A browser the user has closed is not closed again when the server dies,
;; which would give back the windows from before it opened
(ert-deftest c-xref-a-server-that-dies-leaves-the-windows-after-a-closed-browser ()
  (let ((c-xref-test-server nil))
    (c-xref-test-start-a-server)
    (c-xref-set-this-frame-dispatch-data nil)
    (delete-other-windows)
    (switch-to-buffer (get-buffer-create "source.c"))
    (c-xref-create-browser-windows nil (list (cons 'caller-window (selected-window))))
    (with-current-buffer (window-buffer (c-xref-is-browser-window-displayed))
      (c-xref-browser-dialog-exit nil))
    (set-window-buffer (split-window-horizontally) (get-buffer-create "other.c"))
    (c-xref-test-kill-the-server)
    (should (equal (mapcar (lambda (w) (buffer-name (window-buffer w))) (window-list))
                   '("source.c" "other.c")))))

;; The user can give the server options of their own, e.g. a log
(ert-deftest c-xref-server-is-started-with-the-startup-options ()
  (let ((c-xref-server-startup-options '("-info" "-log=c-xref.log"))
        (c-xref-test-server nil)
        (arguments nil)
        (start-a-process (symbol-function 'start-process))
        (kill-emacs-hook nil))
    (cl-letf (((symbol-function 'start-process)
               (lambda (&rest args)
                 (setq arguments args)
                 (funcall start-a-process "c-xref-test-cat" nil "cat"))))
      (unwind-protect
          (c-xref-start-server-process '("-server") "ofile" 'c-xref-test-server #'ignore)
        (delete-process (car c-xref-test-server))))
    (should (equal (last arguments 3) '("-server" "-info" "-log=c-xref.log")))))

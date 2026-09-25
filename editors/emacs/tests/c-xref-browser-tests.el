;;; c-xref-browser-tests.el --- tests of the symbol browser's windows  -*- lexical-binding: t; -*-

;; These look at the window tree - which windows there are and what they
;; show - never at sizes, which depend on the frame.

(require 'ert)

(defun c-xref-test-window-buffers ()
  "The names of the buffers in the frame's windows, sorted, without the browser's buffer counters."
  (sort (mapcar (lambda (w)
                  (replace-regexp-in-string " ([0-9]+)$" "" (buffer-name (window-buffer w))))
                (window-list))
        #'string<))

(defun c-xref-test-split-frame ()
  "Show source.c, selected, above other.c, and return the dispatch data of a request from source.c."
  (c-xref-set-this-frame-dispatch-data nil)   ; no browser in this frame yet
  (delete-other-windows)
  (switch-to-buffer (get-buffer-create "source.c"))
  (set-window-buffer (split-window-vertically) (get-buffer-create "other.c"))
  (list (cons 'caller-window (selected-window))))

(ert-deftest c-xref-browser-opens-beside-the-callers-window-only ()
  (let ((dispatch-data (c-xref-test-split-frame)))
    (c-xref-create-browser-windows nil dispatch-data)
    (should (equal (c-xref-test-window-buffers)
                   '(" *references*" " *symbols*" "source.c")))
    (c-xref-close-resolution-dialog-windows dispatch-data)))

(ert-deftest c-xref-browser-gives-back-the-windows-it-replaced ()
  (let ((dispatch-data (c-xref-test-split-frame)))
    (c-xref-create-browser-windows nil dispatch-data)
    (c-xref-close-resolution-dialog-windows dispatch-data)
    (should (equal (c-xref-test-window-buffers) '("other.c" "source.c")))))

(ert-deftest c-xref-resolution-dialog-keeps-its-message-window ()
  ;; The resolution dialog lays out the frame itself - the caller's window
  ;; and a message window - before it asks for the browser windows.
  (let ((dispatch-data (c-xref-test-split-frame)))
    (delete-other-windows)
    (c-xref-display-and-set-new-dialog-window c-xref-browser-info-buffer nil t)
    (other-window 1)
    (c-xref-create-browser-windows t dispatch-data)
    (should (member c-xref-browser-info-buffer (c-xref-test-window-buffers)))
    (c-xref-close-resolution-dialog-windows dispatch-data)))

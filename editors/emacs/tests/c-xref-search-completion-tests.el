;;; c-xref-search-completion-tests.el --- tests of completing in the search prompt  -*- lexical-binding: t; -*-

;; Emacs calls the completion function in the minibuffer, which has no file.
;; A search is over the whole project, so it sends none.

(require 'ert)
(require 'cl-lib)

(defvar c-xref-test-sent nil)

(ert-deftest c-xref-completing-a-search-sends-no-file ()
  (let* ((dir (make-temp-file "c-xref-search-completion-test" t))
         (c-xref-tmp-dir dir)
         (c-xref-active-project dir)
         (c-xref-test-sent nil))
    (unwind-protect
        (cl-letf (((symbol-function 'c-xref-send-data-to-running-process)
                   (lambda (data _proc) (setq c-xref-test-sent data)))
                  ((symbol-function 'c-xref-wait-until-task-sync) #'ignore)
                  ((symbol-function 'c-xref-server-read-answer-file-and-dispatch) #'ignore))
          (with-selected-window (minibuffer-window)
            (with-temp-buffer
              (insert "sh")
              (c-xref-symbols-completionfun "sh" nil nil)))
          (should (string-match-p "\"-search=sh\\*\"" c-xref-test-sent))
          (should-not (string-match-p "\"nil\"\\|-cursor=" c-xref-test-sent)))
      (delete-directory dir t))))

;; A search that needs many files analysed asks first, also while completing
(ert-deftest c-xref-a-question-while-completing-is-shown-beside-the-minibuffer ()
  (let ((c-window (selected-window)))
    (cl-letf (((symbol-function 'minibuffer-selected-window) (lambda () c-window))
              ((symbol-function 'c-xref-modal-dialog-loop) #'ignore))
      (with-selected-window (minibuffer-window)
        (with-temp-buffer
          (let ((dispatch-data (c-xref-get-basic-server-dispatch-data 'c-xref-server-process)))
            (should (eq (cdr (assoc 'caller-window dispatch-data)) c-window))
            (c-xref-yes-or-no-window "Analyse all before search?" t dispatch-data)))))))

;;; c-xref-search-completion-tests.el ends here

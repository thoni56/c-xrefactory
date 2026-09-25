;;; c-xref-database-tests.el --- tests of removing the reference database  -*- lexical-binding: t; -*-

;; A real database file in a temporary project, so the command really deletes
;; it; only the server is stubbed. Two orderings matter, and each is observed by
;; asking whether the database still exists at the moment the stub is called:
;;
;;   the server is stopped while the database is still there - it writes the
;;   database on the way out, so deleting first would be undone;
;;
;;   the project is established again once the database is gone - browsing
;;   continuations never send -getproject themselves and would otherwise meet a
;;   server that has never been told which project this is.

(require 'ert)
(require 'cl-lib)

(defvar c-xref-test-database-at nil
  "Whether the database existed when each stub ran, as (stub . existed) pairs.")

(defun c-xref-test-make-project ()
  "A temporary project root holding a .c-xref/db, and return the root."
  (let ((root (make-temp-file "c-xref-test-project" t)))
    (make-directory (expand-file-name ".c-xref" root))
    (with-temp-file (expand-file-name ".c-xref/db" root) (insert "references"))
    root))

(defmacro c-xref-test-removing-references (root answer-yes &rest body)
  "Run BODY with the server stubbed, as a request from a file in ROOT.
The confirmation answers yes when ANSWER-YES is non-nil."
  (declare (indent 2))
  `(let ((c-xref-test-database-at nil)
         (database (expand-file-name ".c-xref/db" ,root)))
     (with-temp-buffer
       ;; A real buffer with a file name: cl-letf on the buffer-file-name
       ;; primitive needs a native-comp trampoline a batch run cannot build.
       (setq buffer-file-name (expand-file-name "source.c" ,root))
       (cl-letf (((symbol-function 'c-xref-entry-point-make-initialisations)
                  (lambda () nil))
                 ((symbol-function 'c-xref-get-env) (lambda (_name) ,root))
                 ((symbol-function 'c-xref-stop-server)
                  (lambda () (push (cons 'stopped (file-exists-p database))
                                   c-xref-test-database-at)))
                 ((symbol-function 'c-xref-lock-project-for-file)
                  (lambda (file) (push (list 'locked (file-exists-p database) file)
                                       c-xref-test-database-at)))
                 ((symbol-function 'y-or-n-p) (lambda (_prompt) ,answer-yes)))
         ,@body))))

(ert-deftest c-xref-remove-references-stops-the-server-before-deleting ()
  (let ((root (c-xref-test-make-project)))
    (c-xref-test-removing-references root t
      (c-xref-project-remove-references-and-restart)
      (should (equal (assq 'stopped c-xref-test-database-at) '(stopped . t)))
      (should-not (file-exists-p database)))))

(ert-deftest c-xref-remove-references-establishes-the-project-afterwards ()
  (let ((root (c-xref-test-make-project)))
    (c-xref-test-removing-references root t
      (c-xref-project-remove-references-and-restart)
      ;; Gone by then, and with the file the project root was derived from.
      (should (equal (assq 'locked c-xref-test-database-at)
                     (list 'locked nil (expand-file-name "source.c" root)))))))

(ert-deftest c-xref-remove-references-does-nothing-when-declined ()
  (let ((root (c-xref-test-make-project)))
    (c-xref-test-removing-references root nil
      (c-xref-project-remove-references-and-restart)
      (should (file-exists-p database))
      (should-not c-xref-test-database-at))))

(ert-deftest c-xref-remove-references-says-the-browser-stack-goes ()
  ;; The stack lived in the server about to be stopped, so the prompt has to say
  ;; so before the answer rather than the message saying it after.
  (let ((root (c-xref-test-make-project))
        (asked nil))
    (c-xref-test-removing-references root nil
      (cl-letf (((symbol-function 'y-or-n-p)
                 (lambda (prompt) (setq asked prompt) nil)))
        (c-xref-project-remove-references-and-restart)))
    (should (string-match-p "browser stack" asked))))

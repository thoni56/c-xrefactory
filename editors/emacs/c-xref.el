;;; c-xref.el - (X)Emacs interface to C-xrefactory (autoloaded functions)

;; Copyright (C) 1997-2004 Marian Vittek, Xref-Tech
;; Put under GPL in 2009 by Marián Vittek
;; Work resurrected and continued by Thomas Nilefalk 2015-
;; (https://github.com/thoni56/c-xrefactory)

;; This file is part of C-xrefactory software; it implements an
;; interface between c-xref task and (X)Emacs editors.  You can
;; distribute it under the terms of the GNU General Public License
;; version 2 as published by the Free Software Foundation.  You should
;; have received a copy of the GNU General Public License along with
;; this program; if not, write to the Free Software Foundation, Inc.,
;; 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  The
;; content of this file is copyrighted by Xref-Tech. This file does
;; not contain any code written by independent developers.

;; This file is autoloaded from c-xrefactory.el

(defvar c-xref-debug-mode nil)          ;; Set to t to debug communication
(defvar c-xref-debug-preserve-tmp-files nil)

;;(toggle-debug-on-error)

(defvar c-xref-tmp-dir nil "Temporary directory for c-xref.")
(defvar c-xref-slash nil "Directory separator for c-xref.")
(defvar c-xref-user-identification nil "User identification for c-xref.")
(defvar c-xref-run-batch-file nil "Batch file for c-xref.")
(defvar c-xref-find-file-on-mouse-delimit nil "Regex for file delimitation in c-xref.")
(defvar c-xref-path-separator nil "Path separator for c-xref.")

(if (eq c-xref-platform 'windows)
    (progn
      (setq c-xref-slash ?\\)
      (setq c-xref-tmp-dir (getenv "TEMP"))
      (setq c-xref-user-identification "user")
      (setq c-xref-run-batch-file (concat (getenv "TEMP") "/c-xrefrun.bat"))
      (setq c-xref-find-file-on-mouse-delimit "[^A-Za-z0-9_\\.~-]")
      (setq c-xref-path-separator ?\;)
      )
  ;; a linux/unix platform
  (progn
    (setq c-xref-slash ?/)
    (setq c-xref-tmp-dir "/tmp")
    (setq c-xref-user-identification (getenv "LOGNAME"))
    (setq c-xref-run-batch-file (format "%s/c-xref-%s-%d.sh" c-xref-tmp-dir c-xref-user-identification (emacs-pid)))
    (setq c-xref-find-file-on-mouse-delimit "[^A-Za-z0-9_/.~-]")
    (setq c-xref-path-separator ?:)
    )
  )

(defvar c-xref-running-under 'emacs)

(defvar c-xref-directory-dep-prj-name "++ Automatic (directory dependent) ++")
(defvar c-xref-abandon-deletion "++ Cancel (no deletion) ++")
(defvar c-xref-c-suffixes '(".c" ".h" ".tc" ".th"))

(defvar c-xref-run-this-option "runthis")
(defvar c-xref-run-option "run")

(random t)
(defvar c-xref-server-tmp-file-counter (random 1000))
(defvar c-xref-server-tmp-file-increment (1+ (random 10)))
(defvar c-xref-ide-last-compile-commands nil)
(defvar c-xref-ide-last-run-commands nil)


(defun c-xref-server-get-new-tmp-file-name ()
  (let ((res))
    (setq res (format "%s/c-xref%s-%d-%d.tmp" c-xref-tmp-dir c-xref-user-identification (emacs-pid) c-xref-server-tmp-file-counter))
    (setq c-xref-server-tmp-file-counter (+ c-xref-server-tmp-file-counter c-xref-server-tmp-file-increment))
    res
    ))

(defun c-xref-modal-buffer-name (title)
  (let ((res))
    (setq res (format " c-xref %s (modal)" title))
    res
    ))


;; process descriptions are cons (process . (pending-output . synced-flag))
(defvar c-xref-server-process nil)
(defvar c-xref-refactorer-process nil)
(defvar c-xref-tags-process nil)

(defvar c-xref-server-tasks-ofile (c-xref-server-get-new-tmp-file-name))
(defvar c-xref-tags-tasks-ofile (format "%s/c-xref%s-%d.log" c-xref-tmp-dir c-xref-user-identification (emacs-pid)))

(defvar c-xref-ppc-synchro-record (format "<%s>" c-xref_PPC_SYNCHRO_RECORD))
(defvar c-xref-ppc-synchro-record-len (length c-xref-ppc-synchro-record))
(defvar c-xref-ppc-progress (format "<%s>" c-xref_PPC_PROGRESS))
(defvar c-xref-ppc-progress-len (length c-xref-ppc-progress))

(defvar c-xref-run-buffer-no-stars "run")
(defvar c-xref-run-buffer (format "*%s*" c-xref-run-buffer-no-stars))
(defvar c-xref-compilation-buffer "*compilation*")
(defvar c-xref-log-view-buffer " *c-xref-log*")
(defvar c-xref-server-answer-buffer "*c-xref-server-answer*")
(defvar c-xref-completions-buffer "*completions*")
(defvar c-xref-tag-results-buffer "*c-xref-search-results*")
(defvar c-xref-project-list-buffer " *project-list*")
(defvar c-xref-extraction-buffer " *code-extraction*")

(defvar c-xref-selection-modal-buffer (c-xref-modal-buffer-name " *selection"))
(defvar c-xref-confirmation-modal-buffer (c-xref-modal-buffer-name " *confirmation"))
(defvar c-xref-error-modal-buffer (c-xref-modal-buffer-name " *error"))
(defvar c-xref-info-modal-buffer (c-xref-modal-buffer-name " *info"))

(defvar c-xref-info-buffer " *info*")
(defvar c-xref-browser-info-buffer " *browser-info*")
(defvar c-xref-symbol-resolution-buffer " *symbols*")
(defvar c-xref-references-buffer " *references*")

(defvar c-xref-completions-windows-counter 0)
(defvar c-xref-completions-dispatch-data nil)
(defvar c-xref-refactorer-dispatch-data nil)
(defvar c-xref-global-dispatch-data nil)
(defvar c-xref-active-project nil)

(defvar c-xref-refactoring-block "")

(defvar c-xref-refactoring-beginning-marker (make-marker))
(defvar c-xref-refactoring-beginning-offset 0)
(defvar c-xref-moving-refactoring-marker (make-marker))
(defvar c-xref-moving-refactoring-line 0)
(defvar c-xref-undo-marker (make-marker))
(defvar c-xref-extraction-marker (make-marker))
(defvar c-xref-extraction-marker2 (make-marker))
(defvar c-xref-completion-marker (make-marker))
(defvar c-xref-completion-pop-marker (make-marker))
(defvar c-xref-completion-id-before-point "")
(defvar c-xref-completion-id-after-point "")
(defvar c-xref-completion-auto-search-list nil)
;; this is used only in automated test suites
(defvar c-xref-renaming-default-name nil)


(defvar c-xref-resolution-dialog-explication "\n")

(defvar c-xref-standard-help-message "Type ? for help.")

(defvar c-xref-backward-pass-identifier-regexp "\\(\\`\\|[^A-Za-z0-9_$]\\)")
(defvar c-xref-forward-pass-identifier-regexp "\\(\\'\\|[^A-Za-z0-9_$]\\)")


(defvar c-xref-c-keywords-regexp
  "\\b\\(auto\\|break\\|c\\(ase\\|har\\|on\\(st\\|tinue\\)\\)\\|d\\(efault\\|o\\(uble\\)?\\)\\|e\\(lse\\|num\\|xtern\\)\\|f\\(loat\\|or\\)\\|goto\\|i\\(f\\|nt\\)\\|long\\|re\\(gister\\|turn\\)\\|s\\(hort\\|i\\(gned\\|zeof\\)\\|t\\(atic\\|ruct\\)\\|witch\\)\\|typedef\\|un\\(ion\\|signed\\)\\|vo\\(id\\|latile\\)\\|while\\)\\b"
  )

(defun c-xref-keywords-regexp ()
  c-xref-c-keywords-regexp
  )

(defvar c-xref-font-lock-compl-keywords
  (cons (cons (c-xref-keywords-regexp) '(c-xref-keyword-face))
        '(("^\\([a-zA-Z0-9_ ]*\\):" c-xref-list-pilot-face))
        )
  "Default expressions to highlight in C-Xref completion list modes.")


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; refactorings codes, names and functions

(eval-and-compile ;; To make it update on re-load, e.g. upgrade
  (defvar c-xref-refactorings-assoc-list
    (list
     (list c-xref_PPC_AVR_NO_REFACTORING "No refactoring" 'c-xref-no-refactoring nil)
     (list c-xref_PPC_AVR_RENAME_SYMBOL "Rename Symbol" 'c-xref-rename-symbol nil)
     (list c-xref_PPC_AVR_RENAME_MODULE "Rename Module" 'c-xref-rename-module nil)
     (list c-xref_PPC_AVR_RENAME_INCLUDED_FILE "Rename Included File" 'c-xref-rename-included-file nil)
     (list c-xref_PPC_AVR_ADD_PARAMETER "Add Parameter" 'c-xref-add-parameter nil)
     (list c-xref_PPC_AVR_DEL_PARAMETER "Delete Parameter" 'c-xref-del-parameter nil)
     (list c-xref_PPC_AVR_MOVE_PARAMETER "Move Parameter" 'c-xref-move-parameter nil)
     (list c-xref_PPC_AVR_EXTRACT_FUNCTION "Extract Function" 'c-xref-extract-function nil)
     (list c-xref_PPC_AVR_EXTRACT_MACRO "Extract Macro" 'c-xref-extract-macro nil)
     (list c-xref_PPC_AVR_EXTRACT_VARIABLE "Extract Variable" 'c-xref-extract-variable nil)
     (list c-xref_PPC_AVR_MOVE_FUNCTION "Move Function" 'c-xref-move-function nil)
     (list c-xref_PPC_AVR_ORGANIZE_INCLUDES "Organize Includes" 'c-xref-organize-includes nil)
     (list c-xref_PPC_AVR_SET_MOVE_TARGET "Set Target for Next Moving Refactoring" 'c-xref-set-moving-target-position nil)
     (list c-xref_PPC_AVR_UNDO "Undo Last Refactoring" 'c-xref-undo-last-refactoring nil)
     ))
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; buffer locals
;;

;;
(defvar c-xref-this-buffer-type 'source-file) ;or  'symbol-list 'reference-list 'completion 'tag-search-results
(make-variable-buffer-local 'c-xref-this-buffer-type)
(defvar c-xref-this-buffer-dispatch-data nil)
(make-variable-buffer-local 'c-xref-this-buffer-dispatch-data)
(defvar c-xref-this-buffer-filter-level c-xref-default-symbols-filter)
(make-variable-buffer-local 'c-xref-this-buffer-filter-level)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; frame locals
;;

(defun c-xref-get-this-frame-dispatch-data ()
  "Retrieve custom dispatch data set for the current frame in Emacs."
  (cdr (assoc 'c-xref-this-frame-dispatch-data (frame-parameters (selected-frame))))
  )

(defun c-xref-set-this-frame-dispatch-data (dispatch-data)
  (modify-frame-parameters (selected-frame)
                           `((c-xref-this-frame-dispatch-data . ,dispatch-data)))
  )


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-add-bindings-for-chars (keymap chars fun)
  (let ((i) (len))
    (setq i 0)
    (setq len (length chars))
    (while (< i len)
      (define-key keymap (substring chars i (+ i 1)) fun)
      (setq i (+ i 1))
      )
    ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                       C-Xrefactory local keymaps                        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar c-xref-query-replace-map (make-sparse-keymap "C-xref-query-replace"))
(define-key c-xref-query-replace-map "y" 'answer-yes)
(define-key c-xref-query-replace-map "Y" 'answer-yes)
(define-key c-xref-query-replace-map "n" 'answer-no)
(define-key c-xref-query-replace-map "N" 'answer-no)
(define-key c-xref-query-replace-map "\e" 'answer-no)
(define-key c-xref-query-replace-map "\C-]" 'answer-abort)
(define-key c-xref-query-replace-map "\C-g" 'answer-abort)
(define-key c-xref-query-replace-map "a" 'answer-all)
(define-key c-xref-query-replace-map "A" 'answer-all)
(define-key c-xref-query-replace-map "c" 'answer-confirmed)
(define-key c-xref-query-replace-map "C" 'answer-confirmed)

(defun c-xref-bind-default-button (map fun)
    (define-key map [mouse-2] fun)
    (if c-xref-bind-left-mouse-button
            (progn
              (define-key map [mouse-1] fun)
              )))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                         Completion local keymap

(defvar c-xref-completion-mode-map (make-keymap)
  "Keymap for c-xref buffer containing completions."
  )
(c-xref-add-bindings-for-chars c-xref-completion-mode-map
                                           " `[];',./-=\\~!@#%^&*()+|{}:\"<>?"
                                           'c-xref-completion-auto-switch)
(define-key c-xref-completion-mode-map " " 'c-xref-interactive-completion-goto)
(define-key c-xref-completion-mode-map [(backspace)] 'c-xref-completion-auto-search-back)
(define-key c-xref-completion-mode-map "\b" 'c-xref-completion-auto-search-back)
(define-key c-xref-completion-mode-map [(delete)] 'c-xref-completion-auto-search-back)
(define-key c-xref-completion-mode-map [iso-lefttab] 'c-xref-completion-auto-search-back) ; Emacs
(define-key c-xref-completion-mode-map "\C-w" 'c-xref-completion-auto-search-w)
(define-key c-xref-completion-mode-map [tab] 'c-xref-completion-auto-search-s)
(define-key c-xref-completion-mode-map "\t" 'c-xref-completion-auto-search-s)
(define-key c-xref-completion-mode-map [(shift left)] 'c-xref-scroll-right)
(define-key c-xref-completion-mode-map [(shift right)] 'c-xref-scroll-left)
(define-key c-xref-completion-mode-map "\?" 'c-xref-interactive-completion-help)
(define-key c-xref-completion-mode-map c-xref-escape-key-sequence 'c-xref-interactive-completion-escape)
(define-key c-xref-completion-mode-map "\C-g" 'c-xref-interactive-completion-close)
(define-key c-xref-completion-mode-map "\C-q" 'c-xref-interactive-completion-escape)
;; can't bind anything to Alt-xx because escape is bound to single key!
;;(define-key c-xref-completion-mode-map "\M-q" 'c-xref-interactive-completion-escape)
;;(define-key c-xref-completion-mode-map [(control return)] 'c-xref-interactive-completion-goto)
;;(define-key c-xref-completion-mode-map [(control space)] 'c-xref-interactive-completion-goto)
;;(define-key c-xref-completion-mode-map "\C- " 'c-xref-interactive-completion-goto)
(define-key c-xref-completion-mode-map "\C-m" 'c-xref-interactive-completion-select)
(define-key c-xref-completion-mode-map "\C-p" 'c-xref-interactive-completion-previous)
(define-key c-xref-completion-mode-map "\C-n" 'c-xref-interactive-completion-next)
(define-key c-xref-completion-mode-map [mouse-3] 'c-xref-compl-3bmenu)
(c-xref-bind-default-button c-xref-completion-mode-map 'c-xref-interactive-completion-mouse-select)
(c-xref-add-bindings-for-chars
 c-xref-completion-mode-map
 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$1234567890"
 'c-xref-completion-auto-search)
(c-xref-add-bindings-to-keymap c-xref-completion-mode-map)
(define-key c-xref-completion-mode-map "q" 'c-xref-interactive-completion-q)


;;;;;;;;;;;;;;;;;;;;; mouse3 menu ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar c-xref-compl-3bmenu (make-sparse-keymap "Completions Menu"))
(fset 'c-xref-compl-3bmenu (symbol-value 'c-xref-compl-3bmenu))
(define-key c-xref-compl-3bmenu [c-xref-compl-3bclose] '("Close" . c-xref-interactive-completion-close))
(define-key c-xref-compl-3bmenu [c-xref-compl-3bexit] '("Close and Return" . c-xref-interactive-completion-escape))
(define-key c-xref-compl-3bmenu [c-xref-compl-3bselect] '("Complete" . c-xref-interactive-completion-mouse-select))
(define-key c-xref-compl-3bmenu [c-xref-compl-3bnext] '("Forward" . c-xref-interactive-completion-mouse-next))
(define-key c-xref-compl-3bmenu [c-xref-compl-3bprev] '("Backward" . c-xref-interactive-completion-mouse-previous))
(define-key c-xref-compl-3bmenu [c-xref-compl-3binspect] '("Inspect Definition" . c-xref-interactive-completion-mouse-goto))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                      tag search results keymap

(defvar c-xref-tag-search-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\e" 'c-xref-interactive-tag-search-escape)
    (define-key map "q" 'c-xref-interactive-tag-search-escape)
    (define-key map "p" 'c-xref-interactive-tag-search-previous)
    (define-key map "n" 'c-xref-interactive-tag-search-next)
    ;;(define-key map [left] 'c-xref-scroll-right)
    ;;(define-key map [right] 'c-xref-scroll-left)
    (define-key map [(shift left)] 'c-xref-scroll-right)
    (define-key map [(shift right)] 'c-xref-scroll-left)
    (define-key map "\C-m" 'c-xref-interactive-tag-search-inspect)
    ;;(define-key map "\C-m" 'c-xref-interactive-tag-search-select)
    (define-key map " " 'c-xref-interactive-tag-search-inspect)
    (define-key map "?" 'c-xref-interactive-tag-search-help)
    (c-xref-bind-default-button map 'c-xref-interactive-tag-search-mouse-inspect)
    map)
  "Keymap for c-xref search results mode."
  )
(define-key c-xref-tag-search-mode-map [mouse-3] 'c-xref-tag-search-3bmenu)
(c-xref-add-bindings-to-keymap c-xref-tag-search-mode-map)


;;;;;;;;;;;;;;;;;;;;; mouse3 menu ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar c-xref-tag-search-3bmenu (make-sparse-keymap "Search Results Menu"))
(fset 'c-xref-tag-search-3bmenu (symbol-value 'c-xref-tag-search-3bmenu))
(define-key c-xref-tag-search-3bmenu [c-xref-ts-3bclose] '("Close Window" . c-xref-interactive-tag-search-escape))
(define-key c-xref-tag-search-3bmenu [c-xref-ts-3bnext] '("Forward" . c-xref-interactive-tag-search-mouse-next))
(define-key c-xref-tag-search-3bmenu [c-xref-ts-3bprev] '("Backward" . c-xref-interactive-tag-search-mouse-previous))
(define-key c-xref-tag-search-3bmenu [c-xref-ts-3bselect] '("Insert Symbol" . c-xref-interactive-tag-search-mouse-select))
(define-key c-xref-tag-search-3bmenu [c-xref-ts-3binspect] '("Inspect Symbol" . c-xref-interactive-tag-search-mouse-inspect))


(defun c-xref-add-basic-modal-keybindings (map)
  (define-key map [up] 'c-xref-modal-dialog-previous-line)
  (define-key map [down] 'c-xref-modal-dialog-next-line)
  (define-key map [kp-up] 'c-xref-modal-dialog-previous-line)
  (define-key map [kp-down] 'c-xref-modal-dialog-next-line)
  (define-key map c-xref-escape-key-sequence 'c-xref-modal-dialog-exit)
                                        ;  (define-key map "\C-g" 'c-xref-modal-dialog-exit)
  (define-key map "q" 'c-xref-modal-dialog-exit)
  (define-key map [f7] 'c-xref-modal-dialog-exit)
  (define-key map [newline] 'c-xref-modal-dialog-continue)
  (define-key map [return] 'c-xref-modal-dialog-continue)
  (define-key map "\C-m" 'c-xref-modal-dialog-continue)
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; HELPS ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defvar c-xref-context-help-buffer " *help*")
(defvar c-xref-help-map (make-sparse-keymap "C-xref refactorings"))
(define-key c-xref-help-map [up] 'c-xref-modal-dialog-previous-line-no-sentinel)
(define-key c-xref-help-map [down] 'c-xref-modal-dialog-next-line-no-sentinel)
(define-key c-xref-help-map [kp-up] 'c-xref-modal-dialog-previous-line-no-sentinel)
(define-key c-xref-help-map [kp-down] 'c-xref-modal-dialog-next-line-no-sentinel)
(define-key c-xref-help-map c-xref-escape-key-sequence 'c-xref-modal-dialog-continue)
(define-key c-xref-help-map [prior] 'c-xref-modal-dialog-page-up)
(define-key c-xref-help-map [next] 'c-xref-modal-dialog-page-down)
(define-key c-xref-help-map [kp-prior] 'c-xref-modal-dialog-page-up)
(define-key c-xref-help-map [kp-next] 'c-xref-modal-dialog-page-down)
(define-key c-xref-help-map "\C-g" 'c-xref-modal-dialog-exit)
(define-key c-xref-help-map "q" 'c-xref-modal-dialog-continue)
(define-key c-xref-help-map " " 'c-xref-modal-dialog-continue)
(define-key c-xref-help-map "?" 'c-xref-modal-dialog-continue)
(define-key c-xref-help-map [newline] 'c-xref-modal-dialog-continue)
(define-key c-xref-help-map [return] 'c-xref-modal-dialog-continue)
(define-key c-xref-help-map "\C-m" 'c-xref-modal-dialog-continue)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;             BROWSER            ;;;;;;;;;;;;;;;;;;;;


(defvar c-xref-browser-dialog-key-map (make-sparse-keymap "C-xref symbol-resolution"))
(define-key c-xref-browser-dialog-key-map [up] 'c-xref-browser-dialog-previous-line)
(define-key c-xref-browser-dialog-key-map [down] 'c-xref-browser-dialog-next-line)
(define-key c-xref-browser-dialog-key-map [kp-up] 'c-xref-browser-dialog-previous-line)
(define-key c-xref-browser-dialog-key-map [kp-down] 'c-xref-browser-dialog-next-line)
(define-key c-xref-browser-dialog-key-map " " 'c-xref-browser-dialog-select-one)
(define-key c-xref-browser-dialog-key-map "t" 'c-xref-browser-dialog-toggle)
(define-key c-xref-browser-dialog-key-map [insert] 'c-xref-browser-dialog-toggle)
(define-key c-xref-browser-dialog-key-map [kp-insert] 'c-xref-browser-dialog-toggle)
;; use t if nothing else works
(define-key c-xref-browser-dialog-key-map "a" 'c-xref-browser-dialog-select-all)
(define-key c-xref-browser-dialog-key-map "n" 'c-xref-browser-dialog-select-none)
(define-key c-xref-browser-dialog-key-map c-xref-escape-key-sequence 'c-xref-browser-dialog-exit)
(define-key c-xref-browser-dialog-key-map "q" 'c-xref-browser-dialog-break)
(define-key c-xref-browser-dialog-key-map [f3] 'c-xref-browser-dialog-previous-reference)
(define-key c-xref-browser-dialog-key-map [f4] 'c-xref-browser-dialog-next-reference)
(define-key c-xref-browser-dialog-key-map [f7] 'c-xref-browser-dialog-exit)
(define-key c-xref-browser-dialog-key-map "o" 'c-xref-browser-dialog-other-window)
(define-key c-xref-browser-dialog-key-map "0" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "1" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "2" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "3" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "4" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "5" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "6" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "7" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "8" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map "9" 'c-xref-interactive-browser-dialog-set-filter)
(define-key c-xref-browser-dialog-key-map [newline] 'c-xref-modal-dialog-continue)
(define-key c-xref-browser-dialog-key-map [return] 'c-xref-modal-dialog-continue)
(define-key c-xref-browser-dialog-key-map "\C-m" 'c-xref-modal-dialog-continue)
(define-key c-xref-browser-dialog-key-map [(shift left)] 'c-xref-modal-dialog-shift-left)
(define-key c-xref-browser-dialog-key-map [(shift right)] 'c-xref-modal-dialog-shift-right)
(define-key c-xref-browser-dialog-key-map [(control left)] 'c-xref-resize-left)
(define-key c-xref-browser-dialog-key-map [(control right)] 'c-xref-resize-right)
(define-key c-xref-browser-dialog-key-map [(control up)] 'c-xref-resize-up)
(define-key c-xref-browser-dialog-key-map [(control down)] 'c-xref-resize-down)
(define-key c-xref-browser-dialog-key-map "?" 'c-xref-interactive-browser-dialog-help)
(c-xref-bind-default-button c-xref-browser-dialog-key-map 'c-xref-modal-dialog-mouse-button1)

(if c-xref-bind-left-mouse-button
        (define-key c-xref-browser-dialog-key-map [(control mouse-1)] 'c-xref-modal-dialog-mouse-button2)
  )
(define-key c-xref-browser-dialog-key-map [(control mouse-2)] 'c-xref-modal-dialog-mouse-button2)
(define-key c-xref-browser-dialog-key-map [mouse-3] 'c-xref-browser-menu-3bmenu)



(defvar c-xref-browser-dialog-refs-key-map (copy-keymap c-xref-browser-dialog-key-map))
(if (eq c-xref-running-under 'emacs)
    (define-key c-xref-browser-dialog-refs-key-map [mouse-3] 'c-xref-browser-refs-3bmenu)
  )


;;;;;;;;;;;;;;;;;;;;; mouse3 menu ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar c-xref-browser-refs-3bmenu (make-sparse-keymap "Search Results Menu"))
(fset 'c-xref-browser-refs-3bmenu (symbol-value 'c-xref-browser-refs-3bmenu))
(define-key c-xref-browser-refs-3bmenu [c-xref-bm-3b-cont] '("Continue" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-refs-3bmenu [c-xref-bm-3b-close] '("Close" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-refs-3bmenu [c-xref-bm-3b-filt3] '("Filter 3" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-refs-3bmenu [c-xref-bm-3b-filt2] '("Filter 2" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-refs-3bmenu [c-xref-bm-3b-filt1] '("Filter 1" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-refs-3bmenu [c-xref-bm-3b-filt0] '("Filter 0" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-refs-3bmenu [c-xref-bm-3b-sel-one] '("Inspect" . c-xref-browser-3b-mouse-selected))

(defvar c-xref-browser-menu-3bmenu (make-sparse-keymap "Search Results Menu"))
(fset 'c-xref-browser-menu-3bmenu (symbol-value 'c-xref-browser-menu-3bmenu))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-cont] '("Continue" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-close] '("Close Window" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-filt2] '("Filter 2" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-filt1] '("Filter 1" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-filt0] '("Filter 0" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-sel-none] '("Unselect All" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-sel-all] '("Select All" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-toggle] '("Toggle Selection" . c-xref-browser-3b-mouse-selected))
(define-key c-xref-browser-menu-3bmenu [c-xref-bm-3b-sel-one] '("Select One and Inspect" . c-xref-browser-3b-mouse-selected))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;          !!! this function is executed at LOADING time !!!

(defun c-xref-remove-old-temporary-files ()
  "Check  if  there are  some  forgotten  temporary  files generated  by
C-xrefactory. This  may happen if c-xref  task was killed, or  a macro was
interrupted by C-g. If there are such files, delete them.
"
  (interactive "")
  (let ((fl) (flist) (ff) (ffl) (modif) (ctime) (ctimecar) (loop) (modifcar))
    (setq ctimecar (car (current-time)))
    (setq flist (directory-files
                         c-xref-tmp-dir t
                         (format "c-xref%s.*\\.tmp" c-xref-user-identification)
                         t))
    (setq loop t)
    (setq fl flist)
    (while (and fl loop)
      (setq ff (car fl))
      (setq modifcar (car (nth 5 (file-attributes ff))))
      (if (> (- ctimecar modifcar) 2)
              (setq loop nil)
            )
      (setq fl (cdr fl))
      )
    (if (not loop)
            (progn
              (if (y-or-n-p "[c-xref] there are some old temporary files, can I delete them ")
                  (progn
                        (setq fl flist)
                        (while fl
                          (setq ff (car fl))
                          (setq modifcar (car (nth 5 (file-attributes ff))))
                          (if (> (- ctimecar modifcar) 1)
                              (delete-file ff)
                            )
                          (setq fl (cdr fl))
                          )
                        ))))
    (message "")
    ))

;;
;; Only clean up old temp files in interactive sessions, not during byte-compilation
(unless noninteractive
  (c-xref-remove-old-temporary-files))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-help-highlight-expr (expr)
  (c-xref-fontify-region (point-min) (point-max) expr)
  ;;  (let ((pos) (mb) (me))
  ;;    (goto-char (point-min))
  ;;    (while (search-forward-regexp expr nil t)
  ;;    (progn
  ;;            (setq mb (match-beginning 0))
  ;;            (setq me (match-end 0))
  ;;            (put-text-property mb me 'face 'bold)
  ;;            ))
  ;;    )
  )

(defun c-xref-interactive-help (text search highlight)
  (let ((help))
    (save-window-excursion
      (setq help (substitute-command-keys text))
      (get-buffer-create c-xref-context-help-buffer)
      (set-buffer c-xref-context-help-buffer)
      (c-xref-erase-buffer)
      (insert help)
      (goto-char (point-min))
      (if highlight (c-xref-help-highlight-expr highlight))
      (set-window-buffer (selected-window) (current-buffer))
      (c-xref-appropriate-window-height t nil)
      (c-xref-appropriate-window-width t nil)
      (if search
              (search-forward search nil t)
            )
      (c-xref-modal-dialog-loop c-xref-help-map "")
      (kill-buffer c-xref-context-help-buffer)
      (set-window-buffer (selected-window) (current-buffer))
      )
    ))

(defun c-xref-interactive-completion-help (event)
  (interactive "i")
  (c-xref-interactive-help
   "Special hotkeys available:

\\[c-xref-interactive-completion-select] \t-- select completion
\\[c-xref-interactive-completion-goto] \t-- inspect symbol's definition
\\[c-xref-interactive-completion-escape] \t-- close completions and return to completion position
\\[c-xref-interactive-completion-close] \t-- close completions
\\[c-xref-pop-and-return] \t-- previous completions
\\[c-xref-re-push] \t-- next completions
`[];',./-=\\~!@#%^&*()+|{}:\"<>\t-- leave completion window, insert character
A-Za-z0-9.\t-- incremental search, insert character
\\[c-xref-completion-auto-search-w] \t-- incremental search insert word
\\[c-xref-completion-auto-search-s] \t-- incremental search next match
\\[c-xref-completion-auto-search-back] \t-- incremental search return to last match
\\[c-xref-interactive-completion-help] \t-- toggle this help page
" nil nil)
  )

(defun c-xref-interactive-tag-search-help (event)
  (interactive "i")
  (c-xref-interactive-help
   "Special hotkeys available:

\\[c-xref-interactive-tag-search-inspect] \t-- inspect symbol
\\[c-xref-interactive-tag-search-select] \t-- insert selected symbol
\\[c-xref-interactive-tag-search-escape] \t-- close window
\\[c-xref-pop-and-return] \t-- previous search results
\\[c-xref-re-push] \t-- next search results
\\[c-xref-scroll-left] \t-- scroll left
\\[c-xref-scroll-right] \t-- scroll right
\\[c-xref-interactive-tag-search-help] \t-- toggle this help page
" nil nil)
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Some file operations modified for c-xref

(defun c-xref-find-file (file)
  (let ((buff))
    (setq buff (get-file-buffer file))
    (if buff
            (switch-to-buffer buff)         ;; be conformant with find-file
      (if c-xref-run-find-file-hooks
              (find-file file)                  ;; full standard find-file
            (setq buff (create-file-buffer file))
            (switch-to-buffer buff)
            (insert-file-contents file t nil nil t)
            (after-find-file nil t t nil t)
            ))
    ))

(defun c-xref-make-buffer-writable ()
  (setq buffer-read-only nil)
  )

(defun c-xref-save-some-buffers (flag)
  (save-some-buffers flag)
  )

(defun c-xref-write-file (file)
  (let ((dir))
    (setq dir (file-name-directory file))
    (if (not (file-exists-p dir))
            (make-directory dir t)
      )
    (write-file file)
    ))

(defun c-xref-delete-file (file)
  (if (file-exists-p file) (delete-file file))
  )


(defun c-xref-move-directory (old-name new-name)
  ;; used only for Java rename package - but could be used for future move/rename C "module"
  (if (eq c-xref-platform 'windows)
      (progn
            ;; MS-Windows
            (shell-command (format "xcopy \"%s\" \"%s\" /E /Y /I /C /Q" old-name new-name))
            (if (yes-or-no-p "Files copied, delete original folder? ")
                (progn
                  (if (string-match "-nt" system-configuration)
                          (shell-command (format "rd /S /Q \"%s\"" old-name))
                        (shell-command (format "deltree /Y \"%s\"" old-name))
                        )
                  )))
    ;; unix
    (shell-command (format "mv \"%s\" \"%s\"" old-name new-name))
    )
  )


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-global-options ()
  (interactive "")
  (c-xref-soft-select-dispach-data-caller-window c-xref-this-buffer-dispatch-data)
  (customize-group 'c-xrefactory)
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; MISC ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-cut-too-long-questions (qq offset space)
  (let ((ww) (ll) (res))
    (setq res qq)
    (setq ww (- (frame-width) space))
    (setq ll (length qq))
    (if (> (+ ll 2) ww)
            (setq res (concat
                           (substring qq 0 offset)
                           "..."
                           (substring qq (+ (- ll ww ) offset 2))))
      )
    res
    ))

(defun c-xref-upcase-first-letter (name)
  (let ((res))
    (setq res name)
    (if (equal (length res) 1)
            (setq res (downcase res))
      (if (> (length res) 1)
              (setq res (concat (upcase (substring res 0 1)) (substring res 1)))
            ))
    res
    ))

(defun c-xref-downcase-first-letter (name)
  (let ((res))
    (setq res name)
    (if (equal (length res) 1)
            (setq res (downcase res))
      (if (> (length res) 1)
              (setq res (concat (downcase (substring res 0 1)) (substring res 1)))
            ))
    res
    ))

(defun c-xref-get-single-yes-no-event (cursor-in-echo prompt)
  (let ((key) (res nil) (ce))
    (if (y-or-n-p prompt)
            (setq res 'answer-yes)
      (setq res 'answer-no)
      )
    ;;  (setq ce cursor-in-echo-area)
    ;;  (if cursor-in-echo
    ;;          (setq cursor-in-echo-area t)
    ;;    (setq cursor-in-echo-area nil)
    ;;    )
    ;;  (while (eq res nil)
    ;;    (setq key (c-xref-read-key-sequence nil))
;;;;      (setq key (read-key-sequence-vector "")
    ;;    (setq res (lookup-key c-xref-query-replace-map key))
    ;;    (if (eq res nil)
    ;;        (message "Invalid key, please answer [y/n].")
    ;;          )
    ;;    )
    ;;  (setq cursor-in-echo-area ce)
    ;;  (if (eq res 'answer-abort)
    ;;          (error "quit")
    ;;    )
    res
    ))

(defun c-xref-current-column ()
  (let ((poin) (res))
    (setq poin (point))
    (beginning-of-line)
    (setq res (- poin (point)))
    (goto-char poin)
    res
    ))

(defun c-xref-is-this-buffer-created-by-xrefactory (name)
  (let ((res))
    (setq res nil)
    (if (or
             (equal name c-xref-log-view-buffer)
             (equal name c-xref-server-answer-buffer)
             (equal name c-xref-completions-buffer)
             (equal name c-xref-tag-results-buffer)
             (equal name c-xref-browser-info-buffer)
             (equal name c-xref-project-list-buffer)
             (equal name c-xref-info-buffer)
             (equal name c-xref-run-buffer)
             (equal name c-xref-compilation-buffer)
             (equal name c-xref-info-modal-buffer)
             (equal name c-xref-error-modal-buffer)
             (equal name c-xref-confirmation-modal-buffer)
             (equal name c-xref-selection-modal-buffer)
             (c-xref-string-has-prefix name c-xref-symbol-resolution-buffer nil)
             (c-xref-string-has-prefix name c-xref-references-buffer nil)
             ;; delete also help buffers
             (c-xref-string-has-prefix name "*Help" nil)
             )
            (setq res t)
      )
    ;;(message "checking %S --> %S" name res)
    res
    ))

(defun c-xref-delete-window ()
  "Remove an (C-xrefactory) window.

Remove a window. This function is like `delete-window' (C-x 0), but it
tries to delete C-xrefactory windows first.
"
  (interactive "")
  (let ((osw) (sw) (i) (loop) (deleted))
    (setq loop t)
    (setq deleted nil)
    (setq i 0)
    (setq osw (selected-window))
    (setq sw osw)
    ;; dangerous loop, securize it
    (while loop
      (if (c-xref-is-this-buffer-created-by-xrefactory (buffer-name))
              (progn
                (if (equal sw (selected-window))
                        (progn
                          (bury-buffer)
                          (delete-window)
                          (other-window -1)
                          )
                  (bury-buffer)
                  (delete-window)
                  (select-window sw)
                  )
                (setq loop nil) ; (not (equal sw (selected-window))))
                (setq deleted t)
                )
            (other-window 1)
            (setq loop (not (equal sw (selected-window))))
            )
      (setq i (+ i 1))
      (if (and loop (> i 50))
              (progn
                (setq loop nil)
                (message "[PROBLEM] A WINDOW DELETING LOOP?")
                ))
      )
    (if (and (not deleted) (not (one-window-p)))
            (progn
              (other-window -1)
              (delete-window)
              ))
    ))

(defun c-xref-exact-string-match (regexp str)
  (let ((res) (si) (ei) (rr))
    (setq rr (string-match (concat ">>>>>" regexp "<<<<<")
                                       (concat ">>>>>" str    "<<<<<")))
    (setq res (eq rr 0))
    res
    ))

(defun c-xref-write-tmp-buff (tmpFile from-p to-p coding-system)
  (write-region from-p to-p tmpFile nil 'no-message nil coding-system)
  )

(defun c-xref-get-identifier-after (poin)
  (let ((opoin) (res))
    (setq opoin (point))
    (goto-char poin)
    (search-forward-regexp c-xref-forward-pass-identifier-regexp)
    (if (not (eq (point) (point-min))) (backward-char))
    (setq res (buffer-substring poin (point)))
    (goto-char opoin)
    res
    ))

(defun c-xref-is-letter (cc)
  (let ((res))
    (setq res (or (and (>= cc ?a) (<= cc ?z))
                          (and (>= cc ?A) (<= cc ?Z))
                          (eq cc ?_) (eq cc ?\$)))
    res
    ))

(defun c-xref-get-identifier-on-point ()
  (let ((sym) (poin) (cc))
    (setq poin (point))
    (search-backward-regexp c-xref-backward-pass-identifier-regexp)
    (if (not (c-xref-is-letter (char-after (point)))) (forward-char 1))
    (setq sym (c-xref-get-identifier-after (point)))
    (goto-char poin)
    sym
    ))

(defun c-xref-get-identifier-before-point ()
  (let ((sym) (poin))
    (setq poin (point))
    (search-backward-regexp c-xref-backward-pass-identifier-regexp)
    (forward-char 1)
    (setq sym (buffer-substring (point) poin))
    (goto-char poin)
    sym
    ))

(defun c-xref-delete-pending-ident-after-completion ()
  (let ((p (point)))
    (search-forward-regexp c-xref-forward-pass-identifier-regexp)
    (if (not (eq (point) (+ (buffer-size) 1)))
            (progn
              (backward-char)
          ;; finally this does not seem very natural
                                        ;         (if (and c-xref-completion-inserts-parenthesis (eq (char-after (point)) ?\())
                                        ;             (progn
                                        ;                               (forward-char 1)
                                        ;                               (if (eq (char-after (point)) ?\))
                                        ;                                       (forward-char 1)
                                        ;                 )))
              ))
    (if (eq p (point))
            ()
      (delete-char (- p (point)) t)
      (if c-xref-completion-delete-pending-identifier
              (message "** Pending identifier killed (C-y restores it) **")
            ))
    ))

(defun c-xref-buffer-has-one-of-suffixes (bname suffixes)
  (let ((suf) (sl) (res))
    (setq suf (concat "." (c-xref-file-name-extension bname)))
    (setq sl suffixes)
    (while (and sl (not (equal suf (car sl))))
      (setq sl (cdr sl))
      )
    (if (not sl)
            (setq res nil)
      (setq res t)
      )
    res
    ))

(defun c-xref-string-has-prefix (str prefix case-unsens)
  (let ((res) (len) (pf))
    (setq res nil)
    (setq len (length prefix))
    (if (>= (length str) len)
            (progn
              (setq pf (substring str 0 len))
              (if (or (equal pf prefix)
                          (and case-unsens  (equal (downcase pf)
                                                               (downcase prefix))))
                  (setq res t)
            )))
    res
    ))

(defun c-xref-file-directory-name (fname)
  (let ((spos) (res) (len))
    (setq res nil)
    (setq len (length fname))
    (if (eq len 0)
            (setq res nil)
      (setq spos (- len 1))
      (while (and (> spos 0)
                          (not (eq (elt fname spos) ?/))
                          (not (eq (elt fname spos) ?\\)))
            (setq spos (- spos 1))
            )
      (if (> spos 0)
              (setq res (substring fname 0 spos))
            (setq res nil)
            )
      (if (and (eq c-xref-platform 'windows)
                   (eq spos 3)
                   (eq (elt fname (- spos 1)) ?\:))
              (setq res nil)
            ))
    res
    ))

(defun c-xref-file-name-extension (fname)
  (let ((spos) (res) (len))
    (setq res nil)
    (setq len (length fname))
    (if (eq len 0)
            (setq res nil)
      (setq spos (- len 1))
      (while (and (> spos 0)
                          (not (eq (elt fname spos) ?.)))
            (setq spos (- spos 1))
            )
      (if (> spos 0)
              (setq res (substring fname (+ spos 1) len))
            (setq res fname)
            ))
    res
    ))

(defun c-xref-file-last-name (fname)
  (let ((spos) (res) (len))
    (setq res nil)
    (setq len (length fname))
    (if (eq len 0)
            (setq res nil)
      (setq spos (- len 1))
      (while (and (> spos 0)
                          (not (eq (elt fname spos) ?/))
                          (not (eq (elt fname spos) ?\\)))
            (setq spos (- spos 1))
            )
      (if (> spos 0)
              (setq res (substring fname (+ spos 1) len))
            (setq res fname)
            ))
    res
    ))

(defun c-xref-file-name-without-suffix (fname)
  (let ((spos) (res) (len))
    (setq res nil)
    (setq len (length fname))
    (if (eq len 0)
            (setq res nil)
      (setq spos (- len 1))
      (while (and (> spos 0)
                          (not (eq (elt fname spos) ?.)))
            (setq spos (- spos 1))
            )
      (if (> spos 0)
              (setq res (substring fname 0 spos))
            (setq res fname)
            ))
    res
    ))
(defun c-xref-backslashify-name (fname)
  "Convert all directory separators in FNAME based on `c-xref-platform`."
  (if (or (eq c-xref-platform 'unix) (zerop (length fname)))
      fname
    (replace-regexp-in-string "[/\\]" (char-to-string c-xref-slash) fname)))

(defun c-xref-optionify-string (ss qt)
  (let ((res))
    (if (string-match " " ss)
            (setq res (format "%s%s%s" qt ss qt))
      (setq res ss)
      )
    res
    ))

(defun c-xref-cut-string-prefix (ss prefix case-unsens)
  (let ((res) (len))
    (setq res ss)
    (setq len (length prefix))
    (if (>= (length ss) len)
            (if (or (equal (substring ss 0 len) prefix)
                        (and case-unsens (equal (downcase (substring ss 0 len))
                                                            (downcase prefix))))
                (setq res (substring ss len))
              )
      )
    res
    ))


(defun c-xref-cut-string-suffix (ss suffix case-unsens)
  (let ((res) (len) (sslen))
    (setq res ss)
    (setq len (length suffix))
    (setq sslen (length ss))
    (if (>= sslen len)
            (if (or (equal (substring ss (- sslen len)) suffix)
                        (and case-unsens (equal (downcase (substring ss (- sslen len)))
                                                            (downcase suffix))))
                (setq res (substring ss 0 (- sslen len)))
              )
      )
    res
    ))


(defun c-xref-soft-delete-window (buffername)
  (let ((displayed))
    (setq displayed (get-buffer-window buffername nil))
    (if displayed (delete-window displayed))
    ))

(defun c-xref-switch-to-marker (marker)
  (switch-to-buffer (marker-buffer marker))
  (goto-char (marker-position marker))
  )

(defun c-xref-set-to-marker (marker)
  (set-buffer (marker-buffer marker))
  (goto-char (marker-position marker))
  )

(defun c-xref-delete-window-in-any-frame (buffername setwidth)
  (let ((displayed (get-buffer-window buffername t)))
    (if displayed
            (progn
              (if setwidth (set setwidth (window-width displayed)))
              (delete-window displayed)
              (kill-buffer buffername)
              )
      )
    )
  )


(defun c-xref-erase-buffer ()
  "Secure buffer erase. Do not erase anything from .c .y"
  (let ((bname) (suf))
    (setq bname (buffer-file-name nil))
    (if bname
            (setq suf (c-xref-file-name-extension bname))
      (setq suf "")
      )
    (if (or (equal suf "c") (equal suf "y"))
            (progn
              (error "[c-xref] internal error: Attempt to erase '%s'!" bname)
              (if (yes-or-no-p (format "![warning] Attempt to erase '%s'! Really erase? " (c-xref-file-last-name bname)))
                  (erase-buffer)
                )
              )
      (erase-buffer)
      )
    ))

(defun c-xref-use-local-map (map)
  (if (c-xref-is-this-buffer-created-by-xrefactory (buffer-name))
      (use-local-map map)
    (error "Attempt to redefine local keymap for non-C-xrefactory buffer %s" (buffer-name))
    )
  )


(defun c-xref-char-search (pos step char-code)
  (let ((res pos))
    (while (and (>= res 1) (< res (point-max))
                        (not (eq (char-after res) char-code)))
      (setq res (+ res step))
      )
    res
    ))

(defun c-xref-char-count (char str)
  (let ((res) (i) (len))
    (setq res 0)
    (setq i 0)
    (setq len (length str))
    (while (< i len)
      (if (eq (elt str i) char)
              (setq res (1+ res))
            )
      (setq i (1+ i))
      )
    res
    ))

(defun c-xref-references-counts (drefn refn)
  (let ((res) (dr) (r))
    (if (equal drefn 0)
            (setq dr "  -")
      (setq dr (format "%3d" drefn))
      )
    (if (equal refn 0)
            (setq r "-  ")
      (setq r (format "%-3d" refn))
      )
    (setq res (format "%s/%s" dr r))
    res
    ))

(defun c-xref-goto-line (n)
  "Go to line N in the buffer to avoid interactive `goto-line`"
  (goto-char (point-min))
  (forward-line (1- n))
  )

(defun c-xref-show-file-line-in-caller-window (file line)
  (let ((sw))
    (setq sw (selected-window))
    (if (not (c-xref-soft-select-dispach-data-caller-window c-xref-this-buffer-dispatch-data))
            (other-window -1)
      )
    (find-file file)
    (if (not (equal line 0))
            (c-xref-goto-line line)
      )
    (sit-for 1)
    (select-window sw)
    ))

(defun c-xref-find-file-on-point ()
  (let ((b) (e) (p) (file) (line 0) (ne) (c1) (c2))
    (setq p (point))
    (search-backward-regexp c-xref-find-file-on-mouse-delimit (point-min) 0)
    (forward-char 1)
    (setq b (point))
    (if (and (eq c-xref-platform 'windows) (>= (point) 3))
            (if (c-xref-exact-string-match "[^A-Za-z][A-Za-z]:" (buffer-substring (- (point) 3) (point)))
                ;; add windows drive letter to file
                (setq b (- (point) 2))
              ))
    (search-forward-regexp c-xref-find-file-on-mouse-delimit (point-max) 0)
    (setq e (point))
    (if (<= (- e 1) b)
            (error "No file name on mouse")
      )
    (setq file (buffer-substring b (- e 1)))
    (if (or (eq (char-after (- e 1)) ?:)
                (eq (char-after (- e 1)) ?#)
                )
            (progn
              ;; probably a line number
              (forward-char 1)
              (search-forward-regexp "[^0-9]" (point-max) 0)
              (setq ne (point))
              (setq line (string-to-number (buffer-substring e (- ne 1))))
              )
      )
    (goto-char p)
    (if (not (file-attributes file))
        (error "Can't open %s" file)
      )
    (if (not (or (equal (substring file 0 1) "/")
                         (and (> (length file) 3) (equal (substring file 1 3) ":\\"))
                         (equal (substring file 0 1) "\\")))
            (progn
              ;; a relative path, concatenate with working dir
              (setq file (concat default-directory file))
              ))
    (c-xref-show-file-line-in-caller-window file line)
    )
  t
  )

(defun c-xref-find-file-on-mouse (event)
  (interactive "e")
  (let ((b) (e) (p) (file) (line 0) (ne) (c1) (c2))
    (mouse-set-point event)
    (c-xref-find-file-on-point)
    ))

(defun c-xref-scroll-left ()
  (interactive "")
  (scroll-left 1)
  (if (not (eq (char-after (point)) ?\n))
      (forward-char 1)
    )
  )

(defun c-xref-scroll-right ()
  (interactive "")
  (scroll-right 1)
  (if (and (> (point) 1) (not (eq (char-after (- (point) 1)) ?\n)))
      (backward-char 1)
    )
  )

(defun c-xref-set-window-width (width)
  (let ((sw) (rightmost-win) (enlarge) (res) (abort))
    (setq sw (selected-window))
    (setq res t)
    (setq enlarge (- width (window-width)))
    (if (>= width window-min-width)
            (progn
              (setq rightmost-win nil)
                  (setq rightmost-win (>= (elt (window-edges (selected-window)) 2) (frame-width)))
              (if rightmost-win
                  (progn
                        (if (> enlarge 0)
                            (progn
                              ;; (set-frame-width (selected-frame) (+ (frame-width) enlarge))
                              (setq res nil)
                              ))
                        )
                (setq abort nil)
                (if (and (> enlarge 0) (c-xref-select-righter-window))
                        (progn
                          (if (< (- (window-width) enlarge) window-min-width)
                              (setq abort (not (c-xref-set-window-width (+ window-min-width enlarge))))
                            )))
                (if abort
                        (setq res nil)
                  (select-window sw)
                  (enlarge-window-horizontally (- width (window-width)))
                  ))))
    res
    ))

;; appropriate window height by resizing upper window(s)
;;
(defun c-xref-set-window-height (height)
  (let ((wh) (sw) (nh) (diff))
    (setq sw (selected-window))
    (setq diff (- height (window-height)))
    (if (c-xref-select-upper-window)
            (progn
              (setq nh (- (window-height) diff))
              (if (<= nh window-min-height)
                  (c-xref-set-window-height (+ window-min-height diff))
                )
              (enlarge-window (- 0 diff))
              )
      (enlarge-window diff)
      )
    (select-window sw)
    ))

;; take care to not delete window on the right, do not resize in this case
(defun c-xref-resize-window-width (val)
  (let ((rightmost-win))
    (setq rightmost-win nil)
        (setq rightmost-win (>= (elt (window-edges (selected-window)) 2) (frame-width)))
    (if rightmost-win
            (progn
;;;                             (set-frame-width (selected-frame) (+ (frame-width) val))
              (if (>= (- (window-width) val) window-min-width)
                  (enlarge-window-horizontally (- 0 val))
                ))
      (if (>= (+ (window-width) val) window-min-width)
              (enlarge-window-horizontally val)
            ))
    ))

;; take care to not delete window on the right, do not resize in this case
(defun c-xref-resize-window-height (val)
  (if (< val 0)
      (if (> (window-height) window-min-height)
              (enlarge-window val)
            )
    ;; todo check the bottom window size!
    (enlarge-window val)
    )
  )

(defun c-xref-resize-left (event)
  (interactive "i")
  (c-xref-resize-window-width -1)
  )

(defun c-xref-resize-right (event)
  (interactive "i")
  (c-xref-resize-window-width 1)
  )

(defun c-xref-resize-up (event)
  (interactive "i")
  (c-xref-resize-window-height -1)
  )

(defun c-xref-resize-down (event)
  (interactive "i")
  (c-xref-resize-window-height 1)
  )

(defun c-xref-window-edges ()
  (let ((res))
    (if (eq c-xref-running-under 'emacs)
            (setq res (window-edges (selected-window)))
      (setq res (window-pixel-edges (selected-window)))
      )
    res
    ))

(defun c-xref-select-righter-window ()
  (let ((owe) (cwe) (loop) (sw) (res))
    (setq owe (c-xref-window-edges))
    (setq sw (selected-window))
    (setq loop t)
    (while loop
      (other-window 1)
      (setq cwe (c-xref-window-edges))
      ;;(message "testing %S %S" cwe owe)
      (if (or
               (and (> (elt cwe 2) (elt owe 2))
                        (not
                         (or (< (elt cwe 3) (elt owe 1))
                             (> (elt cwe 1) (elt owe 3))
                             )))
               (equal (selected-window) sw))
              (setq loop nil)
            )
      )
    (if (equal (selected-window) sw)
            (setq res nil)
      (setq res t)
      )
    res
    ))

(defun c-xref-select-upper-window ()
  (let ((owe) (cwe) (loop) (sw) (res))
    (setq owe (c-xref-window-edges))
    (setq sw (selected-window))
    (setq loop t)
    (while loop
      (other-window -1)
      (setq cwe (c-xref-window-edges))
      (if (or
               (and (< (elt cwe 1) (elt owe 1))
                        (not
                         (or (< (elt cwe 2) (elt owe 0))
                             (> (elt cwe 0) (elt owe 2))
                             )))
               (equal (selected-window) sw))
              (setq loop nil)
            )
      )
    (if (equal (selected-window) sw)
            (setq res nil)
      (setq res t)
      )
    res
    ))

(defun c-xref-appropriate-window-height (aplus aminus)
  (let ((pp) (lnnum) (wsize) (sw))
    (setq sw (selected-window))
    (setq pp (point))
    (setq lnnum (count-lines 1 (point-max)))
    (setq wsize (window-height (selected-window)))
    (if (< lnnum c-xref-window-minimal-size)
            (setq lnnum c-xref-window-minimal-size)
      )
    (if (> lnnum (- (frame-height) window-min-height))
            (setq lnnum (- (frame-height) window-min-height))
      )
    (if (or
             (and aplus (> lnnum wsize))
             (and aminus (< lnnum wsize)))
            (progn
              ;; the constant contains scrollbar and information line
              (c-xref-set-window-height (+ lnnum 3))
              ))
    (goto-char pp)
    ))

(defun c-xref-appropriate-window-width (aplus aminus)
  (let ((pp) (i) (wsize) (wwidth) (width) (w))
    (setq pp (point))
    (setq wsize (window-height))
    (setq width 10)
    (setq i 1)
    (goto-char (point-min))
    (end-of-line)
    (while (and (<= i wsize) (< (point) (point-max)))
      (forward-line)
      (end-of-line)
      (setq w (c-xref-current-column))
      (if (> w width) (setq width w))
      (setq i (+ i 1))
      )
    (setq wwidth (window-width))
    (if (or
             (and aplus (> width wwidth))
             (and aminus (< width wwidth)))
            (c-xref-set-window-width (+ width 2))
      )
    (goto-char pp)
    ))

(defun c-xref-appropriate-browser-windows-sizes (oldwins)
  (if (not oldwins)
      (progn
            (if c-xref-browser-windows-auto-resizing
                (progn
                  (c-xref-appropriate-window-height nil t)
                  (c-xref-appropriate-window-width t t)
                  )
              (c-xref-set-window-height c-xref-symbol-selection-window-height)
              (c-xref-set-window-width c-xref-symbol-selection-window-width)
              )
            ))
  )


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;  highlighting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-font-lock-list-keywords (sym)
  (cons (cons (c-xref-keywords-regexp) '(c-xref-keyword-face))
            (cons (cons "^.\\([^:]*:[0-9]*\\):" '(c-xref-list-pilot-face))
                      (if (eq sym nil)
                          nil
                        (cons (cons (concat "\\b" sym "\\b") '(c-xref-list-symbol-face))
                                  nil
                                  ))
                      ))
  )


(defun c-xref-create-escaped-cased-string (sstr)
  (let ((ind) (maxi) (ress) (cc))
    (setq ress "")
    (setq ind 0)
    (setq maxi (length sstr))
    (while (< ind maxi)
      (setq cc (substring sstr ind (+ ind 1)))
      (if (string-match "[A-Za-z]" cc)
              (setq ress (concat ress "[" (downcase cc) (upcase cc) "]" ))
            (if (string-match "[][.*+?^$ \\]" cc)
                (setq ress (concat ress "\\" cc))
              (setq ress (concat ress cc))
              ))
      (setq ind (+ ind 1))
      )
    ress
    ))

(defun c-xref-create-tag-search-fontif (sstr)
  (let ((res) (ress) (loop) (ind) (ind0) (prefix))
    (setq prefix "\\(")
    (setq loop t)
    (setq ind 0)
    (while loop
      (setq ind0 (string-match "[^ \n\t]" sstr ind))
      (if (not ind0)
              (setq loop nil)
            (setq ind (string-match "[ \t\n]" sstr ind0))
            (if (not ind)
                (setq loop nil)
              (setq ress (concat ress prefix (c-xref-create-escaped-cased-string (substring sstr ind0 ind))))
              (setq prefix "\\|")
              ))
      )
    (if ind0
            (setq ress (concat ress prefix (c-xref-create-escaped-cased-string (substring sstr ind0))))
      )
    (setq ress (concat ress "\\)"))
    (setq res
              (cons (cons ress '(c-xref-list-symbol-face))
                    (cons (cons ":.*$" '(c-xref-list-default-face))
                              nil))
              )
    ;;(message "fontif == %S" res)
    res
    ))

(defun c-xref-completion-symbol-highlight (begpos endpos)
  (let ((line) (sym) (spos) (mbeg) (mend))
    (setq line (buffer-substring begpos endpos))
    (setq spos (string-match "[ \t\n]" line 0))
    (if (and spos (> spos 0))
            (progn
              (setq sym (c-xref-create-escaped-cased-string (substring line 0 spos)))
              (setq mbeg (string-match (concat "\\b[a-zA-Z0-9_\\.]*" sym "(") line spos))
              (if mbeg
                  (progn
                        (setq mend (match-end 0))
                        (put-text-property (+ mbeg begpos)
                                                   (- (+ mend begpos) 1)
                                                   'face 'c-xref-list-symbol-face)
                        )
                (setq mbeg (string-match (concat "\\b[a-zA-Z0-9_\\.]*" sym "\\[") line spos))
                (if mbeg
                        (progn
                          (setq mend (match-end 0))
                          (put-text-property (+ mbeg begpos)
                                                     (- (+ mend begpos) 1)
                                                     'face 'c-xref-list-symbol-face)
                          )
                  (setq mbeg (string-match (concat "\\b[a-zA-Z0-9_\\.]*" sym "$") line spos))
                  (if mbeg
                          (progn
                            (setq mend (match-end 0))
                            (put-text-property (+ mbeg begpos)
                                                       (+ mend begpos)
                                                       'face 'c-xref-list-symbol-face)
                            ))))))
    ))

(defun c-xref-highlight-keywords (begpos endpos keywords-regexp keyword-face)
  (let ((buff) (bpos) (loop-flag) (mbeg) (mend)
            (ocfs))
    ;;(message "highlighting keywords %d %d" begpos endpos)
    (if (< begpos endpos)
            (progn
              (setq buff (buffer-substring begpos endpos))
              (setq bpos 0)
              (setq loop-flag t)
              (setq ocfs case-fold-search)
              (setq case-fold-search nil)
              (while loop-flag
                (setq mbeg (string-match keywords-regexp buff bpos))
                (if (eq mbeg nil)
                        (setq loop-flag nil)
                  (setq mend (match-end 0))
                  (put-text-property (+ mbeg begpos) (+ mend begpos) 'face keyword-face)
                  (setq bpos mend)
                  ))
              (setq case-fold-search ocfs)
              ))
    ))

(defun c-xref-fontify-region (begpos endpos kw-font-list)
  (let ((kwl) (kw))
    ;;(message "c-xref-fontify-region %d %d %S" begpos endpos kw-font-list)
    (setq kwl kw-font-list)
    (while kwl
      (setq kw (car kwl))
      (c-xref-highlight-keywords begpos endpos (car kw) (car (cdr kw)))
      (setq kwl (cdr kwl))
      )
    ))

(defun c-xref-line-hightlight (begpos endpos multiple-lines-flag hbeg-offset
                                                      fontification completion-highlighting)
  (let ((bp) (loop-flag) (cpos) (prev-line) (dp-pos)
            )
    ;;(message "c-xref-line-hightlight [ %d-%d ]" begpos endpos)
    (setq cpos (c-xref-char-search begpos -1 ?\n))
    (if (< cpos 0) (setq cpos 0))
    (if (and c-xref-coloring (< (+ cpos 1) endpos))
            (c-xref-fontify-region (+ cpos 1) endpos fontification)
      )
    (if (or c-xref-mouse-highlight c-xref-coloring)
            (progn
              (while (< (+ cpos 1) endpos)
                (setq bp cpos)
                (setq loop-flag t)
                (while (and loop-flag
                                    multiple-lines-flag
                                    (eq (char-after (+ bp 1)) ?\ )
                                    (eq (char-after (+ bp 2)) ?\ )
                                    )
                  (setq prev-line (c-xref-char-search (- bp 1) -1 ?\n))
                  (if (< prev-line 1)
                          (setq loop-flag nil)
                        (setq bp prev-line)
                        )
                  )
                (setq cpos (c-xref-char-search (+ cpos 1) 1 ?\n))
                (if (and c-xref-mouse-highlight (< (+ bp hbeg-offset) cpos))
                        (put-text-property (+ bp hbeg-offset) cpos 'mouse-face 'highlight)
                  )
                (if (and c-xref-coloring completion-highlighting (< (+ bp 1) cpos))
                        (c-xref-completion-symbol-highlight (+ bp 1) cpos)
                  )
                )))
    ))



(defun c-xref-entry-point-make-initialisations-no-project-required ()
  (c-xref-soft-select-dispach-data-caller-window c-xref-this-buffer-dispatch-data)
  )

(defun c-xref-entry-point-make-initialisations ()
  (c-xref-entry-point-make-initialisations-no-project-required)
  ;; hack in order to permit asking for version without src project
  (setq c-xref-active-project (c-xref-compute-active-project))
  (if c-xref-display-active-project-in-minibuffer
      (message "Project: %s" c-xref-active-project)
    )
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; C-XREF-TASK ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defvar c-xref-buffers-counter 1)
(defun c-xref-new-symbol-resolution-buffer ()
  (let ((res))
    (setq res (format "%s (%d)" c-xref-symbol-resolution-buffer c-xref-buffers-counter))
    (setq c-xref-buffers-counter (+ c-xref-buffers-counter 1))
    res
    ))
(defun c-xref-new-references-buffer ()
  (let ((res))
    (setq res (format "%s (%d)" c-xref-references-buffer c-xref-buffers-counter))
    (setq c-xref-buffers-counter (+ c-xref-buffers-counter 1))
    res
    ))

(defun c-xref-start-server-process (initopts ofile proc filter)
  (let ((oldpct) (opts))
    (setq oldpct process-connection-type)
    (setq process-connection-type nil)
    (setq opts (list "-xrefactory-II"
                     "-crlfconversion"
                     "-crconversion"
                     "-o" ofile))

;;    (when c-xref-debug-mode
;;      (setq opts (append opts (list "-debug" "-log=log"))))

    (setq opts (append opts initopts))

    (if c-xref-debug-mode (message "calling: %S" opts))
    (set proc (cons
                   (apply 'start-process
                              (concat c-xref-exec-directory "c-xref")
                              nil
                              (concat c-xref-exec-directory "c-xref")
                              opts
                              )
                   (cons "" nil)))
    (set-process-filter (car (eval proc)) filter)
    (setq process-connection-type oldpct)
    (set-process-query-on-exit-flag (car (eval proc)) nil)
    ))

(defvar c-xref-interrupt-dialog-map (make-sparse-keymap "C-xref interrupt dialog"))
(c-xref-add-basic-modal-keybindings c-xref-interrupt-dialog-map)


(defun c-xref-interrupt-current-process (process tmp-files)
  (let ((sel))
    (setq sel (c-xref-modal-dialog c-xref-selection-modal-buffer
                                   "Interrupt current process?
----
 1.) Yes
 2.) No, just interrupt Emacs macro
 3.) No
----
"
                                           3 0 t c-xref-interrupt-dialog-map nil))
    (if (eq sel 3)
            (progn
              ;; execution interrupted by C-g, kill process, clean tmp files
              (delete-process (car process))
              (c-xref-server-remove-tmp-files tmp-files)
              (if (file-exists-p c-xref-server-tasks-ofile) (delete-file c-xref-server-tasks-ofile))
              (setq inhibit-quit nil)
              (error "Process killed")
              )
      (if (eq sel 4)
              (progn
                (setq inhibit-quit nil)
                (error "Quit")
                )
            (setq quit-flag nil)
            ))
    (sit-for .1)
    ))

(defun c-xref-wait-until-task-sync (proc tmp-files)
  (let ((process))
    ;; I will check quit-flag manually
    (setq inhibit-quit t)
    (setq process (eval proc))
    (while (and
                (not (cdr (cdr process)))
                (eq (process-status (car process)) 'run))
      (accept-process-output (car process) 0 100)
      (if quit-flag (c-xref-interrupt-current-process process tmp-files))
      )
    (setq inhibit-quit nil)
    ))

(defun c-xref-send-data-to-running-process (data proc)
  (let ((cbuffer))
    (setq cbuffer (current-buffer))
    (if (eq (eval proc) nil)
            (c-xref-start-server-process '("-server") c-xref-server-tasks-ofile proc 'c-xref-server-filter)
      (if (not (eq (process-status (car (eval proc))) 'run))
              (progn
                (delete-process (car (eval proc)))
                (c-xref-start-server-process '("-server") c-xref-server-tasks-ofile proc 'c-xref-server-filter)
                )))
    (if c-xref-debug-mode (message "sending: %s" data))
    (get-buffer-create " *c-xref-server-options*")
    (set-buffer " *c-xref-server-options*")
    (c-xref-erase-buffer)
    (insert data)
    (insert "\nend-of-options\n\n")
    (c-xref-init-server-filter proc)
    (process-send-region (car (eval proc)) 1 (point))
    (kill-buffer nil)
    (set-buffer cbuffer)
    ))

(defun c-xref-init-server-filter (proc)
  (setcdr (cdr (eval proc)) nil)
  )

(defun c-xref-processes-filter (process output proc)
  (let ((pn) (ss) (i) (j) (len))
    (if c-xref-debug-mode (message "got: %s" output))
    (setq ss (format "%s%s" (car (cdr (eval proc))) output))
    (setq len (length ss))
    (setq i 0)
    (setq i (c-xref-server-dispatch-skip-blank ss i len))
    (while (and (< (+ i c-xref-ppc-progress-len) len)
                        (equal (substring ss i (+ i c-xref-ppc-progress-len)) c-xref-ppc-progress)
                        )
      (progn
            (setq i (+ i c-xref-ppc-progress-len))
            (setq j i)
            (while (and (< i len)
                            (>= (elt ss i) ?0)
                            (<= (elt ss i) ?9)
                            )
              (setq i (+ i 1))
              )
            (setq pn (string-to-number (substring ss j i)))
            (message "progress %s%%" pn)
            (setq i (c-xref-server-dispatch-skip-blank ss i len))
            ))
    (if (< (+ i c-xref-ppc-synchro-record-len) len)
            (progn
              (if (equal (substring ss i (+ i c-xref-ppc-synchro-record-len)) c-xref-ppc-synchro-record)
                  (progn
                        (setq i (+ i c-xref-ppc-synchro-record-len))
                        (setcdr (cdr (eval proc)) t)
                        )
                (error "SYNCHRO PROBLEM: %s" ss)
                )))
    (setcar (cdr (eval proc)) (substring ss i))
    ))

(defun c-xref-server-filter (process output)
  (c-xref-processes-filter process output 'c-xref-server-process)
  )

(defun c-xref-refactorer-filter (process output)
  (c-xref-processes-filter process output 'c-xref-refactorer-process)
  )

(defun c-xref-tags-filter (process output)
  (c-xref-processes-filter process output 'c-xref-tags-process)
  )

(defun c-xref-server-add-buffer-to-tmp-files-list (buffer lst)
  (let ((res))
    (setq res (cons (list buffer (c-xref-server-get-new-tmp-file-name)) lst))
    res
    ))

(defun c-xref-server-save-buffers-to-tmp-files (lst)
  (let ((bb) (tt) (bbt) (cb) (res) (coding))
    (setq cb (current-buffer))
    (setq res nil)
    (while lst
      (setq bbt (car lst))
      (setq bb (car bbt))
      (setq tt (car (cdr bbt)))
      (set-buffer bb)
      (if (boundp 'buffer-file-coding-system)
              (setq coding buffer-file-coding-system)
            (setq coding nil)
            )
      (c-xref-write-tmp-buff tt 1 (+ (buffer-size) 1) coding)
      (setq res (append (list "-preload" (buffer-file-name bb) tt) res))
      (setq lst (cdr lst))
      )
    (set-buffer cb)
    res
    ))

(defun c-xref-server-remove-tmp-files (lst)
  (let ((bb) (tt) (bbt))
    (while lst
      (setq bbt (car lst))
      (setq bb (car bbt))
      (setq tt (car (cdr bbt)))
      (if (and c-xref-debug-mode c-xref-debug-preserve-tmp-files)
              (message "keeping tmp file %s" tt)
            (delete-file tt)
            )
      (setq lst (cdr lst))
      )
    ))

(defun c-xref-server-get-list-of-buffers-to-save-to-tmp-files (can-add-current-buffer)
  (let ((bl) (bb) (res) (cb))
    (setq res nil)
    (setq bl (buffer-list))
    (setq cb (current-buffer))
    (while bl
      (setq bb (car bl))
      (if (and (buffer-file-name bb)
                   (buffer-modified-p bb)
                   )
              (progn
                (if (or can-add-current-buffer
                            (not (equal bb cb)))
                        (setq res (c-xref-server-add-buffer-to-tmp-files-list bb res))
                  )))
      (setq bl (cdr bl))
      )
    (set-buffer cb)
    res
    ))

(defun c-xref-server-get-point-and-mark-options ()
  (let ((res))
    (setq res (cons (format "-olcursor=%d" (- (point) 1)) res))
    (if (region-active-p)
        (setq res (cons (format "-olmark=%d" (- (mark t) 1)) res))
      )
    res
    ))

(defun c-xref-buffer-char-list ()
  (let ((res) (ll) (i) (max))
    (setq res (make-list (1- (point-max)) ?\n))
    (setq ll res)
    (setq max (point-max))
    (setq i 1)
    (while (< i max)
      (setcar ll (char-after i))
      (setq ll (cdr ll))
      (setq i (1+ i))
      )
    res
    ))

(defun c-xref-char-list-substring (list from to)
  (let ((res) (i) (ll) (len) (max))
    (if (stringp list)
            (setq res (substring list from to))
      (setq ll (nthcdr from list))
      (if to
              (setq len (- to from))
            (setq len (length ll))
            )
      (setq res (make-string len ?\n))
      (setq i 0)
      (while (< i len)
            (aset res i (car ll))
            (setq ll (cdr ll))
            (setq i (1+ i))
            )
      )
    ;;(message "returning substring(%S %S %S) --> %S" list from to res)
    res
    ))

(defun c-xref-server-read-answer-file-and-dispatch (dispatch-data tmp-files)
  (let ((res) (cb) (i) (coding-system-for-read 'utf-8))
    (setq cb (current-buffer))
    (get-buffer-create c-xref-server-answer-buffer)
    (set-buffer c-xref-server-answer-buffer)
    ;;(c-xref-erase-buffer)
    (insert-file-contents c-xref-server-tasks-ofile nil nil nil t)
    (if (fboundp 'buffer-substring-no-properties)
            (setq res (buffer-substring-no-properties 1 (point-max)))
          (setq res (buffer-substring 1 (point-max)))
          )
    (setq i 0)
    (if c-xref-debug-mode
            (write-region (point-min) (point-max) (c-xref-server-get-new-tmp-file-name))
      (kill-buffer c-xref-server-answer-buffer)
      (delete-file c-xref-server-tasks-ofile))
    (c-xref-server-remove-tmp-files tmp-files)
    (set-buffer cb)
    (setq i (c-xref-server-dispatch res i (length res) dispatch-data))
    res
    ))

(defun c-xref-send-data-to-process-and-dispatch (commands dispatch-data tmp-files)
  (let ((proc) (frame-id) (opts))
    (setq proc (cdr (assoc 'process dispatch-data)))
    (setq frame-id (cdr (assoc 'frame-id dispatch-data)))
    (setq opts (format "%s %s -p \"%s\""
                               commands
                               (if c-xref-browser-lists-source-lines "" "-rlistwithoutsrc ")
                               c-xref-active-project
                               ))

    (c-xref-send-data-to-running-process opts proc)
    (c-xref-wait-until-task-sync proc tmp-files)
    (c-xref-server-read-answer-file-and-dispatch dispatch-data tmp-files)
    ))

(defun c-xref-server-call-on-current-buffer (options dispatch-data obl)
  (let ((res) (bl) (data) (topt) (blo))
    (setq bl (c-xref-server-add-buffer-to-tmp-files-list
                  (current-buffer) obl))
    (setq blo (c-xref-server-save-buffers-to-tmp-files bl))
    (setq blo (append blo (c-xref-server-get-point-and-mark-options )))
    (setq topt "")
    (while blo
      (setq topt (format "%s \"%s\"" topt (car blo)))
      (setq blo (cdr blo))
      )
    (setq data (format "%s %s \"%s\"" options topt (buffer-file-name nil)))
    (c-xref-send-data-to-process-and-dispatch data dispatch-data bl)
    ))

(defun c-xref-server-call-on-current-buffer-no-saves (options dispatch-data)
  (c-xref-server-call-on-current-buffer options dispatch-data nil)
  )

(defun c-xref-server-call-on-current-buffer-all-saves (options dispatch-data)
  (let ((bl))
    (setq bl (c-xref-server-get-list-of-buffers-to-save-to-tmp-files nil))
    (c-xref-server-call-on-current-buffer options dispatch-data bl)
    ))

(defun c-xref-kill-refactorer-process-if-any ()
  (if (not (eq c-xref-refactorer-process nil))
      (delete-process (car c-xref-refactorer-process))
    )
  (setq c-xref-refactorer-process nil)
  )

(defun c-xref-server-call-refactoring-task (opts)
  (let ((bl) (frame-id))
    (if (and (not (eq c-xref-refactorer-process nil))
                 (eq (process-status (car c-xref-refactorer-process)) 'run))
            ;;(if (c-xref-yes-or-no-window "A refactoring process is running, can I kill it? " t nil)
            (progn
              (c-xref-kill-refactorer-process-if-any)
              )
          ;;  (error "Cannot run two refactoring processes")
          ;;  )
      )
    (setq c-xref-refactorer-process nil)
    (setq c-xref-refactorer-dispatch-data (c-xref-get-basic-server-dispatch-data 'c-xref-refactorer-process))
    (setq bl (c-xref-server-get-list-of-buffers-to-save-to-tmp-files nil))
    (setq bl (c-xref-server-add-buffer-to-tmp-files-list
                  (current-buffer) bl))
    (setq frame-id (cdr (assoc 'frame-id c-xref-refactorer-dispatch-data)))
    (setq opts (append opts (c-xref-server-save-buffers-to-tmp-files bl)))
    (setq opts (append opts (c-xref-server-get-point-and-mark-options )))
    (setq opts (append (list "-refactory"
                                         "-p" c-xref-active-project
                                         )
                               opts))
    (setq opts (append opts (cons
                             (format "%s" (buffer-file-name))
                             nil)))

    (c-xref-start-server-process opts c-xref-server-tasks-ofile 'c-xref-refactorer-process 'c-xref-refactorer-filter)
    (c-xref-wait-until-task-sync 'c-xref-refactorer-process bl)
    (c-xref-server-read-answer-file-and-dispatch c-xref-refactorer-dispatch-data bl)
    (c-xref-kill-refactorer-process-if-any)
    ))

(defun c-xref-server-tags-process (opts)
  (let ((bl))
    (if (and (not (eq c-xref-tags-process nil))
                 (eq (process-status (car c-xref-tags-process)) 'run))
            (if (c-xref-yes-or-no-window "tags maintenance process is running, can I kill it? " t nil)
                (progn
                  (delete-process (car c-xref-tags-process))
                  (setq c-xref-tags-process nil)
                  )
              (error "Cannot run two tag maintenance processes.")
              ))
    (setq bl (c-xref-server-get-list-of-buffers-to-save-to-tmp-files t))
    (setq opts (append opts (list "-errors"
                                                  "-p" c-xref-active-project
                                                  (expand-file-name default-directory)
                                                  )))
    (setq opts (append opts (c-xref-server-save-buffers-to-tmp-files bl)))
    ;;  (setq c-xref-tags-dispatch-data (c-xref-get-basic-server-dispatch-data 'c-xref-tags-process))
    (c-xref-start-server-process opts c-xref-tags-tasks-ofile 'c-xref-tags-process 'c-xref-tags-filter)
    (c-xref-wait-until-task-sync 'c-xref-tags-process bl)
    ;;  (c-xref-server-read-answer-file-and-dispatch c-xref-tags-dispatch-data nil)
    (delete-process (car c-xref-tags-process))
    (setq c-xref-tags-process nil)
    (c-xref-server-remove-tmp-files bl)
    ))

(defun c-xref-get-basic-server-dispatch-data (proc)
  (let ((res))
    (setq res (cons (cons 'dummy-parameter-to-make-cdr-in-memory nil)
                            (cons (cons 'caller-window (selected-window))
                                      (cons (cons 'process proc)
                                                (cons (cons 'frame-id (c-xref-get-this-frame-id))
                                                      nil)))))
    res
    ))

(defun c-xref-call-process-with-basic-file-data-no-saves (option)
  (setq c-xref-global-dispatch-data (c-xref-get-basic-server-dispatch-data
                                                     'c-xref-server-process))
  (c-xref-server-call-on-current-buffer-no-saves option
                                                                     c-xref-global-dispatch-data)
  )

(defun c-xref-call-process-with-basic-file-data-all-saves (option)
  (setq c-xref-global-dispatch-data (c-xref-get-basic-server-dispatch-data
                                                     'c-xref-server-process))
  (c-xref-server-call-on-current-buffer-all-saves option
                                                                      c-xref-global-dispatch-data)
  )

(defun c-xref-compute-simple-information (option)
  (let ((res))
    (c-xref-call-process-with-basic-file-data-no-saves option)
    (setq res (cdr (assoc 'info c-xref-global-dispatch-data)))
    res
    ))

(defun c-xref-compute-active-project ()
  (let ((res) (proc))
    (if c-xref-current-project
            (setq c-xref-active-project c-xref-current-project)
      (setq proc 'c-xref-server-process)
      (setq c-xref-global-dispatch-data (c-xref-get-basic-server-dispatch-data
                                                         proc))
      (c-xref-send-data-to-running-process
       (format "-olcxgetprojectname \"%s\"" (buffer-file-name))
       proc)
      (c-xref-wait-until-task-sync proc nil)
      (c-xref-server-read-answer-file-and-dispatch c-xref-global-dispatch-data nil)
      (setq res (cdr (assoc 'info c-xref-global-dispatch-data)))
      res
      )
    ))

(defun c-xref-softly-preset-project (pname)
  (let ((actp))
    (setq actp c-xref-active-project)
    (setq c-xref-active-project pname)
    ;; this is just a hack, it may cause problems, because current buffer
    ;; is passed to c-xref (be careful on which buffer you call it)
    ;; but c-xref needs to parse something to softsetup project, maybe
    ;; I should implement special option '-softprojectset'?
    (c-xref-call-process-with-basic-file-data-no-saves "-olcxgetprojectname")
    ;; Hmm. this is also dispatching, hope it is no problem.
    (setq c-xref-active-project actp)
    ))

(defun c-xref-get-env (name)
  "Get value of an C-xrefactory environment variable.

This  function  gets a  value  associated  with  the C-xref  environment
variable set by the -set option in the .c-xrefrc file (its value depends
on active project selection).
"
  (let ((res))
    (setq c-xref-global-dispatch-data (c-xref-get-basic-server-dispatch-data 'c-xref-server-process))
    (c-xref-server-call-on-current-buffer-no-saves (format "-get \"%s\"" name)
                                                                           c-xref-global-dispatch-data)
    (setq res (cdr (assoc 'info c-xref-global-dispatch-data)))
    res
    ))

(defvar c-xref-frame-id-counter 0)
(defun c-xref-get-this-frame-id ()
  (let ((res) (dd))
    (setq dd (c-xref-get-this-frame-dispatch-data))
    (if dd
            (setq res (cdr (assoc 'frame-id dd)))
      (setq c-xref-frame-id-counter (+ c-xref-frame-id-counter 1))
      (c-xref-set-this-frame-dispatch-data (cons (cons 'frame-id c-xref-frame-id-counter) nil))
      (setq res c-xref-frame-id-counter)
      )
    res
    ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DISPATCH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defvar c-xref-server-ctag "")
(defvar c-xref-server-ctag-attributes nil)
(defvar c-xref-server-cstring-value "")

(defun c-xref-server-dispatch-skip-blank (ss i len)
  (while (and (< i len)
                  (or (eq (elt ss i) ?\ )
                          (eq (elt ss i) ?\t)
                          (eq (elt ss i) ?\n)))
    (setq i (1+ i))
    )
  i
  )

(defun c-xref-server-parse-xml-tag (ss i len)
  (let ((b) (e) (att) (attval))
    (setq c-xref-server-ctag c-xref_PPC_NO_TAG)
    (setq i (c-xref-server-dispatch-skip-blank ss i len))
    (if (< i len)
            (progn
              (if (not (eq (elt ss i) ?<))
                  (error "tag starts by %c instead of '<'" (elt ss i))
                )
              (setq i (1+ i))
              (setq b i)
              (while (and (< i len)
                              (not (eq (elt ss i) ? ))
                              (not (eq (elt ss i) ?>)))
                (setq i (1+ i))
                )
              (setq c-xref-server-ctag (c-xref-char-list-substring ss b i))
          ;;(message "c-xref-server-ctag == %s" c-xref-server-ctag)
              (setq i (c-xref-server-dispatch-skip-blank ss i len))
              (setq c-xref-server-ctag-attributes nil)
              (while (and (< i len) (not (eq (elt ss i) ?>)))
                (setq b i)
                (while (and (< i len)
                                    (not (eq (elt ss i) ? ))
                                    (not (eq (elt ss i) ?=))
                                    (not (eq (elt ss i) ?>)))
                  (setq i (1+ i))
                  )
                (setq att (c-xref-char-list-substring ss b i))
            ;;(message "att %s" att)
                (if (eq (elt ss i) ?=)
                        (progn
                          (setq i (1+ i))
                          (if (eq (elt ss i) ?\")
                              (progn
                                    (setq i (1+ i))
                                    (setq b i)
                                    (while (and (< i len) (not (eq (elt ss i) ?\")))
                                      (setq i (1+ i))
                                      )
                                    (setq attval (c-xref-char-list-substring ss b i))
                                    (setq i (1+ i))
                                    )
                            (setq b i)
                            (while (and (< i len)
                                                (not (eq (elt ss i) ? ))
                                                (not (eq (elt ss i) ?>)))
                              (setq i (1+ i))
                              )
                            (setq attval (c-xref-char-list-substring ss b i))
                            )
                  ;;(message "attval %s == %s" att attval)
                          (setq c-xref-server-ctag-attributes
                                    (cons (cons att attval)
                                          c-xref-server-ctag-attributes))
                          ))
                (setq i (c-xref-server-dispatch-skip-blank ss i len))
                )
              (if (< i len) (setq i (1+ i)))
              ))
    i
    ))

(defun c-xref-server-dispatch-get-int-attr (attr)
  (let ((as) (res))
    (setq res 0)
    (setq as (assoc attr c-xref-server-ctag-attributes))
    (if as
            (setq res (string-to-number (cdr as)))
      )
    res
    ))


(defun c-xref-server-dispatch-require-ctag (tag)
  (if (not (equal c-xref-server-ctag tag))
      (error "[c-xref answer parsing] <%s> expected instead of <%s>" tag c-xref-server-ctag)
    )
  )

(defun c-xref-server-dispatch-require-end-ctag (tag)
  (c-xref-server-dispatch-require-ctag (format "/%s" tag))
  )


(defun c-xref-soft-select-dispach-data-caller-window (dispatch-data)
  (let ((winassoc) (win) (res))
    (setq res nil)
    (if dispatch-data
            (progn
              (setq winassoc (assoc 'caller-window dispatch-data))
              (if winassoc
                  (progn
                        (setq win (cdr winassoc))
                        (if (and win (windowp win) (window-live-p win))
                            (progn
                              (select-window  win)
                              (set-buffer (window-buffer win))
                              (setq res t)
                              ))))))
    res
    ))

(defun c-xref-select-dispach-data-caller-window (dispatch-data)
  (let ((ww))
    (setq ww (cdr (assoc 'caller-window dispatch-data)))
    (if ww
            (progn
              (if (not (window-live-p ww))
                  (error "The associated window no longer exists.")
                (select-window ww)
                (set-buffer (window-buffer (selected-window)))
                )))
    ))

(defun c-xref-select-dispach-data-refs-window (dispatch-data)
  (select-window  (cdr (assoc 'linked-refs-window dispatch-data)))
  (set-buffer (window-buffer (selected-window)))
  )

(defun c-xref-select-dispach-data-resolution-window (dispatch-data)
  (select-window  (cdr (assoc 'linked-resolution-window dispatch-data)))
  (set-buffer (window-buffer (selected-window)))
  )

(defun c-xref-display-and-set-new-dialog-window (buff horizontal in-new-win)
  (let ((res))
    (if horizontal
            (progn
              (if (< (window-width) (* 2 window-min-width))
                  (c-xref-set-window-width (* 2 window-min-width))
                )
              (split-window-horizontally)
              )
      (if (< (window-height) (* 2 window-min-height))
              (c-xref-set-window-height (* 2 window-min-height))
            )
      (split-window-vertically)
      )
    (other-window 1)
    (setq res (selected-window))
    (other-window -1)
    (if in-new-win (other-window 1))
    (set-buffer (get-buffer-create buff))
    (setq buffer-read-only nil)
    (c-xref-erase-buffer)
    (set-window-buffer (selected-window) (current-buffer))
    (setq truncate-lines t)
    res
    ))

(defun c-xref-display-and-set-maybe-existing-window (buff horizontal in-new-win)
  (let ((ww))
    (setq ww (get-buffer-window buff))
    (if ww
            (select-window ww)
      (c-xref-display-and-set-new-dialog-window buff  horizontal in-new-win)
      )
    ))

(defun c-xref-server-dispatch-error (ss i len dispatch-data tag)
  (let ((tlen) (cc) (cw))
    (setq cw (selected-window))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag tag)
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (c-xref-display-and-set-new-dialog-window c-xref-error-modal-buffer nil t)
    (insert "[error] : ")
    (insert cc)
    (goto-char (point-min))
    (c-xref-appropriate-window-height nil t)
    (beep t)
    (c-xref-read-key-sequence "Press a key to continue")
    (c-xref-delete-window-in-any-frame (current-buffer) nil)
    (select-window cw)
    (c-xref-kill-refactorer-process-if-any)
    (error "exiting")
    i
    ))

(defun c-xref-server-dispatch-information (ss i len dispatch-data tag)
  (let ((tlen) (cc) (cw) (dw))
    (setq cw (selected-window))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag tag)
    (if (and (equal tag c-xref_PPC_DEBUG_INFORMATION)
                 (eq c-xref-debug-mode nil))
            () ;; do nothing if debug information and mode is off
      (c-xref-select-dispach-data-caller-window dispatch-data)
      (setq dw (c-xref-display-and-set-new-dialog-window c-xref-info-modal-buffer nil t))
      ;;(insert "[info] : ")
      (insert cc)
      (goto-char (point-min))
      (c-xref-appropriate-window-height nil t)
      (c-xref-read-key-sequence "Press a key to continue")
      (delete-window dw)
      (select-window cw)
      )
    i
    ))

(defun c-xref-server-dispatch-call-macro (ss i len dispatch-data)
  (let ((tlen) (cc))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_CALL_MACRO)
    (call-last-kbd-macro 1)
    i
    ))

(defun c-xref-server-dispatch-kill-buffer-remove-file (ss i len dispatch-data)
  (let ((tlen) (cc))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_KILL_BUFFER_REMOVE_FILE)
    (if (c-xref-yes-or-no-window cc t dispatch-data)
            (progn
              ;; do not kill buffer really, this would make undo impossible
              ;; rather clear it
              (erase-buffer)
              (set-buffer-modified-p nil)
              ;; and really delete the file
              (c-xref-delete-file (buffer-file-name))
              (bury-buffer)
              ))
    i
    ))

(defun c-xref-server-dispatch-no-project (ss i len dispatch-data)
  (let ((tlen) (cc))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_NO_PROJECT)
    (if (c-xref-yes-or-no-window
             (format "No project covers %s,\ncreate new project?" cc)
             t dispatch-data)
            (progn
              (c-xref-project-new)
              ;; a non local exit
              (error "Done.")
              )
      (error "Exiting")
      )
    i
    ))

(defun c-xref-server-dispatch-project-mismatch (ss i len dispatch-data)
  "Handle project mismatch - server is locked to a different project."
  (let ((tlen) (locked-project))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq locked-project (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_PROJECT_MISMATCH)
    (if (c-xref-yes-or-no-window
         (format "Server is locked to project '%s'.\nSwitch to this file's project? (Server will restart)"
                 locked-project)
         t dispatch-data)
        (progn
          ;; Kill the server process to release the lock
          (c-xref-kill-xref-process nil)
          ;; Signal that we should retry - caller should handle this
          (error "Server restarted, please retry operation.")
          )
      (error "Cancelled - staying with current project")
      )
    i
    ))

(defvar c-xref-confirmation-dialog-map (make-sparse-keymap "C-xref confirmation"))
(define-key c-xref-confirmation-dialog-map [up] 'c-xref-modal-dialog-previous-line)
(define-key c-xref-confirmation-dialog-map [down] 'c-xref-modal-dialog-next-line)
(define-key c-xref-confirmation-dialog-map [kp-up] 'c-xref-modal-dialog-previous-line)
(define-key c-xref-confirmation-dialog-map [kp-down] 'c-xref-modal-dialog-next-line)
(define-key c-xref-confirmation-dialog-map c-xref-escape-key-sequence 'c-xref-modal-dialog-exit)
(define-key c-xref-confirmation-dialog-map "\C-g" 'c-xref-modal-dialog-exit)
(define-key c-xref-confirmation-dialog-map "q" 'c-xref-modal-dialog-exit)
(define-key c-xref-confirmation-dialog-map [f7] 'c-xref-modal-dialog-exit)
(define-key c-xref-confirmation-dialog-map [newline] 'c-xref-modal-dialog-continue)
(define-key c-xref-confirmation-dialog-map [return] 'c-xref-modal-dialog-continue)
(define-key c-xref-confirmation-dialog-map "\C-m" 'c-xref-modal-dialog-continue)
(define-key c-xref-confirmation-dialog-map "?" 'c-xref-confirmation-dialog-help)

(defun c-xref-confirmation-dialog-help (event)
  (interactive "i")
  (c-xref-interactive-help
   "Special hotkeys available:

\\[c-xref-modal-dialog-continue] \t-- select
\\[c-xref-modal-dialog-exit] \t-- cancel
\\[c-xref-confirmation-dialog-help] \t-- this help
" nil nil)
  )

(defun c-xref-yes-or-no-window (message default dispatch-data)
  (let ((sw) (sel) (yes-line) (default-line) (res))
    (setq sw (selected-window))
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (setq yes-line (+ (c-xref-char-count ?\n message) 4))
    (if default
            (setq default-line yes-line)
      (setq default-line (1+  yes-line))
      )
    (setq sel (c-xref-modal-dialog c-xref-confirmation-modal-buffer (format
                                                                     "%s

---
Yes.
No.
---
" message)
                                   default-line 0 nil c-xref-confirmation-dialog-map dispatch-data))
    (select-window sw)
    (setq res (eq sel yes-line))
    res
    ))

(defun c-xref-server-dispatch-ask-confirmation (ss i len dispatch-data tag)
  (let ((tlen) (cc) (conf))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag tag)
    (setq conf (c-xref-yes-or-no-window cc t dispatch-data))
    (if (not conf)
            (progn
              (if c-xref-debug-mode (message "exiting: %s" tag))
              (error "exiting")
              )
      )
    i
    ))

(defun c-xref-server-dispatch-bottom-information (ss i len dispatch-data tag)
  (let ((tlen) (cc) (beep))
    (setq beep (c-xref-server-dispatch-get-int-attr c-xref_PPCA_BEEP))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag tag)
    (message "%s" cc)
    (if (not (equal beep 0)) (beep t))
    i
    ))

(defun c-xref-server-dispatch-insert-completion (cc )
  (let ((cb) (i) (len) (id) (ccc))
    (setq cb (current-buffer))
    (c-xref-select-dispach-data-caller-window c-xref-completions-dispatch-data)
    (c-xref-insert-completion cc)
    (set-buffer cb)
    ))

(defun c-xref-server-dispatch-single-completion (ss i len dispatch-data)
  (let ((tlen) (cc))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (c-xref-server-dispatch-insert-completion cc)
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_SINGLE_COMPLETION)
    i
    ))

(defun c-xref-hard-prepend-to-dispatch-data (dispatch-data new-data)
  (let ((p) (dd))
    (if (not dispatch-data)
            (error "[c-xref] appending to empty dispatch-data, internal error")
      )
    ;;(message "hard append: %S to %S" new-data dispatch-data)
    (setq dd (cdr dispatch-data))
    (if new-data
            (progn
              (setcdr dispatch-data new-data)
              (setq p dispatch-data)
              (while (cdr p) (setq p (cdr p)))
              (setcdr p dd)
              ))
    ))

(defun c-xref-server-dispatch-set-info (ss i len dispatch-data)
  (let ((tlen) (cc))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_SET_INFO)
    (c-xref-hard-prepend-to-dispatch-data dispatch-data (cons (cons 'info cc) nil))
    i
    ))

(defvar c-xref-refactorings-dialog-map (make-sparse-keymap "C-xref refactorings"))
(define-key c-xref-refactorings-dialog-map [up] 'c-xref-modal-dialog-previous-line)
(define-key c-xref-refactorings-dialog-map [down] 'c-xref-modal-dialog-next-line)
(define-key c-xref-refactorings-dialog-map [kp-up] 'c-xref-modal-dialog-previous-line)
(define-key c-xref-refactorings-dialog-map [kp-down] 'c-xref-modal-dialog-next-line)
(define-key c-xref-refactorings-dialog-map c-xref-escape-key-sequence 'c-xref-modal-dialog-exit)
(define-key c-xref-refactorings-dialog-map "\C-g" 'c-xref-modal-dialog-exit)
(define-key c-xref-refactorings-dialog-map "q" 'c-xref-modal-dialog-exit)
(define-key c-xref-refactorings-dialog-map [f7] 'c-xref-modal-dialog-exit)
(define-key c-xref-refactorings-dialog-map [newline] 'c-xref-modal-dialog-continue)
(define-key c-xref-refactorings-dialog-map [return] 'c-xref-modal-dialog-continue)
(define-key c-xref-refactorings-dialog-map "\C-m" 'c-xref-modal-dialog-continue)
(define-key c-xref-refactorings-dialog-map "?" 'c-xref-refactorings-dialog-help)

(defvar c-xref-info-dialog-map (make-sparse-keymap "C-xref refactorings"))
(define-key c-xref-info-dialog-map [newline] 'c-xref-modal-dialog-continue)
(define-key c-xref-info-dialog-map [return] 'c-xref-modal-dialog-continue)
(define-key c-xref-info-dialog-map "\C-m" 'c-xref-modal-dialog-continue)

(defun c-xref-get-line-content ()
  (let ((res) (bb) (pos))
    (setq pos (point))
    (beginning-of-line)
    (setq bb (point))
    (end-of-line)
    (setq res (buffer-substring bb (point)))
    (goto-char pos)
    res
    ))

(defun c-xref-refactorings-dialog-help (event)
  (interactive "i")
  (let ((crf) (rr) (r) (doc))
    (setq crf (c-xref-get-line-content))
    ;;(message "looking for %s" crf)
    (setq rr c-xref-refactorings-assoc-list)
    (setq r nil)
    (while rr
      (setq r (car rr))
      (if (equal (car (cdr r)) crf)
              (progn
                (setq doc r)
                ))
      (setq rr (cdr rr))
      )
    (c-xref-interactive-help (format
                              "Select a refactoring to perform.
Special hotkeys available:

\\[c-xref-modal-dialog-continue] \t-- select
\\[c-xref-modal-dialog-exit] \t-- cancel
\\[c-xref-refactorings-dialog-help] \t-- this help
%s
" (c-xref-refactoring-documentation))
                                         (format "%s:" crf)
                                         (list
                                          (list "^\\*[a-zA-Z0-9 ]*:" 'bold)
                                          (list "^[a-zA-Z0-9 ]*:" 'bold-italic)
                                          (list "--*-" 'c-xref-list-symbol-face)
                                          ;;(list (c-xref-keywords-regexp) 'c-xref-keyword-face)
                                          )
                                         )
    ))


(defun c-xref-server-dispatch-available-refactorings (ss i len dispatch-data)
  (let ((code) (selectedref) (srefn) (selectedline) (refs) (menu)
            (tlen) (cc) (rfirst-line 3) (dd2))
    (setq refs nil)
    ;; unique completion window in all frames
    (setq menu "Select action to perform:\n---")
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (while (equal c-xref-server-ctag c-xref_PPC_INT_VALUE)
      (setq code (c-xref-server-dispatch-get-int-attr c-xref_PPCA_VALUE))
      (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
      (setq menu (format "%s\n%s" menu (car (cdr (assoc code c-xref-refactorings-assoc-list)))))
      (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
      (setq i (+ i tlen))
      (setq refs (append refs (list (list code cc))))
      (setq i (c-xref-server-parse-xml-tag ss i len))
      (c-xref-server-dispatch-require-end-ctag c-xref_PPC_INT_VALUE)
      (setq i (c-xref-server-parse-xml-tag ss i len))
      )
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_AVAILABLE_REFACTORINGS)
    (setq menu (format "%s\n---\n" menu))
    (setq selectedline (c-xref-modal-dialog c-xref-selection-modal-buffer menu rfirst-line 0 nil c-xref-refactorings-dialog-map dispatch-data))
    (setq srefn (elt refs (- selectedline rfirst-line)))
    (setq selectedref (assoc (car srefn) c-xref-refactorings-assoc-list))
    (apply (elt selectedref 2) srefn nil)
    i
    ))

(defun c-xref-server-dispatch-create-and-set-completions-buffer (dispatch-data)
  (let ((cid))
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (setq cid (c-xref-get-identifier-before-point))
    (setq c-xref-completion-id-before-point cid)
    (setq c-xref-completion-id-after-point (c-xref-get-identifier-after (point)))
    (setq c-xref-completion-auto-search-list (list cid (+ (length cid) 1)))
    (set-marker c-xref-completion-marker (- (point) (length cid)))
    (c-xref-display-and-set-new-dialog-window c-xref-completions-buffer nil t)
    (buffer-disable-undo c-xref-completions-buffer)
    (setq c-xref-this-buffer-type 'completion)
    ))

(defun c-xref-server-dispatch-show-completions-buffer (focus dispatch-data)
  (c-xref-auto-search-search (car c-xref-completion-auto-search-list))
  (c-xref-use-local-map c-xref-completion-mode-map)
  (setq buffer-read-only t)
  (c-xref-appropriate-window-height nil t)
  (if (and focus c-xref-completion-auto-focus)
      (message c-xref-standard-help-message)
    (c-xref-select-dispach-data-caller-window dispatch-data)
    )
  )

(defun c-xref-reset-or-increment-completion-windows-counter ()
  (let ((win))
    (setq win (get-buffer-window c-xref-completions-buffer))
    (if (and win (windowp win) (window-live-p win))
            (setq c-xref-completions-windows-counter (1+ c-xref-completions-windows-counter))
      (setq c-xref-completions-windows-counter 0)
      )
    ))

(defun c-xref-server-dispatch-all-completions (ss i len dispatch-data)
  (let ((tlen) (cc) (nofocus))
    ;; unique completion window in all frames
    (c-xref-reset-or-increment-completion-windows-counter)
    (c-xref-delete-window-in-any-frame c-xref-completions-buffer nil)
    (c-xref-server-dispatch-create-and-set-completions-buffer dispatch-data)
    (setq c-xref-this-buffer-dispatch-data dispatch-data)
    (setq nofocus (c-xref-server-dispatch-get-int-attr c-xref_PPCA_NO_FOCUS))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (insert cc)
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_ALL_COMPLETIONS)
    (c-xref-line-hightlight 0 (point-max) nil 1 c-xref-font-lock-compl-keywords t)
    (goto-char (point-min))
    (c-xref-server-dispatch-show-completions-buffer (eq nofocus 0) dispatch-data)
    i
    ))

(defun c-xref-server-dispatch-goto (ss i len dispatch-data)
  (let ((tlen) (path) (offset) (line) (col) (rpos))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq path (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (if (equal c-xref-server-ctag c-xref_PPC_OFFSET_POSITION)
            (progn
              (setq offset (c-xref-server-dispatch-get-int-attr c-xref_PPCA_OFFSET))
              (c-xref-select-dispach-data-caller-window dispatch-data)
              (c-xref-find-file path)
              (goto-char (+ offset 1))
              (setq i (c-xref-server-parse-xml-tag ss i len))
              (c-xref-server-dispatch-require-end-ctag c-xref_PPC_OFFSET_POSITION)
              )
      (c-xref-server-dispatch-require-ctag c-xref_PPC_LC_POSITION)
      (setq line (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LINE))
      (setq col (c-xref-server-dispatch-get-int-attr c-xref_PPCA_COL))
      (c-xref-select-dispach-data-caller-window dispatch-data)
      (c-xref-find-file path)
      (c-xref-goto-line line)
      (beginning-of-line)
      (setq rpos (+ (point) col))
      (end-of-line)
      (if (> rpos (point))
              (setq rpos (point))
            )
      (goto-char rpos)
      (setq i (c-xref-server-parse-xml-tag ss i len))
      (c-xref-server-dispatch-require-end-ctag c-xref_PPC_LC_POSITION)
      )
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_GOTO)
    i
    ))

(defun c-xref-server-dispatch-update-report (ss i len dispatch-data)
  (let ((tlen) (cc))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (while (and (< i len) (not (equal c-xref-server-ctag (format "/%s" c-xref_PPC_UPDATE_REPORT))))
      (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
      (if (equal c-xref-server-ctag c-xref_PPC_FATAL_ERROR)
              (c-xref-server-dispatch-error ss i len dispatch-data c-xref_PPC_FATAL_ERROR)
            )
      (setq i (+ i tlen))
      (setq i (c-xref-server-parse-xml-tag ss i len))
      )
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_UPDATE_REPORT)
    i
    ))

(defun c-xref-server-dispatch-parse-string-value (ss i len dispatch-data)
  (let ((tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-ctag c-xref_PPC_STRING_VALUE)
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq c-xref-server-cstring-value (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_STRING_VALUE)
    i
    ))

(defun c-xref-server-dispatch-replacement (ss i len dispatch-data)
  (let ((tlen) (str) (with) (slen) (rcc))
    (setq i (c-xref-server-dispatch-parse-string-value ss i len dispatch-data))
    (setq str c-xref-server-cstring-value)
    (setq i (c-xref-server-dispatch-parse-string-value ss i len dispatch-data))
    (setq with c-xref-server-cstring-value)
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_REFACTORING_REPLACEMENT)
    (setq slen (length str))
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (setq rcc (buffer-substring (point) (+ (point) slen)))
    (if (not (equal str rcc))
            (error "[internal error] '%s' expected here" str)
      )
    (c-xref-make-buffer-writable)
    (delete-char slen)
    (insert with)
    i
    ))

(defun c-xref-server-dispatch-refactoring-cut-block (ss i len dispatch-data)
  (let ((blen))
    (setq blen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_VALUE))
    (setq c-xref-refactoring-block (buffer-substring (point) (+ (point) blen)))
    (delete-char blen t)
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_REFACTORING_CUT_BLOCK)
    i
    ))

(defun c-xref-server-dispatch-refactoring-paste-block (ss i len dispatch-data)
  (insert c-xref-refactoring-block)
  (setq i (c-xref-server-parse-xml-tag ss i len))
  (c-xref-server-dispatch-require-end-ctag c-xref_PPC_REFACTORING_PASTE_BLOCK)
  i
  )

(defun c-xref-server-dispatch-indent (ss i len dispatch-data)
  (let ((blen) (ii) (bb))
    (setq blen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_VALUE))
    (if (fboundp 'indent-region)
            (progn
              (save-excursion
                (setq bb (point))
                (forward-line blen)
                (indent-region bb (point) nil)
                )))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_INDENT)
    i
    ))

(defun c-xref-server-dispatch-precheck (ss i len dispatch-data)
  (let ((tlen) (cc) (rcc))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (setq rcc (buffer-substring (point) (+ (point) tlen)))
    ;; (message "prechecking '%s' <-> '%s'" cc rcc)
    (if (not (equal cc rcc))
            (error "[error] '%s' expected here" cc)
      )
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_REFACTORING_PRECHECK)
    i
    ))

(defun c-xref-create-browser-main-window (buff split-horizontal docking-top-left dispatch-data)
  (let ((sw) (nw) (res))
    (if docking-top-left
            (progn
              (setq sw (selected-window))
              (setq nw (c-xref-display-and-set-new-dialog-window buff split-horizontal nil))
              ;; exchange caller window in dispatch-data
              (c-xref-reset-cdr 'caller-window sw nw dispatch-data)
              (c-xref-reset-all-windows-caller-window sw nw)
              (c-xref-set-window-width c-xref-window-width)
              (setq res sw)
              )
      (setq nw (c-xref-display-and-set-new-dialog-window buff split-horizontal t))
      (setq res nw)
      )
    res
    ))

(defun c-xref-cr-new-symbol-resolution-window (force-bottom-position dispatch-data)
  (let ((res))
    (if force-bottom-position
            (setq res (c-xref-create-browser-main-window (c-xref-new-symbol-resolution-buffer)
                                                                             nil nil
                                                                             dispatch-data))
      (setq res (c-xref-create-browser-main-window (c-xref-new-symbol-resolution-buffer)
                                                                           c-xref-browser-splits-window-horizontally
                                                                           c-xref-browser-position-left-or-top
                                                                           dispatch-data))
      )
    res
    ))

(defun c-xref-cr-new-references-window (force-bottom-position)
  (let ((res) (sw))
    (if force-bottom-position
            (setq res (c-xref-display-and-set-new-dialog-window (c-xref-new-references-buffer)
                                                                                        t t))
      (setq res (c-xref-display-and-set-new-dialog-window (c-xref-new-references-buffer)
                                                                                      (not c-xref-browser-splits-window-horizontally)
                                                                                      t))
      )
    ;; scrolling in Emacs is mysterious. Set up some values
                                        ; (setq sw (selected-window))
                                        ; (select-window res)
                                        ; (setq scroll-margin 2)
                                        ; (setq scroll-conservatively 1)
                                        ; (setq scroll-step 1)
                                        ; (select-window sw)
    ;;
    res
    ))

(defun c-xref-valid-window (win bnameprefix)
  (let ((res))
    (setq res nil)
    (if (and win (windowp win) (window-live-p win))
        (progn
              (if (c-xref-string-has-prefix (buffer-name (window-buffer win))
                                                        bnameprefix nil)
                  (setq res t)
                )))
    res
    ))


(defun c-xref-clean-unused-browser-buffers ()
  (let ((bl) (bb) (bn) (win))
    (setq bl (buffer-list))
    (while bl
      (setq bb (car bl))
      (setq bn (buffer-name bb))
      (if (or (c-xref-string-has-prefix bn c-xref-symbol-resolution-buffer nil)
                  (c-xref-string-has-prefix bn c-xref-references-buffer nil))
              (progn
                (setq win (get-buffer-window bb t))
                (if (or (not win) (not (windowp win)) (not (window-live-p win)))
                        (kill-buffer bb)
                  )))

      (setq bl (cdr bl))
      )
    ))

(defun c-xref-create-browser-windows (refactoring-resolution-flag dispatch-data)
  (let ((resolvewin) (listwin) (oldwins) (frame-id) (new-dispatch-data) (frw))
    (if (c-xref-get-this-frame-dispatch-data)
            (progn
              (setq resolvewin (cdr (assoc 'linked-resolution-window (c-xref-get-this-frame-dispatch-data))))
              (setq listwin (cdr (assoc 'linked-refs-window (c-xref-get-this-frame-dispatch-data))))
              (cond
               (
                (and (c-xref-valid-window resolvewin c-xref-symbol-resolution-buffer)
                         (c-xref-valid-window listwin c-xref-references-buffer))
                ;; do nothing as both windows exists
                nil
                )
               (
                (c-xref-valid-window resolvewin c-xref-symbol-resolution-buffer)
                ;; only symbol window exists
                (select-window resolvewin)
                (setq listwin (c-xref-cr-new-references-window refactoring-resolution-flag))
                )
               (
                (c-xref-valid-window listwin c-xref-references-buffer)
                ;; only reference window exists, hmmm.
                (setq resolvewin listwin)
                (select-window resolvewin)
                (rename-buffer (c-xref-new-symbol-resolution-buffer))
                (setq listwin (c-xref-cr-new-references-window refactoring-resolution-flag))
                )
               (t
                (setq resolvewin (c-xref-cr-new-symbol-resolution-window refactoring-resolution-flag dispatch-data))
                (setq listwin (c-xref-cr-new-references-window refactoring-resolution-flag))
                )))
      (setq resolvewin (c-xref-cr-new-symbol-resolution-window refactoring-resolution-flag dispatch-data))
      (setq listwin (c-xref-cr-new-references-window refactoring-resolution-flag))
      )
    ;; maybe I should erase them here, but check before that
    ;; buffer is the one created by c-xref, (user may load his source here)!!
    (select-window listwin)
    (setq c-xref-this-buffer-type 'reference-list)
    (setq c-xref-this-buffer-dispatch-data dispatch-data)
    (c-xref-use-local-map c-xref-browser-dialog-refs-key-map)
    (select-window resolvewin)
    (setq c-xref-this-buffer-type 'symbol-list)
    (setq c-xref-this-buffer-dispatch-data dispatch-data)
    (c-xref-use-local-map c-xref-browser-dialog-key-map)
    ;; add information about windows to dispatch-data
    ;; big hack, if there is some problem with browser/refactoring
    ;; windows, the problem is probably somewhere here!
    (setq frame-id (c-xref-get-this-frame-id))
    (if refactoring-resolution-flag
            (progn
              (c-xref-set-this-frame-dispatch-data
               (cons (cons 'linked-refactoring-window listwin)
                         (c-xref-get-this-frame-dispatch-data)))
              ))
    (c-xref-hard-prepend-to-dispatch-data
     dispatch-data
     (cons (cons 'linked-refs-window listwin)
               (cons (cons 'linked-resolution-window resolvewin)
                         (cons (cons 'frame-id frame-id)
                               nil))))
    (if (not refactoring-resolution-flag)
            (progn
              (setq frw (c-xref-is-failed-refactoring-window-displayed))
              (if frw
                  (c-xref-hard-prepend-to-dispatch-data
                   dispatch-data
                   (cons (cons 'linked-refactoring-window frw) nil)
                   ))
              (c-xref-set-this-frame-dispatch-data dispatch-data)
          ))
    oldwins
    ))

(defun c-xref-appropriate-other-window-height (win aplus aminus)
  (let ((sw))
    (setq sw (selected-window))
    (select-window win)
    (c-xref-appropriate-window-height nil t)
    (select-window sw)
    ))

(defun c-xref-server-dispatch-display-resolution (ss i len dispatch-data)
  (let ((tlen) (mess) (messagewin) (winconfig) (fdd))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq mess (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_DISPLAY_RESOLUTION)
    (c-xref-select-dispach-data-caller-window dispatch-data)
    ;; this window excursion causes that after reporting missinterpreted references
    ;; the windows are lost for further browsing, try it differently
    (setq fdd (c-xref-get-this-frame-dispatch-data))
    (setq winconfig (current-window-configuration))
    (delete-other-windows)
    (setq messagewin (c-xref-display-and-set-new-dialog-window c-xref-browser-info-buffer nil t))
    (insert mess)
    (insert c-xref-resolution-dialog-explication)
    (goto-char (point-min))
    (c-xref-create-browser-windows t dispatch-data)
    (c-xref-browser-dialog-set-new-filter dispatch-data)
    (c-xref-appropriate-browser-windows-sizes nil)
    (c-xref-appropriate-other-window-height messagewin t t)
    (c-xref-modal-dialog-loop c-xref-browser-dialog-key-map " ? - help; q - quit modal dialog")
    (c-xref-close-resolution-dialog-windows dispatch-data)
    (sit-for 0.1)
    (set-window-configuration winconfig)
    (c-xref-set-this-frame-dispatch-data fdd)
    ;; send nothing to continue
    (c-xref-send-data-to-process-and-dispatch "-continuerefactoring" dispatch-data nil)
    i
    ))

(defun c-xref-server-dispatch-display-or-update-browser (ss i len dispatch-data)
  (let ((tlen) (mess) (messagewin))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq mess (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_DISPLAY_OR_UPDATE_BROWSER)
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (c-xref-create-browser-windows nil dispatch-data)
    ;; do not update content, it is done automatically after
    ;;(c-xref-browser-dialog-set-new-filter dispatch-data)
    (message mess)
    i
    ))

(defun c-xref-server-dispatch-symbol-resolution (ss i len dispatch-data)
  (let ((tlen) (symbol) (clas) (indent) (deps) (ru) (rd)
            (selected) (sch) (divnewline) (firstline))
    (setq divnewline "")
    (setq firstline 0)
    (c-xref-select-dispach-data-resolution-window dispatch-data)
    (setq buffer-read-only nil)
    (c-xref-erase-buffer)
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (while (equal c-xref-server-ctag c-xref_PPC_SYMBOL)
      (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
      (setq symbol (c-xref-char-list-substring ss i (+ i tlen)))
      (setq i (+ i tlen))
          (setq divnewline "\n")
          (setq selected (c-xref-server-dispatch-get-int-attr c-xref_PPCA_SELECTED))
          (setq rd (c-xref-server-dispatch-get-int-attr c-xref_PPCA_DEF_REFN))
          (setq ru (c-xref-server-dispatch-get-int-attr c-xref_PPCA_REFN))
          (if (equal selected 1) (setq sch "+")
        (setq sch " "))
          (insert (format "%s%s  %s  %s" divnewline sch (c-xref-references-counts rd ru) symbol))
          (if (<= firstline 0)
          (setq firstline (count-lines 1 (point))))
          (setq i (c-xref-server-parse-xml-tag ss i len))
          (c-xref-server-dispatch-require-end-ctag c-xref_PPC_SYMBOL)
          (setq i (c-xref-server-parse-xml-tag ss i len))
      (setq divnewline "\n")
      )
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_SYMBOL_RESOLUTION)
    (c-xref-line-hightlight 0 (point-max) nil 1 nil nil)
    (goto-char (point-min))
    (if (> firstline 0)
        (c-xref-goto-line firstline))
    (setq buffer-read-only t)
    i
    ))

(defun c-xref-reset-cdr (attr oldval newval dispatch-data)
  (let ((ss))
    (setq ss (assoc attr dispatch-data))
    (if (and ss (equal (cdr ss) oldval))
            (setcdr ss newval)
      )
    ))

(defun c-xref-reset-all-windows-caller-window (old-caller-window new-caller-window)
  (let ((sw) (loop))
    (setq sw (selected-window))
    (setq loop t)
    (while loop
      (if c-xref-this-buffer-dispatch-data
              (c-xref-reset-cdr 'caller-window
                                        old-caller-window
                                        new-caller-window
                                        c-xref-this-buffer-dispatch-data)
            )
      (other-window 1)
      (setq loop (not (equal sw (selected-window))))
      )
    (if (c-xref-get-this-frame-dispatch-data)
            (c-xref-reset-cdr 'caller-window
                                      old-caller-window
                                      new-caller-window
                                      (c-xref-get-this-frame-dispatch-data))
      )
    ))

(defun c-xref-server-dispatch-reference-list (ss i len dispatch-data)
  (let ((tlen) (srcline) (n) (divnewline) (aline) (line) (j) (pointer)
            (listed-symbol))
    (setq j 0)
    (setq divnewline "")
    (setq aline (c-xref-server-dispatch-get-int-attr c-xref_PPCA_VALUE))
    (setq listed-symbol (cdr (assoc c-xref_PPCA_SYMBOL c-xref-server-ctag-attributes)))
    (if (equal listed-symbol "")
            (setq listed-symbol nil)
      )
    (setq line 0)
    (c-xref-select-dispach-data-refs-window dispatch-data)
    (setq buffer-read-only nil)
    (c-xref-erase-buffer)
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (while (equal c-xref-server-ctag c-xref_PPC_SRC_LINE)
      (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
      (setq srcline (c-xref-char-list-substring ss i (+ i tlen)))
      (setq i (+ i tlen))
      (setq n (c-xref-server-dispatch-get-int-attr c-xref_PPCA_REFN))
      (while (> n 0)
            (progn
              (if (eq line aline)
                  (setq pointer ">")
                (setq pointer " ")
                )
              (insert (format "%s%s%s" divnewline pointer srcline))
              (setq divnewline "\n")
              (setq line (+ line 1))
              (setq n (- n 1))
              ))
      (setq i (c-xref-server-parse-xml-tag ss i len))
      (c-xref-server-dispatch-require-end-ctag c-xref_PPC_SRC_LINE)
      (setq i (c-xref-server-parse-xml-tag ss i len))
      )
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_REFERENCE_LIST)
    (c-xref-line-hightlight 0 (point-max) nil 1 (c-xref-font-lock-list-keywords listed-symbol) nil)
    (goto-char (point-min))
    (setq buffer-read-only t)
    i
    (+ i j)
    ))

(defun c-xref-server-dispatch-symbol-list (ss i len dispatch-data)
  (let ((tlen) (sym) (sl) (ll) (j))
    (setq j 0)
    (setq sl (cons 'begin nil))
    (setq ll sl)
    (setq i (c-xref-server-parse-xml-tag ss i len))
    ;;(message "start %S len %d" (current-time) len)
    (while (equal c-xref-server-ctag c-xref_PPC_STRING_VALUE)
      (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
      (setq sym (c-xref-char-list-substring ss i (+ i tlen)))
      (setq i (+ i tlen))
      (setcdr ll (cons sym nil))
      (setq ll (cdr ll))
      (setq i (c-xref-server-parse-xml-tag ss i len))
      (c-xref-server-dispatch-require-end-ctag c-xref_PPC_STRING_VALUE)
      (setq i (c-xref-server-parse-xml-tag ss i len))
      )
    ;;(message "stop %S" (current-time))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_SYMBOL_LIST)
    (c-xref-hard-prepend-to-dispatch-data
     dispatch-data
     (cons (cons 'symbol-list (cdr sl)) nil))
    (+ i j)
    ))


(defun c-xref-server-dispatch-move-file-as (ss i len dispatch-data)
  (let ((tlen) (cc) (rcc))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (c-xref-undoable-move-file cc)
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_MOVE_FILE_AS)
    i
    ))

(defun c-xref-server-dispatch-extraction-dialog (ss i len dispatch-data)
  (let ((tlen) (minvocation) (mhead) (mtail) (mline) (dname))
    (setq dname (cdr (assoc c-xref_PPCA_TYPE c-xref-server-ctag-attributes)))

    ;; Invocation code string
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-ctag c-xref_PPC_STRING_VALUE)
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq minvocation (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_STRING_VALUE)

    ;; Extraction header
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-ctag c-xref_PPC_STRING_VALUE)
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq mhead (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_STRING_VALUE)

    ;; Extraction tail
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-ctag c-xref_PPC_STRING_VALUE)
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq mtail (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_STRING_VALUE)

    ;; ?????
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-ctag c-xref_PPC_INT_VALUE)
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq mline (c-xref-server-dispatch-get-int-attr c-xref_PPCA_VALUE))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_INT_VALUE)

    (c-xref-extraction-dialog minvocation mhead mtail mline dname)

    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag c-xref_PPC_EXTRACTION_DIALOG)
    i
    ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DISPATCH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-server-dispatch (ss i len dispatch-data)
  (let ((x) (j))
    (setq j 0)
    (if c-xref-debug-mode (message "dispatching: %s" ss))
    (setq i (c-xref-server-dispatch-skip-blank ss i len))
    (while (and (< i len))  ;; (eq (elt ss i) ?<))
      (setq i (c-xref-server-parse-xml-tag ss i len))
      ;;(message "tag == %s" c-xref-server-ctag)
      (cond
       (
            (equal c-xref-server-ctag c-xref_PPC_SET_INFO)
            (setq i (c-xref-server-dispatch-set-info ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_ALL_COMPLETIONS)
            (setq i (c-xref-server-dispatch-all-completions ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_SINGLE_COMPLETION)
            (setq i (c-xref-server-dispatch-single-completion ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_GOTO)
            (setq i (c-xref-server-dispatch-goto ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_AVAILABLE_REFACTORINGS)
            (setq i (c-xref-server-dispatch-available-refactorings ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_UPDATE_REPORT)
            (setq i (c-xref-server-dispatch-update-report ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_REFACTORING_PRECHECK)
            (setq i (c-xref-server-dispatch-precheck ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_REFACTORING_REPLACEMENT)
            (setq i (c-xref-server-dispatch-replacement ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_REFACTORING_CUT_BLOCK)
            (setq i (c-xref-server-dispatch-refactoring-cut-block ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_REFACTORING_PASTE_BLOCK)
            (setq i (c-xref-server-dispatch-refactoring-paste-block ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_INDENT)
            (setq i (c-xref-server-dispatch-indent ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_DISPLAY_RESOLUTION)
            (setq i (c-xref-server-dispatch-display-resolution ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_SYMBOL_RESOLUTION)
            (setq i (c-xref-server-dispatch-symbol-resolution ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_DISPLAY_OR_UPDATE_BROWSER)
            (setq i (c-xref-server-dispatch-display-or-update-browser ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_REFERENCE_LIST)
            (setq i (c-xref-server-dispatch-reference-list ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_SYMBOL_LIST)
            (setq i (c-xref-server-dispatch-symbol-list ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_MOVE_FILE_AS)
            (setq i (c-xref-server-dispatch-move-file-as ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_EXTRACTION_DIALOG)
            (setq i (c-xref-server-dispatch-extraction-dialog ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_CALL_MACRO)
            (setq i (c-xref-server-dispatch-call-macro ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_KILL_BUFFER_REMOVE_FILE)
            (setq i (c-xref-server-dispatch-kill-buffer-remove-file ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_NO_PROJECT)
            (setq i (c-xref-server-dispatch-no-project ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_PROJECT_MISMATCH)
            (setq i (c-xref-server-dispatch-project-mismatch ss i len dispatch-data)))
       (
            (equal c-xref-server-ctag c-xref_PPC_ASK_CONFIRMATION)
            (setq i (c-xref-server-dispatch-ask-confirmation ss i len dispatch-data c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_INFORMATION)
            (setq i (c-xref-server-dispatch-information ss i len dispatch-data c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_BOTTOM_INFORMATION)
            (setq i (c-xref-server-dispatch-bottom-information ss i len dispatch-data c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_WARNING)
            (setq i (c-xref-server-dispatch-information ss i len dispatch-data c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_BOTTOM_WARNING)
            (setq i (c-xref-server-dispatch-bottom-information ss i len dispatch-data c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_ERROR)
            (setq i (c-xref-server-dispatch-error ss i len dispatch-data c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_FATAL_ERROR)
            (setq i (c-xref-server-dispatch-error ss i len dispatch-data c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_DEBUG_INFORMATION)
            (setq i (c-xref-server-dispatch-information ss i len dispatch-data c-xref-server-ctag)))
       (
            t
            (error "unknown tag: %s" c-xref-server-ctag))
           ;;(message "unknown tag: %s" c-xref-server-ctag))
       )
      (setq i (c-xref-server-dispatch-skip-blank ss i len))
      )
    (+ i j)
    ))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;; MODAL DIALOG ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-modal-dialog-sentinel ()
  (let ((pp) (res))
    (setq pp (point))
    (setq res nil)
    (if (and (eq (char-after pp) ?-)
                 (eq (char-after (+ pp 1)) ?-)
                 (eq (char-after (+ pp 2)) ?-))
            (setq res t)
      )
    res
    ))

(defun c-xref-modal-dialog-page-up (event)
  (interactive "i")
  (condition-case nil
      (scroll-down nil)
    (error nil)
    )
  )

(defun c-xref-modal-dialog-page-down (event)
  (interactive "i")
  (condition-case nil
      (scroll-up nil)
    (error nil)
    )
  )

(defun c-xref-modal-dialog-previous-line-no-sentinel (event)
  (interactive "i")
  (condition-case nil
      (forward-line -1)
    (error nil)
    )
  )

(defun c-xref-modal-dialog-next-line-no-sentinel (event)
  (interactive "i")
  (condition-case nil
      (forward-line 1)
    (error nil)
    )
  )

(defun c-xref-modal-dialog-previous-line (event)
  (interactive "i")
  (forward-line -1)
  (if (c-xref-modal-dialog-sentinel) (forward-line 1))
  (beginning-of-line)
  )

(defun c-xref-modal-dialog-next-line (event)
  (interactive "i")
  (forward-line 1)
  (if (c-xref-modal-dialog-sentinel) (forward-line -1))
  (beginning-of-line)
  )

(defun c-xref-modal-dialog-exit (event)
  (interactive "i")
  (let ((winassoc) (dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (delete-window (selected-window))
    (setq winassoc (assoc 'caller-window dispatch-data))
    (if winassoc (select-window (cdr winassoc)))
    (error "Cancel")
    ))

(defun c-xref-modal-dialog-loop (keymap mess)
  (let ((event) (ev) (key))
    (setq ev (c-xref-read-key-sequence mess))
    (setq event (elt ev 0))
    (setq key (lookup-key keymap ev))
    (message mess)
    (while (not (eq key 'c-xref-modal-dialog-continue))
      (if key
              (apply key event nil)
            (message "Invalid key")
            )
      (condition-case nil
              (progn
                (setq ev (c-xref-read-key-sequence mess))
                (setq event (elt ev 00))
                (setq key (lookup-key keymap ev))
                (message mess)
                )
            (error nil)
            )
      )
    ))

(defun c-xref-modal-dialog (title text line col blick keymap dispatch-data)
  (let ((key) (res) (win) (owin))
    (setq owin (selected-window))
    (setq win (c-xref-display-and-set-new-dialog-window title nil t))
    (insert text)
    (c-xref-use-local-map keymap)
    (c-xref-goto-line line)
    (c-xref-appropriate-window-height nil t)
    (goto-char (+ (point) col))
    (setq c-xref-this-buffer-dispatch-data dispatch-data)
    (if blick
            (progn
              (other-window -1)
              (sit-for 1)
              (other-window 1)
              ))
    (c-xref-modal-dialog-loop keymap "? - help")
    (setq res (count-lines 1 (+ (point) 1)))
    (delete-window win)
    (select-window owin)
    res
    ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;    Projects    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-split-path-on-path-list (pname cut-slash)
  (let ((lplist) (i) (path) (len) (lchar))
    ;; cut project path
    (setq lplist nil)
    (setq i (length pname))
    (while (> i 0)
      (setq i (- i 1))
      (if (eq (elt pname i) c-xref-path-separator)
              (progn
                (setq path (substring pname (+ i 1)))
                (if cut-slash (setq path (c-xref-remove-pending-slash path)))
                (setq lplist (append lplist (cons path nil)))
                (setq pname (substring pname 0 i))
                )))
    (setq lplist (append lplist (cons pname nil)))
    lplist
    ))


(defun c-xref-get-project-list ()
  (let ((new-name) (loop) (mbeg) (mend) (pname "") (project-list) (i) (len) (lplist))
    (with-current-buffer
            (get-buffer-create " c-xref-project-list")
      ;;(c-xref-erase-buffer)
      (insert-file-contents c-xref-options-file  nil nil nil t)
      (goto-char (point-min))
      (setq project-list nil)
      (setq loop t)
      (while loop
            (setq loop (search-forward-regexp "\\[\\([^\]]*\\)\\]"
                                                              (buffer-size) 1))
            (if loop
                (progn
                  (setq mbeg (match-beginning  1))
                  (setq mend (match-end  1))
                  (setq pname (buffer-substring mbeg mend))
                  (setq lplist (c-xref-split-path-on-path-list pname nil))
                  (setq project-list (append lplist project-list))
                  )))
      (kill-buffer nil)
      )
    project-list
    ))

(defun c-xref-prj-list-get-prj-on-line ()
  (let ((res) (ppp) (bl) (el))
    (setq ppp (point))
    (beginning-of-line)
    (setq bl (point))
    (end-of-line)
    (setq el (point))
    (setq res (buffer-substring (+ bl 2) el))
    (goto-char ppp)
    res
    ))

(defun c-xref-interactive-project-select (&optional argp)
  "Go to the reference corresponding to this line."
  (interactive "P")
  (let ((bl) (el) (prj))
    (setq prj (c-xref-prj-list-get-prj-on-line))
    (if (string-equal prj c-xref-directory-dep-prj-name)
            (progn
              (setq c-xref-current-project nil)
              ;; and reseting of softly selected project
              (c-xref-softly-preset-project "")
              )
      (setq c-xref-current-project prj)
      )
    (c-xref-delete-window-in-any-frame  c-xref-project-list-buffer nil)
    (message "Project '%s' is active." prj)
    )
  t
  )

(defun c-xref-interactive-m-project-select (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-project-select)
  (other-window -1)
  t
  )

(defun c-xref-interactive-project-escape (&optional argp)
  "Escape from the project selection window."
  (interactive "P")
  (c-xref-delete-window-in-any-frame  c-xref-project-list-buffer nil)
  )

(defvar c-xref-project-list-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\e" 'c-xref-interactive-project-escape)
    (define-key map "q" 'c-xref-interactive-project-escape)
    (define-key map "\C-m" 'c-xref-interactive-project-select)
    ;;    (define-key map " " 'c-xref-interactive-project-select)
    (define-key map "?" 'c-xref-interactive-project-selection-help)
    (c-xref-bind-default-button map 'c-xref-interactive-m-project-select)
    map)
  "Keymap for `c-xref-project-list-mode'."
  )
(c-xref-add-bindings-to-keymap c-xref-project-list-mode-map)

(defun c-xref-interactive-project-selection-help ()
  (interactive "")
  (c-xref-interactive-help
   "Special hotkeys available:

\\[c-xref-interactive-project-select] \t-- select project
\\[c-xref-interactive-project-escape] \t-- close
\\[c-xref-interactive-project-selection-help] \t-- toggle this help page
" nil nil)
  )

(defun c-xref-display-project-list (last-project local-keymap)
  (let ((c-xref-project-list) (dd))
    (c-xref-delete-window-in-any-frame c-xref-project-list-buffer nil)
    (setq dd (c-xref-get-basic-server-dispatch-data 'c-xref-server-process))
    (setq c-xref-this-buffer-dispatch-data dd)
    (setq c-xref-project-list (c-xref-get-project-list))
    (c-xref-display-and-set-new-dialog-window c-xref-project-list-buffer nil t)
    (setq c-xref-this-buffer-dispatch-data dd)
    (insert "  ")
    (insert last-project)
    (put-text-property 3 (point) 'mouse-face 'highlight)
    (newline)
    (while c-xref-project-list
      (goto-char (point-min))
      (if (string-equal c-xref-current-project (car c-xref-project-list))
              (insert "> ")
            (insert "  ")
            )
      (insert (car c-xref-project-list))
      (put-text-property 3 (point) 'mouse-face 'highlight)
      (newline)
      (setq c-xref-project-list (cdr c-xref-project-list))
      )
    (setq buffer-read-only t)
    (c-xref-use-local-map local-keymap)
    (message c-xref-standard-help-message)
    ))

(defun c-xref-project-set-active ()
  "Set  active  project.

This function is meningful only if your '.c-xrefrc' file
contains a section defining options for your project. After
setting a project to be active all C-xrefactory functions will
proceed according to options corresponding to this project name.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations-no-project-required)
  (c-xref-display-project-list c-xref-directory-dep-prj-name
                                           c-xref-project-list-mode-map)
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-interactive-project-delete (&optional argp)
  (interactive "P")
  (let ((prj) (eprj) (ppp) (eppp))
    (setq prj (c-xref-prj-list-get-prj-on-line))
    (if (string-equal prj c-xref-abandon-deletion)
            (progn
              (c-xref-delete-window-in-any-frame  c-xref-project-list-buffer nil)
              (message "Deletion canceled.")
              )
      (forward-line)
      (setq eprj (c-xref-prj-list-get-prj-on-line))
      (c-xref-delete-window-in-any-frame  c-xref-project-list-buffer nil)
      (find-file c-xref-options-file)
      (goto-char (point-min))
      (setq ppp (search-forward (format "[%s]" prj) nil t))
      (if (not ppp)
              (error "Project section not found, it's probably sharing options, delete it manually.")
            (setq ppp (- ppp (length (format "[%s]" prj))))
            )
      (if (equal eprj c-xref-abandon-deletion)
              (setq eppp (point-max))
            (setq eppp (search-forward (format "[%s]" eprj) nil t))
            (if (not eppp)
                (setq eppp (search-forward (format "[%s:" eprj) nil t))
              )
            (if (not eppp)
                (error "Can't find end of project section, internal error, sorry.")
              (setq eppp (- eppp (length (format "[%s]" eprj))))
              ))
      (if (> ppp eppp)
              (error "[ppp>eppp] internal check failed, sorry")
            (goto-char ppp)
            (if (yes-or-no-p (format "Really delete %s? " prj))
                (progn
                  (delete-char (- eppp ppp))
                  (save-buffer)
                  (kill-buffer (current-buffer))
                  (message "Project %s has been deleted." prj)
                  )
              (message "No deletion.")
              )
            ))
    t
    ))

(defun c-xref-interactive-m-project-delete (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-project-delete)
  (other-window -1)
  t
  )

(defvar c-xref-project-delete-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\e" 'c-xref-interactive-project-escape)
    (define-key map "q" 'c-xref-interactive-project-escape)
    (define-key map "\C-m" 'c-xref-interactive-project-delete)
    ;; (define-key map " " 'c-xref-interactive-project-delete)
    (define-key map "?" 'c-xref-interactive-project-delete-help)
    (c-xref-bind-default-button map 'c-xref-interactive-m-project-delete)
    map)
  "Keymap for `c-xref-project-delete-mode'."
  )
(c-xref-add-bindings-to-keymap c-xref-project-delete-mode-map)


(defun c-xref-interactive-project-delete-help ()
  (interactive "")
  (c-xref-interactive-help
   "Special hotkeys available:

\\[c-xref-interactive-project-delete] \t-- delete project
\\[c-xref-interactive-project-escape] \t-- close
\\[c-xref-interactive-project-delete-help] \t-- toggle this help page
" nil nil)
  )


(defun c-xref-project-delete ()
  "Delete a project.

This function asks you to select the project you wish to
delete. Then the part of .c-xrefrc file describing this project
will be deleted.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations-no-project-required)
  (c-xref-display-project-list c-xref-abandon-deletion
                                           c-xref-project-delete-mode-map)
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-remove-pending-slash (pfiles)
  (let ((dlen))
    (setq dlen (- (length pfiles) 1))
    (if (> dlen 0)
            (progn
              (if (or (eq (elt pfiles dlen) ?/) (eq (elt pfiles dlen) ?\\))
                  (setq pfiles (substring pfiles 0 dlen))
                )))
    pfiles
    ))

(defun c-xref-char-replace (ss schars dchar)
  (let ((res) (i) (j) (len) (slen) (cc) (sc) (loop))
    (setq res "")
    (setq len (length ss))
    (setq slen (length schars))
    (setq i 0)
    (while (< i len)
      (setq cc (elt ss i))
      (setq j 0)
      (setq loop t)
      (while (and (< j slen) loop)
            (setq sc (elt schars j))
            (if (eq cc sc)
                (setq loop nil)
              )
            (setq j (+ j 1))
            )
      (if loop
              (setq res (format "%s%c" res cc))
            (setq res (format "%s%c" res dchar))
            )
      (setq i (1+ i))
      )
    res
    ))


(defun c-xref-append-new-project-section (pname pcomments
                                                            pfiles apfiles ifiles
                                                            rest refs exactp)
  (let ((comment) (dlen) (cldopt) (tdq))
    (setq comment (or (equal pcomments "y") (equal pcomments "Y")))
    (goto-char (point-max))
    (insert (concat "\n\n[" pname "]\n"))
    (if comment
            (insert "  //  input files and directories (processed recursively)\n")
      )
    (insert (concat "  " apfiles))
    (newline)
    (setq pfiles (c-xref-remove-pending-slash pfiles))
    (if comment
            (insert "  //  directory where refrences database (tag files) is stored\n")
      )
    (insert (format "  -refs %s\n" (c-xref-optionify-string refs "\"")))
    (insert "  //  number of tag files\n")
    (if (or (equal exactp "y") (equal exactp "Y"))
            (insert "  -refnum=100\n")
      (insert "  -refnum=10\n")
      )
        (if (not (equal ifiles ""))
            (insert (format "%s\n" ifiles))
          )
        (if (or (equal exactp "y") (equal exactp "Y"))
            (insert "  // resolve symbols using definition place\n  -exactpositionresolve\n")
          )
        (if comment (insert "  //  setting for Emacs compile and run\n"))
        (insert "  -set compilefile \"cc %s\"\n")
        (insert "  -set compiledir \"cc *.c\"\n")
        (insert (format "  -set compileproject \"\n\tcd %s\n\tmake\n\t\"\n" pname))
        (if (eq c-xref-platform 'windows)
            (insert "  -set run1 \"a.exe\"\n")
          (insert "  -set run1 \"a.out\"\n")
          )
        (insert "  -set run5 \"\"  // an empty run; C-F8 will only compile\n")
        (insert "  //  set default to run1\n")
        (insert "  -set run ${run1}\n")
    (if (not (equal rest ""))
            (insert (format "%s\n" rest))
      )
    ))


(defun c-xref-path-completionfun (cstr filter type)
  (let ((res) (cc) (fname) (dir) (sep) (str) (prefix))
    (setq str cstr)
    (setq sep (string-match (format "\\%c" c-xref-path-separator) str))
    (setq prefix "")
    (while sep
      (progn
            (setq prefix (concat prefix (substring str  0 (+ sep 1))))
            (setq str (substring str (+ sep 1) nil))
            (setq sep (string-match (format "\\%c" c-xref-path-separator) str))
        ))
    ;;  (setq dir (concat (c-xref-file-directory-name str) "/"))
    (setq dir (file-name-directory str))
    (setq fname (file-name-nondirectory str))
    (if (eq type t)
            (setq res (file-name-all-completions fname dir))
      (setq res (file-name-completion fname dir))
      (if (eq c-xref-platform 'windows)
              (setq res (c-xref-backslashify-name res))
            )
      )
    (if (stringp res)
            (setq res (concat cstr (substring res (length fname))))
      )
    res
    ))

(defun c-xref-read-path-from-minibuffer (prompt default)
  (let ((res))
    (setq res (completing-read prompt 'c-xref-path-completionfun nil nil default))
    res
    ))

(defvar c-xref-foo-macros-counter 1)
(defun c-xref-collect-macros-for-new-project (prefix mess1 mess2)
  (let ((ifloop) (rrr) (rest) (aaa) (deflt))
    (setq rest "")
    (setq ifloop t)
    (while ifloop
      (if (eq c-xref-foo-macros-counter 1)
              (setq deflt "FOO")
            (setq deflt (format "FOO%d" c-xref-foo-macros-counter))
            )
      (setq c-xref-foo-macros-counter
                (+ c-xref-foo-macros-counter 1))
      (setq rrr (read-from-minibuffer mess1 deflt))
      (setq rest (format "%s\n  %s-D%s" rest prefix rrr))
      (setq aaa (read-from-minibuffer
                         (format "%s [yn]? " mess2) "n"))
      (if (not (or (equal aaa "y") (equal aaa "Y"))) (setq ifloop nil))
      )
    rest
    ))

(defun c-xref-remove-dangerous-fname-chars (ss)
  (let ((res) (i) (len))
    (setq res ss)
    (if (eq c-xref-platform 'windows)
            (progn
              (setq i 0)
              (setq len (length ss))
              (setq res "")
              (while (< i len)
                (if (and (> i 2) (eq (elt ss i) ?\:))
                        (setq res res)
                  (setq res (concat res (char-to-string (elt ss i))))
                  )
                (setq i (1+ i))
                )
              )
      )
    res
    ))

(defun c-xref-get-dir-from-path-and-package (dir package)
  (let ((res) (i) (len))
    (setq res (format "%s/" dir))
    (setq len (length package))
    (setq i 0)
    (while (< i len)
      (if (eq (elt package i) ?.)
              (setq res (format "%s/" res))
            (setq res (format "%s%c" res (elt package i)))
            )
      (setq i (+ i 1))
      )
    res
    ))


(defun c-xref-infer-package-proposal ()
  (let ((package) (ff))
    (save-excursion
      (goto-char (point-min))
      (setq ff (search-forward-regexp "package[ \t]+\\([a-zA-Z0-9$.]*\\)" nil t))
      (if ff
              (setq package (buffer-substring (match-beginning 1) (match-end 1)))
            (setq package "")
            )
      )
    package
    ))

(defun c-xref-find-project-root ()
  "Find project root by looking for .git directory.
Returns the directory containing .git, or the current file's directory if not found."
  (let ((git-root (locate-dominating-file (buffer-file-name) ".git")))
    (if git-root
        (expand-file-name git-root)
      (file-name-directory (buffer-file-name)))))

(defun c-xref-project-new ()
  "Create a new C-xrefactory project.

Creates a .c-xrefrc file in the project root directory.
The project root is auto-detected from .git location, or you can
specify it manually.

The created config file is minimal - just the project name and
source directory. You can edit it later to add include paths (-I)
or macro definitions (-D) if needed.
"
  (interactive "")
  (let ((project-root) (project-name) (config-file) (create-tags))
    (c-xref-soft-select-dispach-data-caller-window c-xref-this-buffer-dispatch-data)
    (c-xref-entry-point-make-initialisations-no-project-required)

    ;; Find project root (look for .git or use current directory)
    (setq project-root (c-xref-find-project-root))
    (setq project-root (read-directory-name "Project root: " project-root nil t))
    (setq project-root (expand-file-name project-root))

    ;; Default project name to directory basename
    (setq project-name (file-name-nondirectory (directory-file-name project-root)))
    (setq project-name (read-string "Project name: " project-name))

    ;; Create .c-xrefrc file in project root
    (setq config-file (expand-file-name ".c-xrefrc" project-root))
    (if (file-exists-p config-file)
        (if (not (y-or-n-p (format "%s already exists. Overwrite? " config-file)))
            (error "Aborted")))

    ;; Write minimal config file
    (with-temp-file config-file
      (insert (format "[%s]\n" project-name))
      (insert "  .\n"))

    (message "Created %s" config-file)

    ;; Ask to create tags
    (setq create-tags (y-or-n-p "Create reference database now? "))
    (if create-tags
        (progn
          (setq c-xref-current-project project-name)
          (c-xref-create-refs)
          (setq c-xref-current-project nil)))

    (message "Project '%s' created. Database at %s.c-xref/db" project-name project-root)
    ))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-project-active ()
  "Show currently  active project name.

This  function is useful  mainly if  you are  not sure  which .c-xrefrc
section applies to the currently edited file.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (if c-xref-current-project
      (message "Active project (manual): %s" c-xref-active-project)
    (message "Active project (auto): %s" c-xref-active-project)
    ))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-project-edit-options ()
  "Edit manually .c-xrefrc file.

This function just loads the .c-xrefrc file and goes to the
active project section.  You need to edit this text file
manually. For more info about the .c-xrefrc file format, read
`c-xrefrc' manual page. For more info about available options,
read the `c-xref' manual page.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (find-file c-xref-options-file)
  (goto-char (point-min))
  (if c-xref-active-project
      (progn
            (if (not (search-forward (concat "[" c-xref-active-project "]") nil t))
                (if (not (search-forward (concat "[" c-xref-active-project ":") nil t))
                        (if (not (search-forward (concat ":" c-xref-active-project "]") nil t))
                            (if (not (search-forward (concat ":" c-xref-active-project ":") nil t))
                                    (message "Options for %s found" c-xref-active-project)
                              ))))
            (beginning-of-line)
            (forward-line)
            ))
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;; (X)EMACS - IDE  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-ide-get-last-command (project commands default)
  (let ((as) (res))
    (setq as (assoc project (eval commands)))
    (if as
            (setq res (cdr as))
      (setq res default)
      )
    res
    ))

(defun c-xref-ide-set-last-command (project command commands)
  (let ((as))
    (setq as (assoc project (eval commands)))
    (if as
            (setcdr as command)
      (set commands (cons (cons project command) (eval commands)))
      )
    ))

;; this function can be used to run a user-defined non-interactive
;; command; the value of 'name' argument has to be set in .c-xrefrc file.

(defun c-xref-compile-function (action)
  (let ((comm) (name) (arg) (bfile) (shcomm) (cwin) (sdir) (dispatch-data))
    (require 'compile)
    (c-xref-entry-point-make-initialisations)
    (setq dispatch-data (c-xref-get-basic-server-dispatch-data nil))
    (if (not (featurep 'compile))
            (error "Packaeg 'compile.el' not found; please install it first.")
      )
    (if (eq action 'compilefile)
            (progn
              (setq name "compilefile")
              (setq arg (c-xref-file-last-name (buffer-file-name)))
              )
      (if (eq action 'compiledir)
              (progn
                (setq name "compiledir")
                (setq arg (c-xref-backslashify-name (c-xref-file-directory-name (buffer-file-name))))
                )
            (if (eq action 'compileproject)
                (progn
                  (setq name "compileproject")
                  (setq arg c-xref-active-project)
                  ))))
    (setq sdir default-directory)
    (setq comm (format (c-xref-get-env name) (c-xref-optionify-string arg "\"")))
    (if (and (not c-xref-always-batch-file) (not (string-match "\n" comm)))
            (setq shcomm comm)
      (setq bfile (c-xref-create-batch-file default-directory comm))
      (if (eq c-xref-platform 'windows)
              (setq shcomm (format "\"%s\"" bfile))
            (setq shcomm (format "%s %s" c-xref-shell bfile))
            ))
    (c-xref-ide-set-last-command c-xref-active-project action 'c-xref-ide-last-compile-commands)
    (c-xref-display-and-set-maybe-existing-window c-xref-compilation-buffer nil t)
    (compile shcomm)
    (sleep-for 0.5)
    (setq cwin (get-buffer-window c-xref-compilation-buffer))
    (if cwin
            (progn
              (select-window cwin)
              (goto-char (point-max))
              (setq default-directory sdir)
              (setq c-xref-this-buffer-dispatch-data dispatch-data)
              (setq tab-width 8)    ;; as usual on terminals
              (bury-buffer (current-buffer))
              ))
    ))

(defun c-xref-ide-compile ()
  "Repeat last  compilation command.

This  is  whichever  of  \"Compile file\",  \"Compile  Directory\"  or
\"Compile Project\" was last executed for this project.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-compile-function (c-xref-ide-get-last-command c-xref-active-project
                                                                                'c-xref-ide-last-compile-commands
                                                                                c-xref-ide-last-compile-command))
  )

(defun c-xref-ide-compile-file ()
  "Compile file.

The compile command must be specified in your .c-xrefrc file via
the `-set compilefile <command>' option.  If the <command> string
contains the %s character sequence, it will be replaced by the
name of the currently edited file. Read the `c-xref' man page for
more info about the -set option.  This function actually just
calls the Emacs compile function with the appropriate command
string. You need to have the `compile' package installed.
"
  (interactive "")
  (c-xref-compile-function 'compilefile)
  )

(defun c-xref-ide-compile-dir ()
  "Compile  directory.

The compile command must be specified in your .c-xrefrc file via
the `-set compiledir <command>' option. If the <command> string
contains the %s character sequence, it will be replaced by the
current directory name.  Read the `c-xref' man page for more info
about the -set option.  This function actually just calls the
Emacs compile function with the appropriate command string.  You
need to have the `compile' package installed.
"
  (interactive "")
  (c-xref-compile-function 'compiledir)
  )

(defun c-xref-ide-compile-project ()
  "Compile  project.

The compile command must be specified in your .c-xrefrc file via
the `-set compileproject <command>' option.  You need to have the
`compile' package installed.
"
  (interactive "")
  (c-xref-compile-function 'compileproject)
  )

(defun c-xref-ide-previous-error ()
  "Move to  the previous  error of the  last compilation.

This function actually calls `next-error' with argument -1. See also
`c-xref-alternative-previous-reference'.

"
  (interactive "")
  (next-error -1)
  )

(defun c-xref-ide-next-error ()
  "Move  to the  next  error  of the  last  compilation.

This  function actually calls `next-error' with argument 1. See also
`c-xref-alternative-next-reference'.

"
  (interactive "")
  (next-error 1)
  )

;; kill the process associated with this buffer (if any)
(defun c-xref-kill-this-buffers-process ()
  (let ((pl) (pp))
    (setq pl (process-list))
    (while pl
      (setq pp (car pl))
      (setq pl (cdr pl))
      (if (equal (process-buffer pp) (current-buffer))
              (progn
                (kill-process pp)
                (sleep-for 0.1)
                (discard-input)
                (while (eq (process-status pp) 'run)
                  (message "Waiting until process dies.")
                  (sleep-for 0.1)
                  (discard-input)
                  )
                (message "Done.")
                (setq pl nil)
                )))
    ))

(defun c-xref-escape-dq (ss)
  (let ((res) (mp) (lmp))
    (setq res ss)
    (setq lmp 0)
    (setq mp (string-match "\"" res))
    (while mp
      (setq res (format "%s\\%s" (substring res 0 mp) (substring res mp)))
      (setq mp (string-match "\"" res (+ mp 2)))
      )
    res
    ))

(defun c-xref-cr-echos-commands (command)
  (let ((res) (mp) (lmp) (line))
    (setq res "")
    (setq lmp 0)
    (setq mp (string-match "\n" command))
    (while mp
      (setq line (substring command lmp mp))
      ;; do not echo exit and empty lines so as not to damage the return value
      (if (or (equal line "exit") (c-xref-exact-string-match "[ \t]*" line))
              (setq res (format "%s\n%s" res line))
            (setq res (format "%s\necho \">%s\"\n%s" res (c-xref-escape-dq line) line))
            )
      (setq lmp (+ mp 1))
      (setq mp (string-match "\n" command lmp))
      )
    ;; don't do this, command ends with \n (last line is empty)
    ;; following lines damages return value
    ;;(setq line (substring command lmp))
    ;;(setq res (format "%s\necho %s\n%s" res line line))
    res
    ))


(defun c-xref-create-batch-file (bdir command)
  (let ((res) (cc))
    (setq cc "")
    (if (and (eq c-xref-platform 'windows)
                 (> (length bdir) 1)
                 (equal (elt bdir 1) ?:))
            (setq cc (format "%s%s:\n" cc (substring bdir 0 1)))
      )
    (setq cc (format "%scd %s\n%s\n" cc (c-xref-backslashify-name bdir) command))
    (if (not (eq c-xref-platform 'windows))
            (setq cc (c-xref-cr-echos-commands cc))
      )
    (get-buffer-create " *c-xref-run-command*")
    (set-buffer " *c-xref-run-command*")
    (c-xref-erase-buffer)
    (insert cc)
    (write-region 1 (+ (buffer-size) 1) c-xref-run-batch-file)
    (kill-buffer nil)
    (setq res c-xref-run-batch-file)
    res
    ))


;; this function can be used to run a C-xref user-defined interactive
;; command; the value of 'name' argument has to be set in .c-xrefrc file.
(defun c-xref-run-function (name skip-one-win)
  (let ((bb) (rc) (args) (ww) (bdir) (cc)
            (bfile) (command) (dispatch-data) (owin) (cwin))
    (setq dispatch-data (c-xref-get-basic-server-dispatch-data nil))
    (setq bdir default-directory)
    (require 'comint)
    (if (not (featurep 'comint))
            (error "Package 'comint.el' not found; please install it first")
      )
    (setq rc (c-xref-get-env name))
    (if rc
            (progn
              (if (or (equal name c-xref-run-this-option) (equal name c-xref-run-option))
                  (setq command (format rc (c-xref-compute-simple-information
                                                            "-olcxcurrentclass -no-errors")))
                (setq command rc)
                )
              (setq bfile (c-xref-create-batch-file bdir command))
              (if (eq c-xref-platform 'windows)
                  (setq args (cons bfile nil))
                (setq args (cons c-xref-shell (cons bfile nil)))
                )
              (setq owin (get-buffer-window c-xref-run-buffer))
              (if owin
                  (select-window owin)
                (setq cwin (get-buffer-window c-xref-compilation-buffer))
                (if cwin (select-window cwin))
                (c-xref-display-and-set-new-dialog-window c-xref-run-buffer nil t)
                (bury-buffer (current-buffer))
                )
              (c-xref-kill-this-buffers-process)
              (c-xref-erase-buffer)
              (c-xref-ide-set-last-command c-xref-active-project name 'c-xref-ide-last-run-commands)
              (apply 'make-comint c-xref-run-buffer-no-stars (car args) nil (cdr args))
              (setq default-directory bdir)
              (setq c-xref-this-buffer-dispatch-data dispatch-data)
              )
      )
    ))

(defun c-xref-ide-run ()
  "Repeat last run command (i.e.  one of run1, run2, ...).

If no run  command was previously executed, run  the command specified
vith  the `-set  run <command>'  option  in your  .c-xrefrc file.   This
function actually  just calls the `make-comint'  function with command
string taken  from .c-xrefrc configuration  file.  You need to  have the
`comint' package installed.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-run-function (c-xref-ide-get-last-command
                                c-xref-active-project
                                'c-xref-ide-last-run-commands "run") nil)
  )

(defun c-xref-ide-run-this ()
  "Run `runthis' command.

Run the command specified by `-set runthis <command>' option in
your .c-xrefrc file.  If `%s' occurs in <command> it will be
replaced by the name of the currently edited class.  It also sets
this command to be executed by the default run function
`c-xref-ide-run'.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-run-function c-xref-run-this-option nil)
  )

(defun c-xref-ide-run1 ()
  "Run `run1' command.

Run the command specified by `-set run1 <command>' option in your
.c-xrefrc file.  It also sets this command to be executed by the
default run function `c-xref-ide-run'.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-run-function "run1" nil)
  )


(defun c-xref-ide-run2 ()
  "Run `run2' command.

Run the command specified by `-set run2 <command>' option in your
.c-xrefrc file.  It also sets this command to be executed by the
default run function `c-xref-ide-run'.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-run-function "run2" nil)
  )


(defun c-xref-ide-run3 ()
  "Run `run3' command.

Run the command specified by `-set run3 <command>' option in your
.c-xrefrc file.  It also sets this command to be executed by the
default run function `c-xref-ide-run'.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-run-function "run3" nil)
  )


(defun c-xref-ide-run4 ()
  "Run `run4' command.

Run the command specified by `-set run4 <command>' option in your
.c-xrefrc file.  It also sets this command to be executed by the
default run function `c-xref-ide-run'.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-run-function "run4" nil)
  )


(defun c-xref-ide-run5 ()
  "Run `run5' command.

Run the command specified by `-set run5 <command>' option in your
.c-xrefrc file.  It also sets this command to be executed by the
default run function `c-xref-ide-run'.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-run-function "run5" nil)
  )

(defun c-xref-ide-compile-run ()
  "Invoke (last)  compile command followed  by (last) run  command.

The run command is invoked only if it is non-empty and the
compilation is successful.  See also `c-xref-ide-compile' and
`c-xref-ide-run'.
"
  (interactive "")
  (let ((cproc) (owin) (cwin) (rc) (opc) (sdir) (dispatch-data))
    (c-xref-soft-delete-window c-xref-run-buffer)
    (c-xref-entry-point-make-initialisations)
    (setq owin (get-buffer-window (current-buffer)))
    (setq dispatch-data (c-xref-get-basic-server-dispatch-data nil))
    (setq sdir default-directory)
    (setq opc process-connection-type)
    (setq  process-connection-type nil)
    (c-xref-ide-compile)
    (setq cproc (get-buffer-process c-xref-compilation-buffer))
    (if cproc
            (set-process-sentinel cproc (list 'lambda '(cc message)
                                              (list 'c-xref-ide-compile-run-sentinel 'cc 'message owin opc)))
      )
    (sit-for .5)
    (setq cwin (get-buffer-window c-xref-compilation-buffer))
    (if cwin
            (progn
              (select-window (get-buffer-window c-xref-compilation-buffer))
              (goto-char (point-max))
              (setq default-directory sdir)
              (setq c-xref-this-buffer-dispatch-data dispatch-data)
              (setq tab-width 8)    ;; as usual on terminals
              (bury-buffer (current-buffer))
              ))
    (setq process-connection-type opc)
    ))

(defun c-xref-ide-compile-run-sentinel (cc message owin opc)
  (let ((rc))
    (select-window owin)
    ;;(setq process-connection-type opc)
    (if (and (>= (length message) 8)
                 (equal (substring message 0 8) "finished"))
            (progn
              (setq rc (c-xref-get-env (c-xref-ide-get-last-command
                                                    c-xref-active-project
                                                    'c-xref-ide-last-run-commands
                                                    "run")))
              (if (and rc (not (equal rc "")))
                  (progn
                        (c-xref-run-function (c-xref-ide-get-last-command
                                                      c-xref-active-project
                                                      'c-xref-ide-last-run-commands
                                                      "run") t)
                    ))))
    ))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;; TAGS maintenance ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-tags-dispatch-error (ss i len tag)
  (let ((tlen) (cc) (cw) (link) (j) (cclen) (ccc) (bp) (tlink))
    (setq cw (selected-window))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag tag)
    (if (equal (substring cc 0 16) "<A HREF=\"file://")
            (progn
              ;; hypertext link
              (setq cclen (length cc))
              (setq j (c-xref-server-parse-xml-tag cc 0 cclen))
              (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
              (setq tlink (cdr (assoc "HREF" c-xref-server-ctag-attributes)))
              ;; tag
              (setq bp (point))
              (insert "HREF=\"" tlink "\"")
              ;;(insert (substring tlink 7) " ")
              (put-text-property bp
                                         (point)
                                         'invisible t)
              (setq bp (point))
              (insert (substring cc j (+ j tlen)))
              (put-text-property bp (point) 'face 'c-xref-error-face)
              ;; after tag text
              (insert (substring cc (+ j tlen 4)))
              (put-text-property bp (point) 'mouse-face 'highlight)
              )
      (insert cc)
      )
    i
    ))

(defun c-xref-tags-dispatch-information (ss i len tag)
  (let ((tlen) (cc) (cw) (dw))
    (setq cw (selected-window))
    (setq tlen (c-xref-server-dispatch-get-int-attr c-xref_PPCA_LEN))
    (setq cc (c-xref-char-list-substring ss i (+ i tlen)))
    (setq i (+ i tlen))
    (setq i (c-xref-server-parse-xml-tag ss i len))
    (c-xref-server-dispatch-require-end-ctag tag)
    (insert cc)
    (newline)
    i
    ))

(defun c-xref-tags-dispatch (ss i len)
  (let ((x) (j))
    (setq j 0)
    (if c-xref-debug-mode (message "tag dispatching: %s" ss))
    (setq i (c-xref-server-dispatch-skip-blank ss i len))
    (while (and (< i len) (eq (elt ss i) ?<))
      (setq i (c-xref-server-parse-xml-tag ss i len))
      (cond
       (
            (equal c-xref-server-ctag c-xref_PPC_INFORMATION)
            (setq i (c-xref-tags-dispatch-information ss i len c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_WARNING)
            (setq i (c-xref-tags-dispatch-error ss i len c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_ERROR)
            (setq i (c-xref-tags-dispatch-error ss i len c-xref-server-ctag)))
       (
            (equal c-xref-server-ctag c-xref_PPC_FATAL_ERROR)
            (setq i (c-xref-tags-dispatch-error ss i len c-xref-server-ctag)))
       (
            t
            ;;    (error "unexpected tag in log file: %s" c-xref-server-ctag))
            (message "unexpected tag in log file: %s" c-xref-server-ctag))
       )
      (setq i (c-xref-server-dispatch-skip-blank ss i len))
      )
    (+ i j)
    ))

(defun c-xref-tags-log-exit ()
  (interactive "")
  (c-xref-soft-delete-window c-xref-log-view-buffer)
  )

(defun c-xref-tags-log-browse ()
  (interactive "")
  (let ((el) (line) (file) (b) (i) (di) (len) (ln))
    (end-of-line)
    (setq el (point))
    (beginning-of-line)
    (setq line (buffer-substring (point) el))
    ;;(message "checking %s" (substring line 0 6))
    (if (equal (substring line 0 6) "HREF=\"")
            (progn
              (setq b 6)
              (if (equal (substring line b (+ b 5)) "file:")
                  (progn
                        (setq b (+ b 5))
                        (if (and (eq (elt line b) ?/)
                                     (eq (elt line (+ b 1)) ?/))
                            (setq b (+ b 2))
                          )))
              (setq i b)
              (setq di 0)
              (setq len (length line))
              (while (and (< i len)
                              (not (eq (elt line i) ?\"))
                              )
                (if (eq (elt line i) ?#) (setq di i))
                (setq i (1+ i))
                )
              (if (eq di 0) (setq di i))
              (setq file (substring line b di))
              (setq ln (string-to-number (substring line (+ di 1) i)))
              (c-xref-show-file-line-in-caller-window file ln)
              )
      (c-xref-find-file-on-point)
      )
    ))

(defun c-xref-tags-log-mouse-button (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-tags-log-browse)
  )

(defvar c-xref-tags-log-key-map (make-sparse-keymap "C-xref symbol-resolution"))
(define-key c-xref-tags-log-key-map "\e" 'c-xref-tags-log-exit)
(define-key c-xref-tags-log-key-map "q" 'c-xref-tags-log-exit)
(define-key c-xref-tags-log-key-map " " 'c-xref-tags-log-browse)
(define-key c-xref-tags-log-key-map [newline] 'c-xref-tags-log-browse)
(define-key c-xref-tags-log-key-map [return] 'c-xref-tags-log-browse)
(define-key c-xref-tags-log-key-map "\C-m" 'c-xref-tags-log-browse)
(c-xref-bind-default-button c-xref-tags-log-key-map 'c-xref-tags-log-mouse-button)

(defun c-xref-tags-process-show-log ()
  (let ((ss) (len) (dispatch-data) (conf))
    (setq dispatch-data (c-xref-get-basic-server-dispatch-data 'nil))
    (get-buffer-create c-xref-server-answer-buffer)
    (set-buffer c-xref-server-answer-buffer)
    (setq buffer-read-only nil)
    ;; (c-xref-erase-buffer)
    (insert-file-contents c-xref-tags-tasks-ofile  nil nil nil t)
    (setq ss (buffer-string))
    (setq len (length ss))
    (kill-buffer c-xref-server-answer-buffer)
    (if (> len 0)
            (progn
              (setq conf (read-from-minibuffer
                              "View log file [yn]? " "n"))
              (if (or (equal conf "y") (equal conf "Y"))
                  (progn
                        (c-xref-delete-window-in-any-frame c-xref-log-view-buffer nil)
                        (c-xref-display-and-set-new-dialog-window c-xref-log-view-buffer nil t)
                        (setq c-xref-this-buffer-dispatch-data dispatch-data)
                        (c-xref-tags-dispatch ss 0 len)
                        (setq buffer-read-only t)
                        (c-xref-use-local-map c-xref-tags-log-key-map)
                        ))
              ))
    ))

(defun c-xref-update-tags (option log)
  (c-xref-server-tags-process (cons option nil))
  (if log (c-xref-tags-process-show-log))
  )

(defun c-xref-before-push-optional-update ()
  (if c-xref-auto-update-tags-before-push
      (progn
            (c-xref-update-tags "-fastupdate" nil)
            ;; clear the 100% message
            (message "")
            ))
  )

(defun c-xref-create-refs ()
  "Create tags.

This function executes `c-xref -create'.  The effect of the
invocation is that the C-xrefactory references database (used by
the source browser and refactorer) are created.  The behavior of
the `c-xref' command is controlled by options read from the
`.c-xrefrc' file.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-server-tags-process (cons "-create" nil))
  (c-xref-tags-process-show-log)
  )

(defun c-xref-fast-update-refs ()
  "Fast update of tags.

This function executes `c-xref -fastupdate'.  The effect of the
invocation is that C-xrefactory references database (used by the
source browser and refactorer) is updated.  The behavior of the
`c-xref' command is controlled by options read from the
`.c-xrefrc' file.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-update-tags "-fastupdate" t)
  )

(defun c-xref-update-refs (log)
  "Full update of tags.

This function executes `c-xref -update'.  The effect of the
invocation is that C-xrefactory references database (used by the source
browser and refactorer) are updated.  The behavior of the
`c-xref' command is controlled by options read from the
`.c-xrefrc' file.
"
  (interactive "P")
  (c-xref-entry-point-make-initialisations)
  (c-xref-server-tags-process (cons "-update" nil))
  (c-xref-tags-process-show-log)
  )


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;; COMPLETION ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-delete-completion-window ()
  (c-xref-delete-window-in-any-frame c-xref-completions-buffer nil)
  )

(defun c-xref-interactive-completion-select (&optional argp)
  "Go to the reference corresponding to this line."
  (interactive "P")
  (let ((lineno 1) (replace-flag) (cw) (offset))
    (if (eq (point) (point-max))
            (setq replace-flag nil)
      (setq replace-flag t)
      (setq lineno (count-lines 1 (+ (point) 1)))
      )
    (c-xref-send-data-to-process-and-dispatch (format "-olcomplselect%d" lineno) c-xref-completions-dispatch-data nil)
    (if (equal c-xref-completions-windows-counter  0)
            (c-xref-delete-completion-window)
      (setq c-xref-completions-windows-counter (1- c-xref-completions-windows-counter))
      ;; display previous completion, but keeps cursor position and focus
      (setq cw (selected-window))
      (c-xref-soft-select-dispach-data-caller-window c-xref-completions-dispatch-data)
      (set-marker c-xref-completion-pop-marker (point))
      (select-window cw)
      (c-xref-interactive-completion-previous nil)
      (c-xref-soft-select-dispach-data-caller-window c-xref-completions-dispatch-data)
      (c-xref-switch-to-marker c-xref-completion-pop-marker)
      )
    t
    ))

(defun c-xref-interactive-completion-previous (event)
  "Previous completions."
  (interactive "i")
  (c-xref-send-data-to-process-and-dispatch "-olcomplback" c-xref-completions-dispatch-data nil)
  t
  )

(defun c-xref-interactive-completion-next (event)
  "Next completions."
  (interactive "i")
  (c-xref-send-data-to-process-and-dispatch "-olcomplforward" c-xref-completions-dispatch-data nil)
  t
  )

(defun c-xref-interactive-completion-mouse-select (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-completion-select)
  t
  )

(defun c-xref-interactive-completion-mouse-previous (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-completion-previous event)
  t
  )

(defun c-xref-interactive-completion-mouse-next (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-completion-next event)
  t
  )

(defun c-xref-interactive-completion-escape (event)
  "Close completions; recover original situation with no completin selected."
  (interactive "i")
  (c-xref-delete-completion-window)
  (c-xref-select-dispach-data-caller-window c-xref-completions-dispatch-data)
  ;; following is to reconstitute original text
  (c-xref-switch-to-marker c-xref-completion-marker)
  (c-xref-insert-completion-and-delete-pending-id
   (format "%s%s" c-xref-completion-id-before-point c-xref-completion-id-after-point))
  (c-xref-set-to-marker c-xref-completion-marker)
  (forward-char (length c-xref-completion-id-before-point))
  ;;(forward-char (length (car c-xref-completion-auto-search-list)))
  t
  )

(defun c-xref-interactive-completion-q (event)
  "Quit or auto search for q."
  (interactive "i")
  (if (and c-xref-completion-quit-on-q (c-xref-completion-auto-search-list-empty))
      (c-xref-interactive-completion-escape event)
    (c-xref-completion-auto-search)
    )
  )

(defun c-xref-interactive-completion-mouse-escape (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-completion-escape event)
  t
  )

(defun c-xref-interactive-completion-close (event)
  "Close completions window."
  (interactive "i")
  (c-xref-delete-completion-window)
  (c-xref-select-dispach-data-caller-window c-xref-completions-dispatch-data)
  t
  )

(defun c-xref-interactive-completion-mouse-close (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-completion-close event)
  t
  )

(defun c-xref-interactive-completion-goto (event)
  "Go to the position corresponding to this line."
  (interactive "i")
  (let ((lineno) (cw))
    (setq lineno (count-lines 1 (+ (point) 1)))
    (setq cw (selected-window))
    (c-xref-send-data-to-process-and-dispatch
     (format "-olcxcgoto%d" lineno)
     c-xref-completions-dispatch-data
     nil)
    (sit-for 1)
    (select-window cw)
    t
    ))

(defun c-xref-interactive-completion-mouse-goto (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-completion-goto event)
  t
  )

(defun c-xref-auto-search-search (str)
  (let ((sss) (ns))
    (setq sss (format "\\(^\\|^[^ ]*\\.\\)\\(%s\\)" str))
    (if (search-forward-regexp sss (point-max) t)
            (setq ns (buffer-substring (match-beginning 2) (match-end 2)))
      (setq ns str)
      )
    (set-text-properties 0 (length ns) nil ns)
    (setq c-xref-completion-auto-search-list
              (cons
               ns
               (cons (point) c-xref-completion-auto-search-list)))
    ))

(defun c-xref-insert-completion-and-delete-pending-id (cc)
  (let ((i) (len) (id) (ccc))
    (setq id (c-xref-get-identifier-after (point)))
    (setq len (length id))
    (setq i 1)
    (while (and (<= i len) (c-xref-string-has-prefix cc (substring id 0 i) nil))
      (setq i (+ i 1)))
    (setq i (- i 1))
    (forward-char i)
    (c-xref-delete-pending-ident-after-completion)
    (setq ccc (substring cc i))
    (if (not (equal ccc "")) (insert ccc))
    ))

(defun c-xref-insert-completion (completion)
  (if c-xref-completion-delete-pending-identifier
      (c-xref-insert-completion-and-delete-pending-id completion)
    (c-xref-insert-completion-and-delete-pending-id
     (concat completion c-xref-completion-id-after-point))
    (backward-char (length c-xref-completion-id-after-point))
    )
  )

(defun c-xref-set-completion-src-window ()
  (let ((sb))
    (setq sb (get-buffer-window (marker-buffer c-xref-completion-marker)))
    (if sb
            (select-window sb)
      (other-window -1)
      (c-xref-switch-to-marker c-xref-completion-marker)
      )
    ))


(defun c-xref-completion-source-mod ()
  (let ((cb))
    (setq cb (get-buffer-window (current-buffer)))
    (c-xref-set-completion-src-window)
    (c-xref-set-to-marker c-xref-completion-marker)
    (c-xref-insert-completion (car c-xref-completion-auto-search-list))
    (select-window cb)
    )
  )

(defun c-xref-completion-auto-search-w ()
  (interactive "")
  (let ((pp) (lend) (ns) (addstr))
    (setq pp (point))
    (end-of-line)
    (setq lend (point))
    (goto-char (+ pp 1))
    (search-forward-regexp c-xref-forward-pass-identifier-regexp lend t)
    (setq addstr (buffer-substring pp (- (point) 1)))
    (setq ns (format "%s%s" (car c-xref-completion-auto-search-list) addstr))
    (beginning-of-line)
    (c-xref-auto-search-search ns)
    (c-xref-completion-source-mod)
    ))

(defun c-xref-completion-auto-search-s ()
  (interactive "")
  (c-xref-auto-search-search (car c-xref-completion-auto-search-list))
  )

(defun c-xref-completion-auto-search-list-empty ()
  (let ((res))
    (setq res (<= (length c-xref-completion-auto-search-list) 4))
    res
    ))

(defun c-xref-completion-auto-search-back ()
  (interactive "")
  (let ((op) (cstr) (pstr))
    (if (not (c-xref-completion-auto-search-list-empty))
        (progn
              (setq pstr (car c-xref-completion-auto-search-list))
              (setq c-xref-completion-auto-search-list
                    (cdr (cdr c-xref-completion-auto-search-list )))
              (setq cstr (car c-xref-completion-auto-search-list))
              (setq op (car (cdr c-xref-completion-auto-search-list)))
              (goto-char op)
              (c-xref-completion-source-mod)
              ))
    ))

(defun c-xref-completion-auto-search ()
  (interactive "")
  (let ((ns) (os) (nns))
    (setq os (car c-xref-completion-auto-search-list))
    (setq ns (format "%s%c" os last-command-event))
    (beginning-of-line)
    (c-xref-auto-search-search ns)
    (setq nns (car c-xref-completion-auto-search-list))
    (c-xref-completion-source-mod)
    ))

(defun c-xref-completion-auto-switch ()
  (interactive "")
  (select-window (get-buffer-window (marker-buffer c-xref-completion-marker)))
  (c-xref-set-to-marker c-xref-completion-marker)
  (forward-char (length (car c-xref-completion-auto-search-list)))
  (self-insert-command 1)
  )

(defun c-xref-completion ()
  "Complete  identifier.

This  function parses the  current buffer  up to  point.  Then  if the
identifier at point has a  unique completion it completes it. If there
is  more  than one  possible  completion, a  list  is  displayed in  a
separate window.
"
  (interactive "")
  (let ((opt))
    (c-xref-entry-point-make-initialisations)
    (setq opt (format "-olcxcomplet -maxcompls=%d" c-xref-max-completions))
    (if c-xref-completion-case-sensitive
            (setq opt (format "%s -completioncasesensitive" opt))
      )
    (if c-xref-completion-truncate-lines
            (setq opt (format "%s -olinelen=50000" opt))
      (setq opt (format "%s -olinelen=%d" opt (window-width)))
      )
    (setq c-xref-completion-id-after-point (c-xref-get-identifier-after (point)))
    (setq c-xref-completions-dispatch-data (c-xref-get-basic-server-dispatch-data 'c-xref-server-process))
    (c-xref-server-call-on-current-buffer-no-saves opt c-xref-completions-dispatch-data)
    ))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; BROWSER ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-is-this-regular-process-dispatch-data (dispatch-data)
  (let ((proc) (res))
    (setq proc (cdr (assoc 'process dispatch-data)))
    (setq res (eq proc 'c-xref-server-process))
    res
    ))

(defun c-xref-is-this-refactorer-process-dispatch-data (dispatch-data)
  (let ((proc) (res))
    (setq proc (cdr (assoc 'process dispatch-data)))
    (setq res (eq proc 'c-xref-refactorer-process))
    res
    ))

(defun c-xref-is-browser-window-displayed ()
  (let ((res) (resolvewin))
    (setq res nil)
    (if (c-xref-get-this-frame-dispatch-data)
            (progn
              (setq resolvewin (cdr (assoc 'linked-resolution-window (c-xref-get-this-frame-dispatch-data))))
              (if (and (windowp resolvewin) (window-live-p resolvewin))
                  (setq res resolvewin)
                )))
    res
    ))

(defun c-xref-is-refactory-window-displayed-in-the-frame (winid)
  (let ((res) (listwin))
    (setq res nil)
    (if (c-xref-get-this-frame-dispatch-data)
            (progn
              (setq listwin (cdr (assoc winid (c-xref-get-this-frame-dispatch-data))))
              (if (and (windowp listwin) (window-live-p listwin))
                  (setq res listwin)
                )))
    res
    ))

(defun c-xref-is-reflist-window-displayed ()
  (let ((res))
    (setq res (c-xref-is-refactory-window-displayed-in-the-frame 'linked-refs-window))
    res
    ))

(defun c-xref-is-failed-refactoring-window-displayed ()
  (let ((res))
    (setq res (c-xref-is-refactory-window-displayed-in-the-frame 'linked-refactoring-window))
    res
    ))

(defun c-xref-browser-of-failed-refactoring-is-displayed ()
  (let ((rw) (sw) (res))
    (setq res nil)
    (setq sw (selected-window))
    (setq rw (c-xref-is-failed-refactoring-window-displayed))
    (if rw
            (progn
              (select-window rw)
              (if (c-xref-is-this-refactorer-process-dispatch-data c-xref-this-buffer-dispatch-data)
                  (setq res rw)
                )
              (select-window sw)
              ))
    res
    ))

(defun c-xref-update-browser-if-displayed (oldwins)
  (let ((bw) (rw) (sw) (dispatch-data))
    (setq sw (selected-window))
    (setq bw (c-xref-is-browser-window-displayed))
    (if bw
        (progn
              (select-window bw)
              (setq dispatch-data c-xref-this-buffer-dispatch-data)
              (select-window sw)
              (c-xref-create-browser-windows nil dispatch-data)
              (c-xref-browser-dialog-set-new-filter dispatch-data)
              (c-xref-appropriate-browser-windows-sizes oldwins)
              (select-window sw)
              )
      (setq rw (c-xref-is-reflist-window-displayed))
      (if rw
              (progn
                (select-window rw)
                (setq dispatch-data c-xref-this-buffer-dispatch-data)
                (select-window sw)
                (c-xref-references-set-filter 0 dispatch-data)
                (select-window sw)
                ))
      )
    ))

(defun c-xref-set-current-reference-list-pointer ()
  (let ((poin (point)) (lastref) (frame) (frame-assoc)
            (lineno (count-lines 1 (+ (point) 1))))
    (setq lastref nil)
    (if (not (eq c-xref-this-buffer-type 'reference-list))
            (error "Not a reference list buffer")
      )
    (goto-char (point-min))
    (setq lastref (search-forward-regexp "^>" (point-max) t))
    (setq buffer-read-only nil)
    (if lastref
            (progn
              (delete-char -1)
              (insert " ")
              )
      )
    (c-xref-goto-line lineno)
    (beginning-of-line)
    (delete-char 1)
    (insert ">")
    (setq buffer-read-only t)
    (goto-char poin)
    ))

(defun c-xref-move-current-reference-list-pointer (direction)
  (let ((cline) (nlines) (lastref) (poin))
    (if (not (eq c-xref-this-buffer-type 'reference-list))
            (error "Not a reference list buffer")
      )
    (setq poin (point))
    (goto-char (point-min))
    (setq lastref (search-forward-regexp "^>" (point-max) t))
    (if (not lastref)
            (goto-char poin)
      )
    (beginning-of-line)
    (setq cline (count-lines (point-min) (point)))
    (setq nlines (count-lines (point-min) (point-max)))
    (if (< direction 0)
            (progn
              (setq cline (- cline 1))
              (if (>= cline 0)
                  (forward-line -1)
                (goto-char (point-max))
                (beginning-of-line)
                ;; (message "Moving to the last reference") (beep t)
                )
              (c-xref-set-current-reference-list-pointer)
              )
      (setq cline (+ cline 1))
      (if (< cline nlines)
              (forward-line)
            (goto-char (point-min))
            ;; (message "Moving to the first reference") (beep t)
            )
      (c-xref-set-current-reference-list-pointer)
      )
    ))

(defun c-xref-move-current-reference-list-pointer-if-browser-displayed (direction)
  (let ((rw) (sw) (cline) (nlines))
    (setq sw (selected-window))
    (setq rw (c-xref-is-reflist-window-displayed))
    (if rw
            (progn
              (select-window rw)
              (if (c-xref-is-this-regular-process-dispatch-data c-xref-this-buffer-dispatch-data)
                  (c-xref-move-current-reference-list-pointer direction)
                )
              (select-window sw)
              ))
    ))

(defun c-xref-push-references ()
  "Push references of a symbol on to the browser stack.

This function parses the current buffer, resolves the symbol
under point and pushes all its references (from active project)
on the browser stack.
"
  (interactive "")
  (let ((oldwins))
    (c-xref-entry-point-make-initialisations)
    (c-xref-before-push-optional-update)
    (setq oldwins (c-xref-is-browser-window-displayed))
    (c-xref-call-process-with-basic-file-data-all-saves "-olcxpushonly")
    (c-xref-update-browser-if-displayed oldwins)
    ))

(defun c-xref-push-and-goto-definition ()
  "Push references of a symbol on to the browser stack, goto definition.

This function parses the current buffer, resolves the symbol
under point and pushes all its references (from the active
project) on the the browser stack, then goes to the symbol's
definition.
"
  (interactive "")
  (let ((oldwins))
    (c-xref-entry-point-make-initialisations)
    (c-xref-before-push-optional-update)
    (setq oldwins (c-xref-is-browser-window-displayed))
    (c-xref-call-process-with-basic-file-data-all-saves
     (concat "-olcxpush"))
    (c-xref-update-browser-if-displayed oldwins)
    ))

(defun c-xref-push-and-browse (push-option)
  (let ((sw) (oldwins))
    (c-xref-before-push-optional-update)
    (setq sw (selected-window))
    (setq c-xref-global-dispatch-data (c-xref-get-basic-server-dispatch-data
                                                       'c-xref-server-process))
    (setq oldwins (c-xref-create-browser-windows nil c-xref-global-dispatch-data))
    ;; (select-window sw)
    ;; reselect it like this, as caller may be moved (left-horizontal split)
    (c-xref-select-dispach-data-caller-window c-xref-global-dispatch-data)
    (c-xref-server-call-on-current-buffer-all-saves push-option c-xref-global-dispatch-data)
    (c-xref-update-browser-if-displayed oldwins)
    ))

(defun c-xref-browse-symbol ()
  "Browse symbol.

This  function parses the  current buffer,  resolves the  symbol under
point,  pushes all  its references  (from the  active project)  on the
browser stack, and launches the C-xrefactory interactive browser.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-clean-unused-browser-buffers)
  (c-xref-push-and-browse "-olcxpushonly")
  )

(defun c-xref-push-global-unused-symbols ()
  "Push unused global symbols.

This function pushes on to the reference stack all references to
global symbols defined in the project but not used. As symbols
are unused, those references will (usually) be only definitions
and declarations.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-push-and-browse "-olcxpushglobalunused")
  )


(defun c-xref-push-this-file-unused-symbols ()
  "Push unused symbols defined in this file.

This function pushes on to the reference stack all references to
file local symbols defined in the current file and not used
anywhere in the project.  As symbols are unused, those references
will (usually) be only definitions and declarations.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (c-xref-push-and-browse "-olcxpushfileunused")
  )

(defun c-xref-push-name ()
  "Push references to a given symbol.

This function asks for a symbol name, then it inspects the tags
looking for symbols of that name.  For each such symbol it loads
all its references.

If there are several symbols of the same name the user is asked
for manual selection.  The selection menu is the same as for
other browsing functions.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (let ((sym) (sstr) (line) (col) (oldwins))
    (setq oldwins (c-xref-is-browser-window-displayed))
    (setq line (count-lines (point-min) (if (eobp) (point) (+ (point) 1))))
    (setq col (c-xref-current-column))
    (setq sym (c-xref-get-identifier-on-point))
    (setq sstr (completing-read "Symbol to browse: "
                                                'c-xref-symbols-completionfun nil nil sym))
    (c-xref-before-push-optional-update)
    (c-xref-call-process-with-basic-file-data-all-saves
     (format "\"-olcxpushname=%s\" -olcxlccursor=%d:%d" sstr line col))
    (c-xref-update-browser-if-displayed oldwins)
    ))

(defun c-xref-push-and-apply-macro ()
  "Push references, traverse them and apply macro to each one.

This function parses the current buffer, resolves the symbol
under point and pushes all its references (from the active
project) on to the browser stack. Then it traverses all
references starting at the last, checks whether the reference is
still pointing to the symbol and invokes a user defined macro on
each reference.

For example if you define an Emacs macro that deletes the old
name and inserts a new name, then this function will rename all
symbol occurences. Unlike a renaming refactoring there are no
safety checks.

NOTE: Be careful when using this function. Be sure that your
macro is not destructive.  Also, even though it is not a
refactoring, changes made by this function can be undone with the
`undo last refactoring' function.
"
  (interactive "")
  (let ((oldwins))
    (c-xref-entry-point-make-initialisations)
    (c-xref-multifile-undo-set-buffer-switch-point "mapping of a macro on all occurences")
    (c-xref-before-push-optional-update)
    (setq oldwins (c-xref-is-browser-window-displayed))
    (c-xref-call-process-with-basic-file-data-all-saves "-olcxpushandcallmacro")
    (c-xref-update-browser-if-displayed oldwins)
    ))

(defun c-xref-show-browser ()
  "Display browser windows."
  (interactive "")
  (let ((oldwins))
    (c-xref-entry-point-make-initialisations)
    (setq oldwins (c-xref-create-browser-windows nil c-xref-global-dispatch-data))
    (c-xref-update-browser-if-displayed oldwins))
  )

(defun c-xref-peek (opt)
  (c-xref-entry-point-make-initialisations)
  (c-xref-call-process-with-basic-file-data-all-saves "-olcxpushonly -olnodialog")
  (c-xref-call-process-with-basic-file-data-no-saves opt)
  (c-xref-call-process-with-basic-file-data-no-saves "-olcxpoponly -olnodialog")
  )

(defun c-xref-previous-next-reference (option direction)
  (let ((sw))
    (setq sw (selected-window))
    (c-xref-entry-point-make-initialisations-no-project-required)
    (c-xref-call-process-with-basic-file-data-no-saves option)
    (c-xref-move-current-reference-list-pointer-if-browser-displayed direction)
    (sit-for .5)
    (select-window sw)
    ))

(defun c-xref-next-reference ()
  "Move to the next reference stored on the top of the browser stack.
"
  (interactive "")
  (c-xref-previous-next-reference "-olcxnext" 1)
  )


(defun c-xref-previous-reference ()
  "Move to the previous reference stored on the top of the browser stack.
"
  (interactive "")
  (c-xref-previous-next-reference "-olcxprevious" -1)
  )

(defun c-xref-alternative-previous-reference ()
  "Move to the previous reference, error or refactoring error.

Move to the previous reference of the symbol under point. If a
compilation buffer is displayed in some window, then move to the
previous error instead. If a browser from an abandoned
refactoring is displayed in some window, then move to the
previous reference in this browser instead.
"
  (interactive "")
  (let ((sw) (rw))
    (setq rw (c-xref-browser-of-failed-refactoring-is-displayed))
    (if rw
            (progn
              (setq sw (selected-window))
              (select-window rw)
              (c-xref-move-current-reference-list-pointer -1)
              (c-xref-browser-dialog-select-one nil)
              (select-window sw)
              )
      (if (and c-xref-inspect-errors-if-compilation-window
                   (get-buffer-window c-xref-compilation-buffer))
              (c-xref-ide-previous-error)
            (c-xref-peek "-olcxprevious")
            ))
    ))

(defun c-xref-alternative-next-reference ()
  "Move to the next reference, error or refactoring error.

Move to the next reference of the symbol under point. If a
compilation buffer is displayed in some window, then move to the
next error instead.  If a browser from an abandoned refactoring
is displayed in some window, then move to the next reference in
this browser instead.
"
  (interactive "")
  (let ((sw) (rw))
    (setq rw (c-xref-browser-of-failed-refactoring-is-displayed))
    (if rw
            (progn
              (setq sw (selected-window))
              (select-window rw)
              (c-xref-move-current-reference-list-pointer 1)
              (c-xref-browser-dialog-select-one nil)
              (select-window sw)
              )
      (if (and c-xref-inspect-errors-if-compilation-window
                   (get-buffer-window c-xref-compilation-buffer))
              (c-xref-ide-next-error)
            (c-xref-peek "-olcxnext")
            ))
    ))

(defun c-xref-pop-and-return ()
  "Pop references from the top of the browser stack.

This function also moves to the position from where those
references were pushed.
"
  (interactive "")
  (let ((oldwins))
    ;; first check special contexts
    (if (eq c-xref-this-buffer-type 'completion)
            (c-xref-interactive-completion-previous nil)
      (if (eq c-xref-this-buffer-type 'tag-search-results)
              (c-xref-interactive-tag-search-previous nil)
            ;; O.K. here we are
            (c-xref-entry-point-make-initialisations-no-project-required)
            (setq oldwins (c-xref-is-browser-window-displayed))
            (c-xref-call-process-with-basic-file-data-no-saves "-olcxpop")
            (c-xref-update-browser-if-displayed oldwins)
            ))
    ))

(defun c-xref-pop-only ()
  "Pop references from the top of the browser stack. Do not move.
"
  (interactive "")
  (let ((oldwins))
    (c-xref-entry-point-make-initialisations-no-project-required)
    (setq oldwins (c-xref-is-browser-window-displayed))
    (c-xref-call-process-with-basic-file-data-no-saves "-olcxpoponly")
    (c-xref-update-browser-if-displayed oldwins)
    ))


(defun c-xref-re-push ()
  "Re-push references removed by the last pop command.

This function also moves to the current reference.
"
  (interactive "")
  (let ((oldwins))
    ;; first check special contexts
    (if (eq c-xref-this-buffer-type 'completion)
            (c-xref-interactive-completion-next nil)
      (if (eq c-xref-this-buffer-type 'tag-search-results)
              (c-xref-interactive-tag-search-next nil)
            ;; O.K. here we are
            (c-xref-entry-point-make-initialisations-no-project-required)
            (setq oldwins (c-xref-is-browser-window-displayed))
            (c-xref-call-process-with-basic-file-data-no-saves "-olcxrepush")
            (c-xref-update-browser-if-displayed oldwins)
            ))
    ))


;; backward compatibility functions
(defun c-xref-pushCxrefs () (c-xref-push-and-goto-definition))
(defun c-xref-listCxrefs () (c-xref-browse-symbol))
(defun c-xref-popCxrefs () (c-xref-pop-and-return))
(defun c-xref-poponly () (c-xref-pop-only))
(defun c-xref-list-top-references() (c-xref-show-browser))
(defun c-xref-previousref () (c-xref-previous-reference))
(defun c-xref-nextref () (c-xref-next-reference))
(defun c-xref-this-symbol-next-ref () (c-xref-alternative-next-reference))
(defun c-xref-this-symbol-previous-ref () (c-xref-alternative-previous-reference))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;; FUNCTIONS COMMON FOR VARIOUS DIALOGS  ;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-interactive-selectable-line-inspect (event option offset)
  (interactive "i")
  (let ((sw) (dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (setq sw (selected-window))
    (c-xref-call-task-on-line option offset)
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (sit-for .5)
    (select-window sw)
    ))

(defun c-xref-interactive-selectable-line-mouse-inspect (event option offset)
  (interactive "i")
  (let ((sw))
    (setq sw (selected-window))
    (mouse-set-point event)
    (beginning-of-line)
    (c-xref-interactive-selectable-line-inspect event option offset)
    (c-xref-modal-dialog-maybe-return-to-caller-window sw)
    ))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;; SYMBOL RETRIEVING ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-get-tags (str searchopt)
  (let ((res))
    (c-xref-call-process-with-basic-file-data-no-saves
     (format "\"-olcxtagsearch=%s\" %s" str searchopt))
    (setq res (cdr (assoc 'symbol-list c-xref-global-dispatch-data)))
    res
    ))

(defun c-xref-get-list-for-completions (ppp)
  (let ((res) (prs))
    (setq res nil)
    (while ppp
      (progn
            (setq res (cons (cons (car ppp) nil) res))
            (setq ppp (cdr ppp))
            ))
    res
    ))

(defun c-xref-list-to-alist (lst)
  (let ((res) (rres))
    (setq res nil)
    (if lst
            (progn
              (setq res (cons (cons (car lst) nil) nil))
              (setq rres res)
              (setq lst (cdr lst))
              (while lst
                (setcdr rres (cons (cons (car lst) nil) nil))
                (setq rres (cdr rres))
                (setq lst (cdr lst))
                )))
    res
    ))

(defun c-xref-symbols-completionfun (str filter type)
  (let ((res) (lst))
    (if (or (eq type t)
                (eq type nil))
            (progn
              ;; list of completions
              (setq lst (c-xref-list-to-alist
                             (c-xref-get-tags
                              (format "%s*" str)
                              (format "-searchshortlist -p \"%s\"" c-xref-active-project))))
              (if (eq type t)
                  (setq res (all-completions str lst))
                (setq res (try-completion str lst))
                ))
      (setq res nil)
      )
    res
    ))

(defun c-xref-get-search-string ()
  (let ((sstr) (sym) (poin) (table))
    (setq sym (c-xref-get-identifier-on-point))
    (setq sstr (completing-read "Expression to search (-help for help): "
                                                'c-xref-symbols-completionfun nil nil sym))
    (while (equal sstr "-help")
      (progn
            (c-xref-interactive-help  "
C-xrefactory search expressions are similar to standard shell
expressions.  They are composed from a sequence of characters
possibly containing wildcard characters.  The following wildcards
can be used: '*' expands to any (possibly empty) string, '?'
expands to any single character and '[...]'  expands to one of
the enclosed characters.  Ranges of characters can be included
between '[' and ']', so for example [a-zA-Z] matches any letter,
[0-9] matches any digit, as in shell expressions. If the first
character following the '[' is a '^' then the result of expansion
is inverted, for example '[^0-9]' expands to any non-digit
character.  A symbol is reported only if its name completely
matches the searched string.  Letters are matched case
insensitively except when enclosed between [ and ].

For example the expression '*get*' will report all symbols
containing the string 'get'.  Expression 'get*' will report all
symbols starting with the string 'get'.  The expression '[A-Z]*'
will report all symbols starting with an upper case letter.

If you enter an expression which does not contain any of the
wildcard characters '*', '?' or '[' then C-xrefactory reports all
symbols containing the entered string. For example, entering
'get' as the expression is equivalent to entering '*get*'.

" nil nil)
            (setq sstr (completing-read "Expression to search (-help for help): "
                                                    'c-xref-symbols-completionfun nil nil sym))
            ))
    sstr
    ))

(defun c-xref-interactive-tag-search-escape (event)
  (interactive "i")
  (c-xref-select-dispach-data-caller-window c-xref-this-buffer-dispatch-data)
  (c-xref-delete-window-in-any-frame c-xref-tag-results-buffer nil)
  )

(defun c-xref-interactive-tag-search-inspect (event)
  (interactive "i")
  (c-xref-interactive-selectable-line-inspect event "-olcxtaggoto" 0)
  )

(defun c-xref-interactive-tag-search-select (event)
  (interactive "i")
  (c-xref-interactive-selectable-line-inspect event "-olcxtagselect" 0)
  )

(defun c-xref-interactive-tag-search-previous-next (option)
  (let ((syms) (sw) (asc))
    (setq sw (selected-window))
    (c-xref-select-dispach-data-caller-window c-xref-this-buffer-dispatch-data)
    (c-xref-call-process-with-basic-file-data-no-saves option)
    (setq asc (assoc 'symbol-list c-xref-global-dispatch-data))
    (if asc
            (progn
              (setq syms (cdr asc))
              (c-xref-display-tag-search-results syms c-xref-global-dispatch-data nil)
              )
      (select-window sw)
      )
    ))

(defun c-xref-interactive-tag-search-next (event)
  (interactive "i")
  (c-xref-interactive-tag-search-previous-next "-olcxtagsearchforward")
  )

(defun c-xref-interactive-tag-search-previous (event)
  (interactive "i")
  (c-xref-interactive-tag-search-previous-next "-olcxtagsearchback")
  )

(defun c-xref-interactive-tag-search-mouse-inspect (event)
  (interactive "e")
  (c-xref-interactive-selectable-line-mouse-inspect event "-olcxtaggoto" 0)
  )

(defun c-xref-interactive-tag-search-mouse-select (event)
  (interactive "e")
  (c-xref-interactive-selectable-line-mouse-inspect event "-olcxtagselect" 0)
  )

(defun c-xref-interactive-tag-search-mouse-previous (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-tag-search-previous event)
  t
  )

(defun c-xref-interactive-tag-search-mouse-next (event)
  (interactive "e")
  (mouse-set-point event)
  (c-xref-interactive-tag-search-next event)
  t
  )

(defun c-xref-display-tag-search-results (syms dispatch-data searched-sym)
  (c-xref-delete-window-in-any-frame c-xref-tag-results-buffer nil)
  (c-xref-select-dispach-data-caller-window dispatch-data)
  (c-xref-display-and-set-new-dialog-window c-xref-tag-results-buffer nil t)
  (buffer-disable-undo c-xref-tag-results-buffer)
  (setq c-xref-this-buffer-type 'tag-search-results)
  (c-xref-erase-buffer)
  (while syms
    (insert (car syms))
    (newline)
    (setq syms (cdr syms))
    )
  (if searched-sym
      (c-xref-line-hightlight 0 (point-max) nil 1 (c-xref-create-tag-search-fontif searched-sym) nil)
    )
  (goto-char (point-min))
  (setq buffer-read-only t)
  (setq c-xref-this-buffer-dispatch-data dispatch-data)
  (c-xref-use-local-map c-xref-tag-search-mode-map)
  )

(defun c-xref-search-in-tag-file ()
  "Search for string(s) amongst references.

This function asks for strings to search, then it inspects the
references in the active project looking for symbols containing
the given string(s). For each such symbol it displays the symbol
name together with its definition location.  Strings are matched
against the full symbol name.
"
  (interactive "")
  (let ((sym) (syms) (line) (col))
    ;;(load "profile")
    ;;(setq profile-functions-list (list 'c-xref-server-dispatch-require-end-ctag 'c-xref-server-dispatch-get-int-attr 'c-xref-server-parse-xml-tag 'c-xref-server-dispatch-symbol-list))
    ;;(profile-functions profile-functions-list)
    (c-xref-entry-point-make-initialisations)
    (setq sym (c-xref-get-search-string))
    (setq line (count-lines (point-min) (if (eobp) (point) (+ (point) 1))))
    (setq col (c-xref-current-column))
    (setq syms (c-xref-get-tags sym (format "-olinelen=%d -olcxlccursor=%d:%d" (window-width) line col)))
    (c-xref-display-tag-search-results syms c-xref-global-dispatch-data sym)
    ;;(profile-results)
    ))

(defun c-xref-search-definition ()
  "Search for a symbol defined in the project.

This function asks for strings to search, then it inspects the
tags of the active project looking for symbols containing the
given string(s). For each such symbol it displays the symbol name
together with its definition location.  When looking for a symbol
only the symbol name (the identifier) is checked against the
given string(s).

"
  (interactive "")
  (let ((sym) (syms) (line) (col))
    (c-xref-entry-point-make-initialisations)
    (setq sym (c-xref-get-search-string))
    (setq line (count-lines (point-min) (if (eobp) (point) (+ (point) 1))))
    (setq col (c-xref-current-column))
    (setq syms (c-xref-get-tags sym (format "-searchdef -olinelen=%d -olcxlccursor=%d:%d" (window-width) line col)))
    (c-xref-display-tag-search-results syms c-xref-global-dispatch-data sym)
    ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;; SYMBOL RESOLUTION DIALOG ;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-call-task-on-line (opt offset)
  (let ((sw) (cline) (dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (setq sw (selected-window))
    (setq cline (count-lines (point-min) (if (eobp) (point) (+ (point) 1))))
    (c-xref-send-data-to-process-and-dispatch
     (format "%s%d" opt (+ cline offset))
     dispatch-data
     nil)
    (select-window sw)
    ))

(defun c-xref-browser-dialog-previous-line (event)
  (interactive "i")
  (c-xref-modal-dialog-previous-line event)
  )

(defun c-xref-browser-dialog-next-line (event)
  (interactive "i")
  (c-xref-modal-dialog-next-line event)
  )

(defun c-xref-close-resolution-dialog-windows (dispatch-data)
  (let ((winassoc) (win))
    (setq winassoc (assoc 'linked-resolution-window dispatch-data))
    (if winassoc
            (progn
              (setq win (cdr winassoc))
              (if (and win (windowp win) (window-live-p win))
                  (progn
                        (setq c-xref-symbol-selection-window-width (window-width (cdr winassoc)))
                        (setq c-xref-symbol-selection-window-height (window-height (cdr winassoc)))
                        (delete-window (cdr winassoc))
                        ))
          ))
    (setq winassoc (assoc 'linked-refs-window dispatch-data))
    (if winassoc
            (progn
              (setq win (cdr winassoc))
              (if (and win (windowp win) (window-live-p win))
                  (delete-window (cdr winassoc))
                )))
    (c-xref-soft-delete-window c-xref-browser-info-buffer)

    (setq winassoc (assoc 'caller-window dispatch-data))
    (if winassoc (select-window (cdr winassoc)))
    ))

(defun c-xref-cancel-with-error (event)
  (interactive "i")
  (error "Cancel.")
  )

(defun c-xref-browser-dialog-exit (event)
  (interactive "i")
  (let ((dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (c-xref-close-resolution-dialog-windows dispatch-data)

    ;; kill refactorer process if refactoring
    (if (eq (cdr (assoc 'process dispatch-data)) 'c-xref-refactorer-process)
            (progn
              (delete-process (car c-xref-refactorer-process))
              (setq c-xref-refactorer-process nil)
              (error "Canceled")
              ))
    ))

(defun c-xref-browser-dialog-break (event)
  (interactive "i")
  (c-xref-soft-delete-window c-xref-browser-info-buffer)
  ;; if called from non-modal mode, close the dialog
  (if (eq this-command 'c-xref-browser-dialog-break)
      (c-xref-browser-dialog-exit event)
    (error (substitute-command-keys "Refactoring abandoned but browser dialog is active (on \\[c-xref-alternative-previous-reference] and \\[c-xref-alternative-next-reference])."))
    )
  )

(defun c-xref-browser-dialog-other-window (event)
  (interactive "i")
  (let ((dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (if (eq c-xref-this-buffer-type 'reference-list)
            (c-xref-select-dispach-data-resolution-window dispatch-data)
      (c-xref-select-dispach-data-refs-window dispatch-data)
      )
    ))

(defun c-xref-browser-dialog-set-selection-sign (sel)
  (beginning-of-line)
  (setq buffer-read-only nil)
  (delete-char 1)
  (if (equal sel 1)
      (insert "+")
    (insert " ")
    )
  (setq buffer-read-only t)
  (beginning-of-line)
  )


(defun c-xref-blink-in-caller-window (dispatch-data)
  (let ((sw))
    (setq sw (selected-window))
    (c-xref-select-dispach-data-caller-window dispatch-data)
    (sit-for .5)
    (select-window sw)
    ))

(defun c-xref-browser-dialog-set-selection-sign-on-all (sign)
  (let ((str))
    (save-excursion
      (goto-char (point-min))
      (while (not (equal (point) (point-max)))
        (beginning-of-line)
        (setq str (buffer-substring (point) (+ (point) 1)))
        (if (or (string-equal str " ") (string-equal str "+"))
                (progn
                  (c-xref-browser-dialog-set-selection-sign sign)
                  ))
        (forward-line 1)
        ))
    ))

(defvar c-xref-menu-selection-line-offset 0)

(defun c-xref-browser-dialog-select-all (event)
  (interactive "i")
  (let ((res) (sw) (dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (setq sw (selected-window))
    (if (eq c-xref-this-buffer-type 'symbol-list)
            (progn
              (c-xref-browser-dialog-set-selection-sign-on-all 1)
              (c-xref-send-data-to-process-and-dispatch "-olcxmenuall" dispatch-data nil)
              ))
    (select-window sw)
    ))

(defun c-xref-browser-dialog-select-none (event)
  (interactive "i")
  (let ((res) (sw) (dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (setq sw (selected-window))
    (if (eq c-xref-this-buffer-type 'symbol-list)
            (progn
              (c-xref-browser-dialog-set-selection-sign-on-all 0)
              (c-xref-send-data-to-process-and-dispatch "-olcxmenunone" dispatch-data nil)
              ))
    (select-window sw)
    ))

(defun c-xref-browser-dialog-select-one (event)
  (interactive "i")
  (let ((res) (sw) (str) (dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (setq sw (selected-window))
    (if (eq c-xref-this-buffer-type 'symbol-list)
            (progn
              (beginning-of-line)
              (setq str (buffer-substring (point) (+ (point) 1)))
              (if (or (string-equal str " ") (string-equal str "+"))
                  (progn
                        (c-xref-browser-dialog-set-selection-sign-on-all 0)
                        (c-xref-browser-dialog-set-selection-sign 1)
                        (c-xref-call-task-on-line "-olcxmenusingleselect" c-xref-menu-selection-line-offset)
                        (c-xref-select-dispach-data-caller-window dispatch-data)
                        (sit-for .5)
                        (select-window sw)
                        )))
      (if (eq c-xref-this-buffer-type 'reference-list)
              (progn
                (c-xref-call-task-on-line "-olcxgoto" 0)
                (c-xref-set-current-reference-list-pointer)
                (c-xref-select-dispach-data-caller-window dispatch-data)
                (sit-for .5)
                (select-window sw)
                )))
    ))

(defun c-xref-browser-dialog-toggle (event)
  (interactive "i")
  (let ((res) (str) (ns) (sw) (dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (if (eq c-xref-this-buffer-type 'symbol-list)
            (progn
              (setq sw (selected-window))
              (beginning-of-line)
              (setq str (buffer-substring (point) (+ (point) 1)))
              (if (or (string-equal str " ") (string-equal str "+"))
                  (progn
                        (if (string-equal str " ") (setq ns 1) (setq ns 0))
                        (c-xref-browser-dialog-set-selection-sign ns)
                        (c-xref-call-task-on-line "-olcxmenuselect" c-xref-menu-selection-line-offset)
                        (c-xref-select-dispach-data-caller-window dispatch-data)
                        (sit-for .5)
                        (select-window sw)
                        ))
              (c-xref-modal-dialog-next-line event)
              ))
    ))

(defun c-xref-browser-dialog-next-reference (event)
  (interactive "i")
  (let ((other-win))
    (setq other-win nil)
    (if (eq c-xref-this-buffer-type 'symbol-list)
            (progn
              (setq other-win t)
              (c-xref-browser-dialog-other-window event)
              ))
    (c-xref-browser-dialog-next-line event)
    (c-xref-browser-dialog-select-one event)
    (if other-win (c-xref-browser-dialog-other-window event))
    ))

(defun c-xref-browser-dialog-previous-reference (event)
  (interactive "i")
  (let ((other-win))
    (setq other-win nil)
    (if (eq c-xref-this-buffer-type 'symbol-list)
            (progn
              (setq other-win t)
              (c-xref-browser-dialog-other-window event)
              ))
    (c-xref-browser-dialog-previous-line event)
    (c-xref-browser-dialog-select-one event)
    (if other-win (c-xref-browser-dialog-other-window event))
    ))

(defun c-xref-modal-dialog-shift-left (event)
  (interactive "i")
  (c-xref-scroll-right)
  )

(defun c-xref-modal-dialog-shift-right (event)
  (interactive "i")
  (c-xref-scroll-left)
  )

(defun c-xref-mouse-set-point (event)
  (let ((sw) (ew) (res))
    (setq res nil)
    (setq sw (selected-window))
    (setq ew (posn-window (event-end event)))
    (if (windowp ew)
            (progn
              (select-window ew)
              (if (or (eq c-xref-this-buffer-type 'symbol-list)
                          (eq c-xref-this-buffer-type 'reference-list))
                  (progn
                        (mouse-set-point event)
                        (setq res t)
                        )
                (select-window sw)
                )))
    res
    ))

(defun c-xref-modal-dialog-maybe-return-to-caller-window (sw)
  (let ((cw))
    (setq cw (cdr (assoc 'caller-window c-xref-this-buffer-dispatch-data)))
    (if (equal cw sw)
            (select-window sw)
      )
    ))

(defun c-xref-undefined (event)
  (interactive "i")
  )

(defun c-xref-scroll-if-on-first-or-last-line ()
  (let ((dir))
    (setq dir nil)
    (save-excursion
      (if (eq (forward-line -1) 0)
              (if (< (point) (window-start))
                  (setq dir 'down)
                )
            ))
    (save-excursion
      (if (eq (forward-line 1) 0)
              (if (and (>= (point) (window-end)) (not (eq (point) (point-max))))
                  (setq dir 'up)
                )
            ))
    (if (eq dir 'up)
            (scroll-up 1)
      (if (eq dir 'down)
              (scroll-down 1)
            ))
    ))

(defun c-xref-modal-dialog-mouse-button1 (event)
  (interactive "e")
  (let ((sw))
    (setq sw (selected-window))
    (c-xref-mouse-set-point event)
    (beginning-of-line)
    ;; scrolling in Emacs is mysterious:
    ;; sometimes it goes automatically, sometimes not
    (if (eq c-xref-this-buffer-type 'reference-list)
            (c-xref-scroll-if-on-first-or-last-line)
      )
    (c-xref-browser-dialog-select-one event)
    (c-xref-modal-dialog-maybe-return-to-caller-window sw)
    ))

(defun c-xref-modal-dialog-mouse-button2 (event)
  (interactive "e")
  (c-xref-mouse-set-point event)
  (beginning-of-line)
  (c-xref-browser-dialog-toggle event)
  )

(defun c-xref-modal-dialog-continue (event)
  (interactive "i")
  ;; first try to execute some default action
  (if (or (eq c-xref-this-buffer-type 'reference-list)
              (eq c-xref-this-buffer-type 'symbol-list))
      (c-xref-browser-dialog-select-one event)
    (error "Not a modal dialog, probably an aborted action, no continuation defined.")
    )
  )

(defun c-xref-references-set-filter (level dispatch-data)
  ;; save all files, so that references refer to buffers rather than files
  (c-xref-server-call-on-current-buffer-all-saves
   (format "-olcxfilter=%d" level)
   dispatch-data)
  )

(defun c-xref-browser-dialog-set-filter (level dispatch-data)
  (c-xref-select-dispach-data-resolution-window dispatch-data)
  (setq c-xref-this-buffer-filter-level level)
  (c-xref-send-data-to-process-and-dispatch
   (format "-olcxmenufilter=%d" level)
   dispatch-data
   nil)
  (c-xref-references-set-filter 0 dispatch-data)
  (c-xref-select-dispach-data-resolution-window dispatch-data)
  )

(defun c-xref-browser-dialog-set-new-filter (dispatch-data)
  (let ((level))
    (c-xref-select-dispach-data-resolution-window dispatch-data)
    (setq level c-xref-default-symbols-filter)
    (if c-xref-keep-last-symbols-filter
            (setq level c-xref-this-buffer-filter-level)
      )
    (c-xref-browser-dialog-set-filter level dispatch-data)
    ))

(defun c-xref-browser-or-refs-set-filter (level)
  (let ((dispatch-data))
    (setq dispatch-data c-xref-this-buffer-dispatch-data)
    (if (eq c-xref-this-buffer-type 'symbol-list)
            (progn
              (if (and (>= level 0) (<= level 2))
                  (c-xref-browser-dialog-set-filter level dispatch-data)
                (error "filter level out of range <0,2>")
                ))
      (if (eq c-xref-this-buffer-type 'reference-list)
              (progn
                (if (and (>= level 0) (<= level 3))
                        (c-xref-references-set-filter level dispatch-data)
                  (error "filter level out of range <0,3>")
                  ))))
    ))

(defun c-xref-interactive-browser-dialog-set-filter (event)
  (interactive "i")
  (let ((level))
    (setq level (string-to-number (char-to-string last-command-event)))
    (c-xref-browser-or-refs-set-filter level)
    ))

(defun c-xref-interactive-browser-dialog-set-filter0 (event)
  (interactive "i")
  (c-xref-browser-or-refs-set-filter 0)
  )

(defun c-xref-interactive-browser-dialog-set-filter1 (event)
  (interactive "i")
  (c-xref-browser-or-refs-set-filter 1)
  )

(defun c-xref-interactive-browser-dialog-set-filter2 (event)
  (interactive "i")
  (c-xref-browser-or-refs-set-filter 2)
  )

(defun c-xref-interactive-browser-dialog-set-filter3 (event)
  (interactive "i")
  (c-xref-browser-or-refs-set-filter 3)
  )

(defun c-xref-browser-symbols-help ()
  (c-xref-interactive-help
   "Special hotkeys available:

\\[c-xref-browser-dialog-select-one] \t-- select one symbol
\\[c-xref-browser-dialog-toggle] \t-- toggle selected/unselected
\\[c-xref-browser-dialog-select-all] \t-- select all
\\[c-xref-browser-dialog-select-none] \t-- unselect all
\\[c-xref-browser-dialog-other-window] \t-- switch to other window
0 \t-- filter 0
1 \t-- filter 1
2 \t-- filter 2
\\[c-xref-modal-dialog-shift-right] \t-- scroll right
\\[c-xref-modal-dialog-shift-left] \t-- scroll left
\\[c-xref-resize-right] \t-- resize right
\\[c-xref-resize-left] \t-- resize left
\\[c-xref-browser-dialog-exit] \t-- close browser
\\[c-xref-browser-dialog-break] \t-- abandon modal dialog but keep browser
\\[c-xref-modal-dialog-continue] \t-- continue
\\[c-xref-interactive-browser-dialog-help] \t-- toggle this help page
" nil nil)
  )

(defun c-xref-browser-refs-help ()
  (c-xref-interactive-help
   "Special hotkeys available:

\\[c-xref-browser-dialog-select-one] \t-- inspect reference
\\[c-xref-browser-dialog-other-window] \t-- switch to other window
\\[c-xref-browser-dialog-previous-reference] \t-- previous reference
\\[c-xref-browser-dialog-next-reference] \t-- next reference
\\[c-xref-browser-dialog-exit] \t-- close browser
0 \t-- filter 0
1 \t-- filter 1
2 \t-- filter 2
3 \t-- filter 3
\\[c-xref-modal-dialog-shift-right] \t-- scroll right
\\[c-xref-modal-dialog-shift-left] \t-- scroll left
\\[c-xref-resize-right] \t-- resize right
\\[c-xref-resize-left] \t-- resize left
\\[c-xref-modal-dialog-continue] \t-- continue
\\[c-xref-interactive-browser-dialog-help] \t-- toggle this help page
" nil nil)
  )

(defun c-xref-interactive-browser-dialog-help (event)
  (interactive "i")
  (if (eq c-xref-this-buffer-type 'symbol-list)
      (c-xref-browser-symbols-help)
    (c-xref-browser-refs-help)
    )
  )

;;;;;;;;;;;;;;;;;;;;; mouse3 menu ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-browser-3b-mouse-selected (event)
  (interactive "e")
  (mouse-set-point event)
  (cond
   (
    (eq last-input-event 'c-xref-bm-3b-sel-one)
    (c-xref-browser-dialog-select-one event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-toggle)
    (c-xref-browser-dialog-toggle event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-sel-all)
    (c-xref-browser-dialog-select-all event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-sel-none)
    (c-xref-browser-dialog-select-none event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-filt0)
    (c-xref-interactive-browser-dialog-set-filter0 event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-filt1)
    (c-xref-interactive-browser-dialog-set-filter1 event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-filt2)
    (c-xref-interactive-browser-dialog-set-filter2 event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-filt3)
    (c-xref-interactive-browser-dialog-set-filter3 event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-close)
    (c-xref-browser-dialog-exit event)
    )
   (
    (eq last-input-event 'c-xref-bm-3b-cont)
    (c-xref-modal-dialog-continue event)
    )
   )
  )


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;; MISC MENU ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-kill-xref-process (coredump)
  "Kill currently running  c-xref process.

If there is an c-xref process creating/updating a reference
database it is killed.  If there is no such process this function
kills the c-xref server process.  This function can be used if
the c-xref task enters an inconsistent state.  The c-xref process
is then restarted automatically at the next invocation of any of
its functions.
"
  (interactive "P")
  (if (and (not (eq c-xref-tags-process nil))
               (eq (process-status (car c-xref-tags-process)) 'run))
      (progn
            (delete-process (car c-xref-tags-process))
            (setq c-xref-tags-process nil)
            (message "Extern c-xref process killed.")
            )
    (setq c-xref-tags-process nil)
    (if (not (eq c-xref-server-process nil))
            (progn
              (if current-prefix-arg
                  (progn
                        (shell-command (format "kill -3 %d && echo Core dumped into this buffer directory." (process-id (car c-xref-server-process))))
                        )
                (delete-process (car c-xref-server-process))
                (setq c-xref-server-process nil)
                (message "Emacs c-xref server process killed.")
                ))
      (message "** No process to kill. **")
      ))
  )

(defvar c-xref-tutorial-directory (format "%s/../../doc/cexercise" (file-name-directory load-file-name))
  "*Directory for tutorial files belonging to package \`c-xrefactory'.")

(require 'dired-aux)

(defun c-xref-tutorial (lang)
  "Run the tutorial."
  (interactive "P")
  (let ((temp-dir (format "/tmp/c-xref-tutorial-%d/" (emacs-pid))))
    (make-directory temp-dir t)
    (dired-copy-file-recursive c-xref-tutorial-directory temp-dir nil nil nil 'always)
    (find-file (concat (file-name-as-directory temp-dir) "cexercise/index.c"))
    )
  )

(defun c-xref-about (exit)
  "Show C-xref version."
  (interactive "P")
  (c-xref-entry-point-make-initialisations-no-project-required)
  ;; if called with prefix argument, exit server task
  ;; to be used for profiler informations
  (setq c-xref-global-dispatch-data (c-xref-get-basic-server-dispatch-data 'c-xref-server-process))
  (if current-prefix-arg
      (c-xref-send-data-to-process-and-dispatch "-about -exit" c-xref-global-dispatch-data nil)
    (c-xref-send-data-to-process-and-dispatch "-about" c-xref-global-dispatch-data nil)
    )
  )

(defvar c-xref-elisp-directory
  (file-name-directory (or load-file-name (buffer-file-name)))
  "The directory where the e-lisp files of c-xref are installed")

(defvar c-xref-install-directory
  (directory-file-name
   (file-name-directory
    (directory-file-name
     (file-name-directory
      (directory-file-name
       (file-name-directory
        (or load-file-name (buffer-file-name))))))))
  "The directory where c-xref is installed, which is two levels above this file.")

(defun c-xref-load-elisp-directory (dir)
  (let ((load-it (lambda (f)
                   (load-file (concat (file-name-as-directory dir) f)))
                 ))
    (mapc load-it (directory-files dir nil "\\.el$"))))

(defun c-xref-get-modified-files ()
  "Return a list of modified files in the current Git repository."
  (with-temp-buffer
    (call-process "git" nil t nil "diff" "--name-only" "HEAD")
    (split-string (buffer-string) "\n" t)))

(defun c-xref-ok-to-upgrade ()
  "Ask for confirmation and then perform `git reset --hard' if
there are modified files. Returns t if there were no changes or
the reset was performed, nil if the reset was cancelled."
  (let ((modified-files (c-xref-get-modified-files)))
    (if (and modified-files
                 (not (null modified-files))
                 (yes-or-no-p "There are modified files. Do you want to overwrite?"))
            (progn
              (call-process "git" nil nil nil "reset" "--hard")
              t)  ; Indicate reset performed
      (if modified-files
              (progn
                nil)  ; Indicate cancellation
            t))))  ; Indicate no changes

(defun c-xref-reload-directory (dir)
  "Load all compiled Emacs Lisp files in DIR."
  (dolist (file (directory-files dir t "\\.elc$"))
    (message (format "file = %s" file))
    (load file nil t))
  )

(defun c-xref-upgrade ()
  "Upgrade the installed c-xref, if available."
  (interactive)
  (if (yes-or-no-p (format "Really upgrade c-xref installation in %s ? "
                           c-xref-install-directory))
      (progn
        (let ((default-directory c-xref-install-directory))
          (if (c-xref-ok-to-upgrade)
              (progn
                (shell-command "git fetch origin") ;; Ensure remote is updated
                (shell-command "git checkout stable") ;; Switch to branch `stable`
                (shell-command "git reset --hard origin/stable") ;; Sync with remote
                (message "Pulled latest stable")
                (c-xref-kill-xref-process nil)
                (delete-file "src/options_config.h") ;; Trigger regeneration of version info
                (unless (zerop (shell-command "make"))
                  (error "Build failed"))
                (message "Built 'c-xref' in %s" c-xref-install-directory)
                (c-xref-reload-directory c-xref-elisp-directory)
                (message "Reloaded compiled elisp files")
                )
            )
          ))
    )
  )

(defun c-xref-interactive-help-escape ()
  (interactive "")
  (delete-window (selected-window))
  )

(defvar c-xref-help-mode-map
  (let ((map (make-keymap)))
    (define-key map "\e" 'c-xref-interactive-help-escape)
    (define-key map "q" 'c-xref-interactive-help-escape)
    map)
  "Keymap for c-xref help buffer."
  )


(defun c-xref-help ()
  "Show basic help informations for use of C-xrefactory."
  (interactive "")
  (let ((iw))
    (setq iw (c-xref-display-and-set-new-dialog-window c-xref-info-buffer nil t))
    (insert " C-xref Help.

C-xrefactory is a refactoring development environment for C (and
Yacc).  Its functions can be accessed via the `c-xref' menu.
Optionally the most frequently used functions are accessible via
hot-keys.  Documentation of a particular function is available
through the \"C-h k\" key combination (i.e.  the `control' key
together with `h' key and then the `k' key) followed by mouse
selection of the corresponding menu item.

PROJECTS:

C-xrefactory is project based: you will need to create or select
an `active project' before doing anything else. In particular,
you should start your work with C-xrefactory by invoking the
'Project -> New' menu item. C-xrefactory will also prompt for
creating a project if you invoke a C-xrefactory function, such as
'Push This Symbol and Goto Definition' in a file not belonging to
a project.

C-xrefactory will automatically try to identify which project you
are working with from the location of the current source file.

Projects may use #ifdef:s and to cover code inside all such
conditional segments C-xrefactory might need to do multiple
passes over the source with different preprocessor settings. You
can define them during the creation of the project or modify them
later. See CUSTOMIZATION below.

REFERENCES DATABASE:

The references database stores all the necessary information
about your project sources, in particular informations about all
symbols, their linking properties, definition place(s) and all
usages.  The maintenance of the database is the responsibility of
the user.  An out of date database will cause mistakes in source
browsing.  However, some functions, like the `Complete
Identifier' function and the 'Extract Function/Macro/Variable'
refactorings, are independent of the database as they depend only
on file-local information.

Three functions are available for maintenance of the references
database, trading time against accuracy.  The `Full Update'
function should guarantee correct content of the database; it is
recomended to re-create the database from time to time in order
to remove garbage. Particularaly when navigation seems to be off.

BROWSING AND REFACTORINGS:

With browsing we mean jumping between all usages of the same
symbol. Note that with c-xref semantically different symbols are
considered different. You can start such browsing either by
placing the position on a symbol and activating the function
c-xref-push-and-goto-definition, which is usually bound to
function key F6, or from the menu `Browsing with Symbol
Stack'. Once you have pushed a symbol you can navigate through
all detected occurences with F3 and F4. F5 will pop that symbol
and move back to where the position was before the symbol
push. This can be done recursively to any depth.

To invoke a refactoring (bound to F11) you have to position the
point on the browsed (refactored) symbol. For a rename you place
the cursor on a symbol, press F11 and select `Rename'.

For parameter manipulations (adding or deleting a parameter) you
need to position the point on the name of the function/macro,
not on the parameter itself.  Before invoking any of the `Extract
Function/Macro/Variable' functions you need to select a
region (with your mouse or by specifying begin of the region by
C-<space> and the end by the point).

C-xrefactory may open some dialogs in a horizontally split
window; in general in those screens the middle mouse button makes
a choice, and the right button gives a menu of available actions.

C-XREF BACKGROUND TASK:

Emacs C-xref functions cooperate with an external `c-xref' task;
if you think that the task has entered an inconsistent state, or
if you wish to interrupt creation or update of the references
database, you can invoke the `Kill c-xref task' function.

CUSTOMIZATION:

C-xrefactory can be customized via the `C-xref -> Options' menu
item and via the ~/.c-xrefrc configuration file.  The `Options'
menu item customizes project independent behaviour which is
mainly the user interface.  In the `.c-xrefrc' file you can
specify your projects' settings and preferences.  There are many
options you can customize via .c-xrefrc; for more information
read the c-xref and c-xrefrc man pages. Before using C-xrefactory
you should also read the README file included in the distribution
package.

TUTORIAL:

You can invoke a tutorial from the menu, `C-xref -> Misc -> Tutorial'.

")
    (goto-char (point-min))
    (c-xref-use-local-map c-xref-help-mode-map)
    ))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; UNDO ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defvar c-xref-multifile-undo-state nil)

(defun c-xref-this-buffer-undo-state ()
  (undo-boundary)
  (cdr buffer-undo-list)
  )

(defun c-xref-is-buffer-undoable (bname)
  (let ((bnamestart) (res))
    (if (not bname)
            (setq res nil)
      (setq bnamestart (substring bname 0 1))
      (if (or (equal bnamestart " ") (equal bnamestart "*"))
              (setq res nil)
            (if (c-xref-buffer-has-one-of-suffixes bname c-xref-undo-allowed-suffixes)
                (setq res t)
              (setq res nil)
              )))
    res
    ))

(defun c-xref-undoable-move-file (new-fname)
  (let ((old))
    (setq old (buffer-file-name))
    (c-xref-multifile-undo-add-buffer-write)
    (c-xref-write-file new-fname)
    (c-xref-delete-file old)
    ))

(defun c-xref-resave-buffers-from-directory (olddir newdir)
  (let ((bb) (cb) (fn) (nfn) (res))
    (setq bb (buffer-list))
    (setq res nil)
    (setq olddir (c-xref-backslashify-name olddir))
    (while bb
      (setq cb (car bb))
      (setq fn (c-xref-backslashify-name (buffer-file-name cb)))
      (if (c-xref-string-has-prefix fn olddir (eq c-xref-platform 'windows))
              (progn
                (set-buffer cb)
                (setq nfn (concat newdir
                                          (c-xref-cut-string-prefix fn olddir (eq c-xref-platform 'windows))))
                (set-visited-file-name nil)
                (c-xref-write-file nfn)
                ))
      (setq bb (cdr bb))
      )
    res
    ))

(defun c-xref-undoable-move-directory (olddir newdir)
  (let ((pdir) (blinks))
    ;; create parent dir first (if it doesn't exist)
    (setq pdir (c-xref-file-directory-name newdir))
    (if (not (file-attributes pdir)) (make-directory pdir))
    ;;  (setq blinks (c-xref-unlink-buffers-from-directory olddir))
    (c-xref-save-some-buffers nil)
    (c-xref-move-directory olddir newdir)
    (c-xref-multifile-undo-add-move-directory olddir newdir)
    (c-xref-resave-buffers-from-directory olddir newdir)
    (if (file-attributes olddir) (delete-directory olddir))
    ;;  (c-xref-resave-unlinked-buffers blinks olddir newdir)
    ))

(defun c-xref-editor-undo-state ()
  (let ((state) (buffs) (bst) (buf))
    (save-excursion
      (setq state nil)
      (setq buffs (buffer-list))
      ;;    (setq buffs (append buffs (cons (current-buffer) nil)))
      (while buffs
            (setq buf (car buffs))
            (setq buffs (cdr buffs))
            (if (c-xref-is-buffer-undoable (buffer-file-name buf))
                (progn
                  (set-buffer buf)
                  (setq bst (c-xref-this-buffer-undo-state))
                  (setq state (cons (cons buf bst) state))
                  ))))
    state
    ))

(defun get-corresponding-undo (ulist buf)
  (let ((ul) (found) (res))
    (setq ul ulist)
    (setq found nil)
    (setq res nil)
    (while (and ul (not found))
      (setq found (eq (car (car ul)) buf))
      (if (not found)
              (setq ul (cdr ul))
            )
      )
    (if found
            (setq res (cdr (car ul)))
      )
    res
    ))

(defun c-xref-save-modified-files-with-question-and-error (flag message)
  (if (c-xref-yes-or-no-window (concat message "Save modified buffers? ") t nil)
      (progn
            (c-xref-save-some-buffers flag)
            (error "Changes saved.")
            )
    (error "Not saved.")
    ))

(defun c-xref-save-modified-files-with-question (flag message)
  (if (c-xref-yes-or-no-window (concat message "Save modified buffers? ") t nil)
      (progn
            (c-xref-save-some-buffers flag)
            (message "Changes saved.")
            )
    (message "Not saved.")
    ))

(defun undo-changes-until (bundo interact-flag)
  (let ((cc) (aa) (iinteract) (loop))
    (setq iinteract interact-flag)
    (setq loop t)
    (while (and pending-undo-list (not (eq pending-undo-list bundo)))
      (c-xref-make-buffer-writable)
      (undo-more 1)
      (if iinteract
              (progn
                (setq aa (c-xref-get-single-yes-no-event nil "Continue undoing "))
                )
            (setq aa 'answer-yes)
            )
      (if (eq aa 'answer-no)
              (c-xref-save-modified-files-with-question-and-error nil "** Undoing breaked. ")
            )
      (if (eq aa 'answer-all)
              (setq iinteract nil)
            )
      (message "")
      (undo-boundary)
      )
    (if (not (eq pending-undo-list bundo))
            (error "Not enough undo information available (check 'undo-limit' and 'undo-strong-limit' variables)!")
      )
    iinteract
    ))

(defun c-xref-undo-single-buffer (buf ustate cont-ustate interact-flag)
  (let ((bnamestart) (bufundo) (iinteract))
    (setq iinteract interact-flag)
    (if (not (buffer-name buf))
            (progn
              (if (not (yes-or-no-p (format "A killed buffer can't be undone, continue? ")))
                  (c-xref-save-modified-files-with-question-and-error nil "Aborted. ")
                )
              )
      (if (c-xref-is-buffer-undoable (buffer-file-name buf))
              (progn
                (switch-to-buffer buf)
                (setq bufundo (get-corresponding-undo ustate buf))
                (if cont-ustate
                        (setq pending-undo-list (get-corresponding-undo
                                                             cont-ustate buf))
                  (c-xref-make-buffer-writable)
                  (undo-start)
                  (if bufundo (undo-more 1))
                  )
                (setq iinteract (undo-changes-until bufundo iinteract))
                ))
      )
    iinteract
    ))

(defun c-xref-undo-until-undo-state (ustate cont-ustate interact)
  (let ((buffs) (buf) (buffs2) (bufundo) (iinteract))
    (setq iinteract interact)
    (setq buffs (buffer-list))
    ;; first process bufferes without undo-state
    (while buffs
      (setq buf (car buffs))
      (setq buffs (cdr buffs))
      (setq bufundo (get-corresponding-undo ustate buf))
      (if (not bufundo)
              (setq iinteract (c-xref-undo-single-buffer buf ustate cont-ustate iinteract))
            )
      )
    ;; than those from list
    (setq buffs2 ustate)
    (while buffs2
      (setq buf (car (car buffs2)))
      (setq buffs2 (cdr buffs2))
      (setq bufundo (get-corresponding-undo ustate buf))
      (if bufundo
              (setq iinteract (c-xref-undo-single-buffer buf ustate cont-ustate iinteract))
            )
      )
    ))

(defun cut-long-refactoring-undo-list ()
  (let ((ul) (deep))
    (if (> (length c-xref-multifile-undo-state) c-xref-multifile-undo-deep)
            (progn
              (setq ul c-xref-multifile-undo-state)
              (setq deep c-xref-multifile-undo-deep)
              (while (and ul (> deep 0))
                (setq deep (- deep 1))
                (setq ul (cdr ul))
                )
              (if ul
                  (setcdr ul nil)
                ))
      )
    ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-undo-last-refactoring (rd)
  "Undo a series of refactorings.

This function undos changes made in multiple buffers.  Buffers
are switched at the start of refactorings. The undo itself is
considered as a refactoring, so further invocations can be used
to re-do undone changes.  It uses the standard Emacs undo
mecanism which may not be appropriate for such a massive undo; in
case of problems check your `undo-strong-limit' and `undo-limit'
variable settings.  Undo can not be performed correctly if you
have killed a modified buffer or saved a buffer with a different
name.

NOTE!  Do not kill any buffer if you are using undo!
C-xrefactory memorizes the state of all opened buffers at the
beginning of each refactoring. If any opened buffer is killed in
between, undo will not work correctly.
"
  (interactive "i")
  (let ((undolist) (cloop) (cont-undo-state) (aa) (cbuff) (interact)
            (u-type) (next-ask))
    (setq interact c-xref-detailed-refactoring-confirmations)
    (setq cbuff (current-buffer))
    (if (eq c-xref-multifile-undo-state nil)
            (error "** Refactorings undo stack is empty **")
      )
    (set-marker c-xref-undo-marker (point))
    (setq cont-undo-state nil)
    (setq cloop t)
    (setq undolist c-xref-multifile-undo-state)
    (setq cloop (c-xref-yes-or-no-window
                         (format "Really undo %s? " (car (car undolist)))
                         t nil
                         ))
    (while cloop
      (c-xref-multifile-undo-set-buffer-switch-point (format "an undone refactoring (redoing %s)" (car (car undolist))))
      (setq next-ask t)
      (setq u-type (car (cdr (car undolist))))
      (if (equal u-type "move-buffer")
              (progn
                (setq interact (c-xref-undo-file-moving (cdr (cdr (car undolist))) interact))
                (setq next-ask nil)
                )
            (if (equal u-type "move-dir")
                (progn
                  (setq interact (c-xref-undo-dir-moving (cdr (cdr (car undolist))) interact))
                  (setq next-ask nil)
                  )
              (setq interact (c-xref-undo-until-undo-state (cdr (cdr (car undolist))) cont-undo-state interact))
              (setq cont-undo-state (cdr (cdr (car undolist))))
              ))
      (setq undolist (cdr undolist))
      (if (eq undolist nil)
              (setq cloop nil)
            (if (not next-ask)
                (setq cloop t)
              (sit-for 0.01)   ;; refresh screen
              (setq cloop (c-xref-yes-or-no-window
                               (format
                                    "The refactoring is undone, continue by undoing\n%s? "
                                    (car (car undolist)))
                               t nil
                               ))))
      )
    (switch-to-buffer cbuff)
    (sit-for 0)
    (c-xref-switch-to-marker c-xref-undo-marker)
    (c-xref-save-modified-files-with-question t "Undone. ")
    ))


(defun c-xref-undo-file-moving (data interact)
  (let ((buf) (fname) (confirmed))
    (setq fname (car data))
    (setq buf (car (cdr data)))
    (set-buffer buf)
    (if interact
            (progn
              (switch-to-buffer buf)
              (setq confirmed (c-xref-yes-or-no-window (format "Move to file %s ? " fname) t nil))
              )
      (setq confirmed t)
      )
    (if confirmed
            (progn
              (c-xref-undoable-move-file fname)
              ))
    )
  interact
  )

(defun c-xref-multifile-undo-add-buffer-write ()
  (let ((comment))
    (if c-xref-multifile-undo-state
            (setq comment (car (car c-xref-multifile-undo-state)))
      (setq comment (format "moving file %s" (c-xref-file-last-name (buffer-file-name nil))))
      )
    (setq c-xref-multifile-undo-state
              (cons
               (list
                comment
                "move-buffer"
                (buffer-file-name nil)
                (current-buffer)
                )
               c-xref-multifile-undo-state
               ))
    (cut-long-refactoring-undo-list)
    ;;(insert (format "multifile-undo-state : %S\n\n" c-xref-multifile-undo-state))
    ))


(defun c-xref-undo-dir-moving (data interact)
  (let ((olddir) (newdir) (confirmed))
    (setq olddir (car data))
    (setq newdir (car (cdr data)))
    (if interact
            (progn
              (setq confirmed (yes-or-no-p (format "Move directory %s to %s ? "
                                                                   (c-xref-file-last-name newdir)
                                                                   (c-xref-file-last-name olddir))
                                                       ))
              )
      (setq confirmed t)
      )
    (if confirmed
            (progn
              (c-xref-undoable-move-directory newdir olddir)
              ))
    )
  interact
  )

(defun c-xref-multifile-undo-add-move-directory (olddir newdir)
  (let ((comment))
    ;;  (if c-xref-multifile-undo-state
    ;;          (setq comment (car (car c-xref-multifile-undo-state)))
    (setq comment (format "moving directory %s to %s"
                                      (c-xref-file-last-name olddir)
                                      (c-xref-file-last-name newdir)))
    ;;    )
    (setq c-xref-multifile-undo-state
              (cons
               (list
                comment
                "move-dir"
                olddir
                newdir
                )
               c-xref-multifile-undo-state
               ))
    (cut-long-refactoring-undo-list)
    ;;(insert (format "multifile-undo-state : %S\n\n" c-xref-multifile-undo-state))
    ))


(defun c-xref-multifile-undo-set-buffer-switch-point (comment)
  "Set a multifile undo `change-point'.

This  function memorizes  the undo  state of  all opened  buffers.  An
invocation  of  `c-xref-undo-last-refactoring'  will then  undo  changes
since  that point  in  all those  buffers.   The COMMENT  is a  string
indicating before which action the state is memorized.
"
  ;;  (interactive "")
  (setq c-xref-multifile-undo-state
            (cons
             (cons
              comment
              (cons
               "standard"
               (c-xref-editor-undo-state)
               ))
             c-xref-multifile-undo-state
             ))
  (cut-long-refactoring-undo-list)
  ;;(insert (format "multifile-undo-state : %S\n\n" c-xref-multifile-undo-state))
  )


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;; REFACTORINGS ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-refactor ()
  "Invoke refactorer.

This function offers a list of refactorings available for the
symbol under point and for the region between mark and point.
You can then select the refactoring to be performed.

C-xrefactory performs refactorings by precomputing all changes on
an internal copy of source files in the memory.  All checks are
computed while working with the model and the refactoring is
applied on real sources only if it can be safely performed. In
consequence, if the refactoring fails due to the accessibility of
symbols or something similar, your source code should not be
modified at all.

Refactoring does not affect other files than source code. Hence,
it's a good idea to clean and rebuild after each important
refactoring.
"
  (interactive "")
  (c-xref-entry-point-make-initialisations)
  (setq c-xref-global-dispatch-data (c-xref-get-basic-server-dispatch-data 'c-xref-server-process))
  (c-xref-server-call-on-current-buffer-no-saves
   "-olcxgetrefactorings"
   c-xref-global-dispatch-data)
  )

;;;;;;;;;;;;;;;;;;;;;;;; REFACTORINGS INIT-FINISH ;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-refactoring-init-actions (description)
  (set-marker c-xref-refactoring-beginning-marker (point))
  (setq c-xref-refactoring-beginning-offset (point))
  (c-xref-multifile-undo-set-buffer-switch-point description)
  (if c-xref-save-files-and-update-tags-before-refactoring
      (progn
            (c-xref-save-some-buffers nil)
            (c-xref-update-tags "-update" nil)
            ))
  )

(defun c-xref-refactoring-finish-actions ()
  (if c-xref-save-files-and-update-tags-after-refactoring
      (progn
            (c-xref-save-some-buffers t)
            (c-xref-update-tags "-update" nil)
            ))
  (if c-xref-move-point-back-after-refactoring
      (progn
            (c-xref-switch-to-marker c-xref-refactoring-beginning-marker)
            (if (and (eq (point) (point-min))
                         (not (eq c-xref-refactoring-beginning-offset (point-min))))
                (goto-char c-xref-refactoring-beginning-offset)
              )))
  (message "Done.")
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; RENAMING ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defun c-xref-non-interactive-renaming (opt old-name new-name)
  (c-xref-refactoring-init-actions (format "renaming of %s to %s" old-name new-name))
  (c-xref-server-call-refactoring-task (list opt (format "-renameto=%s" new-name)))
  (c-xref-refactoring-finish-actions)
  )

(defun c-xref-renaming (opt default)
  (let ((tcount) (prompt) (new-name) (old-name))
    (if default
            (setq old-name default)
      (setq old-name (c-xref-get-identifier-on-point))
      )
    (setq new-name (read-from-minibuffer
                            (format "Rename '%s' to : " old-name) old-name))
    (c-xref-non-interactive-renaming opt old-name new-name)
    ))

(defun c-xref-rename-symbol (rd)
  (c-xref-renaming "-rfct-rename" nil)
  )

(defun c-xref-rename-module (rd)
  (c-xref-renaming "-rfct-rename-module" nil)
  )

(defun c-xref-rename-included-file (rd)
  "Find the filename in a C #include statement and pass it to c-xref-renaming."
  (save-excursion
    (let (filename)
      ;; Ensure we're on an #include line
      (beginning-of-line)
      (if (looking-at "#\\s-*include\\s-*\\(.*\\)")
        (progn
          (setq filename (match-string 1))
          (setq filename (substring filename 1 -1))
          (c-xref-renaming "-rfct-rename-included-file" filename))
        (message "No valid #include statement found at point")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; PARAMETER MANIPULATIONS  ;;;;;;;;;;;;;;;;;;;;

(defun c-xref-non-interactive-add-parameter (name arg textdef textval)
  (c-xref-refactoring-init-actions
   (format "insertion of %s's parameter" name))
  (c-xref-server-call-refactoring-task
   (list "-rfct-add-param"
             (format "-olcxparnum=%s" arg)
             (format "-rfct-parameter-name=%s" textdef);
             (format "-rfct-parameter-value=%s" textval)))
  (c-xref-refactoring-finish-actions)
  )

(defun c-xref-add-parameter (rd)
  (let ((name) (argns) (arg) (textdef) (textval) (default))
    (setq name (c-xref-get-identifier-on-point))
    (setq argns (read-from-minibuffer
                         (format
                          "Insert parameter at position [ 1 - arity('%s') ] : " name)
                         "1"
                         ))
    (setq arg (string-to-number argns))
    (if (and rd (equal (car (cdr rd)) "macro"))
            (setq default "ARG")
      (setq default "int arg")
      )
    (setq textdef (read-from-minibuffer
                           "Declaration of the new parameter: "
                           default
                           ))
    (setq textval (read-from-minibuffer
                           "Actual value of the new parameter: "
                           "0"
                       ))
    (c-xref-non-interactive-add-parameter name arg textdef textval)
    ))

(defun c-xref-non-interactive-del-parameter (name arg)
  (c-xref-refactoring-init-actions
   (format "deletion of %s's parameter" name))
  (c-xref-server-call-refactoring-task
   (list "-rfct-del-param" (format "-olcxparnum=%s" arg)))
  (c-xref-refactoring-finish-actions)
  )

(defun c-xref-del-parameter (rd)
  (let ((name) (argns) (arg) (textdef) (textval))
    (setq name (c-xref-get-identifier-on-point))
    (setq argns (read-from-minibuffer
                         (format
                          "Delete parameter from position [ 1 - arity('%s') ] : " name)
                         "1"
                         ))
    (setq arg (string-to-number argns))
    (c-xref-non-interactive-del-parameter name arg)
    ))


(defun c-xref-non-interactive-move-parameter (name arg1 arg2)
  (c-xref-refactoring-init-actions
   (format "move of %s's parameter" name))
  (c-xref-server-call-refactoring-task
   (list "-rfct-move-param"
             (format "-olcxparnum=%s" arg1)
             (format "-olcxparnum2=%s" arg2)))
  (c-xref-refactoring-finish-actions)
  )

(defun c-xref-move-parameter (rd)
  (let ((name) (argns) (arg) (textdef) (textval) (arg1) (arg2))
    (setq name (c-xref-get-identifier-on-point))
    (setq argns (read-from-minibuffer
                         (format
                          "Position of parameter to move [ 1 - arity('%s') ] : " name)
                         "1"
                         ))
    (setq arg1 (string-to-number argns))
    (setq argns (read-from-minibuffer
                         (format
                          "Move to position [ 1 - arity('%s') ] : " name)
                         "2"
                         ))
    (setq arg2 (string-to-number argns))
    (c-xref-non-interactive-move-parameter name arg1 arg2)
    ))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; MOVING ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-set-moving-target-position (rd)
  (interactive "i")
  (set-marker c-xref-moving-refactoring-marker (point))
  (setq c-xref-moving-refactoring-line (count-lines (point-min) (point)))
  (if (bolp) (setq c-xref-moving-refactoring-line (1+ c-xref-moving-refactoring-line)))
  (if rd
      (progn
            (message "Next moving refactoring will move to %s:%d"
                         (c-xref-file-last-name (buffer-file-name))
                         c-xref-moving-refactoring-line)
            ))
  )

(defun c-xref-moving (moveopt)
  (let ((name) (tf))
    (if (eq c-xref-moving-refactoring-line 0)
            (error "No target position. Use 'Set target position' first.")
      )
    (setq name (c-xref-get-identifier-on-point))

    ;; check target place
    (save-excursion
      (c-xref-set-to-marker c-xref-moving-refactoring-marker)
      (if (bobp) (forward-line (- c-xref-moving-refactoring-line 1)))
      (if (not (bolp))
              (progn
                (message "moving target marker at the beginning of line")
                (beginning-of-line)
                ))
      (setq tf (buffer-file-name))
      )

    ;; move
    (c-xref-server-call-refactoring-task
     (append moveopt (list
                              (format "-commentmovinglevel=%d" c-xref-comments-moving-level)
                              (format "-movetargetfile=%s" tf)
                              (format "-rfct-target-line=%s" c-xref-moving-refactoring-line))))

    ;; all done
    (save-excursion
      (c-xref-set-to-marker c-xref-moving-refactoring-marker)
      (c-xref-set-moving-target-position nil)
      )
    ))

(defun c-xref-move-function (rd)
  (let ((name))
        (setq name (c-xref-get-identifier-on-point))
        (c-xref-refactoring-init-actions (format "moving %s" name))
        (c-xref-moving '("-rfct-move-function"))
        (c-xref-refactoring-finish-actions)
))

(defun c-xref-organize-includes (rd)
  (c-xref-refactoring-init-actions "organizing includes")
  (c-xref-server-call-refactoring-task (list "-rfct-organize-includes"))
  (c-xref-refactoring-finish-actions)
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; EXTRACT ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun c-xref-add-macro-line-continuations (reg-beg-pos reg-end-pos)
  (let ( (unmoved) (loopfl))
    (goto-char reg-end-pos)
    (setq loopfl t)
    (while loopfl
      (end-of-line 1)
      (insert "\\")
      (setq unmoved (forward-line -1))
      (setq loopfl (and (eq unmoved 0) (>= (point) reg-beg-pos)))
      )
    ))

(defun c-xref-extraction-dialog (minvocation mhead mtail mline dname)
  ;; dname - name of the extracted entity, need to contain one of the strings
  (let ((mbody) (name) (saved-case-fold-search-mode) (mbuff) (kind) (sw)
            (mm) (pp) (bb) (ee) (ilen) (sr))
    (setq sw (selected-window))
    (setq mbuff (current-buffer))
    (setq mm (min (mark) (point)))
    (setq pp (max (mark) (point)))
    (setq mbody (buffer-substring mm pp))
    (c-xref-delete-window-in-any-frame c-xref-extraction-buffer nil)

    (setq saved-case-fold-search-mode case-fold-search)
    (setq case-fold-search nil) ;; Ensure case-insensitive search

    (setq case-fold-search saved-case-fold-search-mode)
    (if (string-match "macro" dname)
        (setq kind "macro")
      (if (string-match "variable" dname)
          (setq kind "variable")
        (setq kind "function")
        ))

    (c-xref-display-and-set-new-dialog-window c-xref-extraction-buffer nil t)
    (set-buffer c-xref-extraction-buffer)
    (c-xref-erase-buffer)
    (setq truncate-lines nil)
    (insert "")
    (insert (format "---------------   C-xrefactory suggests following new %s:\n\n" kind))
    (insert mhead)
    (if (not (string= kind "variable"))
            ;; If extracting a variable we just want to show the declaration
            (insert "\t// Original code start\n"))
    (setq bb (point))
    (insert mbody)

    ;; Ensure next output start on a new line unless we're extracting a variable
    (if (and (not (bolp)) (not (string= kind "variable")))
            (progn
              (newline)
              (setq mbody (buffer-substring bb (point)))
              ))

    (if (equal kind "macro")
            ;; For macros we want to add the newline escapes
            (progn
              (c-xref-add-macro-line-continuations bb (- (point) 1))
              (goto-char (point-max))
              (setq mbody (buffer-substring bb (point)))
              ))
    (if (not (string= kind "variable"))
            (insert "\t// Original code end\n"))
    (insert mtail)

    (if (not (string= kind "variable"))
            (progn
              (insert "---------------   which will be invoked by the command:\n\n")
              (insert minvocation)))

    (goto-char (point-min))
    (display-buffer c-xref-extraction-buffer)
    (if c-xref-renaming-default-name
            (setq name c-xref-renaming-default-name)
      (setq name (read-from-minibuffer (format "Enter name for the new %s (empty string cancels the extraction): " kind)))
      )

    (c-xref-delete-window-in-any-frame c-xref-extraction-buffer nil)
    (select-window sw)
    (if (not (equal name ""))
            (progn
              (set-buffer mbuff)
              (c-xref-goto-line mline)
              (set-marker c-xref-extraction-marker (point))
              (goto-char mm)
              (set-marker c-xref-extraction-marker2 (point))
              (kill-region mm pp)
              (insert minvocation)
              (if (fboundp 'indent-region)
                  (indent-region (- (point) (length minvocation)) (point) nil)
                )
              (setq ilen (- (point) mm))
              (c-xref-set-to-marker c-xref-extraction-marker)
              (if (not (string= kind "variable"))
                  (newline 2))
              (setq bb (point))
              (insert mhead)
              (insert mbody)
              (insert mtail)
              (if (string= kind "variable")
                  (newline))
              (setq ee (point))
              (if (fboundp 'indent-region)
                  (indent-region bb (point) nil)
                )
              (c-xref-set-to-marker c-xref-extraction-marker2)
              (setq case-fold-search nil)
              (setq sr (search-forward dname (+ (point) ilen) t))
              (setq case-fold-search saved-case-fold-search-mode)
              (if (not sr)
                  (error "[c-xref] internal error, can't find identifier")
                )
              (backward-char 2)
              (c-xref-server-call-refactoring-task
               (list "-rfct-rename" (format "-renameto=%s" name)))
              ))
    ))

(defun c-xref-extract-function (rd)
  (c-xref-refactoring-init-actions (format "extract function"))
  (c-xref-server-call-refactoring-task (list "-rfct-extract-function"))
  (c-xref-refactoring-finish-actions)
  )

(defun c-xref-extract-macro (rd)
  (c-xref-refactoring-init-actions (format "extract macro"))
  (c-xref-server-call-refactoring-task (list "-rfct-extract-macro"))
  (c-xref-refactoring-finish-actions)
  )

(defun c-xref-extract-variable (rd)
  (c-xref-refactoring-init-actions (format "extract variable"))
  (c-xref-server-call-refactoring-task (list "-rfct-extract-variable"))
  (c-xref-refactoring-finish-actions)
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

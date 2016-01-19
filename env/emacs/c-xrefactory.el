;;; c-xrefactory.el - (X)Emacs interface to C-xrefactory

;; Copyright (C) 1997-2004 Marian Vittek, Xref-Tech
 
;; This  file  is  part  of  C-xrefactory  software;  it  implements  an
;; interface  between  xref  task   and  (X)Emacs  editors.   You  can
;; distribute it  under the  terms of the  GNU General  Public License
;; version 2 as published by the Free Software Foundation.  You should
;; have received a  copy of the GNU General  Public License along with
;; this program; if not, write  to the Free Software Foundation, Inc.,
;; 59  Temple Place  - Suite  330,  Boston, MA  02111-1307, USA.   The
;; content of  this file is  copyrighted by Xref-Tech. This  file does
;; not contain any code  written by independent developers.  Xref-Tech
;; reserves  all rights  to make  any  future changes  in this  file's
;; license conditions.

(provide 'c-xrefactory)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; You can edit the following in order to change the default key-bindings


(defun c-xref-add-bindings-to-keymap (keymap)

  (define-key keymap [(f11)] 'c-xref-refactor)

  (define-key keymap [(f8)] 'c-xref-completion)
  (define-key keymap [(control f8)] 'c-xref-ide-compile-run)
  (define-key keymap [(f7)] 'c-xref-delete-window)

  (define-key keymap [(f6)] 'c-xref-push-and-goto-definition)
  (define-key keymap [(control f6)] 'c-xref-browse-symbol)
  (define-key keymap [(f5)] 'c-xref-pop-and-return)
  (define-key keymap [(control f5)] 'c-xref-re-push)
  (define-key keymap [(f4)] 'c-xref-next-reference)
  (define-key keymap [(control f4)] 'c-xref-alternative-next-reference)
  (define-key keymap [(f3)] 'c-xref-previous-reference)
  (define-key keymap [(control f3)] 'c-xref-alternative-previous-reference)

)

;; this function applies only if 'c-xref-key-binding is set to 'local;
;; in this case you can bind more functions

(defun c-xref-add-key-bindings-to-local-keymap (keymap)
  ;; mapping available only for local keymaps
  (define-key keymap [(meta ?.)] 'c-xref-push-name)
  (define-key keymap [(meta ?\t)] 'c-xref-completion)

  ;; plus standard mappings
  (c-xref-add-bindings-to-keymap keymap)
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; An  invocation  of  the  following  function  adds  C-xrefactory  key
;; bindings to  current local keymap.  You can use this  function when
;; defining your own programming modes (for example yacc-mode).

(defun c-xref-add-key-bindings-to-current-local-keymap ()
  (if (not (current-local-map))
	  (use-local-map (make-sparse-keymap))
	)
  (c-xref-add-key-bindings-to-local-keymap (current-local-map))
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Suffixes  determining  buffers where  information  for undoing  are
;; recorded.  C-xrefactory  undo will operate  only on files  with those
;; suffixes

(defvar c-xref-undo-allowed-suffixes '(".java" ".c" ".h" ".tc" ".th" 
									 ".tcc" ".thh" ".y"))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; current platform identification (will be used in default settings)

(if (or (string-match "-nt" system-configuration) 
		(string-match "-win" system-configuration))
	(defvar c-xref-platform 'windows)
  (defvar c-xref-platform 'unix)
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Completion coloring, you can choose your colors here!

(if (fboundp 'make-face)
	(progn
	  (if (not (boundp 'c-xref-list-default-face)) 
		  (make-face 'c-xref-list-default-face)
		)
	  (if (not (boundp 'c-xref-list-pilot-face)) 
		  (progn
			(make-face 'c-xref-list-pilot-face)
			(set-face-foreground 'c-xref-list-pilot-face "navy")
			))
	  (if (not (boundp 'c-xref-list-symbol-face)) 
		  (progn
			(make-face 'c-xref-list-symbol-face)
			(set-face-foreground 'c-xref-list-symbol-face "Sienna")
			))
	  (if (not (boundp 'c-xref-keyword-face)) 
		  (progn
			(make-face 'c-xref-keyword-face)
			(set-face-foreground 'c-xref-keyword-face "blue")
			))
	  (if (not (boundp 'c-xref-error-face)) 
		  (progn
			(make-face 'c-xref-error-face)
			(set-face-foreground 'c-xref-error-face "red")
			))
	  )
 ;; if do not know how to make faces, set the coloring off
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar c-xref-preferable-undo-limit 2500000)
(defvar c-xref-preferable-undo-strong-limit 3000000)

(if (and (boundp 'undo-limit) (> c-xref-preferable-undo-limit undo-limit))
	(setq undo-limit c-xref-preferable-undo-limit)
  )
(if (and (boundp 'undo-strong-limit) 
		 (> c-xref-preferable-undo-strong-limit undo-strong-limit))
	(setq undo-strong-limit c-xref-preferable-undo-strong-limit)
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                      NOTHING MORE TO SETUP                          ;;
;;                    CUSTOMIZATION PART IS OVER                       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Here  follows  some e-lisp  code  which  can't  be autoloaded  from
;; c-xref.el file


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; following code adds standard c-xrefactory keybinding for current session

(if (not (boundp 'c-xref-key-binding))
	(defvar c-xref-key-binding 'global)
  )

(if (eq c-xref-key-binding 'global)
	(progn
	  (c-xref-add-bindings-to-keymap global-map)
	  ;; following is always set in the GLOBAL keymap !!
	  ;; if you are using some binding for S-mouse-2, put it in comment
	  (global-set-key [(shift mouse-2)] 'c-xref-find-file-on-mouse)
	  )

  (if (eq c-xref-key-binding 'local)
	  (progn
		(if (load "cc-mode" t)
			(progn
			  (if (boundp 'c-mode-map)
				  (c-xref-add-key-bindings-to-local-keymap c-mode-map)
				(message "[C-xrefactory] c-mode-map not found, skipping keymap setting")
				)
			  (if (boundp 'java-mode-map)
				  (c-xref-add-key-bindings-to-local-keymap java-mode-map)
				(message "[C-xrefactory] java-mode-map not found, skipping keymap setting")
				)
			  )
		  (message "[C-xrefactory] cc-mode file not found, cannot setup local key binding, making it global.")
		  (c-xref-add-bindings-to-keymap global-map)
		  )

		;; following is always set in the GLOBAL keymap !!
		;; if you are using some binding for S-mouse-2, put it in comment
		(global-set-key [(shift mouse-2)] 'c-xref-find-file-on-mouse)
		))
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; C-xrefactory configuration  variables setting. Majority  of variables
;; can be set interactively using  'Options' item of 'C-Xref' main menu.
;; the first one is the exception :)

;; this variable specifies how to escape from C-xrefactory windows
;; usually on simple Escape, but on alphanumerical consoles, where
;; special keys are coded via Escape prefix, close window on escape-escape
;; sequence
(if (not (boundp 'c-xref-escape-key-sequence)) 
	(progn
	  (defvar c-xref-escape-key-sequence "\e")
	  (if (not window-system)
		  (setq c-xref-escape-key-sequence "\e\e")
		)))

(if (not (boundp 'c-xref-version-control)) 
	(defvar c-xref-version-control nil)
  )

(if (not (boundp 'c-xref-version-control-checkin-on-auto-saved-buffers)) 
	(defvar c-xref-version-control-checkin-on-auto-saved-buffers nil)
  )

(if (not (boundp 'c-xref-inspect-errors-if-compilation-window)) 
	(defvar c-xref-inspect-errors-if-compilation-window t)
  )

;; by default xrefactory does binds left button to its functions
(if (not (boundp 'c-xref-bind-left-mouse-button)) 
	(defvar c-xref-bind-left-mouse-button t)
  )

;; when displaying browser, default filter level will be 2
(if (not (boundp 'c-xref-default-symbols-filter))
	(defvar c-xref-default-symbols-filter 2)
  )

(if (not (boundp 'c-xref-default-references-filter))
	(defvar c-xref-default-references-filter 0)
  )

;; when updating browser, keep lastly set filter level
(if (not (boundp 'c-xref-keep-last-symbols-filter))
	(defvar c-xref-keep-last-symbols-filter t)
  )

(if (not (boundp 'c-xref-commentary-moving-level)) 
	(defvar c-xref-commentary-moving-level 0)
  )

(if (not (boundp 'c-xref-prefer-import-on-demand)) 
	(defvar c-xref-prefer-import-on-demand t)
  )

(if (not (boundp 'c-xref-save-files-and-update-tags-after-refactoring)) 
	(defvar c-xref-save-files-and-update-tags-after-refactoring nil)
  )

(if (not (boundp 'c-xref-save-files-and-update-tags-before-refactoring)) 
	(defvar c-xref-save-files-and-update-tags-before-refactoring nil)
  )

;; by default c-xref asks before starting to browse a javadoc URL
(if (not (boundp 'c-xref-ask-before-browse-javadoc)) 
	(defvar c-xref-ask-before-browse-javadoc t)
  )

(if (not (boundp 'c-xref-browse-url-directly))
	(if (eq c-xref-platform 'windows)
		(defvar c-xref-browse-url-directly nil)
	  (defvar c-xref-browse-url-directly t)
	  )
  )

;; autoredirect is turn off on windows because of some problems
(if (not (boundp 'c-xref-browse-url-manual-redirect))
	(if (eq c-xref-platform 'windows)
		(defvar c-xref-browse-url-manual-redirect t)
	  (defvar c-xref-browse-url-manual-redirect nil)
	  )
  )

;; by  default multibyte  buffer representation  is allowed in order
;; NOT USED ANYMORE, it is here for backward compatibility only,
;; use 'c-xref-files-encoding now.
(if (not (boundp 'c-xref-allow-multibyte)) 
	(defvar c-xref-allow-multibyte t)
  )

(if (not (boundp 'c-xref-files-encoding)) 
	(defvar c-xref-files-encoding 'generic)
  )

;; by default truncation is disallowed in order to see profiles
(if (not (boundp 'c-xref-completion-truncate-lines)) 
	(defvar c-xref-completion-truncate-lines nil)
  )

(if (not (boundp 'c-xref-completion-inserts-parenthesis)) 
	(defvar c-xref-completion-inserts-parenthesis nil)
  )

;; by default truncation is disallowed in order to see profiles
(if (not (boundp 'c-xref-completion-overload-wizard-deep)) 
	(defvar c-xref-completion-overload-wizard-deep 1)
  )

;; by default the automatic project selection is on.
(if (not (boundp 'c-xref-current-project)) 
	(defvar c-xref-current-project nil)
  )


;; by default  the accessibility and linkage checks  are not performed
;; on proposed completions
(if (not (boundp 'c-xref-java-completion-access-check)) 
	(defvar c-xref-java-completion-access-check nil)
  )
(if (not (boundp 'c-xref-java-completion-linkage-check)) 
	(defvar c-xref-java-completion-linkage-check nil)
  )

;; by default  fully qualified type names are proposed when completing
; Java type name
(if (not (boundp 'c-xref-java-fqt-name-completion-level)) 
	(defvar c-xref-java-fqt-name-completion-level 2)
  )

(if (not (boundp 'c-xref-coloring)) 
	(defvar c-xref-coloring (fboundp 'make-face))
  )

(if (not (boundp 'c-xref-highlight-java-keywords)) 
	(defvar c-xref-highlight-java-keywords t)
  )

(if (not (boundp 'c-xref-mouse-highlight)) 
	(defvar c-xref-mouse-highlight t)
  )

(if (not (boundp 'c-xref-multifile-undo-deep)) 
	(defvar c-xref-multifile-undo-deep 50)
  )

(if (not (boundp 'c-xref-ide-last-run-command)) 
	(defvar c-xref-ide-last-run-command "run")
)

;; the default variable executed by first c-xref-ide-compile command
(if (not (boundp 'c-xref-ide-last-compile-command)) 
	(defvar c-xref-ide-last-compile-command 'compiledir)
)
;; can be also 'compilefile or 'compileproject

;; the default limit for number of completions
(if (not (boundp 'c-xref-max-completions)) 
	(defvar c-xref-max-completions 500)
)

;; shell to interpret multi-line compile and run commands
(if (not (boundp 'c-xref-shell)) 
	(defvar c-xref-shell "sh")
)

;; generate batch file also when compiling with single line command
(if (not (boundp 'c-xref-always-batch-file))
	(defvar c-xref-always-batch-file t)
)

(if (not (boundp 'c-xref-move-point-back-after-refactoring))
	(defvar c-xref-move-point-back-after-refactoring nil)
)

(if (not (boundp 'c-xref-detailed-refactoring-confirmations))
	(defvar c-xref-detailed-refactoring-confirmations nil)
)

(if (not (boundp 'c-xref-auto-update-tags-before-push))
	(defvar c-xref-auto-update-tags-before-push nil)
)

(if (not (boundp 'c-xref-browser-lists-source-lines))
	(defvar c-xref-browser-lists-source-lines t)
)

(if (not (boundp 'c-xref-close-windows-on-pop))
	(defvar c-xref-close-windows-on-pop nil)
)

(if (not (boundp 'c-xref-completion-case-sensitive))
	(defvar c-xref-completion-case-sensitive nil)
)

(if (not (boundp 'c-xref-completion-quit-on-q))
	(defvar c-xref-completion-quit-on-q t)
)

(if (not (boundp 'c-xref-completion-delete-pending-identifier))
	(defvar c-xref-completion-delete-pending-identifier t)
)

(if (not (boundp 'c-xref-completion-auto-focus))
	(defvar c-xref-completion-auto-focus t)
)

(if (not (boundp 'c-xref-browse-url-focus-delay))
	(defvar c-xref-browse-url-focus-delay 0)
)

(if (not (boundp 'c-xref-window-minimal-size))
	(defvar c-xref-window-minimal-size 4)
)

(if (not (boundp 'c-xref-browser-splits-window-horizontally))
	(defvar c-xref-browser-splits-window-horizontally nil)
)

(if (not (boundp 'c-xref-class-tree-splits-window-horizontally))
	(defvar c-xref-class-tree-splits-window-horizontally t)
)

(if (not (boundp 'c-xref-browser-position-left-or-top))
	(defvar c-xref-browser-position-left-or-top nil)
)

(if (not (boundp 'c-xref-class-tree-position-left-or-top))
	(defvar c-xref-class-tree-position-left-or-top t)
)

(if (not (boundp 'c-xref-class-tree-window-width))
	(defvar c-xref-class-tree-window-width 30)
)

(if (not (boundp 'c-xref-symbol-selection-window-width))
	(defvar c-xref-symbol-selection-window-width 30)
)

(if (not (boundp 'c-xref-symbol-selection-window-height))
	(defvar c-xref-symbol-selection-window-height 10)
)

(if (not (boundp 'c-xref-browser-windows-auto-resizing))
	(defvar c-xref-browser-windows-auto-resizing t)
)

(if (not (boundp 'c-xref-display-active-project-in-minibuffer))
	(defvar c-xref-display-active-project-in-minibuffer t)
)

(if (not (boundp 'c-xref-run-find-file-hooks))
	(defvar c-xref-run-find-file-hooks t)
)

(if (not (boundp 'c-xref-options-file))
	(if (eq c-xref-platform 'windows)
		(if (getenv "HOME")
			(defvar c-xref-options-file (concat (getenv "HOME") "/_c-xrefrc"))
		  (defvar c-xref-options-file "c:/_c-xrefrc")
		  )
	  (defvar c-xref-options-file (concat (getenv "HOME") "/.c-xrefrc"))
	  )
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; setting CXrefactory customization group

(if (commandp 'customize)
	(progn
	  (defgroup c-xrefactory nil
"

Here you  can set variables controlling those  behaviors of C-xrefactory
which are not project-dependent, and its general appearance.

")


;;;;;;;;;;;;;;;; c-xrefactory-general ;;;;;;;;;

	  (defgroup c-xrefactory-general nil
		"

Here  you  can  set  variables  controlling the  general  behavior  of
C-xrefactory functions.

"
		:group 'c-xrefactory
)

	  (defcustom c-xref-options-file c-xref-options-file
		"This option determines where the file describing C-xrefactory project-specific options (projects and their setting) is stored. You will need to kill (and restart) the c-xref process if changing this option."
		:type '(string)
		;; :type '(file) ;; this puts there ~, etc making problems
		:group 'c-xrefactory-general)

	  (defcustom c-xref-version-control nil
		"This variable stores identification of version control system used by C-xrefactory: nil means no VC system; t means the Emacs VC system implemented by the `vc' package; any other value is interpreted as an identification of a user-defined version system."
		:type '(symbol)
		:group 'c-xrefactory-general)

	  (defcustom c-xref-version-control-checkin-on-auto-saved-buffers nil
		"If on and if version control is on, then automatic save after refactoring also checks in modified files. Be careful with this option as this clears undo buffers. Too frequent checkins will make undoing impossible."
		:type '(boolean)
		:group 'c-xrefactory-general)

	  (defcustom c-xref-bind-left-mouse-button t
		"If on, C-xrefactory will bind the left mouse button in its dialog windows. The button will be bound to the same function as the middle button. If you change this value, you will need to restart Emacs in order for that new value take effect."
		:type '(boolean)
		:group 'c-xrefactory-general)

	  (defcustom c-xref-files-encoding 'generic
		"This variable specifies C-xrefactory multi language file encoding.  Available values are 'generic', 'ascii', 'european', 'euc', 'sjis', 'utf' and 'project'. If you use only 7-bit ascii charset, set this option to 'ascii. If you use 8-bit europeen encoding, set this value to 'european. If you use a kind of EUC encoding (multiple 8-bits Japanese, Korean, ...), set it to 'euc. If you use Japanese SJIS encoding, set it to 'sjis. If you use one of unicode encodings (utf-8 or utf-16) set it to 'utf'. Otherwise, use 'generic settings, which should work fine at least for completions and browsing. The value 'project allows to specify encoding for each project separately. It is highly recommended to set this option to 'project and to specify encodings for each of your projects within project options."
		:type '(symbol)
		:group 'c-xrefactory-general)

	  (defcustom c-xref-window-minimal-size 5
		"Minimal size of window displaying completions and references. If a positive value is given the window's size will be proportional to number of completions, but at least c-xref-window-minimal-size lines."
		:type '(integer)
		:group 'c-xrefactory-general)

	  (defcustom c-xref-display-active-project-in-minibuffer t
		"If on, C-xrefactory will display the active project in minibuffer after each invocation of its functions (except completions)."
		:type '(boolean)
		:group 'c-xrefactory-general)



;;;;;;;;;;;;;;;; IDE ;;;;;;;;;

	  (defgroup c-xrefactory-compile-run nil
		"

Here you can set variables controlling the Emacs IDE interface.

"
		:group 'c-xrefactory
)

	  (defcustom c-xref-ide-last-compile-command 'compiledir
		"Can be either 'compilefile, 'compiledir or 'compileproject. This variable indicates which compilations will be invoked by 'Emacs IDE -> Last Compile' command. You can preset it to your preferred default value."
		:type '(symbol)
		:group 'c-xrefactory-compile-run)

	  (defcustom c-xref-always-batch-file t
		"If on, C-xrefactory will generate and then execute a batch file when executing an Emacs IDE compile function. If off, C-xrefactory will generate a batch file only when the compile command exceeds one line."
		:type '(boolean)
		:group 'c-xrefactory-compile-run)

	  (defcustom c-xref-shell "sh"
		"This option determines which shell is used for interpreting batch files generated for Emacs IDE compile and run commands. This variable applies only on Unix like platforms."
		:type '(string)
		:group 'c-xrefactory-compile-run)


;;;;;;;;;;;;;;;; c-xrefactory-completion ;;;;;;;;;

	  (defgroup c-xrefactory-completion nil
		"

Here  you  can  set  variables controlling  behavior  of  C-xrefactory's
completions.

"
		:group 'c-xrefactory
)

	  (defcustom c-xref-completion-case-sensitive nil
		"If on, then completion is case-sensitive and does not suggest completions differing in cases from the prefix. Keeping this option off is a good idea for lazy users who don't type case correctly."
		:type '(boolean)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-completion-delete-pending-identifier t
		"If on, after inserting a completion C-xrefactory deletes the rest of the old identifier after the point."
		:type '(boolean)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-completion-inserts-parenthesis nil
		"If on, C-xrefactory will insert an open parenthesis after a method name."
		:type '(boolean)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-completion-quit-on-q t
		"If on, the 'q' key in the completions window will close the window (if there is no interactive search in progress). Users may find this more useful than searching for completions starting with 'q'."
		:type '(boolean)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-completion-truncate-lines nil
		"If on, C-xrefactory will truncate lines in buffers containing completions. Also default formatting of completions will be turned off, so exactly one completion will appear per line. Truncated parts of lines can be viewed by scrolling left and right (Shift-Left and Shift-Right)."
		:type '(boolean)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-java-completion-access-check nil
		"If on, then when completing a Java attribute, C-xrefactory will suggest only accessible symbols checking protected/public/private attributes."
		:type '(boolean)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-java-completion-linkage-check nil
		"If on, then when completing a Java attribute in a static context, C-xrefactory will suggest only static fields/methods."
		:type '(boolean)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-completion-auto-focus t
		"If on, window with multiple completions get the focus automatically after being displayed. Otherwise the window with source code keeps the focus."
		:type '(boolean)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-java-fqt-name-completion-level 2
		"Level of completion for fully qualified type names. 
'0' means that no fully qualified names are suggested. '1' causes classes read from .jar archives to be suggested. For for 'LinkedLi', 'java.util.LinkedList' is suggested. '2' adds classes from the active project. '3' adds classes stored in classpath directories; finally '4' adds classes stored in sourcepath directories."
		:type '(integer)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-completion-overload-wizard-deep 1
		"Level of inheritance for completion of overloaded methods. When completion is invoked on an empty string at a position when a method definition can start, then all methods from the superclasses are suggested. This option specifies how deeply superclasses are scanned."
		:type '(integer)
		:group 'c-xrefactory-completion)

	  (defcustom c-xref-max-completions 500
		"Maximum number of suggested completions. If there are more available, an ellipsis (...) is shown."
		:type '(integer)
		:group 'c-xrefactory-completion)


;;;;;;;;;;;;;;;; class tree ;;;;;;;;;

	  (defgroup c-xrefactory-class-tree nil
		"

Here you can set variables controlling C-xrefactory's class-tree viewer.

"
		:group 'c-xrefactory
)

	  (defcustom c-xref-class-tree-splits-window-horizontally t
		"If on, C-xrefactory displays the class tree in a horizontally split window. Otherwise it splits the window vertically."
		:type '(boolean)
		:group 'c-xrefactory-class-tree)

	  (defcustom c-xref-class-tree-position-left-or-top nil
		"If on, the class tree is displayed at the left or top of the frame (depending on whether the window is split vertically or not). Otherwise, the class tree is displayed at the right or bottom."
		:type '(boolean)
		:group 'c-xrefactory-class-tree)

	  (defcustom c-xref-class-tree-window-width 30
		"Default width of class tree window."
		:type '(integer)
		:group 'c-xrefactory-class-tree)


;;;;;;;;;;;;;;;; xrefactory-browsing-configuration ;;;;;;;;;

	  (defgroup c-xrefactory-source-browser nil
		"

Here you can set variables controlling the behavior of
C-xrefactory's source browsing functions.

"
		:group 'c-xrefactory
)

	  (defcustom c-xref-browser-lists-source-lines t
		"If on, the C-xrefactory browser will display one line of source code for each reference in the browser window. This may slow down the display when there are large numbers of references."
		:type '(boolean)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-auto-update-tags-before-push nil
		"If on, C-xrefactory will update the tags file before pushing references on to browser stack. If you are working on a small project and you have a fast computer, then it may be convenient to turn this option on."
		:type '(boolean)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-browser-splits-window-horizontally nil
		"If on, C-xrefactory displays class tree in a horizontally split window. Otherwise it splits the window vertically."
		:type '(boolean)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-browser-position-left-or-top nil
		"If on, the browser is displayed at the left or top of the frame (depending on whether the window was split vertically or not). Otherwise, the browser is displayed at the right or bottom."
		:type '(boolean)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-default-symbols-filter 2
		"Controls which symbols/classes are shown when displaying a new browser window. '2': only symbols with the same name and profile as the browsed symbol, in a class related (via inheritance) to that of the browsed symbol; '1': all symbols with the same profile; '0': all symbols of the same name."
		:type '(integer)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-default-references-filter 0
		"Controls which references are shown. References to a variable work differently from references to other symbols. For variables: '3': only definition and declarations; '2': also l-value usages; Level 1: also usages where the address of the variable is taken; Level 0: all references. For other symbols: Level 3: only definitions and declarations; Level 2: also usages in EXTENDS and IMPLEMENTS clauses (meaningful only for Java); Level 1: also usages in the top level scope (global variable and function definitions; this can be used to see all the functions of a particular type); Level 0: all references."
		:type '(integer)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-keep-last-symbols-filter t
		"If on, then C-xrefactory remembers the last filter level when pushing and popping in symbols/classes window. Otherwise the default filter level is used."
		:type '(boolean)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-browser-windows-auto-resizing t
		"If on, C-xrefactory will resize newly created browser windows appropriately."
		:type '(boolean)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-symbol-selection-window-height 10
		"Browser window's default height. This value applies only to the first appearance of the browser window and only if c-xref-browser-windows-auto-resizing is off."
		:type '(integer)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-symbol-selection-window-width 30
		"Browser window's default width. This value applies only to the first appearance of the browser window and only if c-xref-browser-windows-auto-resizing is off."
		:type '(integer)
		:group 'c-xrefactory-source-browser)

	  (defcustom c-xref-inspect-errors-if-compilation-window t
		"If on, and if a compilation buffer is displayed in the current frame, then c-xref-alternative-previous-reference and c-xref-alternative-next-reference will inspect the previous/next error rather than the previous/next reference. In this way shortcuts (usually C-F3, C-F4) can be used to browse errors after an unsuccessful compilation."
		:type '(boolean)
		:group 'c-xrefactory-source-browser)




;;;;;;;;;;;;;;;; xrefactory-extern-html-browser ;;;;;;;;;

	  (defgroup c-xrefactory-extern-html-browser nil
		"

Here you can set variables  controlling the external HTML browser used
by C-xrefactory.

"
		:group 'c-xrefactory
)

	  (defcustom c-xref-ask-before-browse-javadoc t
		"If on, C-xrefactory asks for confirmation before launching the JavaDoc browser when trying to move to a definition of symbol without available source code."
		:type '(boolean)
		:group 'c-xrefactory-extern-html-browser)

	  (defcustom c-xref-browse-url-focus-delay 0
		"This variable specifies for how long the web browser window displaying JavaDoc will be raised before Emacs regains the focus. The value is a floating point number of seconds. If it is less than zero, then Emacs will not regain the focus."
		:type '(number)
		:group 'c-xrefactory-extern-html-browser)

	  (defcustom c-xref-browse-url-directly c-xref-browse-url-directly
		"If off, then instead of browsing a URL directly, C-xrefactory will browse a temporary file containing an auto-redirection to browsed URL. This option is used to cope with a problem with browse-url under Windows."
		:type '(boolean)
		:group 'c-xrefactory-extern-html-browser)

	  (defcustom c-xref-browse-url-manual-redirect c-xref-browse-url-manual-redirect
		"If both 'c-xref-browse-url-directly' and 'c-xref-browse-url-manual-redirect' are on, then C-xrefactory will generate and browse temporary files with links to JavaDoc when browsing the JavaDoc for a method having more than one parameter. The user has to click on the link manually in order to move to the JavaDoc documentation. This option is used to cope with a problem with auto-redirection in Internet Explorer."
		:type '(boolean)
		:group 'c-xrefactory-extern-html-browser)



;;;;;;;;;;;;;;;; c-xrefactory-refactoring ;;;;;;;;;

	  (defgroup c-xrefactory-refactoring nil
		"

Here you can set customizable variables controling behavior of
C-xrefactory refactoring functions.

"
		:group 'c-xrefactory
)


	  (defcustom c-xref-save-files-and-update-tags-before-refactoring nil
		"If on, C-xrefactory saves all modified files and updates tags before each refactoring. This can speed up refactorings."
		:type '(boolean)
		:group 'c-xrefactory-refactoring)

	  (defcustom c-xref-save-files-and-update-tags-after-refactoring nil
		"If on, C-xrefactory saves all modified files and updates tags after each refactoring."
		:type '(boolean)
		:group 'c-xrefactory-refactoring)

	  (defcustom c-xref-prefer-import-on-demand t
		"If on, C-xrefactory will prefer to generate import on demand statements (import com.pack.*;) instead of single type imports (import com.pack.Type;). This applies whenever C-xrefactory needs to add an import statement, which may be during moving refactorings or when reducing long type names."
		:type '(boolean)
		:group 'c-xrefactory-refactoring)

	  (defcustom c-xref-commentary-moving-level 0
		"The value of this variable determines how much commenting preceding a field, method or class definition is moved together with the field, method or class when a moving refactoring is applied. '0' moves no comments at all. '1' moves one doubleslashed comments (// ...). '2' moves one standard comment (/* ... */). '3' moves one doubleslashed and one standard comment. '4' moves all doubleslashed comments. '5' moves all standard comments. Finally, '6' moves all doubleslashed and all standard comments."
		:type '(integer)
		:group 'c-xrefactory-refactoring)

	  (defcustom c-xref-move-point-back-after-refactoring nil
		"If on, then after finishing a refactoring the point will be moved back to the position where the refactoring started. This is convenient for moving a suite of fields/methods into another file. In such cases it is best that the point is not moved to target position, because you wish to select the next field/method to be moved. Note that the target position is kept after a refactoring, so it can be reused without needing to set it again."
		:type '(boolean)
		:group 'c-xrefactory-refactoring)

	  (defcustom c-xref-run-find-file-hooks t
		"If on, C-xrefactory will use the standard function 'find-file' when loading new files. Otherwise it will load buffers with its own function avoiding running of find-file-hooks. This means that new buffers will not be highlighted, etc. This option can be used to speed up big refactorings on larges projects where running file-local hooks can take a long time. However, not running hooks may cause unwanted side effects."
		:type '(boolean)
		:group 'c-xrefactory-refactoring)

	  (defcustom c-xref-multifile-undo-deep 50
		"This variable determines how many refactorings are kept in the undo list, i.e. how many refactorings can be undone. C-xrefactory's undo mechanism is built on standard Emacs undo; to be able to undo large number of refactorings, you have to set 'undo-limit' and 'undo-strong-limit' to sufficiently large values."
		:type '(integer)
		:group 'c-xrefactory-refactoring)



;;;;;;;;;;;;;;;; highlighting colors faces ;;;;;;;;;

	  (defgroup c-xrefactory-highlighting-coloring nil
		"

Here you  can set variables controlling  everything concerning colors,
faces and highlighting in buffers created by C-xrefactory.

"
		:group 'c-xrefactory
)

	  (defcustom c-xref-coloring c-xref-coloring
		"If on, C-xrefactory will color symbols in completion and reference list buffers. This may be slow for large projects."
		:type '(boolean)
		:group 'c-xrefactory-highlighting-coloring)

	  (defcustom c-xref-highlight-java-keywords nil
		"If on, C-xrefactory will color Java keywords in completion and reference list buffers. Otherwise it will color C keywords."
		:type '(boolean)
		:group 'c-xrefactory-highlighting-coloring)

	  (defcustom c-xref-mouse-highlight t
		"If on, C-xrefactory will highlight symbols under the mouse in its buffers."
		:type '(boolean)
		:group 'c-xrefactory-highlighting-coloring)


))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar c-xref-home-directory "")

;; Now follows C-xref menu definitions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                      Emacs

(if (not (string-match "XEmacs" emacs-version))
	(progn
	  ;; IDE menu
	  (defvar c-xref-ide-menu (make-sparse-keymap "C-xref interface to Emacs IDE functions"))
	  (fset 'c-xref-ide-menu (symbol-value 'c-xref-ide-menu))
	  (define-key c-xref-ide-menu [c-xref-ide-compile-run] '("(Last) Compile & Run" . c-xref-ide-compile-run))
	  (define-key c-xref-ide-menu [c-xref-ide-sep1] '("--"))
	  (define-key c-xref-ide-menu [c-xref-ide-run] '("(Last) Run" . c-xref-ide-run))
	  (define-key c-xref-ide-menu [c-xref-ide-runthis] '("Run This" . c-xref-ide-run-this))
	  (define-key c-xref-ide-menu [c-xref-ide-run5] '("Run5" . c-xref-ide-run5))
	  (define-key c-xref-ide-menu [c-xref-ide-run4] '("Run4" . c-xref-ide-run4))
	  (define-key c-xref-ide-menu [c-xref-ide-run3] '("Run3" . c-xref-ide-run3))
	  (define-key c-xref-ide-menu [c-xref-ide-run2] '("Run2" . c-xref-ide-run2))
	  (define-key c-xref-ide-menu [c-xref-ide-run1] '("Run1" . c-xref-ide-run1))
	  (define-key c-xref-ide-menu [c-xref-ide-sep2] '("--"))
	  (define-key c-xref-ide-menu [c-xref-ide-next-err] '("Next Error (or Alternative Reference)" . c-xref-alternative-next-reference))
	  (define-key c-xref-ide-menu [c-xref-ide-previous-err] '("Previous Error (or Alternative Reference)" . c-xref-alternative-previous-reference))
	  ;;(define-key c-xref-ide-menu [c-xref-ide-preverror] '("Previous Error" . c-xref-ide-previous-error))
	  ;;(define-key c-xref-ide-menu [c-xref-ide-nexterror] '("Next Error" . c-xref-ide-next-error))
	  (define-key c-xref-ide-menu [c-xref-ide-sep3] '("--"))
	  (define-key c-xref-ide-menu [c-xref-ide-cmpl] '("(Last) Compile" . c-xref-ide-compile))
	  (define-key c-xref-ide-menu [c-xref-ide-cprj] '("Compile Project" . c-xref-ide-compile-project))
	  (define-key c-xref-ide-menu [c-xref-ide-cdir] '("Compile Directory" . c-xref-ide-compile-dir))
	  (define-key c-xref-ide-menu [c-xref-ide-cfile] '("Compile File" . c-xref-ide-compile-file))

	  ;; local browse menu
	  (defvar c-xref-lm-menu (make-sparse-keymap "Local Motion"))
	  (fset 'c-xref-lm-menu (symbol-value 'c-xref-lm-menu))
	  (define-key c-xref-lm-menu [c-xref-lm-next] '("Next Usage of This Symbol (or Alternatives)" . c-xref-alternative-next-reference))
	  (define-key c-xref-lm-menu [c-xref-lm-previous] '("Previous Usage of This Symbol (or Alternatives)" . c-xref-alternative-previous-reference))

	  (defvar c-xref-sb-menu (make-sparse-keymap "Browsing with Symbol Stack"))
	  (fset 'c-xref-sb-menu (symbol-value 'c-xref-sb-menu))
	  (define-key c-xref-sb-menu [c-xref-sb-repush] '("Re-push Popped Symbol" . c-xref-re-push))
	  (define-key c-xref-sb-menu [c-xref-sb-sep3] '("--"))
	  (define-key c-xref-sb-menu [c-xref-sb-pop-and-ret] '("Pop Symbol and Return" . c-xref-pop-and-return))
	  (define-key c-xref-sb-menu [c-xref-sb-pop] '("Pop Symbol" . c-xref-pop-only))
	  (define-key c-xref-sb-menu [c-xref-sb-sep2] '("--"))
	  (define-key c-xref-sb-menu [c-xref-sb-current-sym-and-refs] '("Display Browser" . c-xref-show-browser))
	  (define-key c-xref-sb-menu [c-xref-sb-next] '("Next Reference" . c-xref-next-reference))
	  (define-key c-xref-sb-menu [c-xref-sb-previous] '("Previous Reference" . c-xref-previous-reference))
	  (define-key c-xref-sb-menu [c-xref-sb-sep1] '("--"))
	  (define-key c-xref-sb-menu [c-xref-sb-push-and-macro] '("Push Symbol and Apply Macro on All References" . c-xref-push-and-apply-macro))
	  (define-key c-xref-sb-menu [c-xref-sb-push-name] '("Push Symbol by Name" . c-xref-push-name))
	  (define-key c-xref-sb-menu [c-xref-sb-push-and-list] '("Push This Symbol and Display Browser" . c-xref-browse-symbol))
	  (define-key c-xref-sb-menu [c-xref-sb-push-and-go] '("Push This Symbol and Goto Definition" . c-xref-push-and-goto-definition))
	  (define-key c-xref-sb-menu [c-xref-sb-push] '("Push This Symbol" . c-xref-push-references))


	  (defvar c-xref-project-menu (make-sparse-keymap "Project"))
	  (fset 'c-xref-project-menu (symbol-value 'c-xref-project-menu))
	  (define-key c-xref-project-menu [c-xref-prj-edit] '("Edit Options" . 
														  c-xref-project-edit-options))
	  (define-key c-xref-project-menu [c-xref-prj-show-active] '("Show Active" . 
																 c-xref-project-active))
	  (define-key c-xref-project-menu [c-xref-prj-set-active] '("Set Active" . 
																c-xref-project-set-active))
	  (define-key c-xref-project-menu [c-xref-prj-del] '("Delete" . 
														 c-xref-project-delete))
	  (define-key c-xref-project-menu [c-xref-prj-new] '("New" . 
														 c-xref-project-new))

	  (defvar c-xref-dead-menu (make-sparse-keymap "Dead code detection"))
	  (fset 'c-xref-dead-menu (symbol-value 'c-xref-dead-menu))
	  (define-key c-xref-dead-menu [c-xref-dm-globals] '("Browse Project Unused Symbols" . c-xref-push-global-unused-symbols))
	  (define-key c-xref-dead-menu [c-xref-dm-locals] '("Browse File Unused Symbols" . c-xref-push-this-file-unused-symbols))

	  (defvar c-xref-misc-menu (make-sparse-keymap "Misc Menu"))
	  (fset 'c-xref-misc-menu (symbol-value 'c-xref-misc-menu))
	  (define-key c-xref-misc-menu [c-xref-about] '("About C-xref" . c-xref-about))
	  (define-key c-xref-misc-menu [c-xref-help] '("C-xref Help" . c-xref-help))
	  (define-key c-xref-misc-menu [c-xref-kill] '("Kill C-xref Process" . c-xref-kill-xref-process))


	  (defvar c-xref-menu (make-sparse-keymap "C-xref"))
	  (if (commandp 'customize)
		  (define-key c-xref-menu [c-xref-global-options] '("Options" . c-xref-global-options))
		)
	  (define-key c-xref-menu [c-xref-misc-menu] '("C-xref Misc" . c-xref-misc-menu))
	  (define-key c-xref-menu [separator-buffers2] '("--"))
	  (define-key c-xref-menu [(f7)] '("Delete One Window" . c-xref-delete-window))
	  (define-key c-xref-menu [separator-buffers5] '("--"))
	  (define-key c-xref-menu [c-xref-undo] '("Undo Last Refactoring" . c-xref-undo-last-refactoring))
	  ;;(define-key c-xref-menu [c-xref-set-target] '("Set Target for Next Move" . c-xref-set-moving-target-position))
	  (define-key c-xref-menu [c-xref-refactor] '("Refactor" . c-xref-refactor))
	  (define-key c-xref-menu [separator-buffers4] '("--"))
	  (define-key c-xref-menu [c-xref-class-tree] '("View Class (Sub)tree" . c-xref-class-tree-show))
	  (define-key c-xref-menu [c-xref-search-tag] '("Search Symbol" . c-xref-search-in-tag-file))
	  (define-key c-xref-menu [c-xref-search-def] '("Search Definition in Tags" . c-xref-search-definition-in-tag-file))
	  (define-key c-xref-menu [c-xref-dm-menu] '("Dead code detection" . c-xref-dead-menu))
	  (define-key c-xref-menu [c-xref-sb-menu] '("Browsing with Symbol Stack" . c-xref-sb-menu))
	  (define-key c-xref-menu [c-xref-lm-menu] '("Local Motion" . c-xref-lm-menu))
	  (define-key c-xref-menu [separator-buffers3] '("--"))
	  (define-key c-xref-menu [c-xref-gen-html] '("Generate HTML Documentation" . c-xref-gen-html-documentation))
	  (define-key c-xref-menu [c-xref-fast-update-refs] '("Fast Update of Tags" . c-xref-fast-update-refs))
	  (define-key c-xref-menu [c-xref-update-refs] '("Full Update of Tags" . c-xref-update-refs))
	  (define-key c-xref-menu [c-xref-create-refs] '("Create C-xref Tags" . c-xref-create-refs))
	  (define-key c-xref-menu [separator-buffers6] '("--"))
	  (define-key c-xref-menu [(f8)] '("Complete Identifier" . c-xref-completion))
	  (define-key c-xref-menu [c-xref-ide-menu] '("Emacs IDE" . c-xref-ide-menu))
	  (define-key c-xref-menu [c-xref-project-menu] '("Project" . c-xref-project-menu))
))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                    XEmacs

(if (string-match "XEmacs" emacs-version)
	(progn
	  (defvar c-xref-xemacs-ide-menu
		'("XEmacs IDE"
		  ["Compile File" c-xref-ide-compile-file t]
		  ["Compile Directory" c-xref-ide-compile-dir t]
		  ["Compile Project" c-xref-ide-compile-project t]
		  ["(Last) Compile" c-xref-ide-compile t]
		  "-----"
		  ["Previous Error (or Alternative Reference)" c-xref-alternative-previous-reference t]
		  ["Next Error (or Alternative Reference)"		c-xref-alternative-next-reference t]
		  ;;["Next Error" c-xref-ide-next-error t]
		  ;;["Previous Error" c-xref-ide-previous-error t]
		  "-----"
		  ["Run1" c-xref-ide-run1 t]
		  ["Run2" c-xref-ide-run2 t]
		  ["Run3" c-xref-ide-run3 t]
		  ["Run4" c-xref-ide-run4 t]
		  ["Run5" c-xref-ide-run5 t]
		  ["Run This" c-xref-ide-run-this t]
		  ["(Last) Run" c-xref-ide-run t]
		  "-----"
		  ["(Last) Compile & Run" c-xref-ide-compile-run t]
		  ) "C-xref interface to XEmacs IDE functions" 
		)

	  (defvar c-xref-xemacs-lm-menu
		'("Local Motion"
		  ["Previous Usage of This Symbol (or Alternatives)" c-xref-alternative-previous-reference t]
		  ["Next Usage of This Symbol (or Alternatives)"		c-xref-alternative-next-reference t]
		  ) "C-xref local motion menu for XEmacs" 
		)

	  (defvar c-xref-xemacs-sb-menu
		'("Browsing with Symbol Stack"
		  ["Push This Symbol" c-xref-push-references t]
		  ["Push This Symbol and Goto Definition" c-xref-push-and-goto-definition t]
		  ["Push This Symbol and Display Browser" c-xref-browse-symbol t]
		  ["Push Symbol by Name" c-xref-push-name t]
		  ["Push Symbol and Apply Macro on All References" c-xref-push-and-apply-macro t]
		  "-----"
		  ["Previous Reference" c-xref-previous-reference t]
		  ["Next Reference" c-xref-next-reference t]
		  ["Display Browser" c-xref-show-browser t]
		  "-----"
		  ["Pop Symbol" c-xref-pop-only t]
		  ["Pop Symbol and Return" c-xref-pop-and-return t]
		  "-----"
		  ["Re-push Popped Symbol" c-xref-re-push t]
		  ) "C-xref Source Browsing menu for XEmacs" 
		)

	  (defvar c-xref-xemacs-project-menu
		'("Project"
		  ["New"		        c-xref-project-new t]
		  ["Delete"	        c-xref-project-delete t]
		  ["Set Active"		c-xref-project-set-active t]
		  ["Show Active"		c-xref-project-active t]
		  ["Edit Options"		c-xref-project-edit-options t]
		  ) "C-xref Project menu for XEmacs" 
		)

	  (defvar c-xref-dead-code-menu
		'("Dead Code Detection"
		  ["Browse File Unused Symbols"	  c-xref-push-this-file-unused-symbols t]
		  ["Browse Project Unused Symbols"      c-xref-push-global-unused-symbols t]
		  ) "C-xref Dead code detection"
		)

	  (defvar c-xref-xemacs-misc-menu
		'("C-xref Misc"
		  ["Kill C-xref Process"				c-xref-kill-xref-process t]
		  ["About C-xref"				    c-xref-about t]
		  ["C-xref Help"				    	c-xref-help t]
		  ) "C-xref Miscellaneous Functions"
		)

	  (defvar c-xref-xemacs-menu
		'("C-xref"
		  ["Complete identifier"			c-xref-completion t]
		  "------"
		  ["Create C-xref Tags"			    c-xref-create-refs t]
		  ["Full Update of Tags"	        c-xref-update-refs t]
		  ["Fast Update of Tags"	        c-xref-fast-update-refs t]
		  ["Generate HTML Documentation"	c-xref-gen-html-documentation t]
		  "--"
		  ;; browsing menus will come here
		  ["Search Definition in Tags"	c-xref-search-definition-in-tag-file t]
		  ["Search Symbol"		c-xref-search-in-tag-file t]
		  ["View Class (Sub)tree"	        c-xref-class-tree-show t]
		  "-----"
		  ["Refactor"    		            c-xref-refactor t]
		  ;;   ["Set Target for Next Move"    c-xref-set-moving-target-position t]
		  ["Undo Last Refactoring"		    c-xref-undo-last-refactoring t]
		  "----"
		  ["Delete One Window"		        c-xref-delete-window t]
		  "---"
		  ) "C-xref menu for XEmacs" 
		)
))


(if (string-match "XEmacs" emacs-version)
	(progn
	;; XEmacs
		(defun c-xref-read-key-sequence (prompt) (read-key-sequence nil prompt))
		(if window-system 
			(progn 
			  (set-buffer (get-buffer-create " *dummytogetglobalmap*"))
			  (add-submenu nil c-xref-xemacs-menu)
			  (add-submenu '("C-xref") c-xref-xemacs-project-menu "Complete identifier")
			  (add-submenu '("C-xref") c-xref-xemacs-ide-menu     "Complete identifier")
			  (add-submenu '("C-xref") c-xref-xemacs-lm-menu      "Search Definition in Tags")
			  (add-submenu '("C-xref") c-xref-xemacs-sb-menu      "Search Definition in Tags")
			  (add-submenu '("C-xref") c-xref-dead-code-menu      "Search Definition in Tags")
			  (add-submenu '("C-xref") c-xref-xemacs-misc-menu nil)
			  (if (commandp 'customize)
				  (add-menu-button '("C-xref")
								   ["Options" c-xref-global-options t]
								   ))
			  (kill-buffer " *dummytogetglobalmap*")
			  )
		  )
		)
  ;; Emacs
  (defun c-xref-read-key-sequence (prompt) (read-key-sequence prompt))
  (let ((menu (lookup-key global-map [menu-bar])))
	(define-key-after menu [c-xref] 
	  (cons "C-xref" c-xref-menu)
	  (car (nth (- (length menu) 3) menu))
	  )
	)
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                           Custom menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar c-xref-custom-menu-symbol-count 1)

(defun c-xref-emacs-add-menu-item (menu text expression)
  ;; first create a new function with expression as body
  (defalias (intern (format "c-xref-custom-menu-fun-%d" c-xref-custom-menu-symbol-count)) 
	(append '(lambda () "C-xrefactory menu custom function" (interactive "")) (list expression)))
  
  ;; now put new menu item for this function
  (define-key menu 
	(vector (make-symbol (format "c-xref-custom-menu-sym-%d" c-xref-custom-menu-symbol-count)))
	(cons text (intern (format "c-xref-custom-menu-fun-%d" c-xref-custom-menu-symbol-count))))

  ;; increment counter
  (setq c-xref-custom-menu-symbol-count (1+ c-xref-custom-menu-symbol-count))
)

(defun c-xref-xemacs-add-menu-item (menu text expression after)
  (add-menu-button menu (vector text expression t) after)
)

(defun c-xref-add-custom-menu-item (text expression)
"Add an item to C-xrefactory's 'Custom' submenu. If the 'Custom' submenu
does not  exist, create it.   TEXT is the  name of the  inserted item.
EXPRESSION is an  expression which will be evaluated  when the item is
selected.  For  example: (c-xref-add-custom-menu-item \"Call java2html\"
'(shell-command  (c-xref-get-env  \"java2html\")))  will  add  a   'Call
java2html' menu item, which will execute the shell command specified by
the 'java2html' c-xref environment variable. This variable should be set
by a '-set java2html <command>' option in your .c-xrefrc file.
"
  (if (string-match "XEmacs" emacs-version)
	  (progn
		(add-submenu '("C-xref") '("Custom") "------")
		(c-xref-xemacs-add-menu-item '("C-xref" "Custom") text expression 
								   "XEmacs IDE")
		)
	(if (not (boundp 'c-xref-custom-menu))
		(progn
		  (defvar c-xref-custom-menu (make-sparse-keymap "C-xref Custom Menu"))
		  (fset 'c-xref-custom-menu (symbol-value 'c-xref-custom-menu))
		  (define-key-after c-xref-menu [c-xref-custom-menu] 
			(cons "Custom" c-xref-custom-menu) 
			(car (nth 3 c-xref-menu)))
		  ))
	(c-xref-emacs-add-menu-item c-xref-custom-menu text expression)
	)
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;  autoloads ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defvar c-xref-default-documentation-string "Documentation not yet available, please invoke an C-xrefactory function first.")


(autoload 'c-xref-global-options "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-project-new "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-project-delete "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-project-set-active "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-project-active "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-project-edit-options "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-ide-compile-file "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-compile-dir "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-compile-project "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-compile "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-previous-error "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-run "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-run-this "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-run1 "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-run2 "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-run3 "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-run4 "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-run5 "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-ide-compile-run "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-create-refs "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-fast-update-refs "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-update-refs "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-gen-html-documentation "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-alternative-previous-reference "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-alternative-next-reference "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-push-references "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-push-and-goto-definition "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-browse-symbol "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-push-name "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-push-and-apply-macro "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-next-reference "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-previous-reference "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-pop-and-return "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-pop-only "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-show-browser "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-re-push "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-search-in-tag-file "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-search-definition-in-tag-file "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-push-this-file-unused-symbols "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-push-global-unused-symbols "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-delete-window "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-set-moving-target-position "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-refactor "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-undo-last-refactoring "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-completion "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-class-tree-show "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-help "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-about "c-xref" c-xref-default-documentation-string t)
(autoload 'c-xref-kill-xref-process "c-xref" c-xref-default-documentation-string t)

(autoload 'c-xref-find-file-on-mouse "c-xref" c-xref-default-documentation-string t)

;; this has to be kept, because of options
(autoload 'c-xref-soft-select-dispach-data-caller-window "c-xref" c-xref-default-documentation-string t)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; examples on how to use 'c-xref-add-custom-menu-item function

;;(c-xref-add-custom-menu-item 
;; "Call Java2html" 
;; '(shell-command (c-xref-get-env "java2html"))
;;)
;;(c-xref-add-custom-menu-item 
;; "Gen HTML Documentation" 
;; '(c-xref-call-non-interactive-process c-xref-current-project "-html" 'assynchronized 
;;									 'newwin "Generating HTML. ")
;;)


;; Added by Package.el.  This must come before configurations of
;; installed packages.  Don't delete this line.  If you don't want it,
;; just comment it out by adding a semicolon to the start of the line.
;; You may delete these explanatory comments.
(package-initialize)

;;; El-get
(add-to-list 'load-path "~/.emacs.d/el-get/el-get")
(require 'el-get)
(add-to-list 'el-get-recipe-path "~/.emacs.d/el-get-user/recipes")

(add-to-list 'load-path "~/.emacs.d/el-get/c-xrefactory/editors/emacs")
(load-file "~/.emacs.d/el-get/c-xrefactory/editors/emacs/c-xrefactory.el")

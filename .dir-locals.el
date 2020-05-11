;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

(add-to-list 'auto-mode-alist '("\\.c\\'" . cov-mode))
((c-mode . (
            (indent-tabs-mode . nil)
            (c-basic-offset . 4)
            )
         )
 )

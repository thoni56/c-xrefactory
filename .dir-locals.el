;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

(
 (c-mode . (
            (indent-tabs-mode . nil)
            (c-file-style . "k&r")
            (c-basic-offset . 4)
            ;;(eval . (c-set-offset 'arglist-close '0))
            (mode . cov)
            )
         )
 )

;; cov-mode (https://github.com/AdamNiederer/cov)
;; Ignore all coverage data except in gcov files
(setq cov-coverage-file-paths '("."))

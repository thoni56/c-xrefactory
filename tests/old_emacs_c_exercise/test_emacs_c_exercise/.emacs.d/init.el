(add-hook 'after-init-hook (lambda () (set-frame-height (selected-frame) 37 )))
(let ((top-dir (file-truename "../../")))
  (add-to-list 'exec-path (concat top-dir "src"))
  (add-to-list 'load-path (concat top-dir "editors/emacs")))
(load "c-xrefactory.el")

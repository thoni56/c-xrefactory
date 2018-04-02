(let ((top-dir (file-truename "../../")))
  (add-to-list 'exec-path (concat top-dir "src"))
  (add-to-list 'load-path (concat top-dir "env/emacs")))
(load "c-xrefactory.el")

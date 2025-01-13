(let ((script-dir (file-name-directory (or load-file-name buffer-file-name))))
  (if script-dir
      (let ((default-directory script-dir))
        (compile "make")
        (load (expand-file-name "editors/emacs/c-xrefactory.el"))
        (setq c-xref-exec-directory (expand-file-name "src/"))
        )
    ))

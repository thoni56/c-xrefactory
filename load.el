;;; load.el --- Build and load c-xrefactory from this clone  -*- lexical-binding: t; -*-

(let ((script-dir (file-name-directory (or load-file-name buffer-file-name))))
  (if script-dir
      (let ((default-directory script-dir))
        (compile "make")
        (load (expand-file-name "editors/emacs/c-xrefactory.el"))
        (setq c-xref-exec-directory (expand-file-name "src/"))
        )
    ))

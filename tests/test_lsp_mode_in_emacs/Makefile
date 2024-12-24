include ../Makefile.boilerplate

$(TEST):
	emacs --batch --no-init-file -l lsp-initialize.el 2>&1 | grep -v "Loading .*..." | cat

interactive:
	emacs --no-init-file -l lsp-initialize.el tmpfile 2>&1 | grep -v "Loading .*..."

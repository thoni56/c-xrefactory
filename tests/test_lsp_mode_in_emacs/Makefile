include ../Makefile.boilerplate

$(TEST):
	emacs --batch --no-init-file --no-splash -l lsp-initialize.el 2>&1 | grep -v "Loading .*..." | cat

interactive:
	emacs --no-init-file --no-splash -l lsp-initialize.el -l ../../utils/rebind-for-lsp.el cexercise/browsing.c 2>&1 | grep -v "Loading .*..."

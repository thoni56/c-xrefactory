include ../Makefile.boilerplate

$(TEST):
	emacs --batch --no-init-file --no-splash -l lsp-initialize.el 2>&1 | grep -v "Loading .*..." | cat

interactive:
	emacs --no-init-file --no-splash -l ../../utils/lsp-initialize.el browsing.c 2>&1 | grep -v "Loading .*..."

debug:
	emacs --no-init-file --no-splash -l ../../utils/lsp-initialize.el browsing.c 2>&1 | grep -v "Loading .*..." &
	@read -p "Ange PID för processen du vill debugga: " pid; \
	if [ -z "$$pid" ]; then \
		echo "Ingen PID angiven. Avbryter."; \
		exit 1; \
	fi; \
	echo "Startar gdb mot PID $$pid..."; \
	gdb -q -p $$pid

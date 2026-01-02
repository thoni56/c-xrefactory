include ../Makefile.boilerplate


ARGUMENTS = -lsp -debug -log=log $(DELAY)

$(TEST): input.lsp
	$(COMMAND) > output.tmp < input.lsp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json > input.lsp

attach: DELAY=-delay=5
attach: input.lsp
	@$(COMMAND) < input.lsp & echo $$! > c-xref.pid
	@sleep 1
	@gdb -q -p `cat c-xref.pid`

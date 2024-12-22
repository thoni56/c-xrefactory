include ../Makefile.boilerplate


ARGUMENTS = -lsp

$(TEST):
	../../utils/lsp_driver.py input.json | $(COMMAND) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS+=-trace -log=log
trace: $(TEST)

include ../Makefile.boilerplate


ARGUMENTS = -lsp

$(TEST): input.lsp
	$(COMMAND) > output.tmp < input.lsp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS+=-trace -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json > input.lsp

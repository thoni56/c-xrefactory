include ../Makefile.lsp


ARGUMENTS = -lsp

$(TEST): input.lsp
	$(COMMAND) > output.tmp < input.lsp
	$(NORMALIZE) output.tmp | tail -n +4 | grep -v Content-Length > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json --curdir=$(CURDIR) > input.lsp

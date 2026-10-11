include ../Makefile.lsp

# Scenario: a request after shutdown
#
# Given an LSP server that has been shut down
# When the client sends another request before exit
# Then the server answers it with an InvalidRequest error, as the protocol says

ARGUMENTS = -lsp

$(TEST): input.lsp
	$(COMMAND) > output.tmp < input.lsp
	$(NORMALIZE) output.tmp | tail -n +4 > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json > input.lsp

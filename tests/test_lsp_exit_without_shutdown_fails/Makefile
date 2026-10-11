include ../Makefile.lsp

# Scenario: the client exits the server without shutting it down first
#
# Given an initialized LSP server
# When the client sends exit without a shutdown before it
# Then the server exits with code 1, as the protocol says

ARGUMENTS = -lsp

$(TEST): input.lsp
	-$(COMMAND) < input.lsp > /dev/null; echo "exit code $$?" > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json > input.lsp

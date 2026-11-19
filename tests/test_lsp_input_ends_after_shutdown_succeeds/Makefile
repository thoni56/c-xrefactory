include ../Makefile.lsp

# Scenario: the client's input ends after a shutdown, without an exit
#
# Given an initialized LSP server
# When its input ends after a shutdown, without an exit
# Then the server exits with code 0, as an exit after a shutdown does

ARGUMENTS = -lsp

$(TEST): input.lsp
	-$(COMMAND) < input.lsp > /dev/null; echo "exit code $$?" > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json > input.lsp

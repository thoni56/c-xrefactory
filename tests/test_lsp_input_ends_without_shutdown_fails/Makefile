include ../Makefile.lsp

# Scenario: the client's input ends without a shutdown or an exit
#
# Given an initialized LSP server
# When its input ends without a shutdown or an exit
# Then the server exits with code 1, as an exit without a shutdown does

ARGUMENTS = -lsp

$(TEST): input.lsp
	-$(COMMAND) < input.lsp > /dev/null; echo "exit code $$?" > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json > input.lsp

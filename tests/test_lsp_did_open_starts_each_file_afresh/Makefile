include ../Makefile.lsp

# Scenario: a file opened after another is parsed without what the first defined
#
# Given a.c is opened, and it defines the macro FEATURE
# And b.c is opened, and it declares wrong under #ifdef FEATURE and right otherwise
# When the definition of the use of right in b.c is requested
# Then the answer is the declaration of right in b.c

ARGUMENTS = -lsp

$(TEST): input.lsp
	$(COMMAND) > output.tmp < input.lsp
	$(NORMALIZE) output.tmp | tail -n +4 | grep -v Content-Length > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json --curdir=$(CURDIR) > input.lsp

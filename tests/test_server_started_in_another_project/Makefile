include ../Makefile.server

# Scenario: a server started in one project is opened on another
#
# Given the server is started in project A, the test directory, whose config
#   sets a variable and a define
# And project B lives in projectB/ with a config of its own
# When the first request asks for the project of a file in B
# Then the answer is project B
# And A's variable is not set
# And B's file is parsed without A's define, so the push finds `right`
#
# The client starts the server wherever Emacs happens to be, so the directory
# it is started in says nothing about the project it will be asked about.

$(TEST): .c-xrefrc
	@rm -rf .c-xref projectB/.c-xref
	@printf '[projectB]\n  %s/projectB\n' '$(CURDIR)' > projectB/.c-xrefrc
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

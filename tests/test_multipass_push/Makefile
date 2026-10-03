include ../Makefile.server

# Scenario: a push works in a project with several passes
#
# Given a config with -pass1 -DPASS1 and -pass2 -DPASS2
# When common is pushed on in source.c
# Then the server answers with its definition

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input --answer-timeout=10 $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

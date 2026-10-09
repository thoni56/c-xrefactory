include ../Makefile.server

# Scenario: a request file changed on disk keeps no references to its old content
#
# Given definer.c defines f on line 2, and nobody has it open in an editor
# And a push from user.c has parsed both CUs
# When definer.c is rewritten on disk, with f moved to line 4, and no preload
# And f is pushed on in definer.c
# Then f is defined on line 4 only

$(TEST): .c-xrefrc
	@cp definer.c.v1 definer.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

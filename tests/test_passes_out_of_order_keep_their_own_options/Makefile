include ../Makefile.server

# Scenario: passes given out of order each get only their own options
#
# Given a config with -pass2 -DPASS2 before -pass1 -DPASS1
# And x declared under #if defined(PASS1) && defined(PASS2), and again under #else
# When x is pushed on in use()
# Then the server goes to the declaration under #else, since no pass has both defines

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input --answer-timeout=10 $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

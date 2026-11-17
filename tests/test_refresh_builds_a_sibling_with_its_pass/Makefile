include ../Makefile.server

# Scenario: a sibling built by the refresh keeps what only pass 2 defines
#
# Given a config with -pass1 -DPASS1 and -pass2 -DPASS2
# And a.c and b.c both including h.h, which declares shared
# And b.c defining shared only under #ifdef PASS2
# When shared is pushed on in a.c
# Then the server goes to the definition in b.c

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input --answer-timeout=10 $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

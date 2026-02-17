include ../Makefile.server

# Scenario: PREVIOUS when source file (where cursor is) is stale
#
# Given the cursor is in source.c which has been modified (stale)
# And the browsing stack has references in both source.c and header.h
# When the user does PREVIOUS
# Then source.c is reparsed first, cursor restored, then navigation proceeds

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads for original content
	cp header.h header.preload
	cp source.c source.preload
# Ensure source.modified has newer timestamp than source.preload
	sleep 2
	touch source.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f header.preload source.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

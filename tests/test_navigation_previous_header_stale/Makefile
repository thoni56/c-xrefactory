include ../Makefile.server

# Scenario: PREVIOUS to directly included stale header
#
# Given header.h is in an editor buffer and has been modified (+3 lines)
# And source.c includes header.h directly
# When the user does PREVIOUS targeting header.h
# Then the system finds source.c includes header.h via include references
# And reparses source.c (which pulls in fresh header.h)
# And positions in header.h reflect the modified file

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads for original content
	cp header.h header.preload
	cp source.c source.preload
# Ensure header.modified has newer timestamp than header.preload
	sleep 2
	touch header.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f header.preload source.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

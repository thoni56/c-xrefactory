include ../Makefile.server

# Scenario: PREVIOUS to transitively included stale header
#
# Given inner.h is in an editor buffer and has been modified (+3 lines)
# And source.c includes middle.h which includes inner.h (transitive)
# When the user does PREVIOUS targeting inner.h
# Then the system finds the transitive include chain via include references
# And reparses source.c (which pulls in middle.h which pulls in fresh inner.h)
# And positions in inner.h reflect the modified file

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads for original content
	cp inner.h inner.preload
	cp source.c source.preload
# Ensure inner.modified has newer timestamp than inner.preload
	sleep 2
	touch inner.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f inner.preload source.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

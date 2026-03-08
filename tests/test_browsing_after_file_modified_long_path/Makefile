include ../Makefile.server

# Scenario: NEXT to a stale CU
#
# Given source.c is in an editor buffer and has been modified (shifted lines)
# And the browsing stack cursor is in header.h
# When the user does NEXT and the target is in source.c
# Then source.c is reparsed and positions reflect the modified file

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
# Create preloads for original content
	cp header.h header.preload
	cp source.c source.preload1
# Ensure source.modified has newer timestamp than source.preload1
	sleep 2
	touch source.modified
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

local-clean:
	rm -f header.preload source.preload1 source.preload2

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

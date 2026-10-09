include ../Makefile.server

# Scenario: a header changed on disk leaves no references behind in its other includers
#
# Given definer.c and user.c both include header.h, which nobody has open in an editor
# When header.h is rewritten on disk, with the declaration of f moved to line 3
# And f is pushed on in definer.c
# Then the declaration is on line 3 only
#
# definer.c is parsed again, but user.c is not, and its reference to line 1 of header.h
# stays. Pass 2 parses the includers of a header only when the header is edited.

$(TEST): .c-xrefrc
	@cp header_v1.h header.h
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

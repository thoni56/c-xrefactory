include ../Makefile.server

# Scenario: a header deleted on disk leaves no references behind
#
# Given definer.c and user.c both include header.h, which declares f
# When header.h is deleted on disk
# And f is pushed on in definer.c
# Then no reference to f is in header.h
#
# Its includers are out of date and built again, and their #include now fails,
# so neither puts anything back in header.h. What was there must be removed
# when the file vanishes.

$(TEST): .c-xrefrc
	@cp header.h.original header.h
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

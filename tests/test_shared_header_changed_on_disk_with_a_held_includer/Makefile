include ../Makefile.server

# Scenario: a header changed on disk leaves no references behind when an includer is held
#
# Given definer.c and user.c both include header.h, and definer.c is held in the editor
# When header.h is rewritten on disk, with the declaration of f moved to line 3
# And f is pushed on in user.c
# Then the declaration is on line 3 only
#
# The held step builds definer.c before Pass 3 looks at the includers of header.h. By
# then definer.c is up to date, so it is not counted as built again, header.h is not
# stripped, and its reference to line 1 of header.h stays.
#
# definer.c is held from the first push, so its preload keeps the time of that request.
# The sleep keeps the change of header.h out of the coarse-clock gap of the build that
# follows. Within the gap definer.c looks out of date again, Pass 3 builds it a second
# time, and the strip happens.

$(TEST): .c-xrefrc
	@cp header_v1.h header.h
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

include ../Makefile.server

# Scenario: a header restored with an older time is seen to change
#
# Given definer.c and user.c both include header.h, which declares f on line 2
# When header.h is replaced by an older version, with its older time, as
#      `cp -p`, `tar` or a checkout can do
# And f is pushed on in definer.c
# Then the declaration of f is found on line 1 of header.h
#
# The restored time is older than the knowledge, so "newer" does not see the
# change (ADR-0032 accepted that). A difference does (ADR-0036).

$(TEST): .c-xrefrc
	@cp header.h.current header.h
	@touch -t 200001010000 header.h.restored
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

include ../Makefile.server

# Scenario: Server saves snapshot to disk on exit
#
# Given an auto-detect project with no prior snapshot
# And ins.c and cla.c both include atr_x.h
# When the server handles a navigation request and receives -exit
# Then .c-xref/db should exist on disk

$(TEST):
	@rm -rf .c-xref
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@if test -f .c-xref/db ; then \
		$(OK) ; \
	else \
		$(FAIL) ; \
		echo "snapshot db not created on exit" ; \
		exit 1 ; \
	fi

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

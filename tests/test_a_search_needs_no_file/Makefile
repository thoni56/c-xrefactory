# Scenario: a search needs no file
#
# Given a project with the symbols main and shared
# When a search for sh* comes without a file, as completing in the search prompt sends it
# Then the answer is shared in the short list, and no error

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer .c-xref
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@sed -n '/^"-search/,/^<\/symbol-list>/p' output.tmp | sed "s+$(CURDIR)+CURDIR+g" > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

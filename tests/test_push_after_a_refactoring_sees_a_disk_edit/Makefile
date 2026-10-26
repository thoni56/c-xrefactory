# A file edited on disk after a refactoring is parsed as it is on disk
#
# Given a rename that the server makes in common.h, source.c and other.c
# When the client has saved the renamed files, and other.c then gets a line
#   at the top outside the editor
# Then a push on read_it in other.c, at its new position, finds it

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@cp common.h.original common.h
	@cp source.c.original source.c
	@cp other.c.original other.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

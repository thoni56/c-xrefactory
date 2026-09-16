# A static is private to its file, so renaming one must not touch a same-named
# static in another file.
#
# Both files are called util.c on purpose: static link names are built from
# simpleFileName(), the BASENAME, so a/util.c and b/util.c produce the same
# link name and the two statics become one symbol.
#
# The push into b/util.c is there to get it parsed before the rename; browsing
# would have done that in a real session.

include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

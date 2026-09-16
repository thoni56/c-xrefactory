# A static outside the project root is spelled /.../<name>
#
# The project is the `project` subdirectory, so `elsewhere/helper.h` is outside
# it and its static cannot be named by a relative path. setStaticFunctionLinkName
# falls back to the "/.../<name>" spelling that utils/cxref_db.py already uses
# for a file outside the tree.
#
# The project root is the directory holding .c-xrefrc, so this test puts one in
# `project` and removes the one the boilerplate generates here - with both in
# place the test directory would be the root and helper.h would be inside it.

include ../Makefile.server

$(TEST):
	@rm -f .c-xrefrc
	@rm -rf project/.c-xref output output.tmp
	@printf '[%s]\n  %s/project\n' '$(TEST)' '$(CURDIR)' > project/.c-xrefrc
	@$(SERVER_DRIVER) commands.input > output.tmp
	@../../utils/cxref_reader project/.c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

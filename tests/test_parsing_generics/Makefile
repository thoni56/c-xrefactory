# Every branch of a _Generic is parsed
#
# A cross-referencer does not need to select the branch the controlling
# expression's type picks. Recording references in all associations is what
# rename and find-references want, so the function in each branch, the typedef
# used as an association type, and the controlling expression are all expected.

include ../Makefile.server


$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

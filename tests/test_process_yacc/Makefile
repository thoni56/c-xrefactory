# A reference from a .y file to an extern function in an included header
#
# The control for test_yacc_included_enum_reference: an extern function is the
# storage class that keeps its plain link name when parsed at nesting level > 0,
# where that test's enum constant did not until a8318945.
#
# Both compilation units are asked for. file2.c includes nothing, so the two
# share no header and nothing would parse file.y on its own. The walk is the
# assertion: pushing from the call in file.y and stepping through the session
# reaches the definition in file2.c, the declaration in file2.h and the call in
# file.y. Three references in one session is what it means for the yacc
# reference to be under the same symbol as the C one.
#
# byacc is not run here: y.tab.c was never what this asserted, and a copy left
# by an older run would be parsed as one more C file in the project.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output.tmp y.tab.c
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

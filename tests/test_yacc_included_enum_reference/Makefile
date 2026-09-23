# A reference from a .y file to an enum constant in an included header
#
# The pair to test_process_yacc, which does the same with an extern function:
# that is the storage class which kept its plain link name when parsed at
# nesting level > 0, where the enum constant did not until a8318945.
#
# Both compilation units are asked for, so the test does not depend on which
# siblings entry refresh chooses to parse. The walk is the assertion: pushing
# from the use in file.y and stepping through the session reaches the
# definition in file2.h, the use in file.y and the use in file2.c. Three
# references in one session is what "the yacc reference is under the same
# symbol as the C one" means, so the enum constant did not get a nested link
# name from the .y outer block.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output.tmp
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

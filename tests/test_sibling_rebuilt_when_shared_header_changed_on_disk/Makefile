include ../Makefile.server

# Scenario: a CU that is not the request file is built again when a header it includes changes on disk
#
# Given a.c and b.c both include x.h, which nobody has open in an editor
# And b.c calls CALL(), which x.h defines as f
# When x.h is rewritten on disk, with CALL defined as g
# And g is pushed on in a.c
# Then the use of g in the macro body, which only b.c puts there, is among its references

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@cp x_v1.h x.h
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

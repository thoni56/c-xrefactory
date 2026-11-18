include ../Makefile.server

# Scenario: a CU changed on disk outside the goal loses its references
#
# Given a.c declares f and calls it, and b.c defines f, and they share no header
# And a.c has been built
# When a.c is rewritten on disk without the call
# And f is pushed on in b.c, whose goal does not contain a.c
# Then f is referenced from its definition in b.c only, since a.c's old references are gone

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@cp a.c.v1 a.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

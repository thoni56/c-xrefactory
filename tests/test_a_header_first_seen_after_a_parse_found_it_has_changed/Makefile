include ../Makefile.server

# Scenario: a header a parse found before the removal saw it has changed when it is first seen
#
# Given a.c includes p.h as <p.h>, which the scan does not follow, and uses v, which p.h declares
# And b.c defines v, and shares no header with a.c
# And a.c has been built, so the parse put p.h's references there
# When p.h is rewritten on disk with its declaration a line further down
# And v is pushed on in b.c
# Then p.h's old declaration is not among v's references, since a file not seen before
# has changed (ADR-0036)

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@cp p.h.v1 p.h
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

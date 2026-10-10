include ../Makefile.server

# Scenario: a held CU drops a reference an included header has switched off
#
# Given a.h defines USE_F as 1, and c.c declares f and calls it under #if USE_F
# And c.c is held in an editor, unchanged, but a.h is not
# And d.c defines f and shares no header with c.c
# When a.h is rewritten on disk with USE_F as 0
# And f is pushed on in d.c
# Then f is referenced from its declaration and definition only, not from the call

$(TEST): .c-xrefrc
	@cp a_v1.h a.h
	@cp c.c c.preload
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

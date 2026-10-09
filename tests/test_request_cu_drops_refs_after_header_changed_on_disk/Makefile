include ../Makefile.server

# Scenario: the request CU drops a reference an included header has switched off
#
# Given a.h defines USE_F as 1, and c.c declares f and calls it under #if USE_F
# And nobody has a.h open in an editor
# When a.h is rewritten on disk with USE_F as 0
# And f is pushed on in c.c
# Then f is referenced from its declaration only, not from the call

$(TEST): .c-xrefrc
	@cp a_v1.h a.h
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

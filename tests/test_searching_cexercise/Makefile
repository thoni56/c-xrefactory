# Automated tests for exercising completion server

include ../Makefile.server


$(TEST): .c-xrefrc
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

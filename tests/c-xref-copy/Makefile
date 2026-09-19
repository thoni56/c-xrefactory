include ../Makefile.server

$(TEST):
	$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp 2>&1
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA=--extra '-debug -log=trace'
trace: $(TEST)

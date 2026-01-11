include ../Makefile.boilerplate

$(TEST):
	$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA+=--extra="-debug -log=log"
trace: $(TEST)

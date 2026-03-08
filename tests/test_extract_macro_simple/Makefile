include ../Makefile.server


$(TEST):
	@# First do the extract
	@-$(SERVER_DRIVER) commands1.input $(EXTRA) > output1.tmp
	@$(NORMALIZE) output1.tmp > output
	@# Then a rename - as the editor would
	@-$(SERVER_DRIVER) commands2.input $(EXTRA) > output2.tmp
	@$(NORMALIZE) output2.tmp >> output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

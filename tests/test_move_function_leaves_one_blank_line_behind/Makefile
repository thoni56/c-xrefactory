include ../Makefile.server

# Scenario: a moved function leaves one blank line behind
#
# Given source.c has main, func and other, with a blank line between each
# When func is moved to target.c
# Then one of the two blank lines left between main and other is removed

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

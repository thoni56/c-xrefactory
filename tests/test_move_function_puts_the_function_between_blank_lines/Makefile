include ../Makefile.server

# Scenario: a moved function is put between blank lines
#
# Given target.c has a and b with a blank line between them
# When func is moved from source.c to that blank line
# Then a blank line is inserted before func, so it is apart from a,
#   and the blank line that was there stays after it, apart from b

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

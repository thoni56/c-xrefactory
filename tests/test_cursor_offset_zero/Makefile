include ../Makefile.server

# When cursor offset = 0, the server enters a special case, creates an include reference
# to the file itself at the first position in the file sets the cursor position at that
# position. This test just executes the code for that special case, hopefully...

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v "<int val=" > output
	@$(VERIFY)

trace: EXTRA = '-debug'
trace: $(TEST)

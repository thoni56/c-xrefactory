include ../Makefile.boilerplate


ARGUMENTS = -xrefactory-II $(CURDIR)/file.java

$(TEST):
	-$(COMMAND) -log=log $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp | grep -v progress > output
	@$(VERIFY)

trace: EXTRA=-trace
trace: $(TEST)

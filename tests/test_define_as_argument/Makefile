include ../Makefile.boilerplate

# There should be one reference for a and one for b, both on line 2
# since we define A=a and B=b and use A and B on that line

ARGUMENTS = -DA=a -DB=b source.c

$(TEST):
	$(COMMAND) > output.tmp
	grep 2/[ab] CXrefs/* | wc -l > output
	$(VERIFY)

trace: ARGUMENTS += -trace
trace: $(TEST)

EXTRA_DEBUG =

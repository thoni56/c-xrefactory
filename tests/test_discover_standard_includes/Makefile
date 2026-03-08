# Automated test for finding standard includes (uses stdbool.h)
# Initial version

include ../Makefile.cli

$(TEST):
	@rm -rf C-Xrefs $(TEST).log
	@$(COMMAND) standard_includes.c -o standard_includes.log
	@# We are using the fact that it *can* find stdbool.h proof that it looks in standard dirs
	@grep "can't open file stdbool.h" standard_includes.log > /dev/null ; \
	if [ $$? -ne 1 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

# Automated test for finding standard includes (uses stdbool.h)
# Initial version

include ../Makefile.boilerplate

$(TEST):
	@rm -rf C-Xrefs $(TEST).log
	@printf "Tests in \"$(TEST)\": "
	@../../src/c-xref -xrefrc .c-xrefrc -refs $(CURDIR)/C-Xrefs $(TEST).c -o $(TEST).log
	@# We are using the fact that it *can* find stdbool.h proof that it looks in standard dirs
	@grep "can't open file stdbool.h" $(TEST).log > /dev/null ; \
	if [ $$? -ne 1 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

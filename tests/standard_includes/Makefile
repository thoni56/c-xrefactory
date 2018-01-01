# Automated test for finding standard includes (uses stdbool.h)
# Initial version

include ../Makefile.boilerplate

$(TEST):
	@rm -rf C-Xrefs $@.log
	@printf "Tests in \"$@\": "
	@../../src/c-xref -xrefrc c-xrefrc -refs $(CURDIR)/C-Xrefs $@.c -o $@.log
	@# We are using the fact that it *can* find stdbool.h proof that it looks in standard dirs
	@grep "can't open file stdbool.h" $@.log > /dev/null ; \
	if [ $$? -ne 1 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

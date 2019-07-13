# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc single_int*.c -o $(TEST).log

$(TEST):
	@printf "Tests in '$@': "
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	@../../utils/cxref_reader.py > output
	@cmp output expected ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		diff expected output ; \
		echo "Expected:" ; \
		hexdump -C expected ; \
		echo "But got:" ; \
		hexdump -C output ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

gdb:
	gdb --args $(COMMAND)

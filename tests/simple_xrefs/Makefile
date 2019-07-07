# Automated tests for html generation
# Initial version

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc single_int*.c -o $(TEST).log

$(TEST):
	@printf "Tests in '$@': "
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	./cxref_reader.py > out ; \
	cmp out expected ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		diff expected out ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

gdb:
	gdb --args $(COMMAND)

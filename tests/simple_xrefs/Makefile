# Automated tests for html generation
# Initial version

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc single_int*.c -o $@.log

$(TEST):
	@printf "Tests in '$@': "
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	@file=`grep single_int1.c CXrefs/XFiles | awk -F' ' '{print $1}'` ; \
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

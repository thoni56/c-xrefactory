# Automated tests for html generation
# Initial version

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc single_int.c -o $@.log

$(TEST):
	@printf "Tests in '$@': "
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	file=`grep single_int.c CXrefs/XFiles | awk -F' ' '{print $1}'` ; \
	echo $$file
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

gdb:
	gdb --args $(COMMAND)

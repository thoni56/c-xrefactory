# Automated tests for html generation
# Initial version

include ../Makefile.boilerplate

$(TEST):
	@printf "Tests in '$@': "
	@rm -rf output $@.log
	@../../src/c-xref -xrefrc .c-xrefrc -html -htmlroot=output -htmlcutcwd $@.[ch] -o $@.log
	@grep '<A HREF="$@.h.html#:5">Options</A> options;' output/$@.c.html > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

gdb:
	gdb --args ../../src/c-xref -xrefrc .c-xrefrc -html -htmlroot=output -htmlcutcwd $(TEST).[ch] -o $(TEST).log

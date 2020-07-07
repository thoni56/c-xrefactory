# Automated tests for html generation
# Initial version

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc -html -htmlroot=output -htmlcutcwd -htmlgxlist -htmllxlist basic_html_generation.[ch] -o $@.log

$(TEST):
	@rm -rf output $@.log
	@$(COMMAND)
	@grep '<A HREF="basic_html_generation.h.html#:5">Options</A>' output/basic_html_generation.c.html > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		exit 1 ; \
	else \
		$(OK) ; \
	fi

gdb:
	gdb --args $(COMMAND)

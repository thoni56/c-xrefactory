# Automated tests for html generation
# Initial version

all: basic_html_generation

basic_html_generation: c-xrefrc
	@rm -rf output $@.log
	@printf "Tests in \"$@\": "
	@../../src/c-xref -xrefrc c-xrefrc -p $@ -html -htmlroot=output -htmlcutcwd $@.[ch] -o $@.log
	@grep '<A HREF="$@.h.html#:5">Options</A> options;' output/$@.c.html > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

c-xrefrc: c-xrefrc.tpl
	@sed -e "s+PWD+$(CURDIR)+g" c-xrefrc.tpl > c-xrefrc

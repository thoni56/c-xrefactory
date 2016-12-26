# Automated tests for html generation
# Initial version

all: basic_html_generation

basic_html_generation:
	@rm -rf output $@.log
	@../../src/c-xref -html -htmlroot=output -htmlcutcwd basic_html_generation.[ch] -o $@.log
	@grep '<A HREF="basic_html_generation.h.html#:5">Options</A> options;' output/basic_html_generation.c.html > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Tests for \"$@\": Failed" ; \
		exit 1 ; \
	else \
		echo "Tests for \"$@\": Ok" ; \
	fi

# Automated test for finding standard includes (uses stdbool.h)
# Initial version

all: standard_includes

standard_includes: c-xref c-xrefrc
	@rm -rf Xrefs $@.log
	@printf "Tests in \"$@\": "
	@../../src/c-xref -xrefrc c-xrefrc -p $@ $@.c -o $@.log
	@grep "can't open file stdbool.h" $@.log > /dev/null ; \
	if [ $$? -ne 1 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

c-xref:
	ln -s ../../src/c-xref

c-xrefrc: c-xrefrc.tpl
	sed -e "s+PWD+$$PWD+g" c-xrefrc.tpl > c-xrefrc

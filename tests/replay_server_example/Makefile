# This should become the golden standard for creating an edit_server test:
#
# 1. generate CXrefs if they don't exist by running c-xref
# 2. use ../../utils/edit_server_driver to feed the edit_server what's in commands.input
# 3. normalize commands.output and compare
#

include ../Makefile.boilerplate

CXREF = ../../src/c-xref
EDIT_SERVER_DRIVER = ../../utils/edit_server_driver.py

$(TEST): CXrefs
	@printf "Tests in '$@': "
	@$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) buffer > output.tmp
	@sed -e "s+$(CURDIR)+CURDIR+g" -e "s/len=[0-9]*/len=<n>/g" output.tmp > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

trace: CXREF+=-trace
trace: $(TEST)

CXrefs:
	@$(CXREF) -xrefrc=.c-xrefrc single_int*.c -o output.log

clean:
	rm -rf CXrefs

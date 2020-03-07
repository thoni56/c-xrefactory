# Automated tests for olcx_push a.k.a goto-definition
#
include ../Makefile.boilerplate

CXREF = ../../src/c-xref
EDIT_SERVER_DRIVER = ../../utils/edit_server_driver.py

$(TEST): CXrefs
	@printf "Tests in '$@': "
	@$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) buffer > output.tmp
	@sed -e "s/len=[0-9]*/len=<n>/g" -e "s#$(CURDIR)#CURDIR#g" output.tmp > output
	@diff output expected > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi
	@exit 0

trace: CXREF+=-trace
trace: $(TEST)

CXrefs:
	@$(CXREF) -xrefrc=.c-xrefrc single_int*.c -o output.log

clean:
	rm -rf CXrefs

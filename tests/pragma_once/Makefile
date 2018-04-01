# Automated tests for pragma once

include ../Makefile.boilerplate

$(TEST):
	@rm -f $@.log CXrefs
	@printf "Tests in \"$@\": "
	@../../src/c-xref include1.h include2.h -xrefrc .c-xrefrc -refs $(CURDIR)/CXrefs -o $@.log
	@grep Generating $@.log > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	fi
	@echo "Ok"
	@exit 0

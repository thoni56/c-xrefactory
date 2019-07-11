# Automated tests for pragma once

include ../Makefile.boilerplate

$(TEST):
	@printf "Tests in '$(TEST)': "
	@rm -rf $(TEST).log CXrefs
	@../../src/c-xref include1.h include2.h -xrefrc .c-xrefrc -refs "$(CURDIR)/CXrefs" -o $(TEST).log
	@grep Generating $(TEST).log > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	fi
	@echo "Ok"
	@exit 0

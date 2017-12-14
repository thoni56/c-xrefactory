# Automated tests for pragma once

all: pragma_once

.PHONY: pragma_once
pragma_once:
	@rm -f $@.log Xrefs
	@printf "Tests in \"$@\": "
	@../../src/c-xref include1.h include2.h -xrefrc c-xrefrc -refs $(CURDIR)/C-Xrefs -o $@.log
	@grep Generating $@.log > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	fi
	@echo "Ok"
	@exit 0

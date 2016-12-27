# Automated tests for pragma once

all: pragma_once

.PHONY: pragma_once
pragma_once:
	@rm -f log Xrefs
	@printf "Tests for \"$@\": "
	@../../src/c-xref include1.h include2.h -o log
	@grep Generating log > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi


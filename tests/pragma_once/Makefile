# Automated tests for pragma once

all: pragma_once

.PHONY: pragma_once
pragma_once:
	@../../src/c-xref include1.h include2.h -o log
	@grep Generating log > /dev/null ; \
	rm log Xrefs ; \
	if [ $$? -ne 0 ] ; then \
		echo "Tests for \"$@\": Failed" ; \
		exit 1 ; \
	else \
		echo "Tests for \"$@\": Ok" ; \
	fi


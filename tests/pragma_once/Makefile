# Automated tests for pragma once

all:
	@../../src/c-xref include1.h include2.h -o log
	@grep Generating log > /dev/null ; \
	rm log Xrefs ; \
	if [ $$? -ne 0 ] ; then \
		echo "Tests for '#pragma once': Failed" ; \
		exit 1 ; \
	else \
		echo "Tests for '#pragma once': Ok" ; \
	fi


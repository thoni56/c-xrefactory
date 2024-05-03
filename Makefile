# This is the top level Makefile for c-xrefactory
#
# It will run using the el-get install process so note that it must
# exist.
#
# All the heavy lifting is in the src directory.  There is no longer
# any way to create a "distribution" or to "make install".

ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif

all:
	$(MAKE) -C src prod

test: all
	$(MAKE) -C tests all

quick: all
	$(MAKE) -C tests quick

.PHONY: doc
doc:
	$(MAKE) -C doc

clean:
	$(MAKE) -C src clean
	$(MAKE) -C byacc-1.9 clean

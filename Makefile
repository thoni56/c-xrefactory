ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif

all:;
	sh CreateXrefDistribution

test: all
	make -C tests

install:;
	sh c-xref/c-xrefsetup

clean:;
	make -C src clean
	#make -C byacc-1.9 clean

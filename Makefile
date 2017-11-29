ifndef VERBOSE
MAKEFLAGS+=--no-print-directory
endif

all:
	make -C src

distribution:
	sh CreateXrefDistribution

test: all
	make -C tests all

quick: all
	make -C tests quick

install:
	sh c-xref/c-xrefsetup

clean:
	make -C src clean
	make -C byacc-1.9 clean

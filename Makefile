all:;
	sh CreateXrefDistribution

test: all
	cd tests; make

install:;
	sh c-xref/c-xrefsetup

clean:;
	make -C src clean
	#make -C byacc-1.9 clean

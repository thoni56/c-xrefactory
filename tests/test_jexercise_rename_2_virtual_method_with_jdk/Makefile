# Automated tests for Java exercise: completions()

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF)

$(TEST): CXrefs
	@if [ ! -d /usr/lib/jvm/java-8-openjdk-amd64 ] ; then \
		echo "Tests in '$(TEST)': $(shell tput setaf 3)No JDK available, skipping$(shell tput sgr0)" ; \
		exit 0 ; \
	else \
		if $(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp ; then \
			$(NORMALIZE) output.tmp > output ; \
			$(VERIFY) ; \
		else \
			cat output.tmp ; \
		fi ; \
	fi

CXrefs: .c-xrefrc
	@# Files in the jexercise subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/jexercise: .c-xrefrc > .c-xrefrc.tmp
	if ! grep jhome .c-xrefrc > /dev/null ; then \
		cat .c-xrefrc.tmp jdkhome.txt > .c-xrefrc ; \
	fi
	@$(CXREF) -create -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) --delay 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep -a c-xref | grep -v emacs | awk '{ print $$1 }'`

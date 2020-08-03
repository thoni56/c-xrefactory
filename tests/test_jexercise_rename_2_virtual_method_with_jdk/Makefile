# Automated tests for Java exercise: completions()

include ../Makefile.boilerplate

CXREF = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc -p $(CURDIR)
COMMAND = $(CXREF)

$(TEST): CXrefs
	@if [ ! -d /usr/lib/jvm/java-8-openjdk-amd64 ] ; then \
		echo "Tests in '$(TEST)': $(shell tput setaf 3)No JDK available, skipping$(shell tput sgr0)" ; \
		exit 0 ; \
	else \
		$(EDIT_SERVER_DRIVER) commands.input $(CURDIR) > output.tmp ; \
		$(NORMALIZE) output.tmp > output ; \
		$(VERIFY) ; \
	fi

CXrefs: .c-xrefrc
	@# Files in the jexercise subdir
	@sed -e s:$(CURDIR)$$:$(CURDIR)/jexercise: .c-xrefrc > .c-xrefrc.tmp
	@cat .c-xrefrc.tmp jdkhome.txt > .c-xrefrc
	@$(CXREF) -create -o $(TEST).log

clean:
	rm -rf CXrefs .c-xrefrc

gdb:
	@$(EDIT_SERVER_DRIVER) commands.input "$(CURDIR)" 5 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1`

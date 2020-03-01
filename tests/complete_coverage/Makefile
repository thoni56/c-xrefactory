# Automated tests for exercising completion functionality

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc single_int1.c

$(TEST): CXrefs
	@printf "Tests in '$@': "
	@rm -rf output* $@.log /tmp/c-xref*.log
	@../../utils/edit_server_driver.py completion-sequence.txt "$(CURDIR)" buffer > output.tmp
	@sed -e "s+$(CURDIR)+CURDIR+g" -e "s/len=[0-9]*/len=<n>/g" output.tmp > output
	@cmp output expected ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		diff output expected ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi
	@exit 0

CXrefs:
init:
	@$(COMMAND) -o $(TEST).log

clean:
	rm -r CXrefs .c-xrefrc

gdb:
	@../../utils/edit_server_driver.py completion-sequence.txt "$(CURDIR)" buffer 15 > output.tmp &
	sleep 1
	sudo gdb -pid `pgrep c-xref | tail -1`

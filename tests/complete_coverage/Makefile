# Automated tests for exercising completion functionality

include ../Makefile.boilerplate

BASE_COMMAND = ../../src/c-xref -xrefrc $(CURDIR)/.c-xrefrc single_int*.c

$(TEST): CXrefs
	@printf "Tests in '$@': "
	@rm -rf output* $@.log /tmp/c-xref*.log
	@python3 completion-sequence.py "$(CURDIR)" $(BASE_COMMAND) -xrefactory-II -o buffer -task_regime_server > output
	@sed -e "s+$(CURDIR)+CURDIR+g" output > output.tmp
	@cmp output.tmp expected
	@if [ $$? -ne 0 ] ; then \
		diff output.tmp expected ; \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi
	@exit 0

CXrefs:
init:
	@$(BASE_COMMAND) -o $(TEST).log

gdb:
	gdb --args $(BASE_COMMAND)

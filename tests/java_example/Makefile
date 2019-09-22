# Automated tests for parsing java (for the example)

#include ../Makefile.boilerplate

TEST = java_example

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc src -o $(TEST).log

$(TEST): .c-xrefrc
	@printf "Tests in '$@': "
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	@grep -i error $(TEST).log ; \
	if [ $$? -eq 1 ] ; then \
		echo "Ok" ; \
	else \
		echo "Failed" ; \
		exit 1 ; \
	fi
	@exit 0

gdb:
	gdb --args $(COMMAND)

.c-xrefrc: ../c-xrefrc.tpl
	@sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" ../c-xrefrc.tpl > .c-xrefrc

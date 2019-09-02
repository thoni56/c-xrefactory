# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref "-xrefactory-II" "-crlfconversion" "-crconversion" "-encoding=default" "-o" "output.log" "-task_regime_server"

$(TEST): commands.input CXrefs/XFiles
	@printf "Tests in '$@': "
	-@$(COMMAND) < commands.input > commands.output
	@sed -e "s+`expr length $$PWD/single_int1.c`>$$PWD/single_int1.c+66>CURDIR/single_int1.c+g" output.log > output.tmp
	@diff output.tmp output.expected
	@if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi
	@exit 0

gdb:
	gdb --args $(COMMAND)

commands.input: commands.tpl
	@sed -e "s+CURDIR+$(CURDIR)+g" $< > $@

CXrefs/XFiles: CXrefs/XFiles.tpl
	@sed -e "s+71:CURDIR+`expr length $$PWD/single_int1.c`:$(CURDIR)+g" $< > $@

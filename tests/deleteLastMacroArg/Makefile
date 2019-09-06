# Automated tests for checking refactoring "Delete argument"
#
# This test does not currently work, the communication between emacs and the server
# has not been sufficiently explored, the current commands does not reflect the correct
# interactions

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref "-xrefactory-II" "-crlfconversion" "-crconversion" "-encoding=default" "-o" "output.log" "-task_regime_server"

$(TEST): commands.input CXrefs/XFiles
	@printf "Tests in '$@': "
	@$(COMMAND) < commands.input > commands.output || true
	@sed -e "s+`echo $$PWD/macroArgs.c| awk '{ print length }'`>$$PWD/macroArgs.c+66>CURDIR/macroArgs.c+g" output.log > output.tmp
	@cat commands.output
	@cat output.log
	@cmp output.tmp output.expected
	@if [ $$? -ne 0 ] ; then \
		diff output.tmp output.expected ; \
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
	@sed -e "s+71:CURDIR+`echo $$PWD/macroArgs.c| awk '{ print length }'`:$(CURDIR)+g" $< > $@

clean:
	rm commands.input CXrefs/XFiles

# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref
OPTIONS = "-xrefactory-II" "-crlfconversion" "-crconversion" "-encoding=default" "-o" "output.log" "-task_regime_server"

$(TEST): commands.input CXrefs/XFiles
	@printf "Tests in '$@': "
	@$(COMMAND) $(OPTIONS) < commands.input > commands.output || true
	@sed -e "s/len=[0-9]*/len=<n>/g" -e "s#$(CURDIR)#CURDIR#g" output.log > output.tmp
	#@sed -e "s+`echo $$PWD/single_int1.c| awk '{ print length }'`>$$PWD/single_int1.c+66>CURDIR/single_int1.c+g" output.log > output.tmp
	@cmp output.tmp output.expected
	@if [ $$? -ne 0 ] ; then \
		diff output.tmp output.expected ; \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi
	@exit 0

trace: COMMAND+=-trace
trace: $(TEST)

gdb:
	gdb --args $(COMMAND) $(OPTIONS)

commands.input: commands.tpl
	@sed -e "s+CURDIR+$(CURDIR)+g" $< > $@

CXrefs/XFiles: CXrefs/XFiles.tpl
	@sed -e "s+<n>:CURDIR+`echo $$PWD/single_int1.c| awk '{ print length }'`:$(CURDIR)+g" $< > $@

clean:
	rm commands.input CXrefs/XFiles

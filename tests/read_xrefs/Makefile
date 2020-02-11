# Automated tests for checking generated c-xreferences
#
# Verification: only that each line in output exists in expected
# This is very crude

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref
OPTIONS = "-xrefactory-II" "-crlfconversion" "-crconversion" "-encoding=default" "-o" "output.log" "-task_regime_server"

$(TEST): commands.input CXrefs/XFiles
	@printf "Tests in '$@': "
	@$(COMMAND) $(OPTIONS) < commands.input > commands.output || true
	@sed -e "s/len=[0-9]*/len=<n>/g" -e "s#$(CURDIR)#CURDIR#g" output.log > output
	@while read -r line ; do \
		grep -q "$$line" output ; \
		ecode=$$? ; \
		if [ $$ecode -ne 0 ] ; then \
			echo "Failed: output did not contain expected: '$$line'" ; \
			exit 1 ; \
		fi ; \
	done < expected
	@echo "Ok"
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

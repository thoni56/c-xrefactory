# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref "-xrefactory-II" "-crlfconversion" "-crconversion" "-encoding=default" "-o" "output.tmp" "-task_regime_server"

$(TEST): commands.input
	@printf "Tests in '$@': "
	@$(COMMAND) < commands.input
	@echo "Pending"
	@exit 0

gdb:
	gdb --args $(COMMAND)

commands.input: commands.tpl
	@sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" $< > $@

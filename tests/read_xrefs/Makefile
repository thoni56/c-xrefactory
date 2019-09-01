# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref "-xrefactory-II" "-crlfconversion" "-crconversion" "-encoding=default" "-o" "output.tmp" "-task_regime_server"

$(TEST):
	@printf "Tests in '$@': "
	@$(COMMAND)
	@echo "Pending"
	@exit 0

gdb:
	gdb --args $(COMMAND)

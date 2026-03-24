include ../Makefile.server

# Scenario: A CU (external.c) exists under an -I directory but was never
# included or parsed by the project. When the user manually opens it and
# the client sends getprojectname, the server should return a mismatch
# — the file is not part of the current project's scope.
#
# Currently SUSPENDED because:
# - The simple fileIsUnderIncludePaths check accepts any file under -I dirs
# - Fixing this properly requires either:
#   (a) not adding getprojectname files to the file table (needs surgery
#       on the init flow which depends on file table registration), or
#   (b) file table support for removing/marking transient entries
# See also: test_getproject_file_under_include_path for the passing case.

# Override: place .c-xrefrc in project/ for auto-detect, not in test root
project/.c-xrefrc:
	@echo "[$(CURDIR)/project]" > $@
	@echo "  $(CURDIR)/project" >> $@
	@echo "  -I $(CURDIR)/external" >> $@

$(TEST): project/.c-xrefrc
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

clean: clean-project-config
clean-project-config:
	@-rm -rf project/.c-xrefrc project/.c-xref

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

include ../Makefile.server

# Scenario: A project includes a header from an external directory via -I.
# When the user navigates to that header and the client sends getprojectname
# for it, the server should return the current project (not a mismatch),
# because the file is reachable via the project's include paths.
#
# Note: this also accepts unknown CUs under -I dirs, which is too broad.
# See test_getproject_unknown_cu_under_include_path (.suspended) for that case.

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

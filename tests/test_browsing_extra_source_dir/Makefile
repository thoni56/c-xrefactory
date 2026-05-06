include ../Makefile.server

# Scenario: A project lists an extra source directory outside the project root.
# CUs in that directory should be discovered by the scan and parsed, so that
# navigation to symbols defined there works.
#
# project/source.c calls lib_function() defined in library/lib.c.
# The .c-xrefrc lists library/ as an additional source directory.

# Override: place .c-xrefrc in project/ for auto-detect, not in test root
project/.c-xrefrc:
	@echo "[$(CURDIR)/project]" > $@
	@echo "  $(CURDIR)/project" >> $@
	@echo "  $(CURDIR)/library" >> $@
	@echo "  -I $(CURDIR)/library" >> $@

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

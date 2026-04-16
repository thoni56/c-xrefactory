include ../Makefile.server

# Scenario: Loading a snapshot with an old version should silently
# ignore it and create a new one with the current version on exit.

$(TEST):
	@-rm -rf .c-xref
	@mkdir -p .c-xref/db
	@cp old_snapshot .c-xref/db
	@$(SERVER_DRIVER) commands.input $(EXTRA) > /dev/null
	@old_version=$$(grep 'file format' old_snapshot); \
	 new_version=$$(grep 'file format' .c-xref/db); \
	 if [ "$$old_version" = "$$new_version" ]; then \
		echo "snapshot version was NOT updated" > output; \
	 else \
		echo "snapshot version was updated" > output; \
	 fi
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

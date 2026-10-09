# The database directory is created by auto-detection, not by naming it
#
# No -p: the server discovers the project from the request file,
# and applyConventionBasedDatabasePath then puts the database, a single file,
# under .c-xref/.

include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	ls .c-xref/db >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-trace -log=debug'
trace: $(TEST)

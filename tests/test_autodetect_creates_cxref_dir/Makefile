# The database directory is created by auto-detection, not by naming it
#
# No -p and no -xrefrc: the server discovers the project from the request file,
# and applyConventionBasedDatabasePath then puts the database under .c-xref/,
# overriding the -refs path in the discovered config. -refnum=10 there is what
# makes the db a directory of ten tag files plus XFiles, which is what is listed.

include ../Makefile.server

$(TEST):
	@rm -rf .c-xref
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	ls .c-xref/db >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-trace -log=debug'
trace: $(TEST)

include ../Makefile.server

$(TEST): .c-xrefrc
	@-rm -rf .c-xref/db
	@ln -sf user@host.12345:1234567890 .#source.c
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@grep -q 'source\.c' .c-xref/db || { echo "the snapshot does not list source.c"; exit 1; }
	@grep '\.#' .c-xref/db > output || true
	$(VERIFY)

clean: clean-lockfile
clean-lockfile:
	@rm -f .#source.c

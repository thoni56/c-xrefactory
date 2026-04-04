include ../Makefile.server

$(TEST): .c-xrefrc
	@ln -sf user@host.12345:1234567890 .#source.c
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@grep '\.#' .c-xref/db/XFiles > output || true
	$(VERIFY)

clean: clean-lockfile
clean-lockfile:
	@rm -f .#source.c

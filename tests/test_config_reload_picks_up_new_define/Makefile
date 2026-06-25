include ../Makefile.server

$(TEST):
	rm -rf .c-xrefrc .c-xref
	cp c-xrefrc.tpl .c-xrefrc
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)


trace: EXTRA = --extra \'-trace\'
trace: $(TEST)

GDB_COMMANDS =

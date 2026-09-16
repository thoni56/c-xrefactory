# A %type declaration is per file, not shared between grammars
#
# Both grammars declare `%type <ast_id> common_rule`. Those declarations are
# StorageAuto, so they have to stay position-keyed even though the yacc file
# scope block is not a nesting level. If they fell back to a plain link name
# the two would become one symbol, and the menu entry would say drefn=2.
#
# The yacc symbol on the next menu line does span both files, and should.

include ../Makefile.server

$(TEST):
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-trace -log=trace'
trace: $(TEST)

GDB_COMMANDS =

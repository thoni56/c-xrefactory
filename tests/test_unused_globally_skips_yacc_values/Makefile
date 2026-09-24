# Unused Symbols Global must not report yacc semantic values
#
# A grammar with a %union makes addRuleLocalVariable() (src/yacc_parser.y)
# declare a symbol for every semantic value position in every rule - $$, $1,
# $2, ... - whether or not an action ever mentions it. They are meant to be
# local: the symbol is created with StorageAuto, and TypeDefault + StorageAuto
# is VisibilityLocal, which both the snapshot writer and the global unused
# filter skip.
#
# The base type it declares them with is `name->symbol`, whatever the grammar
# symbol's name resolves to in the symbol table. When that is a file-scope C
# symbol - here the static `term` in the prologue - completeDeclarator()
# (src/semact.c) copies its storage over the StorageAuto, and the $N comes out
# VisibilityGlobal. A static or a typedef also keeps UsageDefined, so it is
# reported; a function or an extern becomes UsageDeclared and only pollutes the
# snapshot.
#
# `unused_globally` and `term` are reported correctly - both really are unused.
# `.number` is the %union member, unrelated noise from the same operation; a
# $N-bearing grammar needs a %union, so it cannot be left out of this test.
# This test asserts only that no $-named symbol is listed.

include ../Makefile.server

$(TEST):
	@-rm -rf .c-xref/db CXrefs
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS = -ex 'br addRuleLocalVariable' -ex 'br mapAddGlobalUnusedSymbolsToHkSelection' -ex 'continue'

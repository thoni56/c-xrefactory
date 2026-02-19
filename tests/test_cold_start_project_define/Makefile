include ../Makefile.server

# Scenario: Cold start server applies -D options from .c-xrefrc
#
# Given .c-xrefrc contains -DFEATURE_ENABLED
# And main.c has a function inside #ifdef FEATURE_ENABLED
# And NO initial project scan (-create) has been run
# When the user PUSHes on feature_function (the call site)
# Then the symbol is found (proving -D was applied during cold start parsing)

$(TEST):
	cp main.c main.preload
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp | sed '/<warning/d;/<\/warning>/d' > output
	$(VERIFY)

local-clean:
	rm -f main.preload

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

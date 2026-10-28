include ../Makefile.server

# A config change is seen when a held CU's buffer closes in the same request
#
# Given a.c and b.c include h.h, and a push in a.c, held by the editor, has analysed them
# When .c-xrefrc changes
# And a push in b.c no longer holds a.c
# Then the config is reloaded, and both CUs are analysed again
# (b.c by the push itself, so one is built)
#
# The output is what the log says about the config and the builds.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer log
	@touch -t 200001010000 h.h a.c b.c .c-xrefrc
	@-$(SERVER_DRIVER) commands.input --extra '-info -log=log' > output.tmp 2>&1
	@grep -oE "Config file .*|Building .*" log | sed "s|$(CURDIR)/||g" > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

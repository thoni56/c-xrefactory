# The symbol menu's three filter levels
#
# Given LIMIT is a macro in a.h and a static variable in c.c, and a.c and b.c
#   include a.h and b.h
# When b.c and c.c are browsed, so that they are known, and then LIMIT and the
#   #include in a.c
# Then 2 lists the browsed symbol only, 1 also the other included file, since
#   every #include has the same link name, and 0 also the static variable

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@sed -n '/^<resolution>/,/^<\/resolution>/p' output.tmp | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

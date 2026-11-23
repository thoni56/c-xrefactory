# The server driver refuses a <refuse> where nothing was asked
#
# <refuse> says that the client answered No to the question in the last answer,
# and so threw that answer's edits away. A rename without conflicts asks nothing,
# so the client applies its edits, and the driver stops and says so.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer driver.err
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2> driver.err; echo "exit=$$?" >> output.tmp
	@cat driver.err >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

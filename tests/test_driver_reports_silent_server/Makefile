# The server driver reports a server that never answers
#
# A server that crashes closes its stdout, and the driver reports that. One that
# stays alive without answering would hang the driver, and with it the test run.
# silent_server reads its input and never writes, so the driver gives up after
# --answer-timeout and says so. The outer timeout only keeps a broken driver from
# hanging the suite.

include ../Makefile.server

$(TEST):
	@rm -rf output output.tmp server-buffer driver.err
	@-timeout 30 $(SERVER_DRIVER) --cxref $(CURDIR)/silent_server --answer-timeout=2 commands.input > output.tmp 2> driver.err; echo "exit=$$?" >> output.tmp
	@cat driver.err >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

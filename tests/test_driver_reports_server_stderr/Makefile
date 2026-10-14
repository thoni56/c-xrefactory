# The server driver reports a server that writes to its stderr
#
# Nothing a server writes to stderr reaches the client, so it is always a
# fault, such as a log_fatal() or a failed assert. Many tests throw the
# driver's stdout away, so the driver says it on its stderr and fails.
# noisy_server answers every request and writes one line to stderr.

include ../Makefile.server

$(TEST):
	@rm -rf output output.tmp server-buffer driver.err
	@-$(SERVER_DRIVER) --cxref $(CURDIR)/noisy_server commands.input > output.tmp 2> driver.err; echo "exit=$$?" >> output.tmp
	@cat driver.err >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

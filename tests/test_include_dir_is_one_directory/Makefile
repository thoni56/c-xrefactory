include ../Makefile.server

# An -I names one directory, as it does to a compiler. A wildcard in it is not
# expanded and a ':' does not split it into several, so both headers here are
# not found and are reported. The two forms were never used, and wildcard
# matches came in readdir() order, so which of two same-named headers won
# depended on the filesystem.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<log [^>]*>[^<]*Can't open file [a-z]*\.h" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

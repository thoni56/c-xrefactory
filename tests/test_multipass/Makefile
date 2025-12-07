include ../Makefile.boilerplate

# Test multi-pass parsing with pass-specific defines
# PASS1 should define `pass1`
# PASS2 should generate syntax errors

ARGUMENTS = -errors -create .

$(TEST): .c-xrefrc
	$(COMMAND) > output.tmp 2>&1 || true
	../../utils/cxref_reader >> output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: ARGUMENTS = -debug -log=trace -filetrace
trace: $(TEST)

include ../Makefile.server

# Running c-xref on itself to ensure it doesn't crash parsing the entire
# c-xrefactory source tree. Captures driver stdout/stderr via
# run-with-progress and greps for visible failure markers:
#   - ERROR/FATAL lines in run.stderr   (log_fatal output + the
#     server_driver.py "ERROR: target c-xref process exited..." lines
#     it emits on subprocess crashes / signal exits)
#   - <error>/<warning>/<fatalError> protocol tags in run.stdout
#     (the -xrefactory-II protocol's error records)
# Any match means failure: output contains the actual error text and
# diff against the empty expected shows it immediately.
#
# What this does NOT catch: per-CU parse errors during the bulk
# parse-all sweep — those are absorbed by ServerMode regardless of
# -errors. See memory note investigate_smoke_scope_and_error_paths.
# So this verifies "no internal asserts, no crashes, no protocol error
# tags" — not "the parser handled every CU correctly." Symbol-dump
# (STANDARD) tests are the rigorous check for the latter.
#
# Source dir and -I path come from c-xrefrc.tpl, not from the command
# line — keeps the project config in one place.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer run.stdout run.stderr
	-@../run-with-progress $(SERVER_DRIVER) commands.input
	@((grep -E "ERROR|FATAL" run.stderr 2>/dev/null; grep -E "<error|<warning|<fatalError" run.stdout 2>/dev/null) || true) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

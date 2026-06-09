include ../Makefile.server

# Generate cross-reference info for the systemd source tree and extract
# specific identifiers. Verification list is restricted to systemd-defined
# symbols (not system headers) so the test stays portable across host
# environments.
#
# Requires ~23M bytes of cxMemory.
#
# Source dir and include paths come from c-xrefrc.tpl. The rm of any
# stale systemd/.c-xrefrc before the run prevents an old XrefMode-era
# leftover from intercepting auto-detect — auto-detect should walk up
# from systemd/src/core/main.c all the way to test_systemd/.c-xrefrc.

IDENTIFIERS = \
	Manager Unit Hashmap Set Iterator UnitRef \
	safe_close parse_boolean strv_free strv_find \
	hashmap_free string_hash_func log_open log_set_target \
	ManagerState UnitActiveState LogTarget streq \
	STRV_FOREACH WHITESPACE

$(TEST):
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
		exit 0 ; \
	fi
	@rm -rf systemd/.c-xref systemd/.c-xrefrc CXrefs .c-xref output output.tmp server-buffer run.stdout run.stderr
	@../run-with-progress $(SERVER_DRIVER) commands.input || exit 1
	@../../utils/cxref_extract .c-xref/db $(IDENTIFIERS) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

expected:
	@rm -rf systemd/.c-xref systemd/.c-xrefrc CXrefs .c-xref output.tmp output run.stdout run.stderr
	@../run-with-progress $(SERVER_DRIVER) commands.input
	@../../utils/cxref_extract .c-xref/db $(IDENTIFIERS) | $(NORMALIZE) > expected

init: systemd

systemd:
	tar zxf systemd.tgz

trace: EXTRA = --extra '-debug -log=log'
trace: $(TEST)

GDB_COMMANDS =

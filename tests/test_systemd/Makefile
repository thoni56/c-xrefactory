include ../Makefile.cli

# Generate cross-reference info for systemd and extract specific identifiers
# This makes the test portable by only checking systemd-defined symbols

# It requires 23854076 (~23M) bytes of cxMemory

ARGUMENTS = -create -o systemd.log

# Portable identifiers defined in systemd sources (not system headers)
IDENTIFIERS = \
	Manager \
	Unit \
	Hashmap \
	Set \
	Iterator \
	UnitRef \
	safe_close \
	parse_boolean \
	strv_free \
	strv_find \
	hashmap_free \
	string_hash_func \
	log_open \
	log_set_target \
	ManagerState \
	UnitActiveState \
	LogTarget \
	streq \
	STRV_FOREACH \
	WHITESPACE

$(TEST):
	@if [ ! -d systemd ] ; then \
		echo "Tests in \"systemd\": not activated, run 'make init', 'make clean' to deactivate" ; \
	fi
	sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" c-xrefrc.tpl > .c-xrefrc
	../run-with-progress $(COMMAND) || exit 1
	../../utils/cxref_extract CXrefs $(IDENTIFIERS) > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

expected:
	rm -rf CXrefs systemd.log
	sed -e "s+CURDIR+$(CURDIR)+g" -e "s+TEST+$(TEST)+g" c-xrefrc.tpl > .c-xrefrc
	../run-with-progress $(COMMAND)
	../../utils/cxref_extract CXrefs $(IDENTIFIERS) > expected

trace: ARGUMENTS += -debug -log=log
trace: $(TEST)

init: systemd

systemd:
	tar zxf systemd.tgz

# Proof of the search-strips-preloaded-file fix: removal of the OP_SEARCH/SEARCH_FULL
# early-return in handleFoundSymbolReference (cxref.c). Sibling of
# test_preprocess_edit_removes_ifdef_define, but here the edit KEEPS FEATURE, so there is
# no variant-B header orphan to confound the output: every source.c reference must survive,
# at SHIFTED positions (the edit adds a blank line) proving the reparse really re-read.
#
# With the fix:   phase 2 keeps main/always_there/feature_only/include at the edited positions.
# Without the fix: a -search over the PRELOADED source.c strips its refs and never repopulates,
#                  so ALL source.c positions vanish (main disappears entirely).
include ../Makefile.server


$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@cp with_feature.preload source.c
	@$(SERVER_DRIVER) $(EXTRA1) commands_phase1.input > /dev/null
	@{ echo "=== phase 1: source.c defines FEATURE ==="; ../../utils/cxref_reader .c-xref/db; } > output
	@$(SERVER_DRIVER) $(EXTRA2) commands_phase2.input > /dev/null
	@{ echo "=== phase 2: edited (FEATURE kept), saved+closed (un-preload reparse) ==="; ../../utils/cxref_reader .c-xref/db; } >> output
	$(VERIFY)

trace: EXTRA1 = --extra '-debug -log=trace1'
trace: EXTRA2 = --extra '-debug -log=trace2'
trace: $(TEST)

GDB_COMMANDS =

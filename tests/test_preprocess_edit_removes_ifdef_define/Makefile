# Variant B, finally observable: force a from-disk reparse of the includer via the "save"
# path (un-preload), which the snapshot DOES persist -- unlike a live preload (editor
# buffer, not snapshotted) and unlike a cold-start disk change (the trace showed Pass 3
# trusts the snapshot and does NOT reparse a snapshot-known CU).
#
# closeEditorBuffersNoLongerPreloaded -> reparseFile -> reparseStaleFile does
#   removeReferenceableItemsForFile(source.c) ; parseFileWithFullInit(source.c from disk)
#
# Phase 1: source.c = with_feature, parsed from disk; snapshot holds feature_only -> precondition.
# Phase 2: fresh server loads that snapshot; source.c is PRELOADED (so it is an editor
#          buffer), then "saved" to without_feature on disk, then a request WITHOUT the
#          preload triggers the un-preload reparse from disk. Exit persists the result.
#          cxref_reader then shows whether feature_only@header was cleaned or lingered.
include ../Makefile.server


$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@cp with_feature.preload source.c
	@$(SERVER_DRIVER) $(EXTRA1) commands_phase1.input > phase1.output
	@$(SERVER_DRIVER) $(EXTRA2) commands_phase2.input > phase2.output
	@../../utils/cxref_reader .c-xref/db >output
	$(VERIFY)

trace: EXTRA1 = --extra '-debug -log=trace1'
trace: EXTRA2 = --extra '-debug -log=trace2'
trace: $(TEST)

GDB_COMMANDS =

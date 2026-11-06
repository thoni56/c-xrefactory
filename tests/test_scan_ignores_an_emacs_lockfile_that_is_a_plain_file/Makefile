include ../Makefile.server

# Scenario: an Emacs lock file that is a plain file is not taken for a compilation unit
#
# Given a.c has unsaved changes in Emacs, which marks that with a lock file .#a.c,
#   a plain file holding user@host.pid:boot-time, as Emacs makes it where it
#   cannot make symlinks, e.g. on Windows
# When the project is opened and a.c is pushed on
# Then the file table does not have .#a.c
#
# The walk takes a file by its suffix, and .#a.c ends in .c. Only the ".#" name
# check in listFilesInDirectory() stops it; the symlink form, which the symlink
# check also stops, is test_scan_ignores_emacs_lockfiles.

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@ln -sf user@host.12345:1700000000 .#a.c
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@rm -f .#a.c
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =

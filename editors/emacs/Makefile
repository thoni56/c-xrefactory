# Ensure protocol is in sync

c-xrefprotocol.el: ../../src/protocol.tc ../../src/refactorings.def c-xrefprotocol.elt
	# Convert protocol.tc into Elisp defvar's
	cpp c-xrefprotocol.elt | grep -v \# > c-xrefprotocol.el

# Ensure protocol is in sync

c-xrefprotocol.el: refactorings.def ../../src/protocol.tc c-xrefprotocol.elt
	# Convert protocol.tc into Elisp defvar's
	cpp c-xrefprotocol.elt | grep -v \# > c-xrefprotocol.el

refactorings.def: ../../src/refactorings.h
	# Convert enums in refactorings.h into #defines
	grep AVR ../../src/refactorings.h | sed -e 's/\s*\(AVR.*\) =/#define \1/' -e 's/,//' > refactorings.def

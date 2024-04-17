# Ensure protocol is in sync

c-xrefprotocol.el: ../../src/protocol.th ../../src/refactorings.def c-xrefprotocol.elt
	# Convert protocol.th into Elisp defvar's
	cpp c-xrefprotocol.elt | grep -v \# > c-xrefprotocol.el
	echo "(provide 'c-xrefprotocol)" >> c-xrefprotocol.el

compile:
	emacs --batch -l compileCxrefactory.el

clean:
	rm *.elc

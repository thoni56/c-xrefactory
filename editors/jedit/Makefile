# NOTE jedit adaptor has not been tried for many years and there are a huge
# number of changes to the c-xrefactory core that might or might not
# affect the adapter, so tread carefully. PR:s are welcome.
# For now we just ensure that the protocol bits are in sync.

Xrefactory/com/xrefactory/jedit/Protocol.java: ../../src/protocol.th ../../src/refactorings.def \
	Protocol.template \
	Makefile
	# Convert protocol.th & refactorings.def into Java declarations
	cpp Protocol.template | grep -v \# > Xrefactory/com/xrefactory/jedit/Protocol.java

# NOTE jedit adaptor has not been tried for many years and there are a huge
# number of changes to the c-xrefactory core that might or might not
# affect the adapter, so tread carefully. PR:s are welcome.
# For now we just ensure that the protocol bits are in sync.

Xrefactory/com/xrefactory/jedit/Protocol.java: ../../src/protocol.tc ../../src/refactorings.def Protocol.template \
	Makefile
	# Convert protocol.tc & refactorings.def into Java declarations
	cpp Protocol.template | grep -v \# > Protocol.tmp
	sed 's/String PPCV/int PPCV/' Protocol.tmp | sed 's/String PPC_AVR/int PPC_AVR/' > Xrefactory/com/xrefactory/jedit/Protocol.java

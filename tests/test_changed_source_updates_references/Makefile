include ../Makefile.boilerplate

$(TEST):
	-rm -rf CXrefs
	$(CXREF) -create
	$(EDIT_SERVER_DRIVER) $(SERVER_EXTRA) commands.input > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: SERVER_EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

db:
	$(CXREF) -create
	../../utils/cxref_reader

fresh:
	-rm -rf CXrefs
	$(CXREF) -create
	../../utils/cxref_reader

fastupdate:
	$(CXREF) -fastupdate -preload source.c source.preload
	../../utils/cxref_reader

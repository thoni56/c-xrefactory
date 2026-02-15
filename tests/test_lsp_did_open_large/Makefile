include ../Makefile.lsp


ARGUMENTS = -lsp

$(TEST): input.lsp
	$(COMMAND) > output.tmp < input.lsp
	$(NORMALIZE) output.tmp | tail -n +4 > output
	$(VERIFY)

trace: ARGUMENTS+=-debug -log=log
trace: $(TEST)

input.lsp: input.json
	../../utils/lsp_driver.py input.json --curdir=$(CURDIR) > input.lsp

input.json: source.c
	python3 -c "\
	import json; \
	content = open('source.c').read(); \
	messages = [ \
	  {'jsonrpc':'2.0','id':1,'method':'initialize','params':{'rootUri':'file://CURDIR','capabilities':{}}}, \
	  {'jsonrpc':'2.0','method':'initialized'}, \
	  {'jsonrpc':'2.0','method':'textDocument/didOpen','params':{'textDocument':{'uri':'file://CURDIR/source.c','languageId':'c','version':1,'text':content}}}, \
	  {'jsonrpc':'2.0','id':2,'method':'shutdown'}, \
	  {'jsonrpc':'2.0','method':'exit'} \
	]; \
	print(json.dumps(messages, indent=2))" > input.json

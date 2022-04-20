all: c-xrefactory-manual.html c-xrefactory-design.html

c-xrefactory-manual.html: c-xrefactory-manual.asciidoc
	asciidoctor c-xrefactory-manual.asciidoc

c-xrefactory-design.html: c-xrefactory-design.asciidoc
	asciidoctor -r asciidoctor-diagram c-xrefactory-design.asciidoc

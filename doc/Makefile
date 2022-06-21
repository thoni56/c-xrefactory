all: c-xrefactory-manual.html c-xrefactory-design.html

c-xrefactory-manual.html: c-xrefactory-manual.adoc
	asciidoctor c-xrefactory-manual.adoc

c-xrefactory-design.html: c-xrefactory-design.adoc
	asciidoctor -r asciidoctor-diagram c-xrefactory-design.adoc

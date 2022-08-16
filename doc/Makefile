all: manual.html design.html

manual.html: manual.adoc
	asciidoctor manual.adoc

design.html: design.adoc
	asciidoctor -r asciidoctor-diagram design.adoc

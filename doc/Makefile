all: manual.html guidebook.html

manual.html: manual.adoc
	asciidoctor manual.adoc

guidebook.html: guidebook.adoc
	asciidoctor -r asciidoctor-diagram guidebook.adoc

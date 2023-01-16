all:
	@echo "'make start' to start Structurizr, 'make html' to make html docs"

start:
	docker pull structurizr/lite
	docker run -it --rm -p 8080:8080 -v $(PWD):/usr/local/structurizr structurizr/lite

html: manual.html guidebook.html

manual.html: manual.adoc
	asciidoctor manual.adoc

guidebook.html: guidebook.adoc
	asciidoctor -r asciidoctor-diagram guidebook.adoc

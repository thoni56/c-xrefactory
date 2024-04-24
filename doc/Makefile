all: guidebook.adoc
	@echo "'make start' to start Structurizr, 'make html' to make html docs"

start:
	docker pull structurizr/lite
	docker run -it --rm -p 8080:8080 -v $(PWD):/usr/local/structurizr structurizr/lite

html: manual.html guidebook.html

manual.html: manual.adoc
	asciidoctor manual.adoc

guidebook.html: guidebook.adoc
	asciidoctor -r asciidoctor-diagram guidebook.adoc

# The guidebook is the one and only document in docs b.c. Structurizr likes it that way
# But to publicise it using GitHub action we need to concat them all into one document
guidebook.adoc: docs/*.adoc docs/*.md 00-guidebook.header
	ls 00-guidebook.header docs/*.adoc docs/*.md | sort | xargs -I {} sh -c 'cat {}; echo ""' > guidebook.adoc
	sed -i '' 's|image::embed:\(.*\)\[\]|image::\.structurizr/images/\1-thumbnail.png[]|g' guidebook.adoc

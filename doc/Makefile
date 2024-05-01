all:
	@echo "'make start' to start Structurizr"
	@echo "'make html' to make html docs in 'dist'"
	@echo "'make ci' to build documentation for GitHub Pages"

start:
	docker pull structurizr/lite
	docker run --rm -p 8080:8080 -v $(PWD):/usr/local/structurizr structurizr/lite

ci html: dist/index.html dist/manual.html dist/guidebook.html

dist/index.html: index.adoc
	asciidoctor index.adoc -D dist

dist/manual.html: manual.adoc
	asciidoctor manual.adoc -D dist

# We convert all references to Structurizr diagrams to PlantUML model references
dist/guidebook.html: exported_images guidebook.adoc
	python ../utils/replace_embeds_with_include_puml.py guidebook.adoc
	asciidoctor -r asciidoctor-diagram guidebook.adoc -D dist

# The guidebook is the one and only document in docs b.c. Structurizr likes it that way
# But to create HTML using asciidoctor we need to concat them all into one document
guidebook.adoc: docs/*.adoc 00-guidebook.header
	ls 00-guidebook.header docs/*.adoc | sort | xargs -I {} sh -c 'cat {}; echo ""' > guidebook.adoc

# And the diagrams we can export as PUML using Structurizr CLI
exported_images:
	docker pull structurizr/cli:latest
	docker run --rm -v $$PWD:/usr/local/structurizr structurizr/cli export -workspace workspace.dsl -format plantuml -output diagrams

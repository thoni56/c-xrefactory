# Makefile for c-xrefactory documentation
#
# Primary targets for daily use:
#   make view     - Generate diagrams and start Structurizr Lite viewer (default)
#   make diagrams - Generate all custom diagram images from PlantUML sources
#   make stop     - Stop the running Structurizr container
#
# CI/HTML generation targets:
#   make html     - Build static HTML documentation for GitHub Pages
#   make ci       - Alias for 'html'
#

.PHONY: all view start stop diagrams clean-diagrams html ci exported-structurizr-diagrams

# Default target
all: view

#==============================================================================
# Structurizr Lite Viewer (primary workflow)
#==============================================================================

# Start Structurizr Lite viewer (generates custom diagrams first if needed)
view: diagrams
	@echo "Starting Structurizr Lite on http://localhost:8080"
	@echo "Press Ctrl+C when done, then run 'make stop' to clean up"
	docker pull structurizr/lite
	docker run --rm -d -p 8080:8080 -v $(PWD):/usr/local/structurizr structurizr/lite
	@echo ""
	@echo "Structurizr Lite is running at http://localhost:8080"
	@echo "Run 'make stop' to stop the container"

# Alias for backward compatibility
start: view

# Stop the Structurizr container
stop:
	@echo "Stopping Structurizr Lite container..."
	@docker ps -q --filter ancestor=structurizr/lite | xargs -r docker stop
	@echo "Container stopped"

#==============================================================================
# Custom Diagram Generation
#==============================================================================

# Directory structure:
#   diagrams/src/      - Source .puml files (hand-written)
#   diagrams/*.svg     - Generated images (working directory)
#   docs/diagrams/     - SVGs copied here for Structurizr import (!docs imports from docs/ tree)
#   diagrams/structurizr-*.puml - Exported from workspace.dsl (via exported-structurizr-diagrams)

# Find all .puml source files in diagrams/src/
PUML_SOURCES := $(wildcard diagrams/src/*.puml)
# Generate corresponding .svg target paths in diagrams/
DIAGRAM_SVGS := $(patsubst diagrams/src/%.puml,diagrams/%.svg,$(PUML_SOURCES))
# And corresponding copies in docs/diagrams/
DOCS_DIAGRAM_SVGS := $(patsubst diagrams/src/%.puml,docs/diagrams/%.svg,$(PUML_SOURCES))

# Generate all custom diagrams from PlantUML sources and copy to docs/
diagrams: $(DIAGRAM_SVGS) $(DOCS_DIAGRAM_SVGS)
	@if [ -n "$(DIAGRAM_SVGS)" ]; then \
		echo "Generated $(words $(DIAGRAM_SVGS)) diagram(s) and copied to docs/diagrams/"; \
	else \
		echo "No custom diagrams found in diagrams/src/"; \
		echo "Create .puml files there to generate diagrams"; \
	fi

# Copy generated SVG to docs/diagrams/ for Structurizr import
docs/diagrams/%.svg: diagrams/%.svg
	@mkdir -p docs/diagrams
	@cp $< $@
	@echo "Copied $< to $@"

# Pattern rule: Generate SVG from PlantUML source
# Only regenerates if .puml is newer than .svg (or .svg doesn't exist)
diagrams/%.svg: diagrams/src/%.puml
	@echo "Generating $@ from $<"
	@mkdir -p diagrams
	docker run --rm \
		-v $(PWD):/work \
		-w /work \
		plantuml/plantuml \
		-tsvg -o diagrams $<

# Clean generated diagram images (keeps source .puml files)
clean-diagrams:
	@echo "Removing generated diagram images..."
	rm -f $(DIAGRAM_SVGS)
	rm -rf docs/diagrams/
	@echo "Source files in diagrams/src/ are preserved"

#==============================================================================
# Structurizr Diagram Export (for HTML generation)
#==============================================================================

# Export C4 diagrams from workspace.dsl as PlantUML (for HTML rendering)
# These go into diagrams/ as structurizr-*.puml
exported-structurizr-diagrams:
	@echo "Exporting Structurizr diagrams to PlantUML..."
	docker pull structurizr/cli:latest
	docker run --rm \
		-v $(PWD):/usr/local/structurizr \
		structurizr/cli \
		export -workspace workspace.dsl -format plantuml -output diagrams
	@echo "Exported diagrams to diagrams/structurizr-*.puml"

#==============================================================================
# Static HTML Generation (for CI/GitHub Pages)
#==============================================================================

html ci: dist/index.html dist/manual.html dist/guidebook.html dist/c-xrefactory.html
	@echo "HTML documentation built in dist/"

dist/index.html: index.adoc
	@mkdir -p dist
	asciidoctor $^ -D dist

dist/c-xrefactory.html: c-xrefactory.md
	@mkdir -p dist
	asciidoctor $^ -D dist

dist/manual.html: manual.adoc
	@mkdir -p dist
	asciidoctor $^ -D dist

# Guidebook combines all docs/*.adoc files into one HTML document
# Requires custom diagrams + exported Structurizr diagrams
dist/guidebook.html: exported-structurizr-diagrams diagrams guidebook.adoc
	@mkdir -p dist
	python ../utils/replace_embeds_with_include_puml.py guidebook.adoc
	asciidoctor -r asciidoctor-diagram guidebook.adoc -D dist

# The guidebook is built from all docs/*.adoc files
guidebook.adoc: docs/*.adoc 00-guidebook.header
	@echo "Building guidebook.adoc from docs/*.adoc..."
	@ls $^ | sort | xargs -I {} sh -c 'cat {}; echo ""' > guidebook.adoc

#==============================================================================
# Cleanup
#==============================================================================

clean: clean-diagrams
	rm -f guidebook.adoc
	rm -rf dist/

.PHONY: clean

import sys
import os

def replace_embeds_with_include_puml(adoc_filename, diagrams_path='diagrams'):
    # Read the original AsciiDoc file
    with open(adoc_filename, 'r', encoding='utf-8') as file:
        lines = file.readlines()

    # Open the same file to write modifications
    with open(adoc_filename, 'w', encoding='utf-8') as file:
        for line in lines:
            if line.startswith('image::embed:'):
                # Extract the diagram name from the embed pattern
                start_index = line.find("embed:") + 6
                end_index = line.find("[]")
                diagram_name = line[start_index:end_index].strip()

                # Create the new include line for PlantUML
                new_line = f"[plantuml, {diagram_name}, png]\n----\ninclude::{diagrams_path}/structurizr-{diagram_name}.puml[]\n----\n"
                file.write(new_line)
            else:
                file.write(line)

def main():
    if len(sys.argv) != 2:
        print("Usage: python replace_embeds_with_include_puml.py <path_to_adoc_file>")
        sys.exit(1)

    adoc_filename = sys.argv[1]
    if not os.path.exists(adoc_filename):
        print(f"Error: The file {adoc_filename} does not exist.")
        sys.exit(1)

    replace_embeds_with_include_puml(adoc_filename)

if __name__ == "__main__":
    main()

#! /bin/env python

import json
import sys

def remove_lsp_frame(data):
    """
    Removes an LSP frame if present.
    This example assumes LSP frames are identified by a specific key or structure.
    Adjust this function based on your LSP frame's structure.
    """
    if isinstance(data, dict) and 'jsonrpc' in data and 'id' in data:
        # Example: Remove an LSP frame if it has 'jsonrpc' and 'id' keys
        return data.get('result', None)  # Replace with appropriate logic
    return data

def load_and_clean_json(file_path):
    """
    Loads and cleans a JSON file by removing LSP frames if present.
    """
    try:
        with open(file_path, 'r') as file:
            data = json.load(file)
        return remove_lsp_frame(data)
    except json.JSONDecodeError as e:
        print(f"Error decoding JSON in file {file_path}: {e}")
        sys.exit(1)
    except FileNotFoundError as e:
        print(f"File not found: {file_path}")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred while loading the file {file_path}: {e}")
        sys.exit(1)

def compare_json_files(file1, file2):
    """
    Compares two JSON files for equality after removing LSP frames.
    """
    json1 = load_and_clean_json(file1)
    json2 = load_and_clean_json(file2)

    if json1 == json2:
        print("The JSON files are identical.")
    else:
        print("The JSON files are different.")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_json.py <file1.json> <file2.json>")
        sys.exit(1)

    file1 = sys.argv[1]
    file2 = sys.argv[2]
    compare_json_files(file1, file2)

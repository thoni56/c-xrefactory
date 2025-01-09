#!/usr/bin/env python

import sys
import time
import json
import argparse
import ijson

def calculate_content_length(payload):
    """Calculate the Content-Length of the JSON payload."""
    return len(payload.encode('utf-8'))

def package_lsp_frame(payload):
    """Package a JSON payload into an LSP frame."""
    content_length = calculate_content_length(payload)
    return f"Content-Length: {content_length}\r\n\r\n{payload}"

def send_requests(file_name, delay):
    """Read JSON requests from a file and send them as LSP frames."""
    try:
        with open(file_name, 'r') as file:
            # Parse JSON objects one at a time
            for obj in ijson.items(file, 'item'):
                try:
                    # Serialize object to JSON string
                    json_payload = json.dumps(obj, indent=2)
                    lsp_frame = package_lsp_frame(json_payload)
                    print(f"{lsp_frame}")
                    time.sleep(delay)
                except Exception as e:
                    print(f"Error processing JSON object: {e}", file=sys.stderr)

    except FileNotFoundError:
        print(f"File {file_name} not found.", file=sys.stderr)
    except Exception as e:
        print(f"An error occurred: {e}", file=sys.stderr)

def main():
    parser = argparse.ArgumentParser(description="Send JSON requests from a file as LSP frames.")
    parser.add_argument("file", help="File containing JSON requests (array of JSON objects).")
    parser.add_argument("-d", "--delay", type=float, default=1.0, help="Delay between sending requests (in seconds).")

    args = parser.parse_args()

    send_requests(args.file, args.delay)

if __name__ == "__main__":
    main()

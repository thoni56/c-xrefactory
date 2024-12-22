#! /bin/env python

import sys
import time
import json

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
            # Read and split requests, assuming each JSON payload is separated by newlines
            requests = file.read().strip().split("\n")

        for request in requests:
            try:
                # Ensure request is valid JSON
                json_payload = json.dumps(json.loads(request.strip()))
                lsp_frame = package_lsp_frame(json_payload)
                print(f"{lsp_frame}")
                # Here you would send `lsp_frame` to your LSP server (e.g., via socket)
                # For now, we simulate the delay
                time.sleep(delay)
            except json.JSONDecodeError:
                print(f"LSP Driver: Invalid JSON payload: {request.strip()}", file=sys.stderr)

    except FileNotFoundError:
        print(f"File {file_name} not found.", file=sys.stderr)
    except Exception as e:
        print(f"An error occurred: {e}", file=sys.stderr)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python lsp_driver.py <file_name>", file=sys.stderr)
        sys.exit(1)

    file_name = sys.argv[1]
    try:
        send_requests(file_name, 1)
    except ValueError:
        print("Invalid delay value. Must be a number.", file=sys.stderr)

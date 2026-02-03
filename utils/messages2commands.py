#! /usr/bin/env python3

# This python script can take a message log from c-xref-debug-mode and
# convert that to a usable commands.input to send to server-driver.py
# to drive the c-xrefactory server.

# It will replace any occurrence of current directory with CURDIR, so it
# is handy to be in the directory where the processed files are located.

# Use --preserve-preloads to copy tmp preload files to local .preload files
# and use them in the generated commands. This requires the tmp files to
# still exist.

import sys
import re
import os
import shutil

preserve_preloads = False
copied_preloads = {}  # Maps tmp file path to local preload filename

def get_preload_filename(source_file):
    """Generate a local preload filename from the source file path."""
    basename = os.path.basename(source_file)
    return basename + ".preload"

def process_preload(line):
    """Process preload arguments - either remove or preserve them."""
    if "-preload" not in line:
        return line

    if not preserve_preloads:
        return re.sub("\"-preload\"( \"[^\"]*\"){2} ", '', line)

    # Extract preload arguments: "-preload" "source_file" "tmp_file"
    match = re.search(r'"-preload" "([^"]*)" "([^"]*)"', line)
    if not match:
        return re.sub("\"-preload\"( \"[^\"]*\"){2} ", '', line)

    source_file = match.group(1)
    tmp_file = match.group(2)

    # Check if we've already copied this tmp file
    if tmp_file not in copied_preloads:
        preload_filename = get_preload_filename(source_file)

        # Copy tmp file to local preload file if it exists
        if os.path.exists(tmp_file):
            shutil.copy(tmp_file, preload_filename)
            copied_preloads[tmp_file] = preload_filename
            print(f"# Copied {tmp_file} to {preload_filename}", file=sys.stderr)
        else:
            print(f"# Warning: tmp file {tmp_file} not found, removing preload", file=sys.stderr)
            return re.sub("\"-preload\"( \"[^\"]*\"){2} ", '', line)

    preload_filename = copied_preloads[tmp_file]

    # Replace the preload arguments with local file references
    # Format: -preload CURDIR/source CURDIR/source.preload
    cwd = os.getcwd()
    local_source = source_file.replace(cwd, "CURDIR")
    local_preload = "CURDIR/" + preload_filename

    new_preload = f'"-preload" "{local_source}" "{local_preload}"'
    line = re.sub(r'"-preload" "[^"]*" "[^"]*"', new_preload, line)

    return line

def replace_curdir(line):
    cwd = os.getcwd()
    return line.replace(cwd, "CURDIR")

def fixup_calling(line):
    _, line = line.split("calling: (")
    line, _ = line.split(")")
    line = re.sub(" \"-o\" \"[^\"]*\"", '', line)
    line = process_preload(line)
    line = replace_curdir(line)
    print("CXREF", line)

def fixup_sending(line):
    _, line = line.split("sending: ")
    line = process_preload(line)
    line = replace_curdir(line)
    print(line[:-1])
    print("<sync>")

# Parse arguments
args = sys.argv[1:]
if "--preserve-preloads" in args:
    preserve_preloads = True
    args.remove("--preserve-preloads")

if not args:
    print("Usage: messages2commands.py [--preserve-preloads] <messages.txt>", file=sys.stderr)
    sys.exit(1)

with open(args[0], "r") as messages:
    for line in messages:
        if line.startswith("calling:"):
            fixup_calling(line)
        if line.startswith("sending:"):
            fixup_sending(line)
print("<exit>")

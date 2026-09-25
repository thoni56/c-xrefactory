#! /usr/bin/env python3

# This python script can take a message log from c-xref-debug-mode and
# convert that to a usable commands.input to send to server-driver.py
# to drive the c-xrefactory server.

# It will replace any occurrence of current directory with CURDIR, so it
# is handy to be in the directory where the processed files are located.

# Preloads are kept: each tmp preload file is copied to a local .preload file
# and the generated commands use that. This requires the tmp files to still
# exist. Use --strip-preloads to drop them instead.

import sys
import re
import os
import shutil

strip_preloads = False
copied_preloads = {}  # Maps tmp file path to local preload filename

def get_preload_filename(source_file):
    """Generate a local preload filename from the source file path."""
    basename = os.path.basename(source_file)
    return basename + ".preload"

PRELOAD = re.compile(r'"-preload" "([^"]*)" "([^"]*)" ')

def keep_preload(match):
    """Copy one preload's tmp file next to the commands and point the -preload at the copy."""
    source_file, tmp_file = match.group(1), match.group(2)
    if tmp_file not in copied_preloads:
        if not os.path.exists(tmp_file):
            print(f"# Warning: tmp file {tmp_file} not found, removing preload", file=sys.stderr)
            return ''
        preload_filename = get_preload_filename(source_file)
        shutil.copy(tmp_file, preload_filename)
        copied_preloads[tmp_file] = preload_filename
        print(f"# Copied {tmp_file} to {preload_filename}", file=sys.stderr)
    local_source = source_file.replace(os.getcwd(), "CURDIR")
    return f'"-preload" "{local_source}" "CURDIR/{copied_preloads[tmp_file]}" '

def process_preload(line):
    """Keep every -preload in the line, or strip them all."""
    if strip_preloads:
        return PRELOAD.sub('', line)
    return PRELOAD.sub(keep_preload, line)

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
if "--strip-preloads" in args:
    strip_preloads = True
    args.remove("--strip-preloads")

if not args:
    print("Usage: messages2commands.py [--strip-preloads] <messages.txt>", file=sys.stderr)
    sys.exit(1)

with open(args[0], "r") as messages:
    for line in messages:
        # Strip optional timestamp prefix: [HH:MM:SS.mmm]
        stripped = re.sub(r'^\[\d{2}:\d{2}:\d{2}\.\d{3}\] ', '', line)
        if stripped.startswith("calling:"):
            fixup_calling(stripped)
        if stripped.startswith("sending:"):
            fixup_sending(stripped)
print("<exit>")

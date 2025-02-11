#! /usr/bin/env python3

# This python script can take a message log from c-xref-debug-mode and
# convert that to a usable commands.input to send to server-driver.py
# to drive the c-xrefactory server.

# It will replace any occurrence of current directory with CURDIR, so it
# is handy to be in the directory where the processed files are located.

import sys
import re
import os

def remove_preload(line):
    if "-preload" in line:
        return re.sub("\"-preload\"( \"[^\"]*\"){2} ", '', line)
    else:
        return line

def replace_curdir(line):
    cwd = os.getcwd()
    return line.replace(cwd, "CURDIR")

def fixup_calling(line):
    _, line = line.split("calling: (")
    line, _ = line.split(")")
    line = re.sub(" \"-o\" \"[^\"]*\"", '', line)
    line = remove_preload(line)
    line = replace_curdir(line)
    print("CXREF", line)

def fixup_sending(line):
    _, line = line.split("sending: ")
    line = remove_preload(line)
    line = replace_curdir(line)
    print(line[:-1])
    print("<sync>")

with open(sys.argv[1], "r") as messages:
    for line in messages:
        if line.startswith("calling:"):
            fixup_calling(line)
        if line.startswith("sending:"):
            fixup_sending(line)
print("<exit>")

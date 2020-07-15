#! /usr/bin/env python3

import sys
import re

def remove_preload(line):
    if "-preload" in line:
        return re.sub("\"-preload\"( \"[^\"]*\"){2} ", '', line)
    else:
        return line

def fixup_calling(line):
    _, line = line.split("calling: (")
    line, _ = line.split(")")
    line = re.sub(" \"-o\" \"[^\"]*\"", '', line)
    line = remove_preload(line)
    print("../../src/c-xref", line)

def fixup_sending(line):
    _, line = line.split("sending: ")
    line = remove_preload(line)
    print(line[:-1])
    print("<sync>")

with open(sys.argv[1], "r") as messages:
    for line in messages:
        if line.startswith("calling:"):
            fixup_calling(line)
        if line.startswith("sending:"):
            fixup_sending(line)
print("<exit>")

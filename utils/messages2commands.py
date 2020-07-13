#! /usr/bin/env python3

import sys
import re

def fixup_calling(line):
    _, line = line.split("calling: (")
    line, _ = line.split(")")
    line = re.sub(" \"-o\" \"[^\"]*\"", '', line)
    print("../../src/c-xref", line)

def fixup_sending(line):
    _, line = line.split("sending: ")
    if "-preload" in line:
        line = re.sub("\"-preload\"( \"[^\"]*\"){2} ", '', line)
    print(line[:-1])
    print("<sync>")

with open(sys.argv[1], "r") as messages:
    for line in messages:
        if line.startswith("calling:"):
            fixup_calling(line)
        if line.startswith("sending:"):
            fixup_sending(line)
print("<exit>")

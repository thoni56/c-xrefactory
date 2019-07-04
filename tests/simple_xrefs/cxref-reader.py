#!/usr/bin/env python3
#
# Read information from cxref-files

# For now only verify the data for c-xrefactory test case 'simple_xrefs'

import re

# Get the fileid for "single_int.c"
with open("CXrefs/XFiles") as origin_file:
    for line in origin_file:
        match = re.findall(r'.*single_int.c', line)
        if match:
            fileid = line.split()[0]
            break
print("Fileid = %s" %(fileid))

# Read refs-line for id

identifier = "single_int_on_line_1_col_4"

import os
for file in os.listdir("CXrefs"):
    if file.startswith("X0"):
        with open(os.path.join("CXrefs", file)) as origin_file:
            for line in origin_file:
                match = re.findall(r''+identifier, line)
                if match:
                    references = line.split()[-1]
                    break
print("References = %s" %(references))

# TODO should search for the fileref somehow...

# Split references into separate substrings
fileref = references[3:9]

# TODO need to search for the 'l'
lineref = references[9:11]

# TODO need to search for the 'c'
colref = references[11:13]

reftype = references[13:14]

print("fileref = %s" % fileref)
print("lineref = %s" % lineref)
print("colref = %s" % colref)
print("reftype = %s" % reftype)

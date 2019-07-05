#!/usr/bin/env python3
#
# Read information from cxref-files

# For now only verify the data for c-xrefactory test case 'simple_xrefs'

import re

# Get the fileid for "single_int.c"
with open("CXrefs/XFiles") as origin_file:
    for line in origin_file:
        match = re.findall(r'.*single_int1.c', line)
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

# Search for the fileref
pos = references.find('uA') # start of fileref
pos = pos+len('uA')

# Split references into separate substrings: fileref
length = references[pos:].find('f')
fileref = references[pos:pos+length]
print("fileref = %s" % fileref)

pos = pos + len(fileref) + 1

while pos < len(references):
    # Search for the 'l'
    length = references[pos:].find('l')
    lineref = references[pos:pos+length]
    print("lineref = %s" % lineref)

    pos = pos + len(lineref) + 1

    # Search for the 'c'
    length = references[pos:].find('c')
    colref = references[pos:pos+length]
    print("colref = %s" % colref)

    pos = pos + len(colref) + 1

    # Search for reftype? Or what is it?
    length = references[pos:].find('r')
    reftype = references[pos:pos+length+1]
    print("reftype? = %s" % reftype)

    pos = pos + len(reftype)


    "4uA 20900f 1l 4c r 4l c r 32710f 1l 4c r 4l c r 48151f 1l 4c r 4l c r"

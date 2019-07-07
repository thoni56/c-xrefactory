#!/usr/bin/env python3
#
# Read information from cxref-files

# For now only verify the data for c-xrefactory test case 'simple_xrefs'

import os
import re
import sys
from collections import namedtuple

# Create the reference structure
Reference = namedtuple('Reference', ['file', 'line', 'column'])

def unpack_refs(string, file=None, line=None, column=None):
    if string == None or string == "":
        return None

    f = re.match("(\d+)f", string)
    if not f == None:
        file = int(string[f.start():f.end()-1])
        string = string[f.end():]

    f = re.match("(\d+)l", string)
    if not f == None:
        line = int(string[f.start():f.end()-1])
        string = string[f.end():]

    f = re.match("(\d+)c", string)
    if not f == None:
        column = int(string[f.start():f.end()-1])

    return Reference(file, line, column)

if __name__ == "__main__":

    identifier = "/single_int_on_line_1_col_4"
    references_string = None

    for file in os.listdir("CXrefs"):
        if file.startswith("X0"):
            with open(os.path.join("CXrefs", file)) as origin_file:
                for line in origin_file:
                    match = re.findall(r''+identifier, line)
                    if match:
                        references_string = line.split()[-1]
                        break
                else:
                    continue
    if not references_string:
        print("Error: identifier '%s' not found" % identifier)
        sys.exit(1)
    print("references_string = %s" %(references_string))

    # Search for the fileref
    pos = references_string.find('uA') # start of fileref
    pos = pos+len('uA')

    references = unpack_refs(references_string[pos:])
    print(references)

    "4uA 20900f 1l 4c r 4l c r 32710f 1l 4c r 4l c r 48151f 1l 4c r 4l c r"

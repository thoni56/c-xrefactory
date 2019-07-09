#!/usr/bin/env python3
#
# Read information from cxref-files

# For now only verify the data for c-xrefactory test case 'simple_xrefs'

import os
import re
import sys
from collections import namedtuple

# Create the reference structure
Reference = namedtuple('Reference', ['fileid', 'lineno', 'colno'])

# Unpack a reference string into a list of References(fileid, lineno, colno)


def unpack_refs(string, fileid=None, lineno=None, colno=None):
    refs = []
    while string != "":
        f = re.match(r"(\d+)f", string)
        if not f == None:
            fileid = int(string[f.start():f.end()-1])
            string = string[f.end():]

        f = re.match(r"(\d+)l", string)
        if not f == None:
            lineno = int(string[f.start():f.end()-1])
            string = string[f.end():]

        f = re.match(r"(\d+)c", string)
        if not f == None:
            colno = int(string[f.start():f.end()-1])
            string = string[f.end():]

        string = string[1:]  # For now, skip 'r' - reference?
        refs.append(Reference(fileid, lineno, colno))

    return refs


# Create a File structure
FileReference = namedtuple(
    'FileReference', ['fileid', 'update', 'access', 'filename'])

# Takes array of lines


def unpack_files(lines):
    if lines == None or len(lines) == 0:
        return []
    filerefs = []
    for line in lines:
        segments = line.split(' ')
        if len(segments) > 0 and segments[0] != '':
            if segments[0][-1] == 'f':
                filerefs.append(FileReference(int(segments[0][:-1]),  # Remove trailing 'f'
                                              segments[1],
                                              segments[2],
                                              segments[3].split(':', 1)[-1]))
    return filerefs

# Return FileReference or None if no or multiple matches


def get_filename_from_id(fileid, file_references):
    filerefs = [
        fileref for fileref in file_references if fileref.fileid == fileid]
    return filerefs[0].filename if len(filerefs) == 1 else None


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

    # Search for the fileref
    pos = references_string.find('uA')  # Don't know what this is yet...
    pos = pos+len('uA')

    references = unpack_refs(references_string[pos:])
    with open("CXrefs/XFiles") as file:
        lines = [line.rstrip('\n') for line in file]
    files = unpack_files(lines)
    for r in references:
        filename = get_filename_from_id(r.fileid, files)
        filename = os.path.basename(filename)
        print("%s:%d:%d" % (filename, r.lineno, r.colno))

    "4uA 20900f 1l 4c r 4l c r 32710f 1l 4c r 4l c r 48151f 1l 4c r 4l c r"

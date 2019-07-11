#!/usr/bin/env python3
#
# Read information from cxref-files

# For now only verify the data for c-xrefactory test case 'simple_xrefs'

import os
import re
import sys
from collections import namedtuple

# Create the Position structure
SymbolPosition = namedtuple(
    'SymbolPosition', ['fileid', 'lineno', 'colno', 'position_string', 'complete_string'])


def unpack_positions(string, fileid=None, lineno=None, colno=None):
    # Unpack a reference string into a list of SymbolPositions
    refs = []
    # What is "4uA" starting many position strings?
    complete_string = string
    if string.startswith("4uA"):
        string = string[len("4uA"):]
    while string != "":
        position_string = string
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
        refs.append(SymbolPosition(fileid, lineno, colno,
                                   position_string[:len(
                                       position_string)-len(string)],
                                   complete_string))

    return refs


# Create a File structure
FileReference = namedtuple(
    'FileReference', ['fileid', 'update', 'access', 'filename'])


def unpack_files(lines):
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


def get_filename_from_id(fileid, file_references):
    # Return FileReference or None if no or multiple matches
    filerefs = [
        fileref for fileref in file_references if fileref.fileid == fileid]
    return filerefs[0].filename if len(filerefs) == 1 else None


# Create Symbol structure
Symbol = namedtuple('Symbol', ['symbolname', 'positions'])


def unpack_symbols(lines):
    symbols = []
    for line in lines:
        if line != "" and line[0] == 't':
            segments = line.split('\t')
            symbolname = segments[1].split('/', 1)[-1]
            symbols.append(Symbol(symbolname, segments[2]))
    return symbols


if __name__ == "__main__":

    if len(sys.argv) > 2:
        print(
            "ERROR: %s only takes 1 argument, the directory to scan, it defaults to 'CXrefs'" % sys.argv[0])
        sys.exit()

    if len(sys.argv) == 2:
        directory_name = sys.argv[1]
    else:
        directory_name = "CXrefs"

    if not os.path.exists(directory_name):
        print("ERROR: directory '%s' does not exist, point to a c-xref index directory" %
              directory_name)
        sys.exit()

    if not os.path.isdir(directory_name):
        print("ERROR: '%s' is not a directory, should be a c-xref index directory" %
              directory_name)
        sys.exit()

    if not os.path.exists(os.path.join(directory_name, "XFiles")):
        print("ERROR: '%s' does not contain an 'XFiles' file" % directory_name)
        sys.exit()

    # Get all file references
    with open(os.path.join(directory_name, "XFiles")) as filename:
        lines = [line.rstrip('\n') for line in filename]
    files = unpack_files(lines)

    # Read all CXref-files and list identifiers
    for filename in os.listdir(directory_name):
        if filename != "XFiles":
            with open(os.path.join(directory_name, filename)) as origin_file:
                symbols = unpack_symbols(origin_file.readlines())
                for symbol in symbols:
                    print(symbol.symbolname)
                    positions = unpack_positions(symbol.positions)
                    for p in positions:
                        filename = get_filename_from_id(p.fileid, files)
                        filename = os.path.basename(filename)
                        print("    %s:%d:%d" % (filename, p.lineno, p.colno))

    "4uA 20900f 1l 4c r 4l c r 32710f 1l 4c r 4l c r 48151f 1l 4c r 4l c r"

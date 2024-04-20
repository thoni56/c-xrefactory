#!/usr/bin/env python3
#
# Read information from cxref-files

import os
import re
import sys
import argparse

from collections import namedtuple


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def vprint(*args, **kwargs):
    if verbose:
        print(*args, **kwargs)


# Create the Position structure
SymbolPosition = namedtuple(
    'SymbolPosition', ['fileid', 'lineno', 'colno', 'position_string', 'complete_string'])


# Somewhere to save previously found fileid, lineno and colno
fileid = None
lineno = None
colno = None

# Make a dict instead
marker_value = {}


def read_header(lines):
    return lines[6:]


def read_marker(marker, string):
    global marker_value
    rest = string
    f = re.match(r"(\d*)"+marker, string)
    if f == None:
        value = marker_value[marker]
    else:
        if string[f.start():f.end()-1] != '':
            value = int(string[f.start():f.end()-1])
        else:
            value = 0
        rest = string[f.end():]
    marker_value[marker] = value
    return (value, rest)


def unpack_references(string):
    # Unpack a reference string into a list of SymbolPositions or something
    # All "segments" have a format of <int><marker>, e.g. 456f
    # If a segment does not have number it means 0 (zero)
    # If the segment is missing it means that it is the same as the last
    global fileid
    global lineno
    global colno

    refs = []
    complete_string = string

    while string != "":
        position_string = string

        # Usage marker
        (usage, string) = read_marker('u', string)

        # Required Access marker
        (accessibility, string) = read_marker('A', string)

        # Symbol Index marker
        #(symbol_index, string) = read_marker('s', string)

        # File marker
        (fileid, string) = read_marker('f', string)

        # Line marker
        (lineno, string) = read_marker('l', string)

        # Column marker
        (colno, string) = read_marker('c', string)

        if string[0] == 'r':
            reference = True
        else:
            eprint("Expected 'r' marker: '%s'" % string)

        string = string[1:]  # For now, skip 'r' - reference?

        if fileid is None or lineno is None or colno is None:
            eprint("ERROR: incomplete position, string is: '%s'" %
                   position_string)
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
        if len(line) > 0:
            segments = line.split(' ')
            if len(segments) > 0 and segments[0] != '':
                if segments[0][-1] == 'f':
                    if segments[1] != '' and segments[1][-1] == 'o':
                        # Only two segments, second is Java source index(?)
                        pass  # for now
                    else:
                        # Remove trailing 'f' and turn fileid into an int
                        filerefs.append(FileReference(int(segments[0][:-1]),
                                                      segments[1],
                                                      segments[2],
                                                      segments[3].split(':', 1)[-1]))
                else:
                    eprint("Unknown line in XFiles: '%s'" % line)
    return filerefs


def get_filename_from_id(fileid, file_references):
    # Return FileReference or None if no or multiple matches
    filerefs = [
        fileref for fileref in file_references if fileref.fileid == fileid]
    return filerefs[0].filename if len(filerefs) == 1 else None


# Create Symbol structure
Symbol = namedtuple('Symbol', ['symbolname', 'references', 'kind'])


def unpack_symbols(lines, cxfilename):
    symbols = []
    marker = ""
    for line in lines:
        if line != "":
            segments = line.split('\t')
            classification = segments[0]
            (t, classification) = read_marker('t', classification)
            (d, classification) = read_marker('d', classification)
            (h, classification) = read_marker('h', classification)
            (a, classification) = read_marker('a', classification)
            (g, classification) = read_marker('g', classification)
            symbolname = segments[1].split('/', 1)[-1]
            symbols.append(
                Symbol(symbolname, unpack_references(segments[2]), marker))
    return symbols


def read_lines_from(directory_name, file_name):
    with open(os.path.join(directory_name, file_name)) as file:
        lines = file.read().splitlines()
    return lines


def verify_directory(directory_name):
    if not os.path.exists(directory_name):
        eprint("ERROR: directory '%s' does not exist, point to a c-xref index directory" %
               directory_name)
        sys.exit()

    if not os.path.isdir(directory_name):
        eprint("ERROR: '%s' is not a directory, should be a c-xref index directory" %
               directory_name)
        sys.exit()

    if not os.path.exists(os.path.join(directory_name, "XFiles")):
        eprint("ERROR: '%s' does not contain an 'XFiles' file" % directory_name)
        sys.exit()


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description='Read the CXrefs of c-xrefactory and present them in readable format.')
    parser.add_argument('-v', '--verbose',
                        help='verbose output', action='store_true')
    parser.add_argument(
        'directory', help='the directory to scan, default is CXrefs', nargs='?', default='CXrefs')

    args = parser.parse_args()

    directory_name = args.directory
    if args.verbose:
        verbose = True
    else:
        verbose = False

    verify_directory(directory_name)

    # Get all file references
    vprint("Unpacking", "XFiles")
    lines = read_lines_from(directory_name, "XFiles")
    lines = lines[6:]  # Skip header
    files = unpack_files(lines)

    symbols = []
    # Read all CXref-files and list identifiers
    for cxfilename in sorted(os.listdir(directory_name)):
        if cxfilename != "XFiles" :
            vprint("Unpacking", cxfilename)
            lines = read_lines_from(directory_name, cxfilename)
            lines = read_header(lines)
            symbols += unpack_symbols(lines, cxfilename)

    symbols.sort(key=lambda s: s.symbolname)
    for symbol in symbols:
        print(symbol.symbolname)
        for p in symbol.references:
            # Save fileid, lineno and colno in case any of them are skipped in next symbol in this file
            filename = get_filename_from_id(p.fileid, files)
            filename = os.path.basename(filename)
            print("    %s@%s:%d:%d" %
                  (symbol.symbolname, filename, p.lineno if p.lineno else 0, p.colno if p.colno else 0))

#!/usr/bin/env python3
#
# Read information from cxref-files

import os
import re
import sys
from collections import namedtuple


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


# Create the Position structure
SymbolPosition = namedtuple(
    'SymbolPosition', ['fileid', 'lineno', 'colno', 'position_string', 'complete_string'])


# Somewhere to save previously found fileid, lineno and colno
fileid = None
lineno = None
colno = None

# Make a dict instead
marker_value = {}

def read_marker(marker, string):
    f = re.match(r"(\d*)"+marker, string)
    if f == None:
        value = marker_value[marker]
    else:
        if string[f.start():f.end()-1] != '':
            value = int(string[f.start():f.end()-1])
        else:
            value = 0
        return (value, string[f.end():])
    return (value, string)

def unpack_positions(string):
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
        f = re.match(r"(\d+)u", string)
        if not f == None:
            usage = int(string[f.start():f.end()-1])
            string = string[f.end():]

        f = re.match(r"(\d+)A", string)
        if not f == None:
            if string[f.start():f.end()-1] != '':
                accessibility_index = int(string[f.start():f.end()-1])
            string = string[f.end():]
        if string[0] == 'A':
            accessibility_index = 0
            string = string[1:]

        # File marker
        f = re.match(r"(\d+)f", string)
        if not f == None:
            fileid = int(string[f.start():f.end()-1])
            string = string[f.end():]
        if string[0] == 'f':
            fileid = 0
            string = string[1:]

        # Line marker
        f = re.match(r"(\d+)l", string)
        if not f == None:
            lineno = int(string[f.start():f.end()-1])
            string = string[f.end():]
        if string[0] == 'l':
            lineno = 0
            string = string[1:]

        # Column marker
        f = re.match(r"(\d+)c", string)
        if not f == None:
            colno = int(string[f.start():f.end()-1])
            string = string[f.end():]
        if string[0] == 'c':
            colno = 0
            string = string[1:]

        if string[0] == 'r':
            reference = True
        else:
            print("Unknown marker(?): '%s'" % string)
            exit(1)

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
                elif segments[0][-1] == 'v':
                    # Version string
                    pass
                elif segments[0][0:3] == '21@':
                    # Marker list (only the marker characters as a string)
                    pass
                else:
                    eprint("Unknown line in XFiles: '%s'" % line)
    return filerefs


def get_filename_from_id(fileid, file_references):
    # Return FileReference or None if no or multiple matches
    filerefs = [
        fileref for fileref in file_references if fileref.fileid == fileid]
    return filerefs[0].filename if len(filerefs) == 1 else None


# Create Symbol structure
Symbol = namedtuple('Symbol', ['symbolname', 'positions', 'kind'])


def unpack_symbols(lines, cxfilename):
    symbols = []
    marker = ""
    for line in lines:
        if line != "":
            if line[0] != '\t':
                # If there is a marker, dechiffer it, else use the previously found one
                marker = re.match(r"(\d*)\D", line).group()
            if marker[-1] == 'v':
                # Version string
                pass
            elif marker[-1] == '@':
                # "Single Records" - all marker characters in the order specified in cxfile.c (generatedSingleRecordMarkers)
                pass
            elif marker[-1] == 't':
                # Symbol type
                segments = line.split('\t')
                symbolname = segments[1].split('/', 1)[-1]
                symbols.append(Symbol(symbolname, segments[2], marker))
            elif marker[-1] == 'g':
                # Storage
                segments = line.split('\t')
                symbolname = segments[1].split('/', 1)[-1]
                symbols.append(Symbol(symbolname, segments[2], marker))
            else:
                print("Unknown marker '%s' in Xrefs file '%s': '%s'" %
                      (marker[-1], cxfilename, line))
    return symbols


def read_lines_from(directory_name, file_name):
    with open(os.path.join(directory_name, file_name)) as file:
        lines = file.read().splitlines()
    return lines


def verify_directory(directory_name):
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


if __name__ == "__main__":

    if len(sys.argv) > 2:
        print(
            "ERROR: %s only takes 1 argument, the directory to scan, it defaults to 'CXrefs'" % sys.argv[0])
        sys.exit()

    if len(sys.argv) == 2:
        directory_name = sys.argv[1]
    else:
        directory_name = "CXrefs"

    verify_directory(directory_name)

    # Get all file references
    lines = read_lines_from(directory_name, "XFiles")
    files = unpack_files(lines)

    # Read all CXref-files and list identifiers
    for cxfilename in os.listdir(directory_name):
        if cxfilename != "XFiles":
            with open(os.path.join(directory_name, cxfilename)) as origin_file:
                lines = read_lines_from(
                    directory_name, cxfilename)
                # Skip file head, 6 lines
                lines = lines[6:]
                symbols = unpack_symbols(lines, cxfilename)
                for symbol in symbols:
                    print(symbol.symbolname)
                    # Use previously found fileid, lineno and colno
                    positions = unpack_positions(symbol.positions)
                    for p in positions:
                        # Save fileid, lineno and colno in case any of them are skipped in next symbol in this file
                        fileid = p.fileid
                        lineno = p.lineno
                        colno = p.colno
                        filename = get_filename_from_id(p.fileid, files)
                        filename = os.path.basename(filename)
                        print("    %s@%s:%d:%d" %
                              (symbol.symbolname, filename, p.lineno if p.lineno else 0, p.colno if p.colno else 0))

    "4uA 20900f 1l 4c r 4l c r 32710f 1l 4c r 4l c r 48151f 1l 4c r 4l c r"

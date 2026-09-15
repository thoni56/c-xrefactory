# Parse the c-xrefactory cross-reference database, .c-xref/db
#
# Shared by cxref_reader

import os
import re
import sys

from collections import namedtuple


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    exit(-1)


# Create the Position structure
SymbolPosition = namedtuple(
    'SymbolPosition', ['fileid', 'lineno', 'colno', 'position_string', 'complete_string'])


# Somewhere to save previously found fileid, lineno and colno
fileid = None
lineno = None
colno = None

# Make a dict for last values where keys are the record keys, all zero'ed out initially
marker_value = dict.fromkeys('uAsflcrpmiao', 0)

legacy_format = None

# The file references, set by the caller before unpack_symbols() since
# include references and local identifiers are resolved through them
files = []

def skip_header(lines):
    global legacy_format
    version_line = lines[2].strip()
    if not version_line.startswith("34v file format: C-xrefactory"):
        raise ValueError(f"Error in file version line: {version_line}")

    try:
        version = version_line.split()[-1]
        legacy_format = version == "1.6.0"
    except IndexError:
        raise ValueError("Could not extract version marking from header.")
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


def unpack_xfiles(lines):
    filerefs = []
    for linenumber, line in enumerate(lines):
        if len(line) > 0:
            segments = line.split(' ')
            if len(segments) > 0 and segments[0] != '':
                if segments[0][-1] == 'f':
                    try:
                        if segments[1] != '' and segments[1][-1] == 'o':
                            # Only two segments, second is Java source index(?)
                            pass  # for now
                        else:
                            # Remove trailing 'f' and turn fileid into an int
                            # Writer stores strlen+1, so subtract 1.  Use length
                            # to bound extraction (same issue as unpack_file_lines).
                            name_field = segments[3].split(':', 1)
                            name_length = int(name_field[0]) - 1
                            filename = name_field[1][:name_length]
                            filerefs.append(FileReference(int(segments[0][:-1]),
                                                          segments[1],
                                                          segments[2],
                                                          filename))
                    except IndexError:
                        eprint("Error in XFiles line %d: '%s'" % (linenumber+6, line))
                else:
                    eprint("Unknown line in XFiles line %d: '%s'" % (linenumber+6, line))
    return filerefs

def unpack_file_lines(lines):
    filerefs = []
    for index, line in enumerate(lines):
        if len(line) > 0:
            segments = line.split(' ')
            if len(segments) > 0 and segments[0] != '':
                if segments[0][-1] == 'f':
                    if segments[1] != '' and segments[1][-1] == 'o':
                        # Only two segments, second is Java source index(?)
                        pass  # for now
                    else:
                        # Remove trailing 'f' and turn fileid into an int
                        # Writer stores strlen+1, so subtract 1.  Use length
                        # to bound extraction since the compact format may
                        # append marker chars after the name without separator.
                        name_field = segments[3].split(':', 1)
                        name_length = int(name_field[0]) - 1
                        filename = name_field[1][:name_length]
                        filerefs.append(FileReference(int(segments[0][:-1]),
                                                      segments[1],
                                                      segments[2],
                                                      filename))
                else:
                    return (filerefs, lines[index:])
    return (filerefs, [])


def get_filename_from_id(fileid, file_references):
    filerefs = [
        fileref for fileref in file_references if fileref.fileid == fileid]
    return filerefs[0].filename if len(filerefs) == 1 else f"<unknown-file:{fileid}>"

def get_relative_filename_from_id(fileid, file_references):
    filename = get_filename_from_id(fileid, file_references)
    cwd = os.getcwd()

    if filename.startswith(cwd + os.sep):
        # Inside project, return relative path
        return filename.replace(cwd + os.sep, "", 1)
    else:
        # System file, return normalized `/.../<filename>`
        return f"/.../{os.path.basename(filename)}"

def is_valid_hex(s):
    try:
        int(s, 16)
        return True
    except ValueError:
        return False

def convert_local_identifier(identifier):
    source, name = identifier.split('!', 1)

    parts = source.split('-')
    if len(parts) != 3:
        return identifier

    file_hex, line_hex, col_hex = parts

    if not all(is_valid_hex(part) for part in [file_hex, line_hex, col_hex]):
        return identifier

    file_id = int(file_hex, 16)
    line = int(line_hex, 16)
    col = int(col_hex, 16)

    relative_filename = get_relative_filename_from_id(file_id, files)

    return f"{relative_filename}-{line}-{col}!{name}"

# Create Symbol structure
Symbol = namedtuple('Symbol', ['symbolname', 'references', 'kind'])

def unpack_symbols(lines):
    symbols = []
    marker = ""
    for line in lines:
        if line != "":
            segments = line.split('\t')
            classification = segments[0]
            (t, classification) = read_marker('t', classification)
            (d, classification) = read_marker('d', classification)
            if legacy_format :
                (h, classification) = read_marker('h', classification)
                (a, classification) = read_marker('a', classification)
            (g, classification) = read_marker('g', classification)
            symbolname = segments[1].split('/', 1)[-1]
            if symbolname == '%%%i': # An include reference
                filename = get_relative_filename_from_id(d, files)
                symbolname = symbolname+':'+filename
            elif '!' in symbolname:
                symbolname = convert_local_identifier(symbolname)
            symbols.append(
                Symbol(symbolname, unpack_references(segments[2]), marker))
    return symbols


def read_lines_from(directory_name, file_name):
    with open(os.path.join(directory_name, file_name)) as file:
        lines = file.read().splitlines()
    return lines

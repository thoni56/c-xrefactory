#!/usr/bin/env python3
#
# Create a representation of the struct lines on standard input
# in a DOT graph description.
#
# Input should only contain lines from a `grep struct *.h` kind of
# command.
#
# Output will be something like
#
#     digraph {
#         ident -> { symbol symbolList typeModifiers }
#     }

import sys

in_struct = False

print("digraph {")
for line in sys.stdin:
    line = line.rstrip()
    if line.startswith('typedef'):
        if in_struct:
            print(" }")
        (_, _, type_name, rest) = line.split(' ', maxsplit=3)
        print("  {0} -> {{".format(type_name), end="")
        in_struct = True
    else:
        if line != '' and in_struct:
            try:
                (struct, type_name, rest) = line.split(maxsplit=2)
                if struct == "struct":
                    print(" {}".format(type_name), end="")
            except Exception as e:
                print(
                    "**** Line '{}' could not be split in (struct, type_name, rest)".format(line))
                print(e)
if in_struct:
    print(" }")
print("}")

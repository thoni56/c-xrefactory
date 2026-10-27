#!/usr/bin/env python3
#
# Print each option the Emacs client sends that the server's option parser
# does not accept. No output means they are in sync.
#
# Usage: client_options_known.py <options.c> <ppc.c> <elisp file>...
#
# The client's options are the string literals in the elisp that start with
# "-" and a letter, outside comments and string-match patterns. The server's
# are the literals that options.c compares argv[i] with, strcmp() for an exact
# name and strncmp() for one that takes a value glued on, and the answers to a
# question that ppc.c compares a line with.

import re
import sys

options_c = sys.argv[1]
answers_c = sys.argv[2]
elisp_files = sys.argv[3:]

with open(options_c) as f:
    parser = f.read()
with open(answers_c) as f:
    answers = f.read()

exact = set(re.findall(r'strcmp\(\s*args\.argv\[i\]\s*,\s*"(-[^"]+)"\s*\)', parser))
prefixes = set(re.findall(r'strncmp\(\s*args\.argv\[i\]\s*,\s*"(-[^"]+)"', parser))
exact |= set(re.findall(r'strcmp\(\s*line\s*,\s*"(-[^"]+)"\s*\)', answers))
# main.c dispatches these before the options are processed
exact |= {"-server", "-lsp", "-help", "-about"}

def accepted(option):
    if option in exact:
        return True
    return any(option.startswith(prefix) for prefix in prefixes)

sent = set()
for name in elisp_files:
    with open(name) as f:
        for line in f:
            if line.lstrip().startswith(";"):
                continue
            code = re.sub(r'\(string-match\s+"[^"]*"', "", line)
            for literal in re.findall(r'"(-[a-z][a-zA-Z0-9-]*=?)', code):
                sent.add(literal)

for option in sorted(sent):
    if not accepted(option):
        print(option)

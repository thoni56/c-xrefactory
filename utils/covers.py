#! /usr/bin/env python3

import sys
import argparse
import glob
import os
import shutil

parser = argparse.ArgumentParser(description="Check which test(s) cover some particular lines")
parser.add_argument("file", help="the name of a source file that should have coverage")
parser.add_argument("line", help="the line that should be covered")

args = parser.parse_args()

dirs = map(os.path.dirname, glob.glob("../tests/*/.coverage"))
basename = args.file.rsplit( ".", 1 )[ 0 ]
linenumber = ":{0:>5}:".format(int(args.line))

for d in dirs:
    f = os.path.join(d, ".coverage", basename+".gcda")
    shutil.copy(f, os.path.join(os.getcwd(), ".objects"))
    outputStream = os.popen("gcov .objects/"+basename+".o")
    outputStream = os.popen("grep '"+linenumber+"' "+basename+".c.gcov");
    parts = outputStream.readline().split(':')
    try:
        int(parts[0])
        print(d)
    except ValueError:
        pass
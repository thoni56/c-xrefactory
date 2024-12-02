#! /usr/bin/env python3

import argparse
import glob
import os
import shutil
import subprocess

parser = argparse.ArgumentParser(description="Check which test(s) cover some particular lines")
parser.add_argument("file", help="the name of a source file that should have coverage")
parser.add_argument("line", help="the line that should be covered")

args = parser.parse_args()

dirs = map(os.path.dirname, glob.glob("../tests/*/.cov"))
basename = args.file.rsplit( ".", 1 )[ 0 ]
linenumber_match = ":{0:>5}:".format(int(args.line))

shutil.copy(os.path.join(".objects", basename+".gcda"), "/tmp")
for d in dirs:
    try:
        f = os.path.join(d, ".cov", basename+".gcda")
        shutil.copy(f, os.path.join(os.getcwd(), ".objects"))
    except FileNotFoundError:
        continue

    result = subprocess.run(["gcov", ".objects/"+basename+".o"], capture_output=True, text=True)
    result = subprocess.run(["grep", linenumber_match, basename+".c.gcov"], capture_output=True, text=True)
    line = result.stdout
    if line != None:
        parts = line.split(':')
        try:
            int(parts[0])
            print(d)
        except ValueError:
            pass
shutil.copy(os.path.join("/tmp", basename+".gcda"), ".objects")
subprocess.run(["gcov", ".objects/"+basename+".o"], capture_output=True, text=True)

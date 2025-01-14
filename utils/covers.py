#! /usr/bin/env python3

import argparse
import glob
import os
import shutil
import subprocess

def find_project_root():
    # Hitta projektets rot genom att leta efter de specifika mapparna
    current_dir = os.getcwd()
    while current_dir != os.path.dirname(current_dir):  # Stoppa vid filsystemets rot
        if all(os.path.isdir(os.path.join(current_dir, d)) for d in ["utils", "src", "tests"]):
            return current_dir
        current_dir = os.path.dirname(current_dir)
    raise FileNotFoundError("Kunde inte hitta projektets rot.")

# Hitta projektroten och definiera viktiga kataloger
project_root = find_project_root()
src_dir = os.path.join(project_root, "src")
objects_dir = os.path.join(src_dir, ".objects")
tests_dir = os.path.join(project_root, "tests")
tmp_dir = "/tmp"

parser = argparse.ArgumentParser(description="Check which test(s) cover some particular lines")
parser.add_argument("file", help="the name of a source file that should have coverage")
parser.add_argument("line", help="the line that should be covered")

args = parser.parse_args()

# Hitta sökvägar till `.cov`-kataloger
dirs = map(os.path.dirname, glob.glob(os.path.join(tests_dir, "*", ".cov")))
basename = args.file.rsplit(".", 1)[0]
linenumber_match = ":{0:>5}:".format(int(args.line))

# Kopiera den ursprungliga `.gcda`-filen till /tmp för säkerhetskopiering
original_gcda = os.path.join(objects_dir, basename + ".gcda")
shutil.copy(original_gcda, tmp_dir)

for d in dirs:
    try:
        # Försök att kopiera `.gcda` från en testkatalog till `.objects`
        test_gcda = os.path.join(d, ".cov", basename + ".gcda")
        shutil.copy(test_gcda, objects_dir)
    except FileNotFoundError:
        continue

    # Kör `gcov` för att generera `.gcov`-fil
    subprocess.run(
        ["gcov", os.path.join(objects_dir, basename + ".o")],
        cwd=src_dir,  # Kör `gcov` i `src`-katalogen
        capture_output=True, text=True
    )

    # Kontrollera om den specifika raden finns i `.gcov`-filen
    gcov_file = os.path.join(src_dir, basename + ".c.gcov")
    result = subprocess.run(
        ["grep", linenumber_match, gcov_file],
        capture_output=True, text=True
    )
    line = result.stdout
    if line:
        parts = line.split(':')
        try:
            int(parts[0])  # Kontrollera om första delen är ett nummer
            print(d)  # Skriv ut katalogen
        except ValueError:
            pass

# Återställ den ursprungliga `.gcda`-filen från `/tmp`
shutil.copy(os.path.join(tmp_dir, basename + ".gcda"), objects_dir)

# Kör `gcov` en sista gång för att återställa tillståndet
subprocess.run(
    ["gcov", os.path.join(objects_dir, basename + ".o")],
    cwd=src_dir,  # Kör `gcov` i `src`-katalogen
    capture_output=True, text=True
)

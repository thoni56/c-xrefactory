#! /bin/env python3

import sys
import subprocess
import io
from shutil import copy

def send_command(p, command):
    print(command[:-1])
    p.stdin.write(command.encode())
    p.stdin.flush()

def end_of_options(p):
    command = 'end-of-options\n\n'
    print(command[:-1])
    p.stdin.write(command.encode())
    p.stdin.flush()

def expect_sync(p):
    line = p.stdout.readline().decode()[:-1]
    print(line)
    if line != '<sync>':
        print("ERROR: did not get expected <sync>")
        sys.exit(-1)
    with open("buffer", 'rb') as file:
        print(file.read().decode())


# First argument is the value if CURDIR
CURDIR = sys.argv[1]
invocation = sys.argv[2:]
print(" ".join(invocation))

p = subprocess.Popen(invocation,
                     stdout = subprocess.PIPE,
                     stdin = subprocess.PIPE)


send_command(p, '-olcxgetprojectname -xrefrc '+CURDIR+'/.c-xrefrc '+CURDIR+'/single_int1.c\n')
end_of_options(p)
expect_sync(p)

send_command(p, '-olcxcomplet '+CURDIR+'/single_int1.c -olcursor=89 -xrefrc '+CURDIR+'/.c-xrefrc -p '+CURDIR+'\n')
end_of_options(p)
expect_sync(p)

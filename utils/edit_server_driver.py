#!/usr/bin/env python3
#
# Drive the c-xrefactory edit server using commands in a file
#
# Usage:
#
#     edit_server_driver.py <commandfile> <curdir> <bufferfile> [ <seconds> ]
#
import sys
import subprocess
import io
import time
from shutil import copy


def send_command(p, command):
    print(command.rstrip())
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

def read_output(filename):
    with open(filename, 'rb') as file:
        print(file.read().decode())


# First argument is the file with the commands
command_file = sys.argv[1]

# Second argument is the value of CURDIR
CURDIR = sys.argv[2]

# Third argument is name of the communication buffer file
buffer = sys.argv[3]

# If there is a fourth argument that is a sleep timer to
# be able to attach a debugger
if len(sys.argv) == 5:
    sleep = int(sys.argv[4])
else:
    sleep = None

with open(command_file, 'rb') as file:
    invocation = file.readline().decode().rstrip()
    print(invocation)

    args = invocation.replace("CURDIR", CURDIR)

    p = subprocess.Popen(args.split(' '),
                         stdout=subprocess.PIPE,
                         stdin=subprocess.PIPE)

    if sleep:
        time.sleep(sleep)

    command = file.readline().decode()
    while command != '':
        while command.rstrip() != '<sync>' and command != '':
            send_command(p, command.replace("CURDIR", CURDIR))
            command = file.readline().decode()
        if command != '':
            end_of_options(p)
            expect_sync(p)
            read_output(buffer)
            command = file.readline().decode()

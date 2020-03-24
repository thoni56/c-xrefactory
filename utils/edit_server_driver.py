#!/usr/bin/env python3
#
# Drive the c-xrefactory edit server using commands in a file
#
# Usage:
#
#     edit_server_driver.py <commandsfile> <curdir> <bufferfile> [ <seconds> ]
#
# Format of commandsfile is just a sequence of lines which are sent to
# the edit server interspersed with <sync> which will cause the driver
# to listen wait for syncronization from the server. Note that the edit
# server might send <progress> a couple of times before sending the <sync>.
#
# The first line is always the invocation command.
#
# All commands will be printed on stdout. After a <sync> is recieved
# the servers answer is in the specified output file. That answer will
# be copied into the output so that the complete interaction can be
# seen. Example input
#
#     ../../src/c-xref -xrefrc CURDIR/.c-xrefrc single_int1.c -xrefactory-II -o buffer -task_regime_server
#     -olcxgetprojectname -xrefrc CURDIR/.c-xrefrc CURDIR/single_int1.c
#     <sync>
#     -olcxcomplet CURDIR/single_int1.c -olcursor=85 -xrefrc CURDIR/.c-xrefrc -p CURDIR
#     <sync>
#
# If the line (probably the last) is '<exit>', the driver will send '-exit',
# 'end-of-options' and exit. This allows the server to shut down nicely.

import sys
import subprocess
import io
import time
import shlex
from shutil import copy

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def send_command(p, command):
    print(command)
    p.stdin.write((command+"\n").encode())
    p.stdin.flush()


def end_of_options(p):
    command = 'end-of-options\n\n'
    print(command[:-1])
    p.stdin.write(command.encode())
    p.stdin.flush()


def wait_for_sync(p):
    line = p.stdout.readline().decode()[:-1]
    while line != '<sync>':
        eprint("Waiting for <sync>, got '{}'".format(line))
        line = p.stdout.readline().decode()[:-1]
    print(line)


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

    p = subprocess.Popen(shlex.split(args),
                         stdout=subprocess.PIPE,
                         stdin=subprocess.PIPE)

    if sleep:
        time.sleep(sleep)

    command = file.readline().decode().rstrip()
    while command != '':
        while command != '<sync>' and command != '<exit>' and command != '':
            send_command(p, command.replace("CURDIR", CURDIR))
            command = file.readline().decode().rstrip()

        if command == '<exit>':
            send_command(p, "-exit")
            end_of_options(p)
            sys.exit(0)

        if command == '<sync>':
            end_of_options(p)
            wait_for_sync(p)
            read_output(buffer)
            command = file.readline().decode().rstrip()

        if command == '<update-report>':
            eprint(command)
            while command != '</update-report>':
                command = file.readline().decode().rstrip()
                eprint(command)

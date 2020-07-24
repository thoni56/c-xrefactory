#!/usr/bin/env python3
#
# Drive the c-xrefactory edit server using commands in a file
#
# Usage:
#
#     server_driver.py <commandsfile> <curdir> [ <seconds> ]
#
# <curdir> is the current directory to replace "CURDIR" with in command input files
# and output so that they are location independent.
#
# Format of <commandsfile> is a sequence of lines which are sent to
# the edit server interspersed with <sync> which will cause the driver
# to listen wait for syncronization from the server. Note that the edit
# server might send <progress> a couple of times before sending the <sync>.
#
# The first line is always the invocation command. It should not have an "-o" option as
# the server_driver will add "-o buffer".
#
# All commands will be printed on stdout. After a <sync> is recieved
# the servers answer is in the specified output file. That answer will
# be copied into the output so that the complete interaction can be
# seen. Example commands file
#
#     ../../src/c-xref -xrefrc CURDIR/.c-xrefrc single_int1.c -xrefactory-II -task_regime_server
#     -olcxgetprojectname -xrefrc CURDIR/.c-xrefrc CURDIR/single_int1.c
#     <sync>
#     -olcxcomplet CURDIR/single_int1.c -olcursor=85 -xrefrc CURDIR/.c-xrefrc -p CURDIR
#     <sync>
#
# If the line (probably the last) is '<exit>', the driver will send '-exit',
# 'end-of-options' and exit. This allows the server to shut down nicely.
#
# A very special case is if the invocation contains '-refactory'. Then
# it is a refactoring "command" which actually sends output even on
# the the first command, without "end-of-options". So then we need to
# wait for a sync and read the output. However, more commands may
# follow.

import sys
import os
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
    line = p.stdout.readline()[:-1].decode()
    while line != '<sync>' and line != '':
        eprint("Waiting for <sync>, got: '{0}'".format(line))
        line = p.stdout.readline().decode()[:-1]
    if line == '':
        eprint("Broken input")
        sys.exit(-1)
    print(line)


def read_output(filename):
    with open(filename, 'rb') as file:
        line = file.readline().decode().rstrip()
        while line != '':
            if line == '<update-report>':
                print(line)
                while line != '</update-report>':
                    line = file.readline().decode().rstrip()
            print(line)
            line = file.readline().decode().rstrip()

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Error - not enough arguments", file=sys.stderr)
        print("Usage:\t"+os.path.basename(sys.argv[0])+" <commandsfile> <curdir> [ <delay> ]")
        sys.exit(1)

    # First argument is the file with the commands
    command_file = sys.argv[1]

    # Second argument is the value of CURDIR
    CURDIR = sys.argv[2]

    server_buffer_filename = "server-buffer"
    with open(server_buffer_filename, "w"):
        pass

    # If there is a third argument that is a sleep timer to
    # be able to attach a debugger
    if len(sys.argv) == 4:
        sleep = int(sys.argv[3])
    else:
        sleep = None

    with open(command_file, 'rb') as file:
        invocation = file.readline().decode().rstrip().replace("CURDIR", CURDIR)
        invocation += " -o "+server_buffer_filename
        print(invocation)

        args = shlex.split(invocation)
        if sleep:
            args = [args[0]] + ["-pause", sys.argv[3]] + args[1:]

        p = subprocess.Popen(args,
                             stdout=subprocess.PIPE,
                             stdin=subprocess.PIPE)

        if "-refactory" in invocation:
            wait_for_sync(p)
            read_output(server_buffer_filename)

        command = file.readline().decode().rstrip()
        while command != '':
            while command != '<sync>' and command != '<exit>' and command != '': #and not "-refactory" in command:
                send_command(p, command.replace("CURDIR", CURDIR))
                command = file.readline().decode().rstrip()

            if command == '<exit>':
                send_command(p, "-exit")
                end_of_options(p)
                sys.exit(0)

            if command == '<sync>':
                end_of_options(p)
                wait_for_sync(p)
                read_output(server_buffer_filename)
                command = file.readline().decode().rstrip()

            if command == '<update-report>':
                eprint(command)
                while command != '</update-report>':
                    command = file.readline().decode().rstrip()
                    eprint(command)

        line = p.stdout.readline()[:-1].decode()
        while line != '':
            eprint("Waiting for end of communication, got: '{0}'".format(line))
            line = p.stdout.readline().decode()[:-1]

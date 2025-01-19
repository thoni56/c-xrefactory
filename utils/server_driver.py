#!/usr/bin/env python3
#
# Drive the c-xrefactory edit server using commands in a file
#
# Usage:
#
#     server_driver.py <commandsfile> --curdir=<curdir> --delay=<seconds> --options=<extra options>
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
# the first command, without "end-of-options". So then we need to
# wait for a sync and read the output. However, more commands may
# follow.

import sys
import os
import subprocess
import io
import time
import shlex
from shutil import copy
import argparse

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
        if not line.startswith("<progress>"):
            eprint("Waiting for <sync>, got: '{0}'".format(line))
        line = p.stdout.readline().decode()[:-1]
    if line == '':
        eprint("server-driver.py: Broken input")
        sys.exit(-1)
    print(line)


def read_output(filename):
    with open(filename, 'r') as file:
        in_update_report = False
        for line in file:
            line = line[:-1]			# Remove newline
            if line.endswith("</fatalError>"):
                print(line)
                sys.exit(-1)
            if not in_update_report:
                print(line)
            if line == '<update-report>':
                in_update_report = True
            if line == '</update-report>':
                print(line)
                in_update_report = False
    open(filename, 'w').close()                 # Erase content

def read_command(file):
    line = file.readline()
    #while len(line) == 0 or (len(line) > 0 and line[0] == '#'):
    #    line = file.readline()
    return line.decode().rstrip()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Read c-xref commands from file and handle synchronization and file buffering")
    parser.add_argument('command_file', help="File with the commands to feed to the c-xref server process, including the command to start the c-xref server (must be the first line)")
    parser.add_argument('--curdir', dest='CURDIR', help="The value to replace CURDIR in command file, default '" + os.getcwd()+"'", default=os.getcwd())
    parser.add_argument('--delay', type=int, dest='delay', help="How many seconds to sleep before starting the c-xref server process", default=0)
    parser.add_argument('--buffer', dest='server_buffer_filename', help="Name of file to use as communication buffer, default 'server-buffer'", default="server-buffer")
    parser.add_argument('--extra', dest='extra_options', help="Extra options to the c-xref startup command", default="")
    parser.add_argument('--cxref', dest='cxref_program', help="Which c-xref program to use, default is to use the one in PATH. Only applies if the command file starts with 'CXREF'", default="c-xref")
    args = parser.parse_args()

    with open(args.server_buffer_filename, "w"):
        pass

    with open(args.command_file, 'rb') as file:
        invocation = read_command(file).replace("CURDIR", args.CURDIR).replace("CXREF", args.cxref_program)
        invocation += " -o "+args.server_buffer_filename
        print(invocation)

        arguments = shlex.split(invocation+" "+args.extra_options)
        if args.delay > 0:
            arguments = [arguments[0]] + [f"-delay={args.delay}"] + arguments[1:]

        p = subprocess.Popen(arguments,
                             stdout=subprocess.PIPE,
                             stdin=subprocess.PIPE)

        if "-refactory" in invocation:
            wait_for_sync(p)
            read_output(args.server_buffer_filename)

        command = read_command(file)
        while command != '':
            while command != '<sync>' and command != '<exit>' and command != '': #and not "-refactory" in command:
                send_command(p, command.replace("CURDIR", args.CURDIR))
                command = read_command(file)

            if command == '<exit>':
                if p.poll() == None:	# no error code yet? So still alive...
                    send_command(p, "-exit")
                    end_of_options(p)
                sys.exit(0)

            if command == '<sync>':
                end_of_options(p)
                wait_for_sync(p)
                read_output(args.server_buffer_filename)
                command = read_command(file)

            if command == '<update-report>':
                eprint(command)
                while command != '</update-report>':
                    command = read_command(file)
                    eprint(command)

        line = p.stdout.readline()[:-1].decode()
        while line != '':
            eprint("Waiting for end of communication, got: '{0}'".format(line))
            line = p.stdout.readline().decode()[:-1]

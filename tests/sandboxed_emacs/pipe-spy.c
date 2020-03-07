/***********************************************************************\

    Program to spy on a stdin/stdout pipe communication between two
    processes, the caller and the target.

    thoni56/Thomas Nilefalk - January 2020

    Make the caller exec this program instead of the real target and
    ensure that the TARGET variable points to the target using a full
    path.

    Then, when called, this program, the parent, will set up three
    child processes, one spy for each of the downstream and upstream
    communication flows and a third which will exec the real target.
    The downstream and upstream childs will hook in to the stdin and
    stdout of the parent and thus read from the output pipe of the
    caller and write to the input pipe it has set up.

    The parent process will setup pipes so that the spy childs can
    hook their pipes to the exec'ed target process.
    While propagating the communication the children will also copy
    that to a logfile.

\**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>

#define READ_END 0
#define WRITE_END 1

FILE *logFile;

/* You need to define TARGET as a compile-time constant using -DTARGET=... */
//#define TARGET "path/to/some/executable"


int main(int argc, char **argv) {
    bool trace = false;         /* To trace, you must change this using
                                   the debugger. We can't add an
                                   option for this since we don't
                                   control argc/argv, the caller does.
                                   And we don't want to have to change
                                   it... */
    char logFileName[100];
    char *fileName;
#ifdef TARGET
    char *target = TARGET;
#else
    char *target = getenv("TARGET");
#endif

    fileName = getenv("LOGFILE");
    if (fileName != NULL)
        strcpy(logFileName, fileName);
    else
        sprintf(logFileName, "/tmp/pipespy%d.log", getpid());
    logFile = fopen(logFileName, "w");

    if (target == NULL) {
        fprintf(logFile, "*** ERROR: NO pipe-spy TARGET DEFINED, neither compile-time or environment ***");
        exit(-1);
    }

    /* Log arguments to the command on the first line in the log */
    for (int a=0; a<argc; a++)
        fprintf(logFile, "%s ", argv[a]);
    fprintf(logFile, "\n");
    fflush(logFile);

    /* Create the pipes for the sub-spies to the target */
    int downstream_pipe[2];
    int upstream_pipe[2];

    if (pipe(downstream_pipe) < 0) {
        perror("pipe(downstream_pipe)");
        _exit(-1);
    }

    if (pipe(upstream_pipe) < 0) {
        close(downstream_pipe[READ_END]);
        close(downstream_pipe[WRITE_END]);
        perror("pipe(upstream_pipe)");
        _exit(-1);
    }

    /* Fork a sub-spy to listen, log and propagate stdin */

    pid_t downstream_pid = fork();
    if (downstream_pid == 0) {
        char buffer[10000];
        if (trace) { fprintf(logFile, "** downstream child to fdopen\n"); fflush(logFile); }
        FILE *toTarget = fdopen(downstream_pipe[WRITE_END], "w");
        if (trace) { fprintf(logFile, "** downstream child did fdopen\n"); fflush(logFile); }

        /* Close the pipe ends we don't use */
        close(downstream_pipe[READ_END]);
        close(upstream_pipe[READ_END]);
        close(upstream_pipe[WRITE_END]);

        /* Read from stdin which is the same as the parent has */
        if (trace) { fprintf(logFile, "** downstream child to fgets\n");  fflush(logFile); }
        while (fgets(buffer, 10000, stdin) != NULL) {
            if (trace) { fprintf(logFile, "** downstream child did fgets\n");  fflush(logFile); }
            fprintf(logFile, "->:%s", buffer); fflush(logFile);
            fputs(buffer, toTarget); fflush(toTarget);
        }
        fprintf(logFile, "** downstream child got NULL\n");
        _exit(1);
    }
    if (trace) { fprintf(logFile, "** parent forked downstream child to %d\n", downstream_pid);  fflush(logFile); }

    /* Fork a sub-spy to listen, log and propagate stdout */

    pid_t upstream_pid = fork();
    if (upstream_pid == 0) {
        char buffer[10000];
        if (trace) { fprintf(logFile, "** upstream child to fdopen\n");  fflush(logFile); }
        FILE *fromTarget = fdopen(upstream_pipe[READ_END], "r");
        if (trace) { fprintf(logFile, "** upstream child did fdopen\n");  fflush(logFile); }

        /* Close the pipe ends we don't use */
        close(downstream_pipe[READ_END]);
        close(downstream_pipe[WRITE_END]);
        close(upstream_pipe[WRITE_END]);

        if (trace) { fprintf(logFile, "** upstream child to fgets\n");  fflush(logFile); }
        while (fgets(buffer, 10000, fromTarget) != NULL) {
            if (trace) { fprintf(logFile, "** upstream child did fgets\n");  fflush(logFile); }
            fprintf(logFile, "<-:%s", buffer); fflush(logFile);
            /* Write to stdout which is the same as the parent has */
            fputs(buffer, stdout); fflush(stdout);
        }
        fprintf(logFile, "** upstream child got NULL\n"); fflush(logFile);
        _exit(1);
    }
    if (trace) { fprintf(logFile, "** parent forked upstream child to %d\n", upstream_pid);  fflush(logFile); }

    /* Fork & exec the target with stdin & stdout pipes connected to
       the upstream and downstream pipes */
    pid_t target_pid = fork();
    if (target_pid == 0) {
        /* In the target, so... */
        /* ... connect the stdin to the downstream pipes read end... */
        if (trace) { fprintf(logFile, "** target child to dup2 on read end\n");  fflush(logFile); }
        if (dup2(downstream_pipe[READ_END], STDIN_FILENO) == -1) exit(errno);
        if (trace) { fprintf(logFile, "** target child did dup2 on read end\n");  fflush(logFile); }
        /* ... the stdout to the upstream pipes write end... */
        if (trace) { fprintf(logFile, "** target child to dup2 in write end\n");  fflush(logFile); }
        if (dup2(upstream_pipe[WRITE_END], STDOUT_FILENO) == -1) exit(errno);
        if (trace) { fprintf(logFile, "** target child did dup2 in write end\n");  fflush(logFile); }
        /* ... and exec ... */
        if (trace) { fprintf(logFile, "** target child to execv\n");  fflush(logFile); }
        execv(target, argv);
        /* ... if we get here execv failed... */
        perror(target);
        _exit(1);
    }
    if (trace) { fprintf(logFile, "** parent forked target to %d\n", target_pid);  fflush(logFile); }

    /* Parent need to close all pipes... */
    close(downstream_pipe[READ_END]);
    close(downstream_pipe[WRITE_END]);
    close(upstream_pipe[READ_END]);
    close(upstream_pipe[WRITE_END]);

    int status;
    waitpid(downstream_pid, &status, 0);
    waitpid(upstream_pid, &status, 0);
    waitpid(target_pid, &status, 0);
}

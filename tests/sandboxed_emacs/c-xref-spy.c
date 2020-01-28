#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>

#define READ_END 0
#define WRITE_END 1

FILE *logFile;

/* You need to define TARGET as a compile-time constant using -DTARGET=... */
//#define TARGET "../../src/c-xref"


int main(int argc, char **argv) {
    bool trace = false;

    logFile = fopen("/tmp/pipespy.log", "w");
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
        if (trace) { fprintf(logFile, "** target child to dup2\n");  fflush(logFile); }
        if (dup2(downstream_pipe[READ_END], STDIN_FILENO) == -1) exit(errno);
        if (trace) { fprintf(logFile, "** target child did dup2\n");  fflush(logFile); }
        /* ... the stdout to the upstream pipes write end... */
        if (trace) { fprintf(logFile, "** target child to dup2\n");  fflush(logFile); }
        if (dup2(upstream_pipe[WRITE_END], STDOUT_FILENO) == -1) exit(errno);
        if (trace) { fprintf(logFile, "** target child did dup2\n");  fflush(logFile); }
        /* ... and exec ... */
        if (trace) { fprintf(logFile, "** target child to execv\n");  fflush(logFile); }
        execv(TARGET, argv);
        /* ... if we get here execv failed... */
        perror(TARGET);
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

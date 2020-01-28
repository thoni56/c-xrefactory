#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define READ_END 0
#define WRITE_END 1

FILE *logFile;

/* You need to define TARGET as a compile-time constant using -DTARGET=... */
//#define TARGET "../../src/c-xref"


int main(int argc, char **argv) {
    logFile = fopen("/tmp/c-xref-spy-input.log", "w");
    for (int a=0; a<argc; a++)
        fprintf(logFile, "%s ", argv[a]);
    fprintf(logFile, "\n");
    fflush(logFile);

    /* Create the pipes for the sub-spies to the target */
    int downstreamPipe[2];
    int upstreamPipe[2];

    if (pipe(downstreamPipe) < 0) {
        perror("pipe(downstreamPipe)");
        _exit(-1);
    }

    if (pipe(upstreamPipe) < 0) {
        close(downstreamPipe[READ_END]);
        close(downstreamPipe[WRITE_END]);
        perror("pipe(upstreamPipe)");
        _exit(-1);
    }

    /* Fork a sub-spy to listen, log and propagate stdin */

    pid_t downstream_pid = fork();
    if (downstream_pid == 0) {
        char buffer[10000];
        FILE *toTarget = fdopen(downstreamPipe[WRITE_END], "w");

        /* Close pipe ends we don't use */
        close(downstreamPipe[READ_END]);
        close(upstreamPipe[READ_END]);
        close(upstreamPipe[WRITE_END]);

        /* Read from stdin which is the same as the parent has */
        while (fgets(buffer, 10000, stdin) != NULL) {
            fprintf(logFile, "<-:%s", buffer);
            fflush(logFile);
            fputs(buffer, toTarget); fflush(toTarget);
        }
        fprintf(logFile, "Downstream got NULL\n");
        _exit(1);
    }

    /* Fork a sub-spy to listen, log and propagate stdout */

    pid_t upstream_pid = fork();
    if (upstream_pid == 0) {
        char buffer[10000];
        FILE *fromTarget = fdopen(upstreamPipe[READ_END], "r");

        /* Close pipe ends we don't use */
        close(downstreamPipe[READ_END]);
        close(downstreamPipe[WRITE_END]);
        close(upstreamPipe[WRITE_END]);

        while (fgets(buffer, 10000, fromTarget) != NULL) {
            fprintf(logFile, "<-:%s", buffer);
            fflush(logFile);
            /* Write to stdout which is the same as the parent has */
            fputs(buffer, stdout); fflush(stdout);
        }
        fprintf(logFile, "Upstream got NULL\n");
        _exit(1);
    }

    /* Fork & exec the target with stdin & stdout pipes connected to the upstream and downstream pipes */
    pid_t target_pid = fork();
    if (target_pid == 0) {
        /* In the target, so... */
        /* ... connect the stdin to the downstream pipes read end... */
        if (dup2(downstreamPipe[READ_END], STDIN_FILENO) == -1) exit(errno);
        /* ... the stdout to the upstream pipes write end... */
        if (dup2(upstreamPipe[READ_END], STDOUT_FILENO) == -1) exit(errno);
        /* ... and exec ... */
        execv(TARGET, argv);
        /* ... if we get here execv failed... */
        perror(TARGET);
        _exit(1);
    }

    /* Parent need to close all pipes... */
    close(downstreamPipe[READ_END]);
    close(downstreamPipe[WRITE_END]);
    close(upstreamPipe[READ_END]);
    close(upstreamPipe[WRITE_END]);

    int status;
    waitpid(downstream_pid, &status, 0);
    waitpid(upstream_pid, &status, 0);
}

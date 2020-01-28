#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

FILE *logFile;

int main(int argc, char **argv) {
    logFile = fopen("/tmp/c-xref-spy-input.log", "w");
    for (int a=0; a<argc; a++)
        fprintf(logFile, "%s ", argv[a]);
    fprintf(logFile, "\n");
    fflush(logFile);

    sleep(20);

    /* Create the real c-xref process */
    int toCxrefPipe[2];
    int fromCxrefPipe[2];

    if (pipe(toCxrefPipe) < 0) {
        perror("pipe(toCxrefPipe)");
        _exit(-1);
    }

    if (pipe(fromCxrefPipe) < 0) {
        close(toCxrefPipe[PIPE_READ]);
        close(toCxrefPipe[PIPE_WRITE]);
        perror("pipe(fromCxrefPipe)");
        _exit(-1);
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* In the child, so... */
        /* ... connect the stdin... */
        if (dup2(toCxrefPipe[PIPE_READ], STDIN_FILENO) == -1) exit(errno);
        /* ... and stdout... */
        //if (dup2(fromCxrefPipe[PIPE_WRITE], STDOUT_FILENO) == -1) exit(errno);
        //close(toCxrefPipe[PIPE_WRITE]);
        //close(fromCxrefPipe[PIPE_WRITE]);
        /* ... and exec ... */
        execv("../../src/c-xref", argv);
        /* ... if we get here execv failed... */
        perror("exec failed");
        _exit(1);
    }

    /* Parent... */
    //close(toCxrefPipe[PIPE_READ]);
    //close(fromCxrefPipe[PIPE_WRITE]);


    /* Read and propagate input, read and propagate output */
    /* Assuming the communication is synchronous!!! */
    char buffer[10000];
    FILE *toCxref = fdopen(toCxrefPipe[PIPE_WRITE], "w");
    //FILE *fromCxref = fdopen(fromCxrefPipe[PIPE_READ], "r");

    while (fgets(buffer, 10000, stdin) != NULL) {
        fprintf(logFile, "<-:%s", buffer);
        fflush(logFile);
        fputs(buffer, toCxref); fflush(toCxref);
        if (fgets(buffer, 10000, fromCxref) == NULL) {
            fprintf(logFile, "Got NULL!\n"); fflush(logFile);
            exit(1);
        }
        fprintf(logFile, "->:%s", buffer); fflush(logFile);
        fprintf(stdout, "%s", buffer); fflush(stdout);
    }

    fclose(logFile);
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

FILE *logFile;

int main(int argc, char **argv) {
    logFile = fopen("/Users/thomas/Utveckling/c-xrefactory/tests/sandboxed_emacs/c-xref-spy-input.log", "w");
    for (int a=0; a<argc; a++)
        fprintf(logFile, "%s ", argv[a]);
    fprintf(logFile, "\n");

    /* Create the real c-xref process */
    int stdinPipes[2];
    int stdoutPipes[2];

    if (pipe(stdinPipes) < 0) {
        perror("pipe(stdinPipes)");
        _exit(-1);
    }

    if (pipe(stdoutPipes) < 0) {
        close(stdinPipes[0]);
        close(stdinPipes[1]);
        perror("pipe(stdoutPipes)");
        _exit(-1);
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* In the child, so... */
        /* ... connect the stdin... */
        if (dup2(stdinPipes[PIPE_READ], STDIN_FILENO) == -1) exit(errno);
        if (dup2(stdoutPipes[PIPE_WRITE], STDOUT_FILENO) == -1) exit(errno);
        close(stdinPipes[PIPE_READ]);
        close(stdinPipes[PIPE_WRITE]);
        close(stdoutPipes[PIPE_READ]);
        close(stdoutPipes[PIPE_WRITE]);
        /* ... and exec ... */
        execv("../../src/c-xref", argv);
        /* ... if we get here execv failed... */
        perror("exec failed");
        _exit(1);
    }

    /* Parent... */
    close(stdinPipes[PIPE_READ]);
    close(stdinPipes[PIPE_WRITE]);

    /* Read and propagate input, read and propagate output */
    /* Assuming the communication is synchronous!!! */
    char buffer[10000];
    FILE *child_stdin = fdopen(stdoutPipes[PIPE_READ], "w");
    FILE *child_stdout = fdopen(stdinPipes[PIPE_WRITE], "w");
    while (fgets(buffer, 10000, stdin) != NULL) {
        fprintf(logFile, "<-:%s", buffer);
        fputs(buffer, child_stdin);
        fgets(buffer, 10000, child_stdout);
        fprintf(logFile, "->:%s", buffer);
        printf("%2s", buffer);
    }

    fclose(logFile);
}

#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#define UNUSED (void)

/* Complement log.c with function to explicitly give file & line */
#define log_with_explicit_file_and_line(level, file, line, ...) log_log(level, file, line, __VA_ARGS__)

extern int creatingOlcxRefs(void);
extern void recursivelyCreateFileDirIfNotExists(char *fpath);

#endif

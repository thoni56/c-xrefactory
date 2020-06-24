/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

#define LOG_VERSION "0.2.0"

typedef void (*log_LockFn)(void *udata, int lock);

typedef enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL, LOG_NONE } LogLevel;

#ifdef DEBUG
#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define log_trace(...)
#define log_debug(...)
#endif
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warning(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal_with_line(file, line, ...) log_log(LOG_FATAL, file, line, __VA_ARGS__)

extern void log_set_udata(void *udata);
extern void log_set_lock(log_LockFn fn);
extern void log_set_fp(FILE *fp);
extern void log_set_console_level(int level);
extern void log_set_file_level(int level);
extern LogLevel log_get_file_level(void);

extern void log_log(LogLevel level, const char *file, int line, const char *fmt, ...);

#endif

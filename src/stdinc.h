#ifndef STDINC_H_INCLUDED
#define STDINC_H_INCLUDED

#if defined(__WIN32) && (! defined(__WIN32__))
#define __WIN32__ 1
#endif

#ifdef __WIN32__
#include <windows.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>

#if defined(__APPLE__)
#include <sys/malloc.h>
#else
#if (! defined(__FreeBSD__)) || (__FreeBSD__ < 5)
#include <malloc.h>
#endif
#endif


#endif

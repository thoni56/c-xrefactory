#ifndef STDINC__H
#define STDINC__H

/* If we are not bootstrapping, then the compiler built-in definitions
   should be generated and included in the generation, but not during compile
   since the whole point is that the compiler already automatically adds them.
 */
#ifdef GENERATION
#include "compiler_defines.g.h"
#endif

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

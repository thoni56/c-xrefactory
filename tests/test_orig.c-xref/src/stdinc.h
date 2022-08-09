#ifndef STDINC__H      /*SBD*/
#define STDINC__H      /*SBD*/

#if defined(__WIN32) && (! defined(__WIN32__))		/*SBD*/
#define __WIN32__ 1									/*SBD*/
#endif												/*SBD*/

#ifdef __WIN32__		/*SBD*/
#include <windows.h>	/*SBD*/
#include <direct.h>		/*SBD*/
#else					/*SBD*/
#ifdef __OS2__			/*SBD*/
#include <os2.h>		/*SBD*/
#else					/*SBD*/
#include <unistd.h>		/*SBD*/
#include <dirent.h>		/*SBD*/
#endif					/*SBD*/
#endif					/*SBD*/


#include <stdio.h>		/*SBD*/
#include <stdlib.h>		/*SBD*/
#include <ctype.h>		/*SBD*/
#include <assert.h>		/*SBD*/
#include <string.h>		/*SBD*/
#include <time.h>		/*SBD*/
#include <setjmp.h>		/*SBD*/
/* #include <stdarg.h> */		/*SBD*/

#include <sys/types.h>	/*SBD*/
#include <sys/stat.h>	/*SBD*/

#if defined(__APPLE__)		/*SBD*/
#include <sys/malloc.h>		/*SBD*/
#else						/*SBD*/
#if (! defined(__FreeBSD__)) || (__FreeBSD__ < 5)		/*SBD*/
#include <malloc.h>			/*SBD*/
#endif						/*SBD*/
#endif						/*SBD*/


#endif      /*SBD*/



#ifndef _ACCESS_H_
#define _ACCESS_H_

/* *********************************************************************** */
/*                       JAVA Access Attribute                             */

typedef enum access {
    ACCESS_DEFAULT		=		(0u << 0), //0x000,
    ACCESS_PUBLIC		=		(1u << 0), //0x001,
    ACCESS_PRIVATE		=		(1u << 1), //0x002,
    ACCESS_PROTECTED	=		(1u << 2), //0x004,
    ACCESS_STATIC		=		(1u << 3), //0x008,
    ACCESS_FINAL		=		(1u << 4), //0x010,
    ACCESS_SYNCHRONIZED	=		(1u << 5), //0x020,
    ACCESS_THREADSAFE	=		(1u << 6), //0x040,
    ACCESS_TRANSIENT	=		(1u << 7), //0x080,
    ACCESS_NATIVE		=		(1u << 8), //0x100,
    ACCESS_INTERFACE	=		(1u << 9), //0x200,
    ACCESS_ABSTRACT		=		(1u << 10), //0x400,
    ACCESS_ALL			=		(1u << 11), //0x800
} Access;

#define ACCESS_PPP_MODIFER_MASK    0x007

/* *********************************************************************** */
// this is maximal value of required access field in usg bits, it is index
// into the table s_requiredeAccessTable

#define MIN_REQUIRED_ACCESS 0
#define MAX_REQUIRED_ACCESS 3
#define MAX_REQUIRED_ACCESS_LN 2

#endif

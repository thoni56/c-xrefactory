#ifndef ACCESS_H_INCLUDED
#define ACCESS_H_INCLUDED

/* *********************************************************************** */
/*                       JAVA Access Attribute                             */

typedef enum {
    AccessDefault		=		(0u << 0), //0x000,
    AccessPublic		=		(1u << 0), //0x001,
    AccessPrivate		=		(1u << 1), //0x002,
    AccessProtected		=		(1u << 2), //0x004,
    AccessStatic		=		(1u << 3), //0x008,
    AccessFinal			=		(1u << 4), //0x010,
    AccessSynchronized	=		(1u << 5), //0x020,
    AccessThreadsafe	=		(1u << 6), //0x040,
    AccessTransient		=		(1u << 7), //0x080,
    AccessNative		=		(1u << 8), //0x100,
    AccessInterface		=		(1u << 9), //0x200,
    AccessAbstract		=		(1u << 10), //0x400,
    AccessAll			=		(1u << 11), //0x800
} Access;

#define ACCESS_PPP_MODIFER_MASK    0x007

/* *********************************************************************** */
// this is maximal value of required access field in usg bits, it is index
// into the table s_requiredeAccessTable

#define MIN_REQUIRED_ACCESS 0
#define MAX_REQUIRED_ACCESS 3

#endif

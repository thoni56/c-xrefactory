#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include "refactorings.h"

#define String char *
#define COMMENT(xx)
#ifdef PROTOCOL_C
#define PROTOCOL_ITEM(type, name, val) type name = val;
#else
#define PROTOCOL_ITEM(type, name, val) extern type name;
#endif

#include "protocol.th"

#undef PROTOCOL_ITEM
#undef COMMENT
#undef String

#endif

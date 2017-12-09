#ifndef PROTOCOL_H
#define PROTOCOL_H

#define String char *
#define COMMENT(xx)
#ifdef PROTOCOL_C
#define PROTOCOL_ITEM(type, name, val) type name = val;
#else
#define PROTOCOL_ITEM(type, name, val) extern type name;
#endif

#include "protocol.tc"

#undef PROTOCOL_ITEM
#undef COMMENT
#undef String

#endif

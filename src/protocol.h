#ifndef PROTOCOL_H
#define PROTOCOL_H

#define String char *
#define COMMENT(xx)
#define PROTOCOL_ITEM(type, name, val) static type name = val;

#include "protocol.tc"

#undef PROTOCOL_ITEM
#undef COMMENT
#undef String

#endif

#ifndef MACROS_H
#define MACROS_H

// First macro - we'll PUSH on this
#define INNER_MACRO(x) ((x) + 1)

// Second macro - uses INNER_MACRO, NEXT should find this reference
#define OUTER_MACRO(y) INNER_MACRO(y)

#endif

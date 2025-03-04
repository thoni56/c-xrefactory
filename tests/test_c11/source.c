_Atomic int x;       // Treated as "atomic int"
int _Atomic y;       // Equivalent syntax

_Thread_local int tls_var;
static _Thread_local int static_tls;
extern _Thread_local int extern_tls;

_Static_assert(sizeof(int) == 4, "Unexpected int size");

#include <stdalign.h>

_Alignas(16) int a16;  // ✅ x is aligned to 16 bytes
alignas(32) double a32;  // ✅ Alternative from <stdalign.h>

int alignment = _Alignof(double);  // ✅ Typically 8 or 16 bytes
long size = alignof(int);        // ✅ Alternative from <stdalign.h>

_Atomic int x;       // Treated as "atomic int"
int _Atomic y;       // Equivalent syntax

_Thread_local int tls_var;
static _Thread_local int static_tls;
extern _Thread_local int extern_tls;

Static_assert(sizeof(int) == 4, "Unexpected int size");

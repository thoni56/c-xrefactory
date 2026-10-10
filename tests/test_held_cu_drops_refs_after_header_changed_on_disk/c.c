#include "a.h"
void f(void);
void h(void) {
#if USE_F
    f();
#endif
}

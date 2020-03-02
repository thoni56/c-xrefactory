#include "globals.h"

#include "memory.h"

/* With this as a separate function it is possible to catch memory resize longjmps */
void memoryResize(void) {
    longjmp(s_memoryResize,1);
}

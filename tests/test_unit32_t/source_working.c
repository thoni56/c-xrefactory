#include <stdint.h>
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
#endif

typedef uint32_t Aint;

typedef struct {
    Aint i;
} Astruct;

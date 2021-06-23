#include <stdint.h>
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
#endif

typedef int_least8_t x;

typedef uint32_t Aint;

typedef struct {
    Aint i;
} Astruct;

int main(int argc, char **argv) {}

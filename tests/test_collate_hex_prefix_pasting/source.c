#define MAKE_HEXU(x)  0x##x##U
#define MAKE_HEX(x)   0x##x

unsigned int v1 = MAKE_HEXU(FF);    // 0x##FF##U -> 0xFFU
int v2 = MAKE_HEX(BEEF);            // 0x##BEEF -> 0xBEEF

#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#define SYMTAB_HASH_FUN_INC(oldval, charcode) {\
    oldval+=charcode; oldval+=(oldval<<10); oldval^=(oldval>>6);\
}
#define SYMTAB_HASH_FUN_FINAL(oldval) {\
    oldval+=(oldval<<3); oldval^=(oldval>>11); oldval+=(oldval<<15);\
}

extern unsigned hashFun(char *s);

#endif

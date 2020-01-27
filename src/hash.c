#include "hash.h"


static unsigned hashFunIncrement(unsigned oldHash, char ch) {
    unsigned newHash = oldHash;
    newHash+=ch; newHash+=(newHash<<10); newHash^=(newHash>>6);
    return newHash;
}

static unsigned hashFunFinal(unsigned oldHash) {
    unsigned newHash = oldHash;
    newHash+=(newHash<<3); newHash^=(newHash>>11); newHash+=(newHash<<15);
    return newHash;
}

unsigned hashFun(char *string) {
    register unsigned h = 0;
    register char *s = string;
    register char ch;
    for(ch = *s; ch ; ch = *++s) h = hashFunIncrement(h, ch);
    h = hashFunFinal(h);
    return h;
}

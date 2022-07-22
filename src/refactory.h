#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED

#include "editor.h"
#include "proto.h"

typedef struct pushAllInBetweenData {
    int minMemi;
    int maxMemi;
} PushAllInBetweenData;

typedef struct tpCheckMoveClassData {
    struct pushAllInBetweenData mm;
    char                       *spack;
    char                       *tpack;
    int                         transPackageMove;
    char                       *sclass;
} TpCheckMoveClassData;

extern void refactory();

#endif

#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED

#include "proto.h"
#include "editor.h"


typedef struct tpCheckMoveClassData {
    struct pushAllInBetweenData  mm;
    char		*spack;
    char		*tpack;
    int			transPackageMove;
    char		*sclass;
} TpCheckMoveClassData;


extern void mainRefactory();

#endif

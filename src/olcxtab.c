#define IN_OLCXTAB_C
#include "olcxtab.h"

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashlist.tc"

/* TODO Remove olcxTab since we now only handle one user */
static SessionData userData;
SessionData *currentUserData = &userData;
OlcxTab s_olcxTab;

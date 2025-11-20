/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#pragma once

#define NULL (void *)(0)

typedef struct Service Service;

struct Service {
};

typedef struct Unit Unit;

/* For casting a unit into the various unit types */
#define DEFINE_CAST(UPPERCASE, MixedCase)                               \
        static inline MixedCase* UPPERCASE(Unit *u) {                   \
                if (_unlikely_(!u || u->type != UNIT_##UPPERCASE))      \
                        return NULL;                                    \
                                                                        \
                return (MixedCase*) u;                                  \
        }

/* For casting the various unit types into a unit */
#define UNIT(u) (&(u)->meta)

#define UNIT_TRIGGER(u) ((Unit*) set_first((u)->dependencies[UNIT_TRIGGERS]))

DEFINE_CAST(SERVICE, Service);

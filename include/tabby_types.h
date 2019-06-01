#ifndef CARACAL_TYPES_H
#define CARACAL_TYPES_H

#include <stdint.h>

typedef struct _ATOM_32 {
    volatile int32_t counter;
} Atom32;

typedef struct _ATOM_64 {
    volatile int64_t counter;
} Atom64;

#endif

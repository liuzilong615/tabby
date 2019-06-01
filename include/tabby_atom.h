#ifndef TABBY_ATOM_H
#define TABBY_ATOM_H

#include "tabby_types.h"

// X86_32
static inline int32_t atom32_read(Atom32 *p) {
    return *(volatile int32_t *)(&p->counter);
}

static inline void atom32_set(Atom32 *p, int32_t i) {
    p->counter = i;
}

static inline int32_t atom32_add_return(Atom32 *p, int32_t v) {
    int32_t ret = v;
    asm volatile ("lock; xaddl %0, %1\n"
            : "+r" (ret), "+m" (p->counter)
            : : "memory", "cc");
    return ret + v;
}

static inline void atom32_add(Atom32 *p, int32_t v) {
    asm volatile ("lock; addl %1, %0\n"
            : "+m" (p->counter)
            : "ir" (v)
            : "memory", "cc");
}

#define atom32_inc_return(p) atom32_add_return((p), 1)
#define atom32_dec_return(p) atom32_add_return((p), -1)
#define atom32_inc(p) atom32_add((p), 1)
#define atom32_dec(p) atom32_add((p), -1)

// x86_64
static inline int64_t atom64_read(Atom64 *p) {
    return *(volatile int64_t *)(&p->counter);
}

static inline void atom64_set(Atom64 *p, int64_t i) {
    p->counter = i;
}

static inline int64_t atom64_add_return(Atom64 *p, int64_t v) {
    int64_t ret = v;
    asm volatile ("lock; xaddq %0, %1\n"
            : "+r" (ret), "+m" (p->counter)
            : : "memory", "cc");
    return ret + v;
}

static inline void atom64_add(Atom64 *p, int64_t v) {
    asm volatile ("lock; addq %1, %0\n"
            : "+m" (p->counter)
            : "ir" (v)
            : "memory", "cc");
}

#define atom64_inc_return(p) atom64_add_return((p), 1)
#define atom64_dec_return(p) atom64_add_return((p), -1)
#define atom64_inc(p) atom64_add((p), 1)
#define atom64_dec(p) atom64_add((p), -1)


#endif

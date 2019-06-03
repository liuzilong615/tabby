#ifndef _TABBY_MACRO_H
#define _TABBY_MACRO_H

#include <stdlib.h>
#include <stddef.h>

#define Var_2(name, tag) \
    name##tag

#define Var_1(name, tag) \
    Var_2(name, tag)


#define TABBY_VAR(name) \
    Var_1(name, __LINE__)

#define tabby_container_of(ptr, type, member) \
({\
   int8_t *TABBY_VAR(_ptr_) = (int8_t *)ptr; \
   ((type *)(TABBY_VAR(_ptr_) - offsetof(type, member)))\
 })


#endif

#ifndef _TABBY_MACRO_H
#define _TABBY_MACRO_H

#define Var_2(name, tag) \
    name##tag

#define Var_1(name, tag) \
    Var_2(name, tag)


#define TABBY_VAR(name) \
    Var_1(name, __LINE__)


#endif

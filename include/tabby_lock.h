#ifndef _TABBY_LOCK_H
#define _TABBY_LOCK_H

#include <pthread.h>
#include <assert.h>
#include <stdint.h>

#include "tabby_macro.h"

typedef enum {
    LOCK_NONE = 0,
    LOCK_SPIN, 
    LOCK_MUTEX,
    LOCK_RW,
    LOCK_MAX
} LockType;

typedef struct _Lock {
    LockType lock_type;
    union {
        pthread_spinlock_t s_lock;
        pthread_mutex_t m_lock;
        pthread_rwlock_t rw_lock;
    } lock_un;
} Lock ;

void tabby_lock_init(Lock *l, LockType type);
void tabby_lock_destroy(Lock *l);

typedef int (*lock_func)(void *l);
typedef int (*unlock_func)(void *l);

typedef struct __func_node {
    lock_func __lock;
    lock_func __lock2;
    unlock_func __unlock;
} FuncNode;

extern FuncNode __fn[];

#define tabby_lock_protect(lock, rd)\
    uint32_t TABBY_VAR(idx) = (lock)->lock_type ;\
    lock_func TABBY_VAR(ll) = __fn[TABBY_VAR(idx)].__lock;\
    if ( rd ) { TABBY_VAR(ll) = __fn[TABBY_VAR(idx)].__lock2 ; }\
    unlock_func TABBY_VAR(un) = __fn[TABBY_VAR(idx)].__unlock;\
    assert( TABBY_VAR(idx) < LOCK_MAX );\
    for ( void *TABBY_VAR(l) = (void *)&((lock)->lock_un), *TABBY_VAR(k) = (void *)&((lock)->lock_un); TABBY_VAR(l) != NULL; TABBY_VAR(un)( TABBY_VAR(k) ) )\
        for ( TABBY_VAR(ll)( TABBY_VAR(k) ); TABBY_VAR(l)!=NULL; TABBY_VAR(l)=NULL)


#endif

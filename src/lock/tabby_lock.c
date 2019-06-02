#include "tabby_lock.h"

static int no_lock(void *l) {
    return 0;
}

FuncNode __fn[LOCK_MAX] = {
    {
        .__lock = no_lock,
        .__lock2 = NULL,
        .__unlock = no_lock,
    },
    {
        .__lock = (lock_func )pthread_spin_lock,
        .__lock2 = NULL,
        .__unlock = (unlock_func )pthread_spin_unlock,
    },
    {
        .__lock = (lock_func )pthread_mutex_lock,
        .__lock2 = NULL,
        .__unlock = (unlock_func )pthread_mutex_unlock,
    },
    {
        .__lock = (lock_func )pthread_rwlock_wrlock,
        .__lock2 = (lock_func )pthread_rwlock_rdlock,
        .__unlock = (unlock_func )pthread_rwlock_unlock,
    }
} ;

static void lock_init(Lock *l, LockType type) {
    l->lock_type = type;

    switch ( type ) {
        case LOCK_NONE:
            break;
        case LOCK_SPIN:
            pthread_spin_init(&l->lock.s_lock, PTHREAD_PROCESS_PRIVATE);
            break;
        case LOCK_MUTEX:
            pthread_mutex_init(&l->lock.m_lock, NULL);
            break;
        case LOCK_RW:
            pthread_rwlock_init(&l->lock.rw_lock, NULL);
            break;
        default:
            assert(0);
            break;
    }
}

static void lock_destroy(Lock *l) {
    switch (l->lock_type ) {
        case LOCK_NONE:
            break;
        case LOCK_SPIN:
            pthread_spin_destroy(&l->lock.s_lock);
            break;
        case LOCK_MUTEX:
            pthread_mutex_destroy(&l->lock.m_lock);
            break;
        case LOCK_RW:
            pthread_rwlock_destroy(&l->lock.rw_lock);
            break;
        default:
            assert(0);
            break;
    }
}

void tabby_lock_init(Lock *l, LockType type) __attribute__ ((alias ("lock_init")));
void tabby_lock_destroy(Lock *l) __attribute__ ((alias ("lock_destroy")));

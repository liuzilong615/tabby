#ifndef _TABBY_LOCK_H
#define _TABBY_LOCK_H

#include <pthread.h>

// spin (lock)
#define TABBY_SPIN_INIT(lock) \
    pthread_spin_init((lock), PTHREAD_PROCESS_PRIVATE)

#define TABBY_SPIN_DESTROY(lock)\
    pthread_spin_destroy(lock)

#define TABBY_SPIN_LOCK(lock) \
    for ( pthread_spin_t *l = (lock), *k = (lock); l != NULL; pthread_spin_unlock(k) )\
        for ( pthread_spin_lock(k); l!=NULL; l=NULL)

// mutex (lock)
#define TABBY_MUTEX_INIT(lock) \
    pthread_mutex_init((lock), NULL)

#define TABBY_MUTEX_DESTROY(lock) \
    pthread_mutex_destroy(lock)

#define TABBY_MUTEX_LOCK(lock) \
    for ( pthread_mutex_t *l = (lock), *k = (lock); l != NULL; pthread_mutex_unlock(k) )\
        for ( pthread_mutex_lock(k); l!=NULL; l=NULL)

// rwlock
#define TABBY_RW_INIT(lock)\
    pthread_rwlock_init((lock), NULL)

#define TABBY_RW_DESTROY(lock)\
    pthread_rwlock_destroy(lock)

#define TABBY_WR_LOCK(lock)\
    for ( pthread_rwlock_t *l = (lock), *k = (lock); l != NULL; pthread_rwlock_unlock(k) )\
        for ( pthread_rwlock_wrlock(k); l!=NULL; l=NULL)

#define TABBY_RD_LOCK(lock)\
    for ( pthread_rwlock_t *l = (lock), *k = (lock); l != NULL; pthread_rwlock_unlock(k) )\
        for ( pthread_rwlock_rdlock(k); l!=NULL; l=NULL)


#endif

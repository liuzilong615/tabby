#ifndef _TABBY_LIST_H
#define _TABBY_LIST_H

#include "tabby_types.h"
#include "tabby_lock.h"

typedef enum {
    LIST_LOCK_NONE = 0,
    LIST_LOCK_SPIN,
    LIST_LOCK_MUTEX,
    LIST_LOCK_RW,
    LIST_LOCK_MAX
} ListLockType;

typedef struct _ListNode {
    struct _ListNode *next;
    struct _ListNode *prev;
} ListNode;

typedef struct _List {
    Atom32 list_num;
    
    // whether the list should be protected
    ListLockType list_lock_type;
    union {
        pthread_spinlock_t s_lock;
        pthread_mutex_t m_lock;
        pthread_rwlock_t rw_lock;
    } list_lock;

    ListNode list_head;
} List;

#define LIST_NODE_INIT(node) \
    do {\
        (node)->prev = (node);\
        (node)->next = (node);\
    } while(0)

extern void tabby_list_init(List *l, ListLockType llt);

#endif

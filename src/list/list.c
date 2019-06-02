#include "tabby_list.h"
#include "tabby_atom.h"
#include <assert.h>

static void list_init(List *l, ListLockType llt) {
    atom32_set(&l->list_num, 0);
    l->list_lock_type = llt;

    switch ( llt ) {
        case LIST_LOCK_NONE:
            break;
        case LIST_LOCK_SPIN:
            TABBY_SPIN_INIT(&l->list_lock.s_lock);
            break;
        case LIST_LOCK_MUTEX:
            TABBY_MUTEX_INIT(&l->list_lock.m_lock);
            break;
        case LIST_LOCK_RW:
            TABBY_RW_INIT(&l->list_lock.rw_lock);
            break;
        default:
            assert(0);
            break;
    }

    LIST_NODE_INIT(&l->list_head);
}

void tabby_list_init(List *l, ListLockType llt) __attribute__ ((alias ("list_init")));

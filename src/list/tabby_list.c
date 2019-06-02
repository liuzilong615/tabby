#include "tabby_list.h"
#include "tabby_atom.h"
#include <assert.h>

static void list_init(List *l, LockType type) {
    atom32_set(&l->list_num, 0);

    tabby_lock_init(&l->list_lock, type);

    LIST_NODE_INIT(&l->list_head);
}

static void list_destroy(List *l) {
    int32_t cnt;
    cnt = atom32_read(&l->list_num);
    assert( cnt == 0 );
    tabby_lock_destroy(&l->list_lock);
}

void tabby_list_init(List *l, LockType type) __attribute__ ((alias ("list_init")));
void tabby_list_destroy(List *l) __attribute__ ((alias ("list_destroy")));

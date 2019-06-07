#include "tabby_list.h"
#include "tabby_atom.h"
#include <assert.h>

#define INVALID_NODE    ((void *)NULL)
#define INVALID_DATA    ((void *)NULL)

typedef struct _ListNode {
    struct _ListNode *next;
    struct _ListNode *prev;
    struct _List *__l; // which list this node belongs to
    void *data;
} ListNode ;

struct _List {
    int list_num;
    Lock list_lock; // protect the list
    ListNode list_head;
} ;

#define _list_first(l) \
    ({ \
     (l->list_head.next);\
     })

#define MALLOC(size) \
    malloc(size)

#define FREE(ptr) \
    free(ptr)

// external functions
List *tabby_list_new(LockType type) __attribute__ ((alias ("_list_new")));
void tabby_list_free(List *l) __attribute__ ((alias ("_list_free")));
int tabby_list_append(List *l, void *data) __attribute__ ((alias ("_list_append")));
int tabby_list_prepend(List *l, void *data) __attribute__ ((alias ("_list_prepend")));
int tabby_list_del(List *l, void *data) __attribute__ ((alias ("_list_del")));
int tabby_list_foreach(List *l, ListNodeProcess proc) __attribute__ ((alias ("_list_foreach")));

static inline void tabby_list_node_init(List *l, ListNode *node) {
    node->prev = node;
    node->next = node;
    node->__l = l;
    node->data = NULL;
}

List *_list_new(LockType type) {
    List *l = MALLOC(sizeof(List));
    if ( l ) {
        l->list_num = 0;
        tabby_lock_init(&l->list_lock, type);
        tabby_list_node_init(l, &l->list_head);
    }
    return l;
}

static inline void _do_list_insert(ListNode *n, ListNode *prev, ListNode *next) {
    n->prev = prev;
    n->next = next;
    prev->next = n;
    next->prev = n;
}

int _list_append(List *l, void *data) {
    ListNode *n = MALLOC(sizeof(ListNode));
    if ( !n ) {
        return -1;
    }
    n->data = data;
    n->__l = l;
    tabby_lock_protect(&l->list_lock, 0)  {
        _do_list_insert(n, l->list_head.prev, &l->list_head);
        l->list_num++;
    }

    return 0;
}

int _list_prepend(List *l, void *data) {
    ListNode *n = MALLOC(sizeof(ListNode));
    if ( !n ) {
        return -1;
    }
    n->data = data;
    n->__l = l;
    tabby_lock_protect(&l->list_lock, 0)  {
        _do_list_insert(n, &l->list_head, l->list_head.next);
        l->list_num++;
    }
    return 0;
}

static inline ListNode *_do_list_del(List *l, ListNode *n) {
    assert( n->__l == l );
    assert( n != &l->list_head );
    n->next->prev = n->prev;
    n->prev->next = n->next;
    n->next = INVALID_NODE;
    n->prev = INVALID_NODE;
    n->data = INVALID_DATA;
    return n;
}

int _list_del(List *l, void *data) {
    ListNode *n; 
    int ret = -1;

    tabby_lock_protect(&l->list_lock, 0)  {
        n = _list_first(l);
        while ( n != &l->list_head ) {
            if ( n->data == data ) {
                _do_list_del(l, n);
                l->list_num--;
                ret = 0;
                break;
            }
            n = n->next;
        }
    }

    if ( 0 == ret ) {
        FREE(n);
    }
    return ret;
}

int _list_foreach(List *l, ListNodeProcess proc) {
    int cnt, num;
    ListNode *n;

    tabby_lock_protect(&l->list_lock, 1) {
        n = _list_first(l);
        cnt = 0;
        num = l->list_num;
        while ( n != &l->list_head ) {
            cnt++;
            proc(n->data);
            n = n->next;
        }

        assert( cnt == num );
    }

    return cnt;
}

void _list_free(List *l) {
    int32_t cnt, num;
    ListNode *n;

    // clear all nodes
    tabby_lock_protect(&l->list_lock, 0) {
        cnt = 0;
        num = l->list_num;
        n = _list_first(l);
        while ( n != &l->list_head ) {
            _do_list_del(l, n);
            FREE(n);
            n = &l->list_head;
            cnt++;
        }

        assert( cnt == num );
        l->list_num = 0;
    }
}



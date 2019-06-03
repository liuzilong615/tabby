#ifndef _TABBY_LIST_H
#define _TABBY_LIST_H

#include "tabby_types.h"
#include "tabby_lock.h"
#include "tabby_assert.h"

typedef struct _ListNode {
    struct _ListNode *next;
    struct _ListNode *prev;
    Lock node_lock; // TODO: add a MACRO, for multi-thread safe
    struct _List *__l; // TODO: enabled by macro, for debug
} ListNode;

typedef struct _List {
    Atom32 list_num;
    // whether the list should be protected
    Lock list_lock;
    ListNode list_head;
} List;

extern void tabby_list_init(List *l, LockType type);
extern void tabby_list_destroy(List *l);

static inline void tabby_list_node_init(ListNode *node) {
    node->prev = node;
    node->next = node;
    node->__l = NULL;
    tabby_lock_init(&node->node_lock, LOCK_MUTEX);
}

static inline void tabby_list_add_tail(List *l, ListNode *n) {
    ListNode *h = &l->list_head;

    // lock node first
    tabby_lock_protect(&n->node_lock, 0) {

        tabby_assert( n->__l == NULL );
        
        tabby_lock_protect(&l->list_lock, 0) {
            n->prev = h->prev;
            n->next = h;
            n->prev->next = n;
            n->next->prev = n;
            n->__l = l;
        }
    }
}

static inline void tabby_list_add_head(List *l, ListNode *n) {
    ListNode *h = &l->list_head;

    // lock node , in case of inserting into different lists at the same time
    tabby_lock_protect(&n->node_lock, 0) {

        tabby_assert( n->__l == NULL );
        tabby_lock_protect(&l->list_lock, 0) {
            n->next = h->next;
            n->prev = h;
            n->next->prev = n;
            n->prev->next = n;
            n->__l = l;
        }
    }
}

static inline void tabby_list_del(ListNode *n) {
    List *l = NULL;
    
    // make sure a node manipulated by only one type of action at the same point
    tabby_lock_protect(&n->node_lock, 0) {
        l = n->__l;
        assert( l != NULL );
        tabby_lock_protect(&l->list_lock, 0) {
            n->next->prev = n->prev;
            n->prev->next = n->next;
            n->next = n;
            n->prev = n;
            n->__l = NULL;
        }
    }
}

#endif

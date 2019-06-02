#ifndef _TABBY_LIST_H
#define _TABBY_LIST_H

#include "tabby_types.h"
#include "tabby_lock.h"

typedef struct _ListNode {
    struct _ListNode *next;
    struct _ListNode *prev;
} ListNode;

typedef struct _List {
    Atom32 list_num;
    // whether the list should be protected
    Lock list_lock;
    ListNode list_head;
} List;

#define LIST_NODE_INIT(node) \
    do {\
        (node)->prev = (node);\
        (node)->next = (node);\
    } while(0)

extern void tabby_list_init(List *l, LockType type);
extern void tabby_list_destroy(List *l);

#endif

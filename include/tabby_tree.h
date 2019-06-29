#ifndef _TABBY_TREE_H
#define _TABBY_TREE_H

#include "tabby_lock.h"

typedef int (*RBTreeCompareFunc)(void *skey, void *tkey);
typedef int (*RBTreeGetFunc)(void *value);
typedef int (*RBTreePutFunc)(void *value);
typedef int (*RBTreeDetachFunc)(void *key, void *value);

typedef void (*RBTreeNodeProc)(void *key, void *value, int y, int x, int color);

typedef struct _RBTree RBTree;

RBTree *tabby_rbtree_new(LockType type, 
        RBTreeCompareFunc _compare,
        RBTreeGetFunc _get,
        RBTreePutFunc _put, 
        RBTreeDetachFunc _det);
int tabby_rbtree_insert(RBTree *tree, void *key, void *value);
int tabby_rbtree_foreach(RBTree *tree, RBTreeNodeProc proc);
void tabby_rbtree_free(RBTree *tree);
int tabby_rbtree_delete(RBTree *tree, void *key);

#endif

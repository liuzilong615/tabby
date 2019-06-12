#ifndef _TABBY_TREE_H
#define _TABBY_TREE_H

typedef int (*TreeCompareFunc)(void *skey, void *tkey);
typedef int (*TreeGetFunc)(void *value);
typedef int (*TreePutFUnc)(void *value);

typedef struct _RBTree RBTree;

#endif

#include <stdio.h>
#include <stdlib.h>

#define MALLOC(size) malloc(size)
#define FREE(ptr) free(ptr)

/*
       case RRB_LL 000000[00]B
                    +---+
                    | b |
           second 0 +---+ 1
                    |   |
            +---+---+   +----+---+
            | r |            |   |
    first 0 +---+ 1          | h |
            |   |            |   |
   +---+----+   |            +---+
   | r |        +-+---+
   +---+          |   |
   |   |          | h |
+--++ ++--+       |   |
|   | |   |       +---+
| h | | h |
|   | |   |
+---+ +---+

    */
typedef enum {
    C_LEFT = 0,
    C_RIGHT = 1
} ChildType;

typedef enum {
    RRB_LL = 0x00,
    RRB_LR = 0x01,
    RRB_RL = 0x02,
    RRB_RR = 0x03
} ADJUST_CASE;

typedef enum {
    COLOR_NONE = 0,
    COLOR_RED,
    COLOR_BLACK
} RBColor;

typedef struct _RBNode {
    RBColor color;
    struct _RBTreeNode *parent;
    struct _RBTreeNode *left;
    struct _RBTreeNode *right;
    void *key;
    void *value;
} RBNode ;

struct _RBTree {
    int cnt;
    Lock tree_lock;
    // for elements operations
    TreeCompareFunc compare;
    TreeGetFunc get;
    TreePutFunc put;

    RBNode *root;
};

static inline ChildType _rbtree_child_type(RBNode *child, RBNode *parent) {
    ChildType type;
    int m1, m2;
    m1 = (child == parent->left);
    m2 = (child == parent->right);
    // do xor 
    assert( ((m1&(!m2))|(m2&(!m1))) == 1);
    // change to mask
    m1 = -m1;
    m2 = -m2;
    type = ((m1&C_LEFT)|(m2&C_RIGHT));
    return type;
}

RBTree *_rbtree_new(LockType type, 
        TreeCompareFunc compare,
        TreeGetFunc get,
        TreePutFunc put) {
    RBTree *tree = malloc(sizeof(RBTree));
    if (  tree ) {
        tabby_lock_init(&tree->tree_lock, type);
        tree->cnt = 0;
        tree->compare = compare;
        tree->get = get;
        tree->put = put;
        tree->root = NULL;
    }
    return tree;
}

static inline void _rbtree_insert_leaf(RBTree *tree, RBNode *n, RBNode *parent, ChildType ctype, void *key, void *value) {
    assert( n != NULL );
    assert( parent != NULL );

    // setup a node
    n->color = COLOR_RED;
    n->left = NULL;
    n->right = NULL;
    n->parent = parent;
    n->key = key;
    n->value = value;

    // insert as a leaf
    switch ( ctype ) {
        case C_LEFT:
            parent->left = n;
            break;
        case C_RIGHT:
            parent->right = n;
            break;
        default:
            assert(0);
            break;
    }
}

static inline void _rbtree_rebalance(RBTree *tree, RBNode *node, RBNode *parent, ChildType ctype) {
    RBNode *grandparent, *p_grandparent ;
    ADJUST_CASE adj_case = 0;
    ChildType cur_type, par_type;

    // only need to rebalance when current node is red
    assert( node->color == COLOR_RED );

    if ( !parent ) {
        node->color = COLOR_BLACK;
        return;
    }
    if ( parent->color == COLOR_BLACK ) {
        return;
    }

    assert( parent->color == COLOR_RED );
    
    // if a grandparent is NULL, parent is the original root, which is RED
    grandparent = parent->parent;
    assert( grandparent != NULL );

    p_grandparent = grandparent->parent;

    cur_type = _rbtree_child_type(node, parent) ;
    par_type = _rbtree_child_type(parent, grandparent) ;
    adj_case = (( cur_type << 1 ) & par_type);

    // switch adj_case
}

int _rbtree_insert(RBTree *tree, void *key, void *value) {
    RBNode *cur, *parent, *child;
    int cmp_ret = -1, dup = 0;
    RBNode  *new_node;
    ChildType ctype;

    new_node = MALLOC(sizeof(RBNode));
    if ( !new_node ) {
        return -2;
    }

    tabby_lock_protect(&tree->tree_lock, 0) {
        if ( tree->cnt == 0 ) {
            // root node
            new_node->color = COLOR_BLACK;
            new_node->parent = NULL;
            new_node->left = NULL;
            new_node->right = NULL;
            new_node->key = key;
            new_node->value = value;
            // insert to tree
            tree->root = new_node;
            tree->cnt++;
        } else {
            // non-root node, find a leaf, don't enable duplicated key
            cur = &tree->root;
            parent = NULL;
            assert( cur != NULL );
            while ( cur != NULL ) {
                cmp_ret = tree->compare(key, cur->key) ;
                if ( cmp_ret < 0 ) { // smaller than cur
                    parent = cur;
                    cur = cur->left;
                    ctype = C_LEFT;
                } else if ( cmp_ret > 0 ) { // larger than cur
                    parent = cur;
                    cur = cur->right;
                    ctype = C_RIGHT;
                } else {    // duplicated
                    dup = 1;
                    break;
                }
            }
            // rebalance the tree
            if ( !dup ) {
                cur = new_node;
                _rbtree_insert_leaf(tree, cur, parent, ctype, key, value) ;
            }
        }
    }

    if ( dup ) {
        FREE(new_node);
    }
    
    // succ: 0
    return -dup;
}

void _rbtree_free(RBTree *tree) {
}

#include <stdio.h>
#include <stdlib.h>
#include "tabby_tree.h"

#define MALLOC(size) malloc(size)
#define FREE(ptr) free(ptr)

/*
                    first   second
                         \  /
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
    RRB_LL = 0x00,  // 00b
    RRB_LR = 0x01,  // 01b
    RRB_RL = 0x02,  // 10b
    RRB_RR = 0x03   // 11b
} ADJUST_CASE;

typedef enum {
    COLOR_NONE = 0,
    COLOR_RED,
    COLOR_BLACK
} RBColor;

typedef struct _RBNode RBNode;
struct _RBNode {
    RBColor color;
    RBNode *parent;
    RBNode *left;
    RBNode *right;
    void *key;
    void *value;
};

struct _RBTree {
    int cnt;
    Lock tree_lock;
    // for elements operations
    RBTreeCompareFunc t_compare;
    RBTreeGetFunc t_get;
    RBTreePutFunc t_put;
    RBTreeDetachFunc t_det;
    RBNode *root;
};

RBTree *tabby_rbtree_new(LockType type, 
        RBTreeCompareFunc _compare,
        RBTreeGetFunc _get,
        RBTreePutFunc _put, 
        RBTreeDetachFunc _det) __attribute__ ((alias("_rbtree_new")));
int tabby_rbtree_insert(RBTree *tree, void *key, void *value) __attribute__ ((alias("_rbtree_insert")));
int tabby_rbtree_foreach(RBTree *tree, RBTreeNodeProc proc) __attribute__ ((alias("_rbtree_foreach")));
void tabby_rbtree_free(RBTree *tree) __attribute__ ((alias("_rbtree_free")));
 

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
        RBTreeCompareFunc _compare,
        RBTreeGetFunc _get,
        RBTreePutFunc _put, 
        RBTreeDetachFunc _det) {
    RBTree *tree = malloc(sizeof(RBTree));
    if (  tree ) {
        tabby_lock_init(&tree->tree_lock, type);
        tree->cnt = 0;
        tree->t_compare = _compare;
        tree->t_get = _get;
        tree->t_put = _put;
        tree->t_det = _det;
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
    // reference the value before holding it in rbtree
    tree->t_get(value);
    n->value = value;

    // insert as a leaf
    switch ( ctype ) {
        case C_LEFT:
            assert( parent->left == NULL );
            parent->left = n;
            break;
        case C_RIGHT:
            assert( parent->right == NULL );
            parent->right = n;
            break;
        default:
            assert(0);
            break;
    }
}

/*
                p_grandparent
                      | 
                      V
                    grandparent
                    +---+
                    | b |
                  0 +---+ 1
            parent  |   |
            +---+---+   +----+---+
            | r |            |   |
          0 +---+ 1          | h |
   cur      |   |            |   |
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
static inline RBNode *_rrb_ll(RBTree *tree, 
        RBNode *cur, RBNode *parent, RBNode *grandparent, 
        RBNode *p_grandparent) {
    RBNode *tmp, *local_root, *local_right, *local_left;
    local_root = parent;
    local_left = cur;
    local_right = grandparent;
    tmp = parent->right;

    // setup local right
    local_right->left = tmp;
    if( tmp ) {
        tmp->parent = local_right;
    }
    // set new local root
    local_root->right = local_right;
    local_right->parent = local_root;

    // change color of local left
    local_left->color = COLOR_BLACK;

    // set new parent of local root
    //local_root->parent = p_grandparent;

    // TODO: setup p_grandparent->child(left/right) after call this func
    return local_root;
} 

static inline RBNode *_rrb_lr(RBTree *tree, 
        RBNode *cur, RBNode *parent, RBNode *grandparent, 
        RBNode *p_grandparent) {
    RBNode *tmp_l, *tmp_r, *local_root, *local_left, *local_right;
    local_root = cur;
    local_left = grandparent;
    local_right = parent;
    tmp_l = cur->left;
    tmp_r = cur->right;

    // setup local left
    local_left->right = tmp_l;
    if ( tmp_l ) {
        tmp_l->parent = local_left;
    }

    // setup local right
    local_right->left = tmp_r;
    if( tmp_r ) {
        tmp_r->parent = local_right;
    }
    local_right->color = COLOR_BLACK;

    // set new local root
    local_root->left = local_left;
    local_left->parent = local_root;
    local_root->right = local_right;
    local_right->parent = local_root;

    // set new parent of local root
    //local_root->parent = p_grandparent;

    // TODO: setup p_grandparent->child(left/right) after call this func
    return local_root;
}

static inline RBNode *_rrb_rl(RBTree *tree, 
        RBNode *cur, RBNode *parent, RBNode *grandparent, 
        RBNode *p_grandparent) {
    RBNode *tmp_l, *tmp_r, *local_root, *local_left, *local_right;
    local_root = cur;
    local_left = parent;
    local_right = grandparent;
    tmp_l = cur->left;
    tmp_r = cur->right;

    // setup local left
    local_left->right = tmp_l;
    if ( tmp_l ) {
        tmp_l->parent = local_left;
    }
    local_left->color = COLOR_BLACK;

    // setup local right
    local_right->left = tmp_r;
    if( tmp_r ) {
        tmp_r->parent = local_right;
    }

    // set new local root
    local_root->left = local_left;
    local_left->parent = local_root;
    local_root->right = local_right;
    local_right->parent = local_root;

    // set new parent of local root
    //local_root->parent = p_grandparent;

    // TODO: setup p_grandparent->child(left/right) after call this func
    return local_root;
}

static inline RBNode *_rrb_rr(RBTree *tree, 
        RBNode *cur, RBNode *parent, RBNode *grandparent, 
        RBNode *p_grandparent) {
    RBNode *tmp, *local_root, *local_right, *local_left;
    local_root = parent;
    local_left = grandparent;
    local_right = cur;
    tmp = parent->left;

    // setup local left
    local_left->right = tmp;
    if( tmp ) {
        tmp->parent = local_left;
    }
    // set new local root
    local_root->left = local_left;
    local_left->parent = local_root;

    // change color of local left
    local_right->color = COLOR_BLACK;

    // set new parent of local root
    //local_root->parent = p_grandparent;

    // TODO: setup p_grandparent->child(left/right) after call this func
    return local_root;
}

static void _rbtree_rebalance(RBTree *tree, RBNode *cur_node, RBNode *parent, ChildType ctype) {
    RBNode *grandparent, *p_grandparent, *local_root = NULL;
    RBNode *cur = cur_node;
    ADJUST_CASE adj_case = 0;
    ChildType cur_type, par_type, lrt_type = -1;

    // only need to check rebalance when current node is red
    assert( cur->color == COLOR_RED );

    // cur is root, job finished
    if ( !parent ) {
        cur->color = COLOR_BLACK;
        tree->root = cur;
        return;
    }
    // parent is black, job finished
    if ( parent->color == COLOR_BLACK ) {
        return;
    }

    assert( parent->color == COLOR_RED );
    
    // if a grandparent is NULL, parent is root with RED color, which is not right
    grandparent = parent->parent;
    assert( grandparent != NULL );

    // get parent of grandparent ready
    p_grandparent = grandparent->parent;

    cur_type = _rbtree_child_type(cur, parent) ;
    par_type = _rbtree_child_type(parent, grandparent) ;
    adj_case = (int)(( (int)cur_type << 1 ) | (int)par_type);

    // switch adj_case, and do rebalance under different conditions
    switch ( adj_case ) {
        case RRB_LL:
            local_root = _rrb_ll(tree, cur, parent, grandparent, p_grandparent);
            break;
        case RRB_LR:
            local_root = _rrb_lr(tree, cur, parent, grandparent, p_grandparent);
            break;
        case RRB_RL:
            local_root = _rrb_rl(tree, cur, parent, grandparent, p_grandparent);
            break;
        case RRB_RR:
            local_root = _rrb_rr(tree, cur, parent, grandparent, p_grandparent);
            break;
        default:
            assert(0);
            break;
    }
    assert( local_root != NULL );
    local_root->parent = p_grandparent;
    if ( p_grandparent ) {
        lrt_type = _rbtree_child_type(grandparent, p_grandparent);
    }

    if ( lrt_type == C_LEFT ) {
        p_grandparent->left = local_root;
    } else if ( lrt_type == C_RIGHT ) {
        p_grandparent->right = local_root;
    } else {
        // p_grandparent is NULL, just do nothing
    }

    // recursively call myself
    _rbtree_rebalance(tree, local_root, p_grandparent, lrt_type) ;
}

int _rbtree_insert(RBTree *tree, void *key, void *value) {
    RBNode *cur, *parent;
    int cmp_ret = -1, dup = 0;
    RBNode  *new_node;
    ChildType ctype;

    // alloc a node first
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
            cur = tree->root;
            parent = NULL;
            assert( cur != NULL );
            while ( cur != NULL ) {
                cmp_ret = tree->t_compare(key, cur->key) ;
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
            // insert leaf and rebalance the tree
            if ( !dup ) {
                cur = new_node;
                _rbtree_insert_leaf(tree, cur, parent, ctype, key, value) ;
                _rbtree_rebalance(tree, cur, parent, ctype) ;
            }
        }
    }

    if ( dup ) {
        FREE(new_node);
    }
    
    // succ: 0
    return -dup;
}

// root first recursively traverse
static void _rbtree_recur(RBNode *node, RBTreeNodeProc proc, int y, int x, int *cnt) {
    if ( !node ) {
        return ;
    }
    // root fisrt
    proc(node->key, node->value, y, x, node->color);
    (*cnt)++;

    // then left
    _rbtree_recur(node->left, proc, y + 1, x * 2 , cnt);

    // right at last
    _rbtree_recur(node->right, proc, y + 1, x *2 + 1, cnt);
}

int _rbtree_foreach(RBTree *tree, RBTreeNodeProc proc) {
    int ret = 0;
    tabby_lock_protect(&tree->tree_lock, 0) {
        _rbtree_recur(tree->root, proc, 0, 0, &ret);
    }
    return ret;
}

void _rbtree_free(RBTree *tree) {
}

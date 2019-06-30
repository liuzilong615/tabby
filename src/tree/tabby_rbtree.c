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

/*
              +---+      +---+            +---+                           +---+
              | b |      | b |            | b |                           | b |
              +-+-+      +-+-+            +-+-+                           +-+-+
                |          |                |                               |
         +------+          +-------+        +-----+                  +------+
         |                         |              |                  |
       +-+-+                     +-+-+          +-+-+              +-+-+
       | r |                     | r |          | r |              | r |
       +-+-+                     +-+-+          +-+-+              +-+-+
         |                         |              |                  |
  +------+                  +------+              +----+             +-------+
  |                         |                          |                     |
+-+-+                     +-+-+                      +-+-+                 +-+-+
| r |                     | r |                      | r |                 | r |
+---+                     +---+                      +---+                 +---+

  RRB_LL                 RRB_LR                 RRB_RR                 RRB_RL
 */

typedef enum {
    RRB_LL = 0x00,  // 00b
    RRB_LR = 0x01,  // 01b
    RRB_RL = 0x02,  // 10b
    RRB_RR = 0x03   // 11b
} INS_CASE;

/*
        +---+                   +---+                 +---+
        | b |                   | b |                 | r |
        +-+-+                   +-+-+                 +-+-+
          |                       |                     |
   +------+-----+          +------+-----+        +------+-----+
   |            |          |            |        |            |
 +-+-+        +-+-+      +-+-+        +-+-+    +-+-+        +-+-+
 | b |        | b |      | b |        | r |    | b |        | b |
 +---+        +---+      +---+        +---+    +---+        +---+
      L_BBB                   L_BBR                  L_BRB

       +---+                   +---+                 +---+
       | b |                   | b |                 | r |
       +-+-+                   +-+-+                 +-+-+
         |                       |                     |
  +------+-----+          +------+-----+        +------+-----+
  |            |          |            |        |            |
+-+-+        +-+-+      +-+-+        +-+-+    +-+-+        +-+-+
| b |        | b |      | r |        | b |    | b |        | b |
+---+        +---+      +---+        +---+    +---+        +---+
      R_BBB                   R_BBR                R_BRB
 */

typedef enum {
    L_BBB = 0X07, // 0-0111b
    L_BBR = 0x06, // 0-0110b
    L_BRB = 0x05, // 0-0101b
    R_BBB = 0X17, // 1-0111b
    R_BBR = 0x16, // 1-0110b
    R_BRB = 0x15, // 1-0101b
} DEL_CASE ;

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
int tabby_rbtree_delete(RBTree *tree, void *key) __attribute__((alias("_rbtree_delete")));

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
    INS_CASE adj_case = 0;
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
                tree->cnt++;
            }
        }
    }

    if ( dup ) {
        FREE(new_node);
    }
    
    // succ: 0
    return -dup;
}

// CAUTION: this func do search without lock
static RBNode *_rbtree_do_search(RBTree *tree, void *key) {
    RBNode *cur;
    int ret; 

    cur = tree->root;
    while ( cur != NULL ) {
        ret = tree->t_compare(key, cur->key);
        if ( ret > 0 ) {
            cur = cur->right;
        } else if ( ret < 0 ) {
            cur = cur->left;
        } else {
            break;
        }
    }

    return cur;
}

void *_rbtree_search(RBTree *tree, void *key){
    int ref;
    RBNode *node;
    void *value = NULL;
    tabby_lock_protect(&tree->tree_lock, 1) {
        node = _rbtree_do_search(tree, key);
        if ( node ) {
            value = node->value;
            ref = tree->t_get(value);
            assert( ref > 1 ); // hold by tree and value
        }
    }
    return value;
}

static RBNode *_rbtree_next(RBNode *n) {
    RBNode *next = n->right;
    while ( next && next->left != NULL ) {
        next = next->left;
    }
    return next;
}

static RBNode *_rbtree_prev(RBNode *n){
    RBNode *prev = n->left;
    while ( prev && prev->right != NULL ) {
        prev = prev->right;
    }
    return prev;
}

static RBColor _rbtree_node_color(RBNode *n) {
    RBColor c;
    c = n?n->color:COLOR_BLACK;
    return c;
}

static RBNode *_l_bbb(RBTree *tree, RBNode *cur, RBNode *parent, RBNode *sibling, int *recur) {
    //RBNode *child, *new_root, *old_root;
    RBNode *local_root, *local_left, *local_right, *local_child;
    RBColor c_color;

    assert( parent->left == cur );
    assert( parent->right == sibling );
    assert( sibling != NULL );

    // get  child color
    c_color = _rbtree_node_color(sibling->left);

    // 2 cases
    if ( c_color == COLOR_RED ) {
        local_root = sibling->left;
        local_left = parent;
        local_right = sibling;

        // reconstruct tree
        local_left->right = local_root->left;
        if ( local_root->left ) {
            local_root->left->parent = local_left;
        }

        local_right->left = local_root->right;
        if ( local_root->right ) {
            local_root->right->parent = local_right;
        }

        local_root->left = local_left;
        local_left->parent = local_root;

        local_root->right = local_right;
        local_right->parent = local_root;

        // change color
        assert( local_root->color == COLOR_RED );
        assert( local_left->color == COLOR_BLACK );
        local_root->color = COLOR_BLACK;

        // finished
        *recur = 0;
    } else if ( c_color == COLOR_BLACK ) {

        local_root = sibling;
        local_left = parent;
        local_child = sibling->left;

        // reconstruct tree
        local_left->right = local_child;
        if ( local_child ) {
            local_child->parent = local_left;
        }

        local_root->left = local_left;
        local_left->parent = local_root;

        // change color
        assert( local_left->color == COLOR_BLACK );
        local_left->color = COLOR_RED;

        // need recur
        *recur = 1;
    } else {
        assert(0);
    }

    return local_root;
}

static RBNode *_r_bbb(RBTree *tree, RBNode *cur, RBNode *parent, RBNode *sibling, int *recur) {
    //RBNode *child, *new_root, *old_root;
    RBNode *local_root, *local_left, *local_right, *local_child;
    RBColor c_color;

    assert( parent->right == cur );
    assert( parent->left == sibling );
    assert( sibling != NULL );

    // get  child color
    c_color = _rbtree_node_color(sibling->right);

    // 2 cases
    if ( c_color == COLOR_RED ) {
        local_root = sibling->right;
        local_left = sibling;
        local_right = parent;

        // reconstruct tree
        local_left->right = local_root->left;
        if ( local_root->left ) {
            local_root->left->parent = local_left;
        }

        local_right->left = local_root->right;
        if ( local_root->right ) {
            local_root->right->parent = local_right;;
        }

        local_root->left = local_left;
        local_left->parent = local_root;

        local_root->right = local_right;
        local_right->parent = local_root;

        // change color
        assert( local_root->color == COLOR_RED );
        assert( local_right->color == COLOR_BLACK );
        local_root->color = COLOR_BLACK;

        // finished
        *recur = 0;
    } else if ( c_color == COLOR_BLACK ) {

        local_root = sibling;
        local_right = parent;
        local_child = sibling->right;

        // reconstruct tree
        local_right->left = local_child;
        if ( local_child ) {
            local_child->parent = local_right;
        }

        local_root->right = local_right;
        local_right->parent = local_root;

        // change color
        assert( local_right->color == COLOR_BLACK );
        local_right->color = COLOR_RED;

        // need recur
        *recur = 1;
    } else {
        assert(0);
    }

    return local_root;
}

static RBNode *_l_bbr(RBTree *tree, RBNode *cur, RBNode *parent, RBNode *sibling, int *recur) {
    RBNode *local_root, *local_left, *local_child;

    local_root = sibling;
    local_left = parent;
    local_child = sibling->left;

    assert( local_child != NULL );
    assert( local_child->color == COLOR_BLACK );

    local_left->right = local_child;
    local_child->parent = local_left;

    local_root->left = local_left;
    local_left->parent = local_root;

    // change color
    assert( local_root->color == COLOR_RED );
    assert( local_left->color == COLOR_BLACK );
    local_root->color = COLOR_BLACK;
    local_left->color = COLOR_RED;

    *recur = 1;
    return local_root;
}

static RBNode *_r_bbr(RBTree *tree, RBNode *cur, RBNode *parent, RBNode *sibling, int *recur) {
    RBNode *local_root, *local_right, *local_child;

    local_root = sibling;
    local_right = parent;
    local_child = sibling->right;

    assert( local_child != NULL );
    assert( local_child->color == COLOR_BLACK );

    local_right->left = local_child;
    local_child->parent = local_right;

    local_root->right = local_right;
    local_right->parent = local_root;

    // change color
    assert( local_root->color == COLOR_RED );
    assert( local_right->color == COLOR_BLACK );
    local_root->color = COLOR_BLACK;
    local_right->color = COLOR_RED;
    
    *recur = 1;
    return local_root;
}

static RBNode *_l_brb(RBTree *tree, RBNode *cur, RBNode *parent, RBNode *sibling, int *recur) {
    RBNode *local_root, *local_left, *local_right, *local_child;
    RBColor c_color;

    assert( sibling != NULL );
    c_color = _rbtree_node_color(sibling->left);

    // 2 cases
    if ( c_color == COLOR_BLACK ) {
        local_root = sibling;
        local_left = parent;
        local_child = sibling->left;

        local_left->right = local_child;
        if (local_child ) {
            local_child->parent = local_left;
        }

        local_root->left = local_left;
        local_left->parent = local_root;

    } else if ( c_color == COLOR_RED ) {

        local_root = sibling->left;
        local_left = parent;
        local_right = sibling;

        assert( local_root != NULL );

        local_left->right = local_root->left;
        if ( local_root->left ) {
            local_root->left->parent = local_left;
        }

        local_right->left = local_root->right;
        if ( local_root->right ) {
            local_root->right->parent = local_right;
        }

        local_root->left = local_left;
        local_left->parent = local_root;

        local_root->right = local_right;
        local_right->parent = local_root;

        // change color
        assert( local_left->color == COLOR_RED );
        local_left->color = COLOR_BLACK;

    } else {
        assert(0);
    }

    *recur = 0;
    
    return local_root;
}

static RBNode *_r_brb(RBTree *tree, RBNode *cur, RBNode *parent, RBNode *sibling, int *recur) {
    RBNode *local_root, *local_left, *local_right, *local_child;
    RBColor c_color;

    assert( sibling != NULL );
    c_color = _rbtree_node_color(sibling->right);

    // 2 cases
    if ( c_color == COLOR_BLACK ) {
        local_root = sibling;
        local_right = parent;
        local_child = sibling->right;

        local_right->left = local_child;
        if (local_child ) {
            local_child->parent = local_right;
        }

        local_root->right = local_right;
        local_right->parent = local_root;

    } else if ( c_color == COLOR_RED ) {

        local_root = sibling->right;
        local_left = sibling;
        local_right = parent;

        assert( local_root != NULL );

        local_left->right = local_root->left;
        if ( local_root->left ) {
            local_root->left->parent = local_left;
        }

        local_right->left = local_root->right;
        if ( local_root->right ) {
            local_root->right->parent = local_right;
        }

        local_root->left = local_left;
        local_left->parent = local_root;

        local_root->right = local_right;
        local_right->parent = local_root;

        // change color
        assert( local_right->color == COLOR_RED );
        local_right->color = COLOR_BLACK;

    } else {
        assert(0);
    }

    *recur = 0;
    
    return local_root;
}

static void _rbtree_del_rebalance(RBTree *tree, RBNode *cur, RBNode *parent) {
    DEL_CASE del_case;
    RBColor my_color, p_color, sib_color;
    RBNode *sibling, *new_cur, *new_parent, *g_parent, *new_root;
    int recur = -1;

    // cur is root now
    if ( !parent ) {
        assert( cur != NULL );
        assert( cur->color == COLOR_BLACK );
        assert( cur->parent == NULL );

        tree->root = cur;
        return;
    }

    // get grand parent;
    g_parent = parent->parent;

    // get colors of cur, parent and sibling
    my_color = _rbtree_node_color(cur);

    assert( my_color == COLOR_BLACK );
    p_color = parent->color;

    if ( cur == parent->left ) {
        sibling = parent->right;
        del_case = 0x00;
    } else if ( cur == parent->right ) {
        sibling = parent->left;
        del_case = 0x10;
    } else {
        // logic error
        assert(0);
    }

    // get sibling 
    assert( sibling != NULL );
    sib_color = sibling->color;

    // get rebalance case
    del_case |= ( (my_color - 1) << 2 );
    del_case |= ( (p_color - 1 ) << 1 );
    del_case |= ( sib_color - 1);

    switch ( del_case ) {
        case L_BBB:
            new_root = _l_bbb(tree, cur, parent, sibling, &recur);
            new_cur = new_root;
            new_parent = g_parent;
            break;
        case L_BBR:
            new_root = _l_bbr(tree, cur, parent, sibling, &recur);
            new_parent = new_root->left;
            assert( new_parent != NULL );
            new_cur = new_parent->left;
            break;
        case L_BRB:
            new_root = _l_brb(tree, cur, parent, sibling, &recur);
            assert( recur == 0 );
            assert( g_parent != NULL );
            break;
        case R_BBB:
            new_root = _r_bbb(tree, cur, parent, sibling, &recur);
            new_cur = new_root;
            new_parent = g_parent;
            break;
        case R_BBR:
            new_root = _r_bbr(tree, cur, parent, sibling, &recur);
            new_parent = new_root->right;
            assert( new_parent != NULL );
            new_cur = new_parent->right;
            break;
        case R_BRB:
            new_root = _r_brb(tree, cur, parent, sibling, &recur);
            assert( recur == 0 );
            assert( g_parent != NULL );
            break;
        default :
            assert(0);
            break;
    }

    new_root->parent = g_parent ;

    if ( g_parent ) {
        if ( g_parent->left == parent ) {
            g_parent->left = new_root;
        } else if ( g_parent->right == parent ) {
            g_parent->right = new_root;
        } else {
            assert( 0);
        }
    } else {
        // recur end
        //assert( new_root->color == COLOR_RED );
        //new_root->color = COLOR_BLACK;
        assert( new_root->color == COLOR_BLACK );
        tree->root = new_root;
        //return ;
    }

    assert( recur == 0 || recur == 1);
    if ( recur == 1) {
        return _rbtree_del_rebalance(tree, new_cur, new_parent);
    } else {
        // no need recur
        return ;
    }
}

static RBNode *_rbtree_do_delete(RBTree *tree, RBNode *node) {
    RBNode *parent, *swap_node = NULL, *ret = NULL;

    // find a node for swap
    swap_node = _rbtree_next(node);
    if ( !swap_node ) {
        swap_node = _rbtree_prev(node);
    }

    // do swap, and point to the final node
    if ( swap_node ) {
        node->key = swap_node->key;
        node->value = swap_node->value;
        if ( swap_node->left != NULL ) {
            assert( swap_node->right == NULL );
            swap_node->key = swap_node->left->key;
            swap_node->value = swap_node->left->value;
            swap_node = swap_node->left;
        } else if ( swap_node->right != NULL ) {
            assert( swap_node->left == NULL );
            swap_node->key = swap_node->right->key;
            swap_node->value = swap_node->right->value;
            swap_node = swap_node->right;
        } else {
            // swap node is the final node 
        }
    } else {
        // it is swap node itself, the final node
        swap_node = node;
    }

    // get delete node and parent node
    assert( swap_node->left == NULL );
    assert( swap_node->right == NULL );
    ret = swap_node;
    parent = swap_node->parent;

    // delete node and rebalance
    if ( !parent ) {
        // delete node is root
        tree->root = NULL;
        assert( tree->cnt == 1 );
    } else {
        if ( parent->left == ret ) {
            parent->left = NULL;
        } else if ( parent->right == ret ) {
            parent->right = NULL;
        } else {
            assert(0);
        }

        // do rebalance when node to be delted is BLACK
        if ( ret->color == COLOR_BLACK ) {
            // rebalance
            _rbtree_del_rebalance(tree, NULL, parent);
        } else {
            // deleting a red node  makes no difference
        }
    }

    return ret;
}

int _rbtree_delete(RBTree *tree, void *key) {
    RBNode *node, *del_node = NULL;
    void *del_value, *del_key;
    int ret = -1;

    tabby_lock_protect(&tree->tree_lock, 0) {
        // search node first
        node = _rbtree_do_search(tree, key);
        if ( node ) {
            del_key = node->key;
            del_value = node->value;
            del_node = _rbtree_do_delete(tree, node);
            tree->cnt--;
            assert(tree->cnt >= 0 );
        }
    }

    // release resource
    if ( del_node ) {
        tree->t_det(del_key, del_value);
        tree->t_put(del_value);
        FREE( del_node );
        ret = 0;
    }

    return ret;
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

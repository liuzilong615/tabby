#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabby_tree.h"

#define BPKEY_SIZE (128)
#define BPTREE_MAX_LEVEL    (24)
#define BPTREE_BRANCH_MIN (5)

#define MALLOC(size) malloc(size)
#define FREE(ptr) free(ptr)

/*
                            +-------+------+------+-----+------+-------+
                    BP_VALUE|       |      |      |     |      | no use|
                            |       |      |      |     |      |       |
                            +------------------------------------------+
                   CHILDREN |       |      |      |     |      |       |
                   PTRS     +---+---+----+-+------+-----+------+-------+
                                |        |
                                |        |
              +-----------------+        +------------------------------------------+
              |                                                                     |
              |                                                                     |
         +----+--+------+------+-----+------+-------+               +-------+------++-----+-----+------+-------+
 BP_VALUE|       |      |      |     |      | no use|       BP_VALUE|       |      |      |     |      | no use|
         |       |      |      |     |      |       |               |       |      |      |     |      |       |
         +------------------------------------------+               +------------------------------------------+
CHILDREN |       |      |      |     |      |       |      CHILDREN |       |      |      |     |      |       |
PTRS     +-------+------+------+-----+------+-------+      PTRS     +-------+------+------+-----+------+-------+

 */
#if 0
typedef struct _BPKey {
    uint8_t bytes[BPKEY_SIZE]; // support max key size 128B
} BPKey;
#endif

typedef struct _BPValue {
    int64_t key;
    void *value;
} BPValue; 

typedef struct _BPNode {
    struct _BPNode *next_sib;
    struct _BPNode *prev_sib;
    struct _BPNode *parent;
    int idx_in_pnode;
    int bp_cnt;         // how many bpvalue structs it has
    struct _BPValue *bp_values;
    struct _BPNode **bp_children;
    uint8_t buf[0]; // including all values and children ptrs
} BPNode ;

// B+ tree
struct _BPTree {
    int bp_branch_max;     // how many branches each BPNode has at most
    int bp_node_size;  // determined by bp_branch_max;

    Lock bp_lock;

    BPTreeGetFunc bp_get;
    BPTreePutFunc bp_put;
    //BPTreeCmpFunc bp_cmp;

    BPNode *root;
} ;

//typedef struct _BPTree BPTree;

typedef int (*BPTreeGetFunc)(void *value);
typedef int (*BPTreePutFunc)(void *value);
//typedef int (*BPTreeCmpFunc)(int64_t me, int64_t other);

BPTree *bptree_new_ex(LockType type, int m, BPTreeGetFunc _get, BPTreePutFunc _put) {

    BPTree *bptree;
    int node_size; int max = m;

    if ( max < BPTREE_BRANCH_MIN ) {
        max = BPTREE_BRANCH_MIN;
    }

    node_size = sizeof(BPNode) + max * sizeof(BPValue) + max * sizeof(BPNode *);

    bptree = MALLOC(sizeof(BPTree));
    if ( bptree ) {
        bptree->bp_branch_max = max;
        bptree->bp_node_size = node_size;
        bptree->root = NULL;
        bptree->bp_get = _get;
        bptree->bp_put = _put;
        //bptree->bp_cmp = _cmp;
        tabby_lock_init(&bptree->bp_lock, type);
    }

    return bptree;
}

static inline void __init_node(BPNode *n, int max_branches, int size) {
    memset(n, 0, size);
    n->bp_values = (struct _BPValue *)&(n->buf[0]);
    n->bp_children = (struct _BPNode **)&(n->buf[max_branches*sizeof(BPValue)]);
}

static inline BPNode *__alloc_node(BPTree *tree) {
    BPNode *n;
    int size, m;

    size = tree->bp_node_size; 
    m = tree->bp_branch_max;

    n = MALLOC(size);
    if ( n ) {
        __init_node(n, m, size);
    }
    return n;
}

static inline int __find_first_equal(BPNode *n, int cur_idx) {
    int ret_idx = cur_idx;
    assert( ret_idx >=0 && ret_idx < n->bp_cnt);
    while ( ret_idx > 0 ) {
        if ( n->bp_values[ret_idx].key == n->bp_values[ret_idx-1].key ) {
            ret_idx--;
        } else {
            break;
        }
    }
    return ret_idx;
}

static inline int __find_last_equal(BPNode *n, int  cur_idx) {
    int ret_idx = cur_idx;
    assert( ret_idx >=0 && ret_idx < n->bp_cnt);
    while ( ret_idx < n->bp_cnt - 1 ) {
        if ( n->bp_values[ret_idx].key == n->bp_values[ret_idx+1].key ) {
            ret_idx++;
        } else {
            break;
        }
    }
    return ret_idx;
}

static inline int __find_idx(BPNode *n, int64_t k) {
    int start, end, mid, ret;
    int64_t mid_key;

    start = 0;
    end = n->bp_cnt - 1;
    while ( end >= start ) {
        mid = (( start + end ) >> 1);
        mid_key = n->bp_values[mid].key;

        if (  mid_key > k ) {
            end = mid_key - 1;
        } else if ( mid_key < k ) {
            start = mid_key + 1;
        } else {
            break;
        }
    }

    if ( end < start ) {
        ret = start;
    } else {
        ret = mid;
    }

    assert( ret >= 0 && ret <= n->bp_cnt);
    if ( ret < n->bp_cnt ) {
        assert( n->bp_values[ret].key >= k );
    }

    return ret;
}

#if 0
static inline void __do_move_bpvalue(BPNode *n, int startidx) {
    int i;
    for ( i=n->bp_cnt; i>startidx; i-- ) {
        memcpy(&n->bp_values[i], &n->bp_values[i-1], sizeof(BPValue));
    }
}
#endif

// insert after cur_idx
static inline void __do_insert_internal(BPTree *tree, BPNode *cur, BPNode *next, int cur_idx, int64_t key) {
    BPNode *parent_node, *new_parent_node;
    int i, left_children_cnt, right_children_cnt;
    int64_t new_key;

    assert( cur_idx < tree->bp_branch_max - 1 );

    if ( cur->parent == NULL ) {
        parent_node = __alloc_node(tree);
        assert( parent_node != NULL );
        assert( cur_idx == 0);
        next->idx_in_pnode = 1;

        parent_node->bp_values[0].key = key; //next->bp_values[0].key;
        parent_node->bp_children[0] = cur;
        parent_node->bp_children[1] = next;
        parent_node->bp_cnt = 1;

        cur->parent = parent_node;
        next->parent = parent_node;

        tree->root = parent_node;
    } else {
        parent_node = cur->parent;
        assert( parent_node->bp_cnt <= tree->bp_branch_max - 1 );
        assert( cur_idx <= parent_node->bp_cnt);
        assert( parent_node->bp_cnt < tree->bp_branch_max - 1 );

#if 0
        if ( parent_node->bp_cnt < tree->bp_branch_max - 1 ) {
#endif
            // do move
            for ( i=parent_node->bp_cnt; i>cur_idx; i--) {
                memcpy(&parent_node->bp_values[i], &parent_node->bp_values[i-1], sizeof(BPValue));
                parent_node->bp_children[i+1] = parent_node->bp_children[i];
                parent_node->bp_children[i+1]->idx_in_pnode++;
                assert(parent_node->bp_children[i+1]->idx_in_pnode == i+1 );
            }
            // do insert node
            parent_node->bp_values[cur_idx].key = key; //next->bp_values[0].key;
            parent_node->bp_values[cur_idx].value = NULL;
            parent_node->bp_children[cur_idx + 1] = next;
            next->idx_in_pnode = cur_idx + 1;
            assert( next->parent == parent_node );

            // update stats
            parent_node->bp_cnt++;

            // check split 
            if ( parent_node->bp_cnt == tree->bp_branch_max - 1 ) {
                new_parent_node = __alloc_node(tree);
                assert( new_parent_node != NULL );
                left_children_cnt = ( ( parent_node->bp_cnt + 1) >> 1 );
                right_children_cnt = ( parent_node->bp_cnt + 1) - left_children_cnt;
                new_key = parent_node->bp_values[left_children_cnt-1].key;

                // setup new parent node
                memcpy( &new_parent_node->bp_values[0], &parent_node->bp_values[left_children_cnt], ( right_children_cnt - 1 ) & sizeof(BPValue));
                memcpy( &new_parent_node->bp_children[0], &parent_node->bp_children[left_children_cnt], right_children_cnt * sizeof(void *));
                // update new_parent_node children's idx_in_pnode
                for ( i=0; i<right_children_cnt; i++ ) {
                    new_parent_node->bp_children[i]->idx_in_pnode = i;
                }

                // update cnt
                new_parent_node->bp_cnt = right_children_cnt - 1;
                parent_node->bp_cnt = left_children_cnt - 1;

                // update list
                new_parent_node->next_sib = parent_node->next_sib;
                new_parent_node->prev_sib = parent_node;
                parent_node->next_sib = new_parent_node;
                if ( new_parent_node->next_sib ) {
                    new_parent_node->next_sib->prev_sib = new_parent_node;
                }

                // update parent
                new_parent_node->parent = parent_node->parent;

                // do recur
                __do_insert_internal(tree, parent_node, new_parent_node, parent_node->idx_in_pnode, new_key) ;
            }
    }

}

static inline void __do_insert_data(BPTree *tree, BPNode *cur, int64_t k, void *v) {
    int ins_before_idx, left_cnt, right_cnt;
    BPNode *new_node;

    // find the idx to insert before
    ins_before_idx = __find_idx(cur, k);

    assert( ins_before_idx < cur->bp_cnt );
    // insert data first
    //__do_move_bpvalue(cur, ins_before_idx);
    memmove(&cur->bp_values[ins_before_idx + 1], &cur->bp_values[cur->bp_cnt], ( cur->bp_cnt - ins_before_idx) * sizeof(BPValue));
    cur->bp_values[ins_before_idx].key = k;
    cur->bp_values[ins_before_idx].value = v;
    cur->bp_cnt++;

    // check split
    if ( cur->bp_cnt == tree->bp_branch_max ) {
        new_node = __alloc_node(tree);
        assert( new_node != NULL );

        left_cnt = (cur->bp_cnt>>1);
        right_cnt = cur->bp_cnt - left_cnt;
        cur->bp_cnt = left_cnt;

        // init new node
        new_node->bp_cnt = right_cnt;
        new_node->next_sib = cur->next_sib;
        new_node->prev_sib = cur;
        new_node->parent = cur->parent;
        new_node->idx_in_pnode = -1;
        //new_node->idx_in_pnode;
        memcpy(new_node->bp_values, &cur->bp_values[left_cnt], right_cnt * sizeof(BPValue));

        // fix list
        if ( new_node->next_sib ) {
            new_node->next_sib->prev_sib = new_node;
        }
        cur->next_sib = new_node;

        // insert into parent
        __do_insert_internal(tree, cur, new_node, cur->idx_in_pnode, new_node->bp_values[0].key);
    } 
}

static inline BPNode *__find_leaf(BPTree *tree, int64_t k) {
    BPNode *cur = tree->root;
    int i;

    // leaves have no child
    while ( cur->bp_children[0] != NULL ) {
        assert( cur->bp_cnt <= tree->bp_branch_max - 1 );

        i = __find_idx(cur, k) ;
#if 0
        for ( i=0; i<cur->bp_cnt; i++ ) {
            if ( cur->bp_values[i].key >= k ) {
                break;
            }
        }
#endif
        /*
           0 | 1 | 2 | 3 | ...
           0 | 1 | 2 | 3 | 4 | ...
         */
        cur = cur->bp_children[i];
    }
    assert( cur->bp_cnt <= tree->bp_branch_max - 1 );
    return cur;
}

int bptree_insert(BPTree *tree, int64_t k, void *v) {
    BPNode *cur ;
    int ref;

    // ref the value first
    ref = tree->bp_get(v);
    assert( ref > 0 );

    tabby_lock_protect(&tree->bp_lock, 0) {
        // make sure root exists
        if ( !tree->root ) {
            tree->root = __alloc_node(tree);
            assert( tree->root != NULL );
        }

        // find the leaf
        cur = __find_leaf(tree, k);
        assert( cur != NULL );

        // do insert
        __do_insert_data(tree, cur, k, v);
    }

    return 0;
}


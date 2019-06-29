#include <tabby/tabby_tree.h>
#include <ncurses/ncurses.h>
#include <tabby/tabby_lock.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int cmp(void *skey, void *dkey) {
    int s, d;
    s = (int)(int64_t)skey;
    d = (int)(int64_t)dkey;

    return s-d;
}

static int get(void *value) {
    return 1;
}

static int put(void *value) {
    return 1;
}

static int det(void *key, void *value) {
    return 0;
}

#define N_RED    1
#define N_BLK    2

void draw_line(int y1, int x1, int y2, int x2) {
    int i, min, max;

    if ( y1 == y2 ) {
        min = (x1<x2)?x1:x2;
        max = (x1>=x2)?x1:x2;
        // horizon
        for ( i=min; i<max; i++ ) {
            //if ( i == max - 1 ) {
            //    mvaddch(y1, i, '+');
            //} else {
                mvaddch(y1, i, '-');
            //}
        }
    } else if ( x1 == x2 ) {
        // vertical
        min = (y1<y2)?y1:y2;
        max = (y1>=y2)?y1:y2;
        for ( i=min; i<max; i++ ) {
            //if ( i==max-1) {
            //    mvaddch(i, x1, '+');
            //} else {
                mvaddch(i, x1, '|');
            //}
        }
    }
}

void draw_polyline(int sline, int scol, int eline, int ecol) {
    //int ldelta, cdelta;
    int y1,x1, y2,x2, y3,x3, y4,x4;

    //ldelta = eline - sline;
    //cdelta = ecol - scol;

    y1 = sline; x1 = scol;
    y4 = eline; x4 = ecol;

    //if ( abs(ldelta) <= abs(cdelta)) {
        y2 = ( y1 + y4 ) / 2; x2 = x1;
        y3 = ( y1 + y4 ) / 2; x3 = x4;
    //} else {
        //y2 = y1; x2 = (x1+x4)/2;
        //y3 = y4; x3 = (x1+x4)/2;
    //}

    //attron(COLOR_PAIR(LINE_COLOR));
    draw_line(y1, x1, y2, x2);
    draw_line(y2, x2, y3, x3);
    draw_line(y3, x3, y4, x4);
    mvaddch(y2, x2, '+');
    mvaddch(y3, x3, '+');
    //attroff(COLOR_PAIR(LINE_COLOR));
}


static void proc(void *key, void *value, int y, int x, int color) {
    int k, pblock_len, block_len, len;
    int my, mx, py, px;
    char buf[10] = {0};

    k = (int)(int64_t)key;
    snprintf(buf, sizeof(buf) - 1, "%d", k);
    len = strlen(buf);

    my = y * 4;
    block_len =  COLS/( 1<<y );
    mx =  block_len * x + block_len/2;

    py = my - 4;
    pblock_len = (COLS)/(1<<(y-1));
    px = pblock_len * ( x /2) + pblock_len/2;

    // draw line
    if ( y > 0 ) {
        draw_polyline(my, mx, py + 1, px); 
    }

    mx -= len/2;
    // draw data
    if ( color == 1 ) {
        attron(COLOR_PAIR(N_RED));
        mvprintw(my, mx, "%s", buf);
        attroff(COLOR_PAIR(N_RED));
    } else {
        attron(COLOR_PAIR(N_BLK));
        mvprintw(my, mx, "%s", buf);
        attroff(COLOR_PAIR(N_BLK));
    }

    //printw("key: %d, y: %d, x: %d, color: %s\n", k, y, x, color==1?"RED":"BLACK");
}

int main() {
    RBTree *tree = NULL;
    char cmd[10];
    int value, ret, tret;

    tree = tabby_rbtree_new(LOCK_NONE, cmp, get, put, det);
    assert( tree != NULL );

    // start ncurses 
    initscr();
    echo();
    start_color();
    init_pair(N_RED, COLOR_RED, COLOR_BLACK);
    init_pair(N_BLK, COLOR_GREEN, COLOR_BLACK);

    do {
        tret = -2;
        ret = mvscanw(LINES -1, 0, "%9s %d", cmd, &value);
        if ( 2 == ret ) {
            if ( strcmp(cmd, "ins") == 0 ) {
                tret = tabby_rbtree_insert(tree, (void *)(int64_t)value, NULL);
            } else if ( strcmp(cmd, "del") == 0 ) {
                tret = tabby_rbtree_delete(tree, (void *)(int64_t)value);
            }
        }
        clear();
        if ( tret == -2 ) {
            mvprintw(LINES-2, 0, "cmd: %s error, type 'ins NUM' to insert, type 'quit' to exit", cmd);
        } else if ( tret == -1 ) {
            mvprintw(LINES-2, 0, "insert %d failed, already exists", value);
        } else {
            mvprintw(LINES-2, 0, "insert %d successfully", value);
        }

        move(0, 0);
        tabby_rbtree_foreach(tree, proc);
        refresh();

    } while ( strcmp(cmd, "quit") != 0 );

    endwin();
    return 0;
}

#if 0
int main() {
    RBTree *tree = NULL;

    tree = tabby_rbtree_new(LOCK_NONE, cmp, get, put, det);
    assert( tree != NULL );

    tabby_rbtree_insert(tree, (void *)1, NULL);
    tabby_rbtree_insert(tree, (void *)2, NULL);
    tabby_rbtree_insert(tree, (void *)3, NULL);
    tabby_rbtree_insert(tree, (void *)4, NULL);
    tabby_rbtree_insert(tree, (void *)5, NULL);

    tabby_rbtree_foreach(tree, proc);

    tabby_rbtree_free(tree);

    return 0;
}
#endif

#include "tabby_atom.h" 
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

static volatile int start = 0;

#define THREADS_CNT (5)
const int g_cnt = 1000000;

static pthread_t s_tids[THREADS_CNT];

void *add_routine(void *arg) {
    Atom32 *p = (Atom32 *)arg;
    int i;

    while (!start);
    for ( i=0; i<g_cnt; i++ ) {
        atom32_inc(p);
    }

    return NULL;

}

int main(int argc, char *argv[]) {
    Atom32 v = {.counter = 0};   
    int i, ret;

    for ( i=0; i<THREADS_CNT; i++ ) {
        ret = pthread_create(&s_tids[i], NULL, add_routine, (void *)&v);
        assert( 0 == ret );
    }

    sleep(1);
    start = 1;

    for ( i=0; i<THREADS_CNT; i++ ) {
        pthread_join(s_tids[i], NULL);
    }

    assert( atom32_read(&v) == THREADS_CNT *g_cnt);

    printf("atom32 reach %d, and exptected value is %d\n",
            atom32_read(&v), THREADS_CNT *g_cnt);

    return 0;
}

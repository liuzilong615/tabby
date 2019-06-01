#include "tabby_atom.h"
#include <pthread.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    int64_t x;
    Atom64 v = {.counter = 0 };

    // test read
    assert( 0 == atom64_read(&v));

    // test set
    atom64_set(&v, 100);
    assert( 100 == atom64_read(&v));

    // test inc
    x = atom64_inc_return(&v);
    assert( 101 == x );
    atom64_inc(&v);
    assert( atom64_read(&v) == 102);

    // test dec
    x = atom64_dec_return(&v);
    assert( 101 == x);
    atom64_dec(&v);
    assert( 100 == atom64_read(&v));

    return 0;
}

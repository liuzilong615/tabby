#include <tabby/tabby_atom.h>
#include <pthread.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    int32_t x;
    Atom32 v = {.counter = 0 };

    // test read
    assert( 0 == atom32_read(&v));

    // test set
    atom32_set(&v, 100);
    assert( 100 == atom32_read(&v));

    // test inc
    x = atom32_inc_return(&v);
    assert( 101 == x );
    atom32_inc(&v);
    assert( atom32_read(&v) == 102);

    // test dec
    x = atom32_dec_return(&v);
    assert( 101 == x);
    atom32_dec(&v);
    assert( 100 == atom32_read(&v));

    return 0;
}

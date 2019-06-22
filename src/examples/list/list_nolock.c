#include <tabby/tabby_list.h>
#include <stdio.h>

static void node_process(void *data) {
    int64_t v = (int64_t)data;
    printf("v: %ld\n", v);
}

int main(int argc, char *argv[]) {
    int ret;
    List *l = tabby_list_new(LOCK_NONE);
    assert( l!= NULL );

    ret = tabby_list_append(l, (void *)1);
    assert( 0 == ret );

    ret = tabby_list_append(l, (void *)2);
    assert( 0 == ret );

    ret = tabby_list_prepend(l, (void *)3);
    assert( 0 == ret );

    ret = tabby_list_prepend(l, (void *)4);
    assert( 0 == ret );

    printf("====== show all =======\n");
    ret = tabby_list_foreach(l, node_process);
    assert( 4 == ret );

    ret = (int64_t)tabby_list_remove(l, (void *)1);
    assert( 0 == ret );

    ret = (int64_t)tabby_list_remove(l, (void *)7);
    assert( 0 != ret );

    printf("====== show after remove 1 ========\n");
    ret = tabby_list_foreach(l, node_process);
    assert( 3 == ret );


    ret = (int64_t)tabby_list_detach(l);
    assert( 2 == ret );

    ret = (int64_t)tabby_list_predetach(l);
    assert( 4 == ret );

    printf("====== show after remove 2, 4 ========\n");
    ret = tabby_list_foreach(l, node_process);
    assert( 1 == ret );

    ret = (int64_t)tabby_list_detach(l);
    assert( ret == 3 );

    ret = (int64_t)tabby_list_detach(l);
    assert( ret == 0 );

    ret = (int64_t)tabby_list_predetach(l);
    assert( ret == 0 );

    printf("====== show empty ========\n");
    ret = tabby_list_foreach(l, node_process);
    assert( 0 == ret );

    return 0;
}

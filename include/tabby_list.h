#ifndef _TABBY_LIST_H
#define _TABBY_LIST_H

#include "tabby_types.h"
#include "tabby_lock.h"
#include "tabby_assert.h"

typedef struct _List List;

typedef void (*ListNodeProcess)(void *data) ;

List *tabby_list_new(LockType type) ;
void tabby_list_free(List *l) ;
int tabby_list_append(List *l, void *data) ;
int tabby_list_prepend(List *l, void *data) ;
int tabby_list_del(List *l, void *data) ;

int tabby_list_foreach(List *l, ListNodeProcess proc);

#endif

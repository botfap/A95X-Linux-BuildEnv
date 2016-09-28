/********************************************
 * name : player_itemlis.c
 * function: item  fifo manage  for muti threads
 * date     : 2011.3.23
 * author :zhouzhi
 ********************************************/

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <itemlist.h>




int itemlist_init(struct itemlist *itemlist)
{
    itemlist->item_count = 0;
    INIT_LIST_HEAD(&itemlist->list);
    ITEM_LOCK_INIT(itemlist);
    return 0;
}

struct item * item_alloc(int ext) {
    return malloc(sizeof(struct item) + ext);
}


void item_free(struct item *item)
{
    free(item);
}


int itemlist_add_tail(struct itemlist *itemlist, struct item *item)
{
    ITEM_LOCK(itemlist);
    if (itemlist->max_items > 0 && itemlist->max_items <= itemlist->item_count) {
        ITEM_UNLOCK(itemlist);
        return -1;
    }
    list_add_tail(&item->list, &itemlist->list);
    itemlist->item_count++;
    ITEM_UNLOCK(itemlist);
    return 0;
}

struct item *  itemlist_get_head(struct itemlist *itemlist) {
    struct item *item = NULL;
    struct list_head *list = NULL;

    ITEM_LOCK(itemlist);
    if (!list_empty(&itemlist->list)) {
        list = itemlist->list.next;
        item = list_entry(list, struct item, list);
        list_del(list);
        itemlist->item_count--;
    }
    ITEM_UNLOCK(itemlist);
    return item;
}

struct item *  itemlist_get_tail(struct itemlist *itemlist) {
    struct item *item = NULL;
    struct list_head *list = NULL;

    ITEM_LOCK(itemlist);
    if (!list_empty(&itemlist->list)) {
        list = itemlist->list.prev;
        item = list_entry(list, struct item, list);
        list_del(list);
        itemlist->item_count--;
    }
    ITEM_UNLOCK(itemlist);
    return item;
}


struct item * itemlist_peek_head(struct itemlist *itemlist) {
    struct item *item = NULL;
    struct list_head *list = NULL;

    ITEM_LOCK(itemlist);
    if (!list_empty(&itemlist->list)) {
        list = itemlist->list.next;
        item = list_entry(list, struct item, list);
    }
    ITEM_UNLOCK(itemlist);
    return item;
}

struct item * itemlist_peek_tail(struct itemlist *itemlist) {
    struct item *item = NULL;
    struct list_head *list = NULL;

    ITEM_LOCK(itemlist);
    if (!list_empty(&itemlist->list)) {
        list = itemlist->list.prev;
        item = list_entry(list, struct item, list);
    }
    ITEM_UNLOCK(itemlist);
    return item;
}

int  itemlist_del_item(struct itemlist *itemlist, struct item *item)
{
    ITEM_LOCK(itemlist);
    list_del(&item->list);
    itemlist->item_count--;
    ITEM_UNLOCK(itemlist);
    return 0;
}

int  itemlist_clean(struct itemlist *itemlist, data_free_fun free_fun)
{
    struct item *item = NULL;
    struct list_head *llist, *tmplist;
    ITEM_LOCK(itemlist);
    list_for_each_safe(llist, tmplist, &itemlist->list) {
        item = list_entry(llist, struct item, list);
        if (free_fun != NULL && item->item_data != 0) {
            free_fun((void *)item->item_data);
        }
        list_del(llist);
        item_free(item);
        itemlist->item_count--;
    }
    ITEM_UNLOCK(itemlist);
    return 0;
}

struct item *  itemlist_get_match_item(struct itemlist *itemlist, unsigned long data) {
    struct item *item = NULL;
    struct list_head *llist, *tmplist;
    struct item *finditem = NULL;
    ITEM_LOCK(itemlist);
    list_for_each_safe(llist, tmplist, &itemlist->list) {
        item = list_entry(llist, struct item, list);
        if (item->item_data == data) {
            finditem = item;
            break;
        }
    }
    if (finditem != NULL) {
        list_del(&finditem->list);
        itemlist->item_count--;
    }
    ITEM_UNLOCK(itemlist);
    return finditem;
}

struct item *  itemlist_find_match_item(struct itemlist *itemlist, unsigned long data) {
    struct item *item = NULL;
    struct list_head *llist, *tmplist;
    struct item *finditem = NULL;
    ITEM_LOCK(itemlist);
    list_for_each_safe(llist, tmplist, &itemlist->list) {
        item = list_entry(llist, struct item, list);
        if (item->item_data == data) {
            finditem = item;
            break;
        }
    }
    ITEM_UNLOCK(itemlist);
    return finditem;
}

/*
we think the item->data is grow,
we find the first item great or equal  item->data;
*/
struct item *  itemlist_find_match_item_ex(struct itemlist *itemlist,struct item *tomatch,item_is_match_fun match,int reveser) {
    struct item *item = NULL;
    struct list_head *llist, *tmplist;
    struct item *finditem = NULL;
    ITEM_LOCK(itemlist);
    if(reveser){
        list_for_each_entry_reverse(item, &itemlist->list,list) {
            if (match(item,tomatch)) {
                finditem = item;
                break;
            }
        }
    }else{
        list_for_each_entry(item, &itemlist->list,list) {
           if (match(item,tomatch)) {
               finditem = item;
               break;
           }
       }
    }
    	
    ITEM_UNLOCK(itemlist);
    return finditem;
}

int itemlist_add_tail_data(struct itemlist *itemlist, unsigned long data)
{
    struct item *item;
    if (itemlist->reject_same_item_data && itemlist_have_match_data(itemlist, data)) {
        return 0;    /*have matched in list*/
    }
    item = item_alloc(itemlist->item_ext_buf_size);
    if (item == NULL) {
        return -12;//noMEM
    }
    item->item_data = data;
    if (itemlist_add_tail(itemlist, item) != 0) {
        item_free(item);
        return -1;
    }
    return 0;
}
int  itemlist_get_head_data(struct itemlist *itemlist, unsigned long *data)
{
    struct item *item = NULL;
    item = itemlist_get_head(itemlist);
    if (item != NULL) {
        *data = item->item_data;
        item_free(item);
        return 0;
    } else {
        return -1;
    }
}

int itemlist_get_tail_data(struct itemlist *itemlist, unsigned long *data)
{
    struct item *item = NULL;
    item = itemlist_get_tail(itemlist);
    if (item != NULL) {
        *data = item->item_data;
        return 0;
    } else {
        return -1;
    }
}


int itemlist_peek_head_data(struct itemlist *itemlist, unsigned long *data)
{
    struct item *item = NULL;
    item = itemlist_peek_head(itemlist);
    if (item != NULL) {
        *data = item->item_data;
        return 0;
    } else {
        return -1;
    }
}




int itemlist_peek_tail_data(struct itemlist *itemlist, unsigned long *data)
{
    struct item *item = NULL;
    item = itemlist_peek_tail(itemlist);
    if (item != NULL) {
        *data = item->item_data;
        return 0;
    } else {
        return -1;
    }
}


int itemlist_clean_data(struct itemlist *itemlist, data_free_fun free_fun)
{
    return itemlist_clean(itemlist, free_fun);
}

int itemlist_have_match_data(struct itemlist *itemlist, unsigned long data)
{
    struct item *item = NULL;
    item = itemlist_find_match_item(itemlist, data);
    return item != NULL;
}


int itemlist_del_match_data_item(struct itemlist *itemlist, unsigned long data)
{
    struct item *item = NULL;
    item = itemlist_get_match_item(itemlist, data);
    if (item) {
        item_free(item);
        return 0;
    }
    return -1;
}

/*
postion must in the itemlist.
flags: 1: before position;
	  2: after postion;
	  3: replace postion;
	  else:
	  	2:after postion;
*/
int itemlist_item_insert(struct itemlist *itemlist, struct itemlist *position,struct itemlist *newitem,int flags)
{
    ITEM_LOCK(itemlist);
    if (flags!=3 && itemlist->max_items > 0 && itemlist->max_items <= itemlist->item_count) {
        ITEM_UNLOCK(itemlist);
        return -1;
    }
    if(flags==1)
        list_add_tail(&newitem->list, &position->list);
    else
        list_add(&newitem->list, &position->list);
    if(flags==3)
        list_del(&position->list);
    else
        itemlist->item_count++;
    ITEM_UNLOCK(itemlist);
    return 0;	
}

int  itemlist_print(struct itemlist *itemlist,printitem_fun print)
{
    struct item *item = NULL;
    struct list_head *llist, *tmplist;
    ITEM_LOCK(itemlist);
    list_for_each_safe(llist, tmplist, &itemlist->list) {
        item = list_entry(llist, struct item, list);
        print(item);
    }
    ITEM_UNLOCK(itemlist);
    return 0;
}


/******
*  init date: 2013.1.23
*  author: senbai.tao<senbai.tao@amlogic.com>
*  description: list used for curl-mod
******/

#include "curl_list.h"

list_t * curl_list_create()
{
	list_t * lst = (list_t *)malloc(sizeof(list_t));
	lst->size = 0;
	lst->head = lst->tail = NULL;
	lst->multi_thread_flag = 1;
	CURL_LIST_INIT(lst);
	return lst;
}

void curl_list_destroy(list_t * lst, curl_list_free_callback pfunc)
{
	curl_list_clear(lst, pfunc);
	CURL_LIST_LOCK(lst);
	if(lst) {
		free(lst);
		lst = NULL;
	}
	CURL_LIST_UNLOCK(lst);
	CURL_LIST_DESTROY(lst);
}

listnode_t * curl_listnode_create(void * data)
{
	listnode_t * node = (listnode_t *)malloc(sizeof(listnode_t));
	node->next = NULL;
	node->data = data;
	return node;
}

void curl_listnode_destroy(listnode_t * node, curl_list_free_callback pfunc)
{
	if(!node) {
		return;
	}
	listnode_t * next;
	while(node) {
		next = node->next;
		if(pfunc) {
			(*pfunc)(node);
		}
		free(node);
		node = next;
	}
}

void curl_list_push_back(list_t * lst, listnode_t * node)
{
	if(!lst) {
		return;
	}
	node->next = NULL;
	CURL_LIST_LOCK(lst);
	if(lst->head) {
		lst->tail->next = node;
		lst->tail = node;
	} else {
		lst->head = lst->tail = node;
	}
	lst->size++;
	CURL_LIST_UNLOCK(lst);
}

listnode_t * curl_list_pop_front(list_t * lst)
{
	if(!lst) {
		return NULL;
	}
	listnode_t * pop = NULL;
	CURL_LIST_LOCK(lst);
	if(lst->head) {
		pop = lst->head;
		lst->head = lst->head->next;
		if(!lst->head) {
			lst->tail = NULL;
		}
		pop->next = NULL;
		lst->size--;
	}
	CURL_LIST_UNLOCK(lst);
	return pop;
}

void curl_list_pop_node(list_t * lst, listnode_t * node)
{
	if(!lst) {
		return;
	}
	CURL_LIST_LOCK(lst);
	listnode_t * prev = lst->head;
	if(prev == node) {
		lst->head = prev->next;
		if(prev == lst->tail) {
			lst->tail = NULL;
		}
		node->next = NULL;
		curl_listnode_destroy(node, 0);
		lst->size--;
		return;
	}
	while(prev && prev->next != node) {
		prev = prev->next;
	}
	if(prev) {
		prev->next = node->next;
		node->next = NULL;
		if(lst->tail == node) {
			lst->tail = prev;
		}
		curl_listnode_destroy(node, 0);
		lst->size--;
	}
	CURL_LIST_UNLOCK(lst);
}

void curl_list_peek_each(list_t * lst, curl_list_peek_callback pfunc)
{
	if(!lst) {
		return;
	}
	CURL_LIST_LOCK(lst);
	if(pfunc) {
		listnode_t * node = lst->head;
		while(node) {
			(*pfunc)(node->data);
			node = node->next;
		}
	}
	CURL_LIST_UNLOCK(lst);
}

void curl_list_clear(list_t * lst, curl_list_free_callback pfunc)
{
	listnode_t * node;
	if(pfunc) {
		while((node = curl_list_pop_front(lst))) {
			(*pfunc)(node);
			free(node);
		}
	} else {
		while((node = curl_list_pop_front(lst))) {
			free(node);
		}
	}
}

size_t curl_list_size(const list_t * lst)
{
	if(!lst) {
		return 0;
	}
	return lst->size;
}
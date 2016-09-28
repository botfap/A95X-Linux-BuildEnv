#ifndef CURL_LIST_H_
#define CURL_LIST_H_

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct _curl_listnode_t {
	struct _curl_listnode_t * next;
	union{
		void * data;
		struct _curl_list_t * list;
		const char * str;
	};
}listnode_t;

typedef struct _curl_list_t {
	size_t size;
	int multi_thread_flag;
	pthread_mutex_t list_mutex;
	listnode_t * head;
	listnode_t * tail;
}list_t, * list_ptr;

#define CURL_LIST_LOCK(lst)\
	if(lst->multi_thread_flag)\
		pthread_mutex_lock(&lst->list_mutex);

#define CURL_LIST_UNLOCK(lst)\
	if(lst->multi_thread_flag)\
		pthread_mutex_unlock(&lst->list_mutex);

#define CURL_LIST_INIT(lst)\
	pthread_mutex_init(&lst->list_mutex, NULL);

#define CURL_LIST_DESTROY(lst)\
	pthread_mutex_destroy(&lst->list_mutex);

typedef void(*curl_list_free_callback)(listnode_t * node);
typedef void(*curl_list_peek_callback)(void * data);

list_t * curl_list_create();
void curl_list_destroy(list_t * lst, curl_list_free_callback pfunc);
listnode_t * curl_listnode_create(void * data);
void curl_listnode_destroy(listnode_t * node, curl_list_free_callback pfunc);
void curl_list_push_back(list_t * lst, listnode_t * node);
listnode_t * curl_list_pop_front(list_t * lst);
void curl_list_pop_node(list_t * lst, listnode_t * node);
void curl_list_peek_each(list_t * lst, curl_list_peek_callback pfunc);
void curl_list_clear(list_t * lst, curl_list_free_callback pfunc);
size_t curl_list_size(const list_t * lst);

#endif
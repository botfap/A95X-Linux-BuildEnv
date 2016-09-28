
#include "avformat.h"
#include <unistd.h>
#include <errno.h>
#include "os_support.h"
#include "url.h"
#include <sys/time.h>

#include <itemlist.h>
#include "tcp_pool.h"

static struct itemlist tcppool_list;
static struct itemlist tcppool_list_free;/*wait next cmd.*/

int tcppool_init(void)
{
	static int inited=0;
	if(!inited){
        tcppool_list.max_items = 0;
        tcppool_list.item_ext_buf_size = 0;
        tcppool_list.muti_threads_access = 1;
        tcppool_list.reject_same_item_data = 1;
        itemlist_init(&tcppool_list);

		tcppool_list_free.max_items = 0;
        tcppool_list_free.item_ext_buf_size = 0;
        tcppool_list_free.muti_threads_access = 1;
        tcppool_list_free.reject_same_item_data = 1;
        itemlist_init(&tcppool_list_free);
	}
	inited++;
	return 0;
}

struct tcppool_link{
	URLContext *h;
	const char *uri;
	int flags;
	int64_t opendtime_us;
	int64_t lastreleasetime_us;
};

int tcppool_opened_tcplink(URLContext *h, const char *uri, int flags)
{
    struct item *item =item_alloc(sizeof(struct tcppool_link));
    struct tcppool_link *link;
    if(!item)
        return -1;
	link=(struct tcppool_link *)&item->extdata[0];
	memset(link,0,sizeof(struct tcppool_link));
    item->item_data=h;
    link->h =h;
    link->uri=strdup(uri);
    link->flags=flags;
	link->opendtime_us=av_gettime();
    itemlist_add_tail(&tcppool_list,item);
	return 0;
}


int tcppool_release_tcplink(URLContext *h)
{
	struct item *item=itemlist_get_match_item(&tcppool_list,h);
	struct tcppool_link *link;
	if(!item)
		return -1;
	link=(struct tcppool_link *)&item->extdata[0];
	link->lastreleasetime_us = av_gettime();
	itemlist_add_tail(&tcppool_list_free,item);
	av_log(NULL,AV_LOG_INFO,"tcppool:release_tcplink %s :\n",link->uri);
	return 0;
}

static int machlink_check(struct item *item,struct item *tomatchitem)
{
	struct tcppool_link *link1,*link2;
	link1=(struct tcppool_link *)&item->extdata[0];
	link2=(struct tcppool_link *)&tomatchitem->extdata[0];
	return (link1->flags==link2->flags)&&!strcmp(link1->uri,link2->uri);
	
}

int tcppool_find_free_tcplink(URLContext **h, const char *uri, int flags)
{
	struct item *tomachitem =item_alloc(sizeof(struct tcppool_link));
	struct tcppool_link *link;
	struct item *item;
	if(!tomachitem)
		return -1;
	link=(struct tcppool_link *)&tomachitem->extdata[0];
	link->flags=flags;
	link->uri=uri;
	item=itemlist_find_match_item_ex(&tcppool_list_free,tomachitem,machlink_check,0);
	if(item){
		int64_t curtime = av_gettime();
		item_free(tomachitem);
		itemlist_del_item(&tcppool_list_free,item);
		link=(struct tcppool_link *)&item->extdata[0];
		if((int)(curtime - link->lastreleasetime_us) < 10*1000*1000){/*10S for default timeout*/
			*h=link->h;
		    itemlist_add_tail(&tcppool_list,item);
		    av_log(NULL,AV_LOG_INFO,"tcppool:Get free tcp link %s : freed time =%d us \n",uri,(int)(curtime-link->lastreleasetime_us));
		    return 0;
		}
		av_log(NULL,AV_LOG_INFO,"tcppool:deled timeout link :%s:outtime:%d\n",uri,(int)(curtime-link->lastreleasetime_us));
		tcppool_close_itemlink(item);
		return -1;
	}
	item_free(tomachitem);
	return -2;
}

int tcppool_close_itemlink(struct item *linkitem)
{
	struct tcppool_link *link=(struct tcppool_link *)&linkitem->extdata[0];
	av_log(NULL,AV_LOG_INFO,"tcppool:del tcp link list:%s\n",link->uri);
	ffurl_close(link->h);
	av_free(link->uri);
	item_free(linkitem);
	return 0;
}

int tcppool_close_tcplink(URLContext *h)
{
	struct item *item=itemlist_get_match_item(&tcppool_list,h);
	if(!item){
		struct item *item=itemlist_get_match_item(&tcppool_list_free,h);
		if(!item){
		    return -1;
		}
	}
	tcppool_close_itemlink(item);
	return 0;
}
int tcppool_refresh_link_and_check(void)
{
	struct tcppool_link *link;
	struct item *item;
	int64_t curtime = av_gettime();
	FOR_EACH_ITEM_IN_ITEMLIST(&tcppool_list_free, item) {
		link = (struct tcppool_link *)&item->extdata[0];
	    if((int)(curtime - link->lastreleasetime_us) > 10*1000*1000){/*10S for default timeout*/
			av_log(NULL,AV_LOG_INFO,"tcppool:deled timeout link :%s:outtime:%d\n",link->uri,(int)(curtime-link->lastreleasetime_us));
			itemlist_del_item_locked(&tcppool_list_free,item);
			tcppool_close_itemlink(item);
	    }
    }
    FOR_ITEM_END(&tcppool_list_free);
	return 0;
}
int tcppool_dump_links_info(void)
{
	struct tcppool_link *link;
	struct item *item;
	av_log(NULL,AV_LOG_INFO,"tcppool:active tcp link list:\n");
	FOR_EACH_ITEM_IN_ITEMLIST(&tcppool_list, item) {
        link = (struct tcppool_link *)&item->extdata[0];
		av_log(NULL,AV_LOG_INFO,"\ttcppool:link:%s",link->uri);
		av_log(NULL,AV_LOG_INFO,"\ttcppool:flags:%d",link->flags);
		av_log(NULL,AV_LOG_INFO,"\ttcppool:opendtime:%lld us",link->opendtime_us);
		av_log(NULL,AV_LOG_INFO,"\ttcppool:lastreleasetime_us:%lld u",link->lastreleasetime_us);
    }
    FOR_ITEM_END(&tcppool_list);
	av_log(NULL,AV_LOG_INFO,"tcppool:free tcp link list:\n");
	FOR_EACH_ITEM_IN_ITEMLIST(&tcppool_list_free, item) {
        link = (struct tcppool_link *)&item->extdata[0];
		av_log(NULL,AV_LOG_INFO,"\ttcppool:link:%s",link->uri);
		av_log(NULL,AV_LOG_INFO,"\ttcppool:flags:%d",link->flags);
		av_log(NULL,AV_LOG_INFO,"\ttcppool:opendtime:%lld us",link->opendtime_us);
		av_log(NULL,AV_LOG_INFO,"\ttcppool:lastreleasetime_us:%lld us",link->lastreleasetime_us);
    }
    FOR_ITEM_END(&tcppool_list_free);
	return 0;
}



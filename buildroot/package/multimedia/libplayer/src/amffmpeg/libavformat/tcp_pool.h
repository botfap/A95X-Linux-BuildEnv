
#ifndef TCP_POOL_HEADER____
#define TCP_POOL_HEADER____



int tcppool_init(void);
int tcppool_opened_tcplink(URLContext *h, const char *uri, int flags);
int tcppool_release_tcplink(URLContext *h);
int tcppool_find_free_tcplink(URLContext **h, const char *uri, int flags);
int tcppool_close_itemlink(struct item *linkitem);
int tcppool_close_tcplink(URLContext *h);
int tcppool_refresh_link_and_check(void);
int tcppool_dump_links_info(void);

#endif

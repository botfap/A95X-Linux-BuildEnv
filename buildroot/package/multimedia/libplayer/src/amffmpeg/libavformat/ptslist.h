#ifndef PTSLIST_HEAD_HH___
#define PTSLIST_HEAD_HH___
#include <itemlist.h>

#define MAX_PTS_ITEM (1024*8)

typedef struct ptslist_mgr {
    int         index;
    int64_t lastoffset;
    struct itemlist ptsitem;
} ptslist_mgr_t;

ptslist_mgr_t *ptslist_alloc(int flags);
int ptslist_free(ptslist_mgr_t *mgr);
int ptslist_chekin(ptslist_mgr_t *mgr, int64_t off, int64_t pts);
int ptslist_lookup(ptslist_mgr_t *mgr, int64_t off, int64_t *pts, int *margin);
int ptslist_dump_all(ptslist_mgr_t *mgr);
#endif


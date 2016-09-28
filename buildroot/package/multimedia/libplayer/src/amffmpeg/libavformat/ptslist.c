#include "avformat.h"
#include "ptslist.h"
#define ITEM_OFF(item)    ITEM_EXT64(item,0)
#define ITEM_PTS(item)      ITEM_EXT64(item,1)

ptslist_mgr_t *ptslist_alloc(int flags)
{
    ptslist_mgr_t *mgr = av_mallocz(sizeof(ptslist_mgr_t));
    if (!mgr) {
        av_log(NULL, AV_LOG_INFO, "alloc_ptslist-fialed,no memory!\n");
        return NULL;
    }
    flags = flags;
    mgr->ptsitem.max_items = MAX_PTS_ITEM;
    mgr->ptsitem.reject_same_item_data = 0;
    mgr->ptsitem.item_ext_buf_size = 16; /*two int64_t */
    itemlist_init(&mgr->ptsitem);
    return mgr;
}
int ptslist_free(ptslist_mgr_t *mgr)
{
    itemlist_clean(&mgr->ptsitem, NULL);
    av_free(mgr);
    return 0;
}
int ptslist_chekin(ptslist_mgr_t *mgr, int64_t off, int64_t pts)
{
    struct item *newitem;
    int ret;
    if (off < 0 || pts < 0) {
        return -1;    //not valied off and pts;
    }
    newitem = item_alloc(mgr->ptsitem.item_ext_buf_size);
    if (!newitem) {
        av_log(NULL, AV_LOG_INFO, "ptslist_chekin-fialed,no memory!\n");
        return AVERROR(ENOMEM);
    }
    newitem->item_data = mgr->index++;
    ITEM_OFF(newitem) = off;
    ITEM_PTS(newitem) = pts;
    mgr->lastoffset = off;
    ret = itemlist_add_tail(&mgr->ptsitem, newitem);
    if (ret) { /*item list fulled,del oldest one.*/
        struct item *t;
        t = itemlist_get_head(&mgr->ptsitem);
        if (t) {
            item_free(t);
        }
        ret = itemlist_add_tail(&mgr->ptsitem, newitem);
    }
    return ret;
}




static int ptslist_is_wanted(struct item *item, struct item *tomatchitem)
{
    struct item *prev;

    if ((ITEM_OFF(item) >= ITEM_OFF(tomatchitem)) &&
        ((ITEM_OFF(PREV_ITEM(item)) < ITEM_OFF(tomatchitem)) ||  /*prev is little*/
         (ITEM_OFF(PREV_ITEM(item)) > ITEM_OFF(item)))) { /*item is the head.*/
        return 1;
    } else {
        return 0;
    }
}
/*
margin:
    in:
        <0,any,
        0,real matched,
        >0: pts-off>=off && (pts->off-off)<=margin
    out:
        (pts->off-off)

pts:
    out;
*/
int ptslist_lookup(ptslist_mgr_t *mgr, int64_t off, int64_t *pts, int *margin)
{
    struct item *temp;
    struct item *find;
    int reverse = 0;
    int omargin = *margin;
    int err = -1;
    temp = item_alloc(mgr->ptsitem.item_ext_buf_size);
    if (!temp) {
        return -1;
    }
    ITEM_OFF(temp) = off;
    if (mgr->lastoffset - off < off) {
        reverse = 1;
    }
    find = itemlist_find_match_item_ex(&mgr->ptsitem, temp, ptslist_is_wanted, reverse);
    if (!find) {
        err = -2;
        goto errout;
    }
    *pts = ITEM_PTS(find);
    *margin = (int)(ITEM_OFF(find) - off);
    if (omargin < 0 || *margin <= omargin) {
        err = -0;
    } else {
        err = -3;
    }
errout:
    item_free(temp);
    return err;
}
static int printitem(struct item *item)
{
    av_log(NULL, AV_LOG_INFO, "pts item:[%06d:%08lld:%08lld] \n", item->item_data, ITEM_OFF(item), ITEM_PTS(item));
    return 0;
}

int ptslist_dump_all(ptslist_mgr_t *mgr)
{
    return itemlist_print(&mgr->ptsitem, printitem);
}

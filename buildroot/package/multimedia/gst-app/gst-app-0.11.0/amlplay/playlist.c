/*
 * Copyright (C) 2009-2010 Freescale Semiconductor, Inc. All rights reserved.
 *
 */
 
/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * =====================================================================================
 *
 *       Filename:  playlist.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/06/2009 02:17:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dr. Fritz Mehner (mn), mehner@fh-swf.de
 *        Company:  FH Südwestfalen, Iserlohn
 *
 * =====================================================================================
 */
#include <stdio.h>
#include "playlist.h"

#define MEM_ALLOC(size) malloc((size))
#define MEM_FREE(ptr) free((ptr))
#define MEM_ZERO(ptr,size) memset((ptr),0,(size))
#define PL_ERR printf

#define DEFAULT_PL_TITLE "default"

typedef struct _PlayList PlayList;

typedef struct _PlayItemCtl{
    PlayItem pi;
    void * buffer;
    PlayList * pl;
    struct _PlayItemCtl * prev;
    struct _PlayItemCtl * next;
}PlayItemCtl;


struct _PlayList{
    PlayItemCtl * head;
    PlayItemCtl * tail;
    fsl_player_s8 * title;
};

static void destroyPlayItemCtl(PlayItemCtl * item)
{
    if (item->buffer)
        MEM_FREE(item->buffer);
    MEM_FREE(item);
}

void * 
createPlayList(char * title)
{
    PlayList * pl = MEM_ALLOC(sizeof(PlayList));
    if (pl==NULL){
        PL_ERR("%s failed, no memory!\n", __FUNCTION__);
        goto err;
    }

    MEM_ZERO(pl, sizeof(PlayList));

    if (title==NULL)
        title = DEFAULT_PL_TITLE;

    
    pl->title = MEM_ALLOC(strlen(title)+1);
    if (pl->title==NULL){
        PL_ERR("%s failed, no memory!\n");
        goto err;
    }
    strcpy(pl->title, title);
    
    
    return (void *)pl;
err:
    if (pl){
        MEM_FREE(pl);
        pl=NULL;
    }
    return (void *)pl;
}

PlayItem * 
addItemAtTail(void * hdl, fsl_player_s8 * iName, fsl_player_s32 copy)
{
    PlayItemCtl * item = NULL;
    PlayList * pl = (PlayList *)hdl;
    
    if ((pl==NULL)||(iName==NULL)){
        PL_ERR("%s failed, parameters error!\n", __FUNCTION__);
        goto err;
    }

    item = MEM_ALLOC(sizeof(PlayItemCtl));
    if (item==NULL){
        PL_ERR("%s failed, no memory!\n");
        goto err;
    }

    MEM_ZERO(item, sizeof(PlayItemCtl));

    if (copy){
        item->buffer = MEM_ALLOC(strlen(iName));
        if (item->buffer==NULL){
            PL_ERR("%s failed, no memory!\n");
            goto err;
        }
        strcpy(item->buffer, iName);
        item->pi.name = item->buffer;
    }else{
        item->pi.name = iName;
    }

    item->pl = pl;

    item->prev = pl->tail;
    
    if (pl->head){
        pl->tail->next = item;
        pl->tail= item;
    }else{
        pl->head = pl->tail = item;
    }
    return (PlayItem *)item;
err:
    if (item){
        MEM_FREE(item);
        item=NULL;
    }
    return (PlayItem *)item;
}

PlayItem * 
getFirstItem(void * hdl)
{
    PlayList * pl = (PlayList *)hdl;
    if (pl==NULL){
        PL_ERR("%s failed, parameters error!\n", __FUNCTION__);
        return NULL;
    }
    return (PlayItem *)(pl->head);
}

PlayItem * 
getLastItem(void * hdl)
{
    PlayList * pl = (PlayList *)hdl;
    if (pl==NULL){
        PL_ERR("%s failed, parameters error!\n", __FUNCTION__);
        return NULL;
    }
    return (PlayItem *)(pl->tail);
}

PlayItem * 
getPrevItem(PlayItem * itm)
{
    PlayItemCtl * item = (PlayItemCtl *)itm;
    if (item==NULL){
        PL_ERR("%s failed, parameters error!\n", __FUNCTION__);
        return NULL;
    }
    return (PlayItem *)(item->prev);
}

PlayItem * 
getNextItem(PlayItem * itm)
{
    PlayItemCtl * item = (PlayItemCtl *)itm;
    if (item==NULL){
        PL_ERR("%s failed, parameters error!\n", __FUNCTION__);
        return NULL;
    }
    return (PlayItem *)(item->next);
}

void 
destroyPlayList(void * hdl)
{
    PlayList * pl = (PlayList *)hdl;
    PlayItemCtl * item, *itemnext;

    if (pl==NULL){
        PL_ERR("%s failed, parameters error!\n", __FUNCTION__);
        return;
    }

     if (pl->title){
      MEM_FREE(pl->title);
     }

    item = pl->head;
    while(item){
        itemnext = item->next;
        destroyPlayItemCtl(item);
        item=itemnext;
    }
    MEM_FREE(pl);
}



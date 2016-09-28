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
 *       Filename:  playlist.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/06/2009 02:17:14 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dr. Fritz Mehner (mn), mehner@fh-swf.de
 *        Company:  FH Südwestfalen, Iserlohn
 *
 * =====================================================================================
 */
#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

#include "fsl_player_types.h"

typedef struct {
    fsl_player_s8 * name;
} PlayItem;

void * createPlayList(fsl_player_s8 * title);
PlayItem * addItemAtTail(void * hdl, fsl_player_s8 * iName, fsl_player_s32 copy);
PlayItem * getFirstItem(void * hdl);
PlayItem * getLastItem(void * hdl);
PlayItem * getPrevItem(PlayItem * itm);
PlayItem * getNextItem(PlayItem * itm);
void destroyPlayList(void * hdl);
 
#endif


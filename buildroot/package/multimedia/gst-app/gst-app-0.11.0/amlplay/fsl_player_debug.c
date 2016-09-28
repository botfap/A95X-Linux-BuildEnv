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

//#include "fsl_player_defs.h"
//#include "fsl_player_types.h"
#include "fsl_player_debug.h"

/****** <Macros> ***************************/
//#define FSL_PLAYER_DEBUG

/****** <Typedefs> *************************/

/****** <Global Variables> *****************/
FILE *fsl_player_logfile = NULL;

#ifdef FSL_PLAYER_DEBUG
static fsl_player_s32 fsl_player_debug_level = LEVEL_DEBUG;
#endif /* FSL_PLAYER_DEBUG */

/* get current debug level */
fsl_player_s32 fsl_player_get_debug_level (void)
{
#ifdef FSL_PLAYER_DEBUG
    return fsl_player_debug_level;
#else
    return 0;
#endif /* FSL_PLAYER_DEBUG */
}

/* set current debug level */
void fsl_player_set_debug_level (fsl_player_s32 dbg_lvl)
{
#ifdef FSL_PLAYER_DEBUG
    fsl_player_debug_level = dbg_lvl;
    FSL_PLAYER_MESSAGE (LEVEL_NONE, "FSL PLAYER debug level set to %d\n", dbg_lvl);
#endif /* FSL_PLAYER_DEBUG */
    return;
}

void fsl_player_set_logfile (fsl_player_s8 *logfile)
{
    FILE *logfd = NULL;

    if (logfile == NULL)
        return;

    if (fsl_player_logfile != NULL && fsl_player_logfile != stdout)
    {
        fclose (fsl_player_logfile);
        fsl_player_logfile = NULL;
    }

    logfd = fopen(logfile, /*"w"*/"a");
    if(logfd != NULL)
        fsl_player_logfile = logfd;

    FSL_PLAYER_PRINT("\n\n\n%s(): filename(%s).\n", __FUNCTION__, logfile);
    
    return;
}


/*
 * AMLOGIC Audio/Video streaming port driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#ifndef VFRAME_PROVIDER_H
#define VFRAME_PROVIDER_H

#include <linux/amports/vframe.h>

typedef struct vframe_states {
	int vf_pool_size;
	int fill_ptr;
	int get_ptr;
	int put_ptr;
	int putting_ptr;
	/*more*/
}vframe_states_t;

typedef struct vframe_provider_s {
    vframe_t * (*peek)(void);
    vframe_t * (*get )(void);
    void       (*put )(vframe_t *);
	int 	   (*vf_states)(vframe_states_t *states);
} vframe_provider_t;

void vf_reg_provider(const vframe_provider_t *p);
void vf_unreg_provider(void);
void vf_light_unreg_provider(void);
unsigned int get_post_canvas(void);
unsigned int vf_keep_current(void);

#endif /* VFRAME_PROVIDER_H */


/*
 * AMLOGIC Audio port driver.
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
 * Author:  Kevin Wang <kevin.wang@amlogic.com>
 *
 */
#ifndef AMAUDIO_H
#define AMAUDIO_H

#include <linux/interrupt.h>

#define AMAUDIO_IOC_MAGIC  'A'

#define AMAUDIO_IOC_GET_I2S_OUT_SIZE		_IOW(AMAUDIO_IOC_MAGIC, 0x00, int)

#define AMAUDIO_IOC_GET_I2S_OUT_PTR			_IOW(AMAUDIO_IOC_MAGIC, 0x01, int)

#define AMAUDIO_IOC_SET_I2S_OUT_OP_PTR	_IOW(AMAUDIO_IOC_MAGIC, 0x02, int)

#define AMAUDIO_IOC_GET_I2S_IN_SIZE			_IOW(AMAUDIO_IOC_MAGIC, 0x03, int)

#define AMAUDIO_IOC_GET_I2S_IN_PTR			_IOW(AMAUDIO_IOC_MAGIC, 0x04, int)

#define AMAUDIO_IOC_SET_I2S_IN_OP_PTR		_IOW(AMAUDIO_IOC_MAGIC, 0x05, int)


#endif


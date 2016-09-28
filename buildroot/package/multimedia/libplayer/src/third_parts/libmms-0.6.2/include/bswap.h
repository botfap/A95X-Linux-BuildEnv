#ifndef BSWAP_H_INCLUDED
#define BSWAP_H_INCLUDED

/*
 * Copyright (C) 2004 Maciej Katafiasz <mathrick@users.sourceforge.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* These macros are for converting an array of bytes containing a
   <macro-name> integer to such an integer in the processors native format */

#include <inttypes.h>

#define BE_16_TO_NATIVE(val) \
	(val[1] | (val[0] << 8))
#define BE_32_TO_NATIVE(val) \
	(val[3] | (val[2] << 8) | (val[1] << 16) | (val[0] << 24))
#define BE_64_TO_NATIVE(val) \
	(val[7] | (val[6] << 8) | (val[5] << 16) | (val[4] << 24) | \
	((uint64_t)val[3] << 32) | ((uint64_t)val[2] << 40) | \
	((uint64_t)val[1] << 48) | ((uint64_t)val[0] << 56))

#define LE_16_TO_NATIVE(val) \
	(val[0] | (val[1] << 8))
#define LE_32_TO_NATIVE(val) \
	(val[0] | (val[1] << 8) | (val[2] << 16) | (val[3] << 24))
#define LE_64_TO_NATIVE(val) \
	(val[0] | (val[1] << 8) | (val[2] << 16) | (val[3] << 24) | \
	((uint64_t)val[4] << 32) | ((uint64_t)val[5] << 40) | \
	((uint64_t)val[6] << 48) | ((uint64_t)val[7] << 56))

#define BE_16(val) BE_16_TO_NATIVE(((uint8_t *)(val)))
#define BE_32(val) BE_32_TO_NATIVE(((uint8_t *)(val)))
#define BE_64(val) BE_64_TO_NATIVE(((uint8_t *)(val)))
#define LE_16(val) LE_16_TO_NATIVE(((uint8_t *)(val)))
#define LE_32(val) LE_32_TO_NATIVE(((uint8_t *)(val)))
#define LE_64(val) LE_64_TO_NATIVE(((uint8_t *)(val)))

#endif

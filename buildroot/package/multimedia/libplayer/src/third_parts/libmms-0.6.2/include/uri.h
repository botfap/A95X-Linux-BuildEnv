/* GNet - Networking library
 * Copyright (C) 2000-2001  David Helder, David Bolcsfoldi
 *
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
 * Boston, MA  02111-1307, USA.
 */

/*
 * Modified by Vaclav Bartacek 06/2011
 * Ported to Android
 */

#ifndef _GNET_URI_H
#define _GNET_URI_H

#ifndef OS_ANDROID
#include <glib.h>
#else

typedef int gint;
typedef gint gboolean;
typedef unsigned int    guint;
typedef const void *gconstpointer;

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#define g_new0(struct_type, n_structs)          \
    ((struct_type *) calloc( n_structs, sizeof (struct_type)))

#define g_malloc(s) \
    (malloc(s))

#define g_free(s) \
    (free(s))

#define g_strdup(s) \
    (strdup(s))

#define g_strndup(s,n) \
    (strndup(s,n))

#define g_return_if_fail(e)
#define g_return_val_if_fail(a,b)

#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 *  GURI:
 *  @scheme: Scheme (or protocol)
 *  @userinfo: User info
 *  @hostname: Host name
 *  @port: Port number
 *  @path: Path
 *  @query: Query
 *  @fragment: Fragment
 *
 *  The #GURI structure represents a URI.  All fields in this
 *  structure are publicly readable.
 *
 **/
typedef struct _GURI GURI;

struct _GURI
{
  char* scheme;
  char* user;
  char* passwd;
  char* hostname;
  gint   port;
  char* path;
  char* query;
  char* fragment;
};



GURI*     gnet_uri_new (const char* uri);
GURI*     gnet_uri_new_fields (const char* scheme, const char* hostname, 
			       const gint port, const char* path);
GURI*
gnet_uri_new_fields_all (const char* scheme, const char* user, 
			 const char* passwd, const char* hostname,
			 const gint port, const char* path, 
			 const char* query, const char* fragment);
GURI*     gnet_uri_clone (const GURI* uri);
void      gnet_uri_delete (GURI* uri);
	       
gboolean  gnet_uri_equal (gconstpointer p1, gconstpointer p2);
guint     gnet_uri_hash (gconstpointer p);

void	  gnet_uri_escape (GURI* uri);
void	  gnet_uri_unescape (GURI* uri);

char* 	  gnet_uri_get_string (const GURI* uri);

void 	  gnet_uri_set_scheme   (GURI* uri, const char* scheme);
void 	  gnet_uri_set_userinfo (GURI* uri, const char* user, const char* passwd);
void 	  gnet_uri_set_hostname (GURI* uri, const char* hostname);
void 	  gnet_uri_set_port     (GURI* uri, gint port);
void 	  gnet_uri_set_path	(GURI* uri, const char* path);
void 	  gnet_uri_set_query 	(GURI* uri, const char* query);
void 	  gnet_uri_set_fragment (GURI* uri, const char* fragment);

char*     gnet_mms_helper(const GURI* uri, int make_absolute);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_URI_H */

/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* rphexfile.h
 *
 * Copyright 2020 Ralph Perlich
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __RP_HEX_FILE_H__
#define __RP_HEX_FILE_H__

#include <stdio.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RP_TYPE_HEX_FILE			(rp_hex_file_get_type ())
#define RP_HEX_FILE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), RP_TYPE_HEX_FILE, RPHexFile))
#define RP_HEX_FILE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), RP_TYPE_HEX_FILE, RPHexFileClass))
#define RP_IS_HEX_FILE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), RP_TYPE_HEX_FILE))
#define RP_IS_HEX_FILE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), RP_TYPE_HEX_FILE))
#define RP_HEX_FILE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), RP_TYPE_HEX_FILE, RPHexFileClass))

typedef struct _RPHexFile		RPHexFile;
typedef struct _RPHexFileClass	RPHexFileClass;

struct _RPHexFile
{
    GObject 			object;
	gchar 				*file_name;
	guint32 			file_size;
	GDataInputStream	*data_stream;
};

struct _RPHexFileClass
{
	GObjectClass	parent_class;
};

GType   	rp_hex_file_get_type (void);

RPHexFile 	*rp_hex_file_new (void);
RPHexFile 	*rp_hex_file_new_with_file (GFile *file);
gchar 		*rp_hex_file_get_file_name (RPHexFile *hex_file);
guint32		rp_hex_file_get_size (RPHexFile *hex_file);
guint8 		rphex_file_read_byte (RPHexFile *hex_file, goffset position);

G_END_DECLS

#endif

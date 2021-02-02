/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* rphexfile.c
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

#include "rphexfile.h"

static GObjectClass *parent_class = NULL;

static void rp_hex_file_finalize	(GObject *object);
static void rp_hex_file_dispose		(GObject *object);

static void rp_hex_file_class_init (RPHexFileClass *klass)
{
	GObjectClass *gobject_class	= G_OBJECT_CLASS (klass);
    
    parent_class = g_type_class_peek_parent(klass);
    
    gobject_class->finalize = rp_hex_file_finalize;
	gobject_class->dispose	= rp_hex_file_dispose;

	g_message ("HexFile: called Class Init");
}

static void rp_hex_file_init (RPHexFile *hex_file)
{
	hex_file->file_name		= NULL;
	hex_file->file_size		= 0;
	hex_file->data_stream	= NULL;

	g_message ("HexFile: called Init");
}

GType rp_hex_file_get_type (void)
{
	static GType info_type = 0;
	
	if (!info_type)
	{
		static const GTypeInfo info_info = {
					sizeof (RPHexFileClass),
					NULL,
					NULL,
					(GClassInitFunc) rp_hex_file_class_init,
					NULL,
					NULL,
					sizeof (RPHexFile),
					0,
					(GInstanceInitFunc) rp_hex_file_init
		};
		
		info_type = g_type_register_static (G_TYPE_OBJECT, "RPHexFile", &info_info, 0);
	}
	
	return info_type;
}

static void rp_hex_file_finalize (GObject *object)
{
	g_message ("HexFile: called Finalize");
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void rp_hex_file_dispose (GObject *object)
{
	g_message ("HexFile: called Dispose");
	RPHexFile *hex_file = RP_HEX_FILE (object);

	g_return_if_fail (RP_IS_HEX_FILE (hex_file));

    /* free stuff */
	g_free (hex_file->file_name);

	G_OBJECT_CLASS (parent_class)->dispose (object);	
}

RPHexFile *rp_hex_file_new ()
{
	RPHexFile *hex_file;

	hex_file = g_object_new (RP_TYPE_HEX_FILE, NULL);
	
	g_return_val_if_fail (hex_file != NULL, NULL);

	return hex_file;
}

RPHexFile *rp_hex_file_new_with_file (GFile *file)
{
	RPHexFile	*hex_file;
	GFileInfo	*hex_file_info;
	GError 		*error = NULL;
	g_autoptr(GFileInputStream)	input_stream = NULL;

	hex_file = rp_hex_file_new ();
	g_return_val_if_fail (hex_file != NULL, NULL);

	hex_file_info = g_file_query_info (file, "*", G_FILE_QUERY_INFO_NONE, NULL, &error );
	g_return_val_if_fail (hex_file_info != NULL, NULL);

	hex_file->file_name = g_file_get_parse_name (file);
	hex_file->file_size = g_file_info_get_size (hex_file_info);

	g_object_unref (hex_file_info);
	
	input_stream = g_file_read (file, NULL, &error);
	g_return_val_if_fail (input_stream != NULL, NULL);

	hex_file->data_stream = g_data_input_stream_new (G_INPUT_STREAM (input_stream));

	return hex_file;
}

gchar *rp_hex_file_get_file_name (RPHexFile *hex_file)
{
	return hex_file->file_name;
}

guint32	rp_hex_file_get_size (RPHexFile *hex_file)
{
	return hex_file->file_size;
}

guint8 rphex_file_read_byte (RPHexFile *hex_file, goffset position)
{
	GError *error = NULL;
	guchar ret;

	g_seekable_seek ((GSeekable*)hex_file->data_stream, position, G_SEEK_SET, NULL, &error);
	ret = g_data_input_stream_read_byte (hex_file->data_stream, NULL, &error);

	return ret;
}

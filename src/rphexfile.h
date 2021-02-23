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

enum mod_type
{
    mod_unknown = '0',          // Helps bug detection
    mod_insert  = 'I',          // Bytes inserted
    mod_replace = 'R',          // Bytes replaced (overtyped)
    mod_delforw = 'D',          // Bytes deleted (using DEL)
    mod_delback = 'B',          // Bytes deleted (using back space)
    mod_repback = '<',          // Replace back (BS in overtype mode)
};

enum { loc_unknown = 'u', loc_file = 'f', loc_mem = 'm' };

typedef struct _doc_loc doc_loc;

struct _doc_loc
{
    gchar	location;  // File or memory?
    guint32	len;
    union
    {
        guint32 fileaddr; 	// File location (if loc_file)
        guchar 	*memaddr;	// Ptr to data (if loc_mem)
    };
};

//const int doc_undo_limit = 5;

typedef struct _doc_undo doc_undo;

struct _doc_undo
{
    int limit;
    enum mod_type utype;        // Type of modification made to file
    guint32 len;                // Length of mod
    guint32 address;            // Address in file of start of mod
    guchar *ptr;                // NULL if utype is del else new data
};

doc_loc *doc_loc_mem_new (guchar *mem, size_t l);
doc_loc *doc_loc_file_new (guint32 file_addr, size_t l);

doc_undo *doc_undo_new (enum mod_type u, guint32 a, guint32 l, guchar *p);

typedef struct _RPHexFile		RPHexFile;
typedef struct _RPHexFileClass	RPHexFileClass;

struct _RPHexFile
{
    GObject 			object;
	gchar 				*file_name;
	guint32 			file_size;
    guint32             real_file_size;
    gboolean            read_only;
    gboolean            is_modified;
	GDataInputStream	*data_stream;
    GList               *loc;
    GList               *undo;
};

struct _RPHexFileClass
{
	GObjectClass	parent_class;

    void (*data_changed)	(RPHexFile *);
};

GType   	rp_hex_file_get_type (void);

RPHexFile 	*rp_hex_file_new (void);
RPHexFile 	*rp_hex_file_new_with_file (GFile *file, gboolean open_read_only, GError *error);
gchar 		*rp_hex_file_get_file_name (RPHexFile *hex_file);
gboolean    rp_hex_file_is_read_only (RPHexFile *hex_file);
guint32     rp_hex_file_get_data (RPHexFile *hex_file, guchar *buf, guint32 len, guint32 address);
void        rp_hex_file_change_data (RPHexFile *hex_file, enum mod_type utype, guint32 address, 
							        guint32 len, guchar *buf, guint num_done);
gboolean    rp_hex_file_get_is_modified (RPHexFile *hex_file);
guint32		rp_hex_file_get_size (RPHexFile *hex_file);
gboolean    rp_hex_file_only_overtype_changes (RPHexFile *hex_file);
gboolean    rp_hex_file_write_in_place (RPHexFile *hex_file);
void        dump_loc_list (RPHexFile *hex_file);

G_END_DECLS

#endif

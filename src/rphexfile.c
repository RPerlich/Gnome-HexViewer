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

enum
{
	DATA_CHANGED,
	LAST_SIGNAL
};

static gint class_signals[LAST_SIGNAL] = { 0 };

doc_loc *doc_loc_mem_new (guchar *mem, size_t l)
{
    doc_loc *dl 	= g_malloc0 (sizeof(doc_loc));
    dl->location	= loc_mem;
    dl->len			= l; 
    dl->memaddr 	= mem;

    return dl;
}

doc_loc *doc_loc_file_new (guint32 file_addr, size_t l)
{
    doc_loc *dl		= g_malloc0 (sizeof(doc_loc));
    dl->location	= loc_file;
    dl->len 		= l;
    dl->fileaddr	= file_addr;

    return dl;
}

doc_undo *doc_undo_new (enum mod_type u, guint32 a, guint32 l, guchar *p)
{
	doc_undo *du = g_malloc0 (sizeof(doc_undo));

	du->limit   = 0;
    du->utype   = u; 
	du->len     = l; 
	du->address = a;
    
	if (p != NULL)
    {
		du->ptr = g_malloc0 (MAX (du->limit, du->len));
		memcpy (du->ptr, p, du->len);
    }
	else
		du->ptr = NULL;

    return du;
}

static GObjectClass *parent_class = NULL;

static void rp_hex_file_finalize    (GObject *object);
static void rp_hex_file_dispose	    (GObject *object);

static void rp_hex_file_class_init (RPHexFileClass *klass)
{
	GObjectClass *gobject_class	= G_OBJECT_CLASS (klass);
    
    parent_class = g_type_class_peek_parent(klass);
    
    klass->data_changed     = NULL;
    gobject_class->finalize = rp_hex_file_finalize;
	gobject_class->dispose	= rp_hex_file_dispose;

    class_signals[DATA_CHANGED]	= g_signal_new ("data_changed",
									G_TYPE_FROM_CLASS (gobject_class),
					  				G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					  				G_STRUCT_OFFSET (RPHexFileClass, data_changed),
					  				NULL,
									NULL,
									NULL,
									G_TYPE_NONE,
									1,
									G_TYPE_BOOLEAN);

	g_message ("HexFile: called Class Init");
}

static void rp_hex_file_init (RPHexFile *hex_file)
{
	hex_file->file_name		= NULL;
	hex_file->file_size		= 0;
	hex_file->data_stream	= NULL;
	hex_file->loc			= NULL;
    hex_file->undo			= NULL;
    hex_file->read_only     = TRUE;
    hex_file->is_modified   = FALSE;

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
	g_list_free (hex_file->loc);

	G_OBJECT_CLASS (parent_class)->dispose (object);	
}

RPHexFile *rp_hex_file_new ()
{
	RPHexFile *hex_file;

	hex_file = g_object_new (RP_TYPE_HEX_FILE, NULL);
	
	g_return_val_if_fail (hex_file != NULL, NULL);

	return hex_file;
}

RPHexFile *rp_hex_file_new_with_file (GFile *file, gboolean open_read_only, GError *error)
{
	RPHexFile	*hex_file = NULL;
	GFileInfo	*hex_file_info = NULL;	
    guint32     fsize = 0;
    gboolean    bCanWrite;
    g_autoptr(GFileInputStream)	input_stream = NULL;
    error = NULL;
	
	hex_file_info = g_file_query_info (file, "*", G_FILE_QUERY_INFO_NONE, NULL, &error );
	g_return_val_if_fail (hex_file_info != NULL, NULL);

    fsize       = g_file_info_get_size (hex_file_info);
    bCanWrite   = g_file_info_get_attribute_boolean (hex_file_info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);

    g_object_unref (hex_file_info);

    if (fsize >= G_MAXUINT32)
    {
        error->domain   = G_FILE_ERROR;
        error->code     = G_FILE_ERROR_FAILED;
        error->message  = "File is too big.";
        return NULL;
    }

	input_stream = g_file_read (file, NULL, &error);
	g_return_val_if_fail (input_stream != NULL, NULL);

	hex_file = rp_hex_file_new ();
	g_return_val_if_fail (hex_file != NULL, NULL);

	hex_file->file_name     = g_file_get_parse_name (file);
	hex_file->file_size     = fsize;
	hex_file->real_file_size = hex_file->file_size;
    hex_file->read_only     = !bCanWrite;
    hex_file->data_stream   = g_data_input_stream_new (G_INPUT_STREAM (input_stream));

	hex_file->loc = g_list_append (hex_file->loc, doc_loc_file_new (0, hex_file->file_size));
	
	return hex_file;
}

gboolean rp_hex_file_is_read_only (RPHexFile *hex_file)
{
    return hex_file->read_only;
}

gchar *rp_hex_file_get_file_name (RPHexFile *hex_file)
{
	return hex_file->file_name;
}

guint32 rp_hex_file_get_data (RPHexFile *hex_file, guchar *buf, guint32 len, guint32 address)
{
	guint32 pos = 0;
	guint32 tocopy;
	GList	*locList;
	GError	*error = NULL;

	g_assert (address >= 0);

	locList = g_list_first (hex_file->loc);

	for (guint i = 0; i < g_list_length (hex_file->loc); i++)
	{
		struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));
		
		if (address < pos + dl->len)
            break;

		pos += dl->len;
		locList = g_list_next (locList);
	}

	guint32 left = len;
	guint32 start = address - pos;
	
	for (left = len; left > 0 && locList != NULL; left -= tocopy, buf += tocopy)
	{
		struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));
		tocopy = MIN (left, dl->len - start);

		if (dl->location == loc_mem)
			memcpy (buf, dl->memaddr + start, tocopy);
		else
		{
        	g_assert (dl->location == loc_file);

			g_seekable_seek ((GSeekable*)hex_file->data_stream, dl->fileaddr + start, G_SEEK_SET, NULL, &error);
			
			guint32 actual = g_input_stream_read (G_INPUT_STREAM(hex_file->data_stream), buf, tocopy, NULL, &error);
			
			if (actual != tocopy)
			{
				// something went wrong here
                g_assert (0);
				left -= actual;
				break;
			}
		}

		start = 0;
		pos += dl->len;
		locList = g_list_next (locList);
    }

    // Return the actual number of bytes written to buf
    return len - left;
}

void loc_split (RPHexFile *hex_file, guint32 address, guint32 pos, GList *locList)
{  
    struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));

	g_assert (address > pos && address < pos + dl->len);
	g_assert (dl->location == loc_file || dl->location == loc_mem);
	
	guint32 split = pos + dl->len - address;

	GList *plNext = g_list_next (locList);
    gint lPos = g_list_position (hex_file->loc, plNext);
    
	if (dl->location == loc_file)
    {
		hex_file->loc = g_list_insert (hex_file->loc, doc_loc_file_new (dl->fileaddr + dl->len - split, split), lPos);
        g_message ("Split: Add file rec: addr %i, len: %i", dl->fileaddr + dl->len - split, split);
    }
    else
    {
		hex_file->loc = g_list_insert (hex_file->loc, doc_loc_mem_new (dl->memaddr + dl->len - split, split), lPos);
        g_message ("Split: Add mem rec: addr: %hhn, len: %i, pos: %i", dl->memaddr + dl->len - split, split, lPos);
    }

    dl->len -= split;
}

GList* loc_del (RPHexFile *hex_file, guint32 address, guint32 len, guint32 *pos, GList *locList)
{
    GList   *byebye;
    guint32 deleted = 0;

	struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));
    
	if (address != *pos)
    {
        loc_split (hex_file, address, *pos, locList);
        *pos += dl->len;
		locList = g_list_next (locList);
        dl = ((struct _doc_loc*)(locList->data));
    }

    while (locList != NULL && len >= deleted + dl->len)
    {        
        byebye = locList;
        deleted += dl->len;
		locList = g_list_next (locList);

		hex_file->loc = g_list_delete_link (hex_file->loc, byebye);
    }

    if (locList != NULL && len > deleted)
    {   
        dl = ((struct _doc_loc*)(locList->data));
		loc_split (hex_file, address + len, address + deleted, locList);
        
        byebye = locList;
        deleted += dl->len;
        locList = g_list_next (locList);
        	
        hex_file->loc = g_list_delete_link (hex_file->loc, byebye);
    }
    
    g_assert (deleted == len);

    return locList;
}

GList* loc_add (RPHexFile *hex_file, struct _doc_undo *pu, guint32 *pos, GList *locList)
{	
	if (locList != NULL && locList->data != NULL && pu->address != *pos)
    {
        struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));
        
        loc_split (hex_file, pu->address, *pos, locList);
        
        *pos += dl->len;
		locList = g_list_next (locList);
    }
	
    if (locList == NULL || locList->data == NULL)
    {
        hex_file->loc = g_list_append (hex_file->loc, doc_loc_mem_new (pu->ptr, pu->len));
    }
    else
    {
        gint lPos = g_list_position (hex_file->loc, locList);
	    hex_file->loc = g_list_insert (hex_file->loc, doc_loc_mem_new (pu->ptr, pu->len), lPos);
    }

    *pos += pu->len;
    
    return locList;
}

void rp_hex_file_recreate_loc_list (RPHexFile *hex_file)
{
	guint32 pos = 0;
	GList 	*undoList;
	GList 	*locList;
    GList   *tmp;

	g_list_free (hex_file->loc);
    hex_file->loc = NULL;

	hex_file->loc = g_list_append (hex_file->loc, doc_loc_file_new (0, hex_file->real_file_size));

    undoList = g_list_first (hex_file->undo);

	for (guint i = 0; i < g_list_length (hex_file->undo); i++)
	{
		struct _doc_undo *du = ((struct _doc_undo*)(undoList->data));

        g_message ("Undo list entry %i: address: %i, len: %i, type: %i ", i, du->address, du->len, du->utype);
		
		pos = 0;
        locList = g_list_first (hex_file->loc);
		
		for (guint il = 0; il < g_list_length (hex_file->loc); il++)
		{
			struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));

			if (du->address < pos + dl->len)
				break;

			pos += dl->len;
			locList = g_list_next(locList);	
		}

        switch (du->utype)
        {
            case mod_insert:
                loc_add (hex_file, du, &pos, locList);
                break;
            case mod_replace:
            case mod_repback:                
                tmp = loc_del (hex_file, du->address, du->len, &pos, locList);
                loc_add (hex_file, du, &pos, tmp);
                break;
            case mod_delforw:
            case mod_delback:
                loc_del (hex_file, du->address, du->len, &pos, locList);
                break;
            default:
                g_assert (0);
        }

		undoList = g_list_next (undoList);
    }

    // Debug only, check location list length against file length
    pos = 0;
    locList = g_list_first (hex_file->loc);
	
    for (guint il = 0; il < g_list_length (hex_file->loc); il++)
	{
		struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));
		pos += dl->len;
		locList = g_list_next (locList);	
	}
    
    g_assert (pos == hex_file->file_size);
}

void rp_hex_file_change_data (RPHexFile *hex_file, enum mod_type utype, guint32 address, 
							guint32 len, guchar *buf, guint num_done)
{
    GList *undoList;

	g_assert (utype == mod_insert || utype == mod_replace ||
    		utype == mod_delforw || utype == mod_delback || 
			utype == mod_repback);
    g_assert (address <= hex_file->file_size);
    g_assert (len > 0);

    undoList = g_list_last (hex_file->undo);
	
	if ((undoList != NULL && (num_done % 2) == 1 && len == 1) || (undoList != NULL && num_done > 0))
    {
        struct _doc_undo *du = ((struct _doc_undo*)(undoList->data));

		g_assert (utype == du->utype);

		if (utype == mod_delforw)
		{
            g_assert (buf == NULL);
            g_assert (du->address == address);
            g_assert (address + len <= hex_file->file_size);
            du->len += len;
        }
        else if (utype == mod_delback)
        {
            g_assert (buf == NULL);
            g_assert (du->address == address + len);
            du->address = address;
            du->len += len;
        }
        else if (utype == mod_repback)
        {
            g_assert (buf != NULL);
            g_assert (du->address == address + len);
            memmove (du->ptr + len, du->ptr, du->len);
            memcpy (du->ptr, buf, len);
            du->address = address;
            du->len += len;
        }
        else if (du->address + du->len == address)
        {
            g_assert (buf != NULL);
            memcpy (du->ptr + du->len, buf, len);
            du->len += len;
        }
        else
        {
            g_assert (du->address + du->len - 1 == address);
            memcpy (du->ptr + du->len - 1, buf, len);
            len -= 1;
            du->len += len;
        }
    }
    else
    {
        g_message ("Add a new elt to undo array");
        if (utype == mod_insert || utype == mod_replace || utype == mod_repback)
			hex_file->undo = g_list_append (hex_file->undo, doc_undo_new (utype, address, len, buf));
        else
			hex_file->undo = g_list_append (hex_file->undo, doc_undo_new (utype, address, len, NULL));
    }

    if (utype == mod_delforw || utype == mod_delback)
        hex_file->file_size -= len;
    else if (utype == mod_insert)
        hex_file->file_size += len;
    else if (utype == mod_replace && address + len > hex_file->file_size)
        hex_file->file_size = address + len;
    else
        g_assert (utype == mod_replace);

    g_message ("-- Start recreate loc list");
    rp_hex_file_recreate_loc_list (hex_file);
    g_message ("-- End recreate loc list");

    hex_file->is_modified = TRUE;
    g_signal_emit_by_name (G_OBJECT(hex_file), "data_changed", hex_file->is_modified);
}

gboolean rp_hex_file_get_is_modified (RPHexFile *hex_file)
{
    return hex_file->is_modified;
}

guint32	rp_hex_file_get_size (RPHexFile *hex_file)
{
	return hex_file->file_size;
}

gboolean rp_hex_file_only_overtype_changes (RPHexFile *hex_file)
{
    guint32 pos = 0;
    GList	*locList;

    locList = g_list_first (hex_file->loc);

    for (guint i = 0; i < g_list_length (hex_file->loc); i++)
	{
		struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));
		
		if (dl->location == loc_file && dl->fileaddr != pos)
            return FALSE;

		pos += dl->len;
		locList = g_list_next (locList);
	}

    // Make sure file length has not changed
    if (hex_file->file_size != hex_file->real_file_size)
        return FALSE;

    return TRUE;
}

gboolean rp_hex_file_write_in_place (RPHexFile *hex_file)
{
    guint32 pos = 0;
    GList   *locList;
    gint    retW = 0;

    FILE *fp = fopen (hex_file->file_name, "r+b");

    if (!fp)
        return FALSE;

    locList = g_list_first (hex_file->loc);
        
    for (guint i = 0; i < g_list_length (hex_file->loc); i++)
    {
        struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));
        
        if (dl->location == loc_mem)
        {
            if (fseek (fp, pos, SEEK_SET) == 0)
                retW = fwrite (dl->memaddr, 1, dl->len, fp);
            
            g_return_val_if_fail (retW == dl->len, FALSE);
        }
        else
            g_return_val_if_fail (dl->fileaddr == pos, FALSE);

        pos+= dl->len;
        locList = g_list_next (locList);
        retW = 0;
    }

    if (fclose (fp) != 0)
        return FALSE;

    g_list_free (hex_file->undo);
    hex_file->undo = NULL;
    
    rp_hex_file_recreate_loc_list (hex_file);

    return (pos == hex_file->file_size);
}

gboolean rp_hex_file_write_data (RPHexFile *hex_file, guchar *file_name, guint32 start, guint32 end)
{
    const guint copy_buffer_len = 16384;
    guint32     address = 0;
    
    FILE *fp = fopen (file_name, "w+b");

    if (!fp)
        return FALSE;
    
    guchar  *buffer = g_try_malloc0 (copy_buffer_len);
    guint   bytesRead;

    for (address = start; address < end; address += bytesRead)
    {
        bytesRead = rp_hex_file_get_data (hex_file, buffer, MIN (end - address, copy_buffer_len), address);
        g_assert (bytesRead > 0);

        fwrite (buffer, 1, bytesRead, fp);
    }
    
    g_assert (address == end);
    
    g_free (buffer);
    buffer = NULL;

    if (fclose (fp) != 0)
        return FALSE;
    
    return TRUE;
}

void dump_loc_list (RPHexFile *hex_file)
{
    GList *locList = g_list_first (hex_file->loc);

    for (guint il = 0; il < g_list_length (hex_file->loc); il++)
    {
        struct _doc_loc *dl = ((struct _doc_loc*)(locList->data));
        g_message ("Dump Loc List %i: Location: %s, Len: %i, File addr: %i", il, (dl->location == 102) ? "File" : "Mem", dl->len, dl->fileaddr);
        locList = locList->next;
    }
}
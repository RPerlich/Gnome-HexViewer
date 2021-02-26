/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* rphexview.h
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

#ifndef __RP_HEX_VIEW_H__
#define __RP_HEX_VIEW_H__

#include <gtk/gtk.h>
#include <cairo.h>
#include "rphexfile.h"

G_BEGIN_DECLS

#define RP_TYPE_HEX_VIEW			(rp_hex_view_get_type ())
#define RP_HEX_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), RP_TYPE_HEX_VIEW, RPHexView))
#define RP_HEX_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), RP_TYPE_HEX_VIEW, RPHexViewClass))
#define RP_IS_HEX_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), RP_TYPE_HEX_VIEW))
#define RP_IS_HEX_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), RP_TYPE_HEX_VIEW))
#define RP_HEX_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), RP_TYPE_HEX_VIEW, RPHexViewClass))

typedef enum
{
	RP_HEX_WINDOW_WIDGET = 1,
	RP_HEX_WINDOW_TEXT
} RPHexWindowType;

typedef struct _RPHexView			RPHexView;
typedef struct _RPHexViewPrivate	RPHexViewPrivate;
typedef struct _RPHexViewClass		RPHexViewClass;

struct _RPHexView
{
	GtkWidget 			parent_instance;
	RPHexViewPrivate	*priv;
};

struct _RPHexViewClass
{
	GtkWidgetClass	parent_class;
	GtkClipboard 	*clipboard;
	
	void (*byte_pos_changed)	(RPHexView *);
	void (*selection_changed)	(RPHexView *);
	void (*edit_cut)			(RPHexView *);
	void (*edit_copy)			(RPHexView *);
	void (*edit_paste)			(RPHexView *);
	void (*edit_delete)			(RPHexView *);
	void (*edit_select_all)		(RPHexView *);
};

GType		rp_hex_view_get_type		(void) G_GNUC_CONST;
GtkWidget	*rp_hex_view_new			(void);
GtkWidget	*rp_hex_view_new_with_file	(RPHexFile *hex_file);

gboolean	rp_hex_view_has_selection			(GtkWidget *widget);
gboolean 	rp_hex_view_print 					(GtkWidget *widget, GtkWindow *parent, const gchar *print_job_name);
void 		rp_hex_view_toggle_draw_addresses	(GtkWidget *widget, gboolean bEnable);
void		rp_hex_view_toggle_draw_characters	(GtkWidget *widget, gboolean bEnable);
void		rp_hex_view_toggle_auto_fit 		(GtkWidget *widget, gboolean bEnable);
void		rp_hex_view_toggle_font 			(GtkWidget *widget, guchar *font);
void		rp_hex_view_toggle_print_font		(GtkWidget *widget, guchar *font);

G_END_DECLS

#endif
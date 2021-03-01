/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* rphexview.c
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

#include "rphexview.h"
#include "rphexfile.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include "gtk/gtkscrollable.h"

#define DEFAULT_FONT "Monospace 12"
#define is_ascii(c) (((c) >= 0x20) && ((c) < 0x7f))

enum
{
	AREA_NONE,
	AREA_ADDRESS,
	AREA_HEX,
	AREA_TEXT,
};

enum
{
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_HSCROLL_POLICY,
  PROP_VSCROLL_POLICY,
};

enum
{
	BYTE_POS_CHANGED,
	SELECTION_CHANGED,
	DATA_CHANGED,
	EDIT_CUT,
	EDIT_COPY,
	EDIT_PASTE,
	EDIT_DELETE,
	EDIT_SELECT_ALL,
	LAST_SIGNAL
};

typedef struct _dataSelection dataSelection;

struct _dataSelection
{
	glong startSel;
	glong endSel;
	glong clipboard_startSel;
	glong clipboard_endSel;
};

static const GtkTargetEntry clip_targets[] = {
	{ "BINARY", 0, 0 }
};
	
static const gint n_clip_targets = sizeof(clip_targets) / sizeof(clip_targets[0]);

struct _RPHexViewPrivate
{
	GtkBorder				border_window_size;
	GdkWindow				*hex_window;
	GtkAdjustment			*hadjustment;
	GtkAdjustment			*vadjustment;
	PangoFontMetrics		*pFontMetrics, *pPrintFontMetrics;
	PangoFontDescription	*pFontDescription, *pPrintFontDescription;
	PangoLayout				*pLayout, *pPrintLayout;
	guchar					*pPrintFontName;

	GdkRGBA cLightGray;
	GdkRGBA cDimGray;
	GdkRGBA cGray;
	GdkRGBA cAddressBg;
	GdkRGBA cAddressFg;
	GdkRGBA cCursor;

	gint	iAddressWidth;
	gint	iCharHeight, iPrintCharHeight;
	gint	iCharWidth, iPrintCharWidth;
	guint32	iRows, iPrintRows;
	guint32	iTopRow, iPrintTopRow;  
	gint	iVisibleRows, iPrintVisibleRows;
	guint32	iLastRow, iPrintLastRow;
	gint	iCols;
	gint	iLeftCol;
	gint	iVisibleCols;
	gint	iBytesPerLine, iPrintBytesPerLine;
	gint	iMaxVisibleBytes, iPrintMaxVisibleBytes;
	guint32 iStartByte, iPrintStartByte;
	guint32	iEndByte, iPrintEndByte;
	guint32	iFileSize;
	guint	iPrintPages;

	gint			cursorArea;
	guint32			iBytePos;
	gboolean		bBytePosIsNibble;
	gboolean		bSelecting;
	gboolean		bLButttonDown;
	dataSelection 	*selection;

	GdkRectangle	rectClient, rectPrintClient;
	GdkRectangle	rectAddresses, rectPrintAddresses;
	GdkRectangle	rectHexBytes, rectPrintHexBytes;
	GdkRectangle	rectCharacters, rectPrintCharacters;

	gboolean	bDrawAddresses;
	gboolean	bAutoBytesPerRow;
	gboolean	bDrawCharacters;

	RPHexFile 	*hex_file;
	gboolean	bIsOvertype;
	gshort 		num_entered;	// How many consecutive characters entered?
    gshort		num_del;		// How many consec. chars deleted?
    gshort		num_bs;			// How many consec. back spaces?

	GSimpleActionGroup 	*action_group_context_menu;
	GtkWidget			*context_menu;

	int left_margin;
	int right_margin;
	int top_margin;
	int bottom_margin;
	int left_padding;
	int right_padding;
	int top_padding;
	int bottom_padding;

	guint hscroll_policy : 1;
	guint vscroll_policy : 1;
};

static gint class_signals[LAST_SIGNAL] = { 0 };

PangoFontMetrics* rp_hex_view_get_fontmetrics (const char *font_name);
static void rp_hex_view_update_character_size (RPHexViewPrivate *priv);
static void rp_hex_view_finalize (GObject *object);
static void rp_hex_view_dispose	(GObject *object);
static void rp_hex_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void rp_hex_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void rp_hex_view_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void rp_hex_view_update_layout (RPHexViewPrivate *priv);
static void rp_hex_view_update_byte_visibility (RPHexViewPrivate *priv);
static gboolean rp_hex_view_draw (GtkWidget *widget, cairo_t *cr);
static void rp_hex_view_draw_address_lines (RPHexViewPrivate *priv, cairo_t *cr);
static void rp_hex_view_draw_hex_lines (RPHexViewPrivate *priv, cairo_t *cr);
static void rp_hex_view_draw_selection (RPHexViewPrivate *priv, cairo_t *cr);
static void rp_hex_view_draw_cursor	(RPHexViewPrivate *priv, cairo_t *cr);
static void rp_hex_view_realize	(GtkWidget *widget);
static void rp_hex_view_unrealize (GtkWidget *widget);
static void rp_hex_view_map	(GtkWidget *widget);
static void rp_hex_view_set_hadjustment	(RPHexView *hex_view, GtkAdjustment *adjustment);
static void rp_hex_view_set_vadjustment	(RPHexView *hex_view, GtkAdjustment *adjustment);
static void rp_hex_view_set_hadjustment_values (RPHexView *hex_view);
static void rp_hex_view_set_vadjustment_values (RPHexView *hex_view);
static void rp_hex_view_scroll_value_changed (GtkAdjustment *adjustment, RPHexView *hex_view);
static gboolean rp_hex_view_button_callback (GtkWidget *widget, GdkEventButton *event);
static gboolean rp_hex_view_motion_callback	(GtkWidget *widget, GdkEventMotion *event);
static gboolean rp_hex_view_scroll_callback	(GtkWidget *widget, GdkEventScroll *event);
static gboolean rp_hex_view_key_press_callback (GtkWidget *widget, GdkEventKey *event);
static gboolean rp_hex_view_key_release_callback (GtkWidget *widget, GdkEventKey *event);
gboolean rp_hex_view_has_selection (GtkWidget *widget);
static void rp_hex_view_remove_selection (GtkWidget *widget);
static void rp_hex_view_set_selection (GtkWidget *widget, glong selStartPos, glong selEndPos);
static void rp_hex_view_set_cursor (GtkWidget *widget, guint32 newPos);
static void rp_hex_view_update_cursor_area (RPHexViewPrivate *priv, gdouble posX, gdouble posY);
static void rp_hex_view_scroll_byte_into_view (RPHexViewPrivate *priv, guint32 curPos);
static guint32 rp_hex_view_bytepos_from_point (RPHexViewPrivate *priv, 
											gdouble posX, gdouble posY, gboolean bSnap);
static gboolean rp_hex_view_context_menu (GtkWidget *widget);
static void rp_hex_view_context_menu_show (GtkWidget *widget, GdkEventButton *event);
static GMenuModel *rp_hex_view_get_context_menu_model (RPHexView *hex_view);
static void rp_hex_view_context_menu_cut (GSimpleAction *action, GVariant *parameter, gpointer data);
static void rp_hex_view_context_menu_copy (GSimpleAction *action, GVariant *parameter,	gpointer data);
static void rp_hex_view_context_menu_paste (GSimpleAction *action, GVariant *parameter, gpointer data);
static void rp_hex_view_context_menu_delete (GSimpleAction *action, GVariant *parameter, gpointer data);
static void rp_hex_view_context_menu_select_all (GSimpleAction *action, GVariant *parameter, gpointer data);
static void rp_hex_view_edit_cut (RPHexView *hex_view);
static void rp_hex_view_edit_copy (RPHexView *hex_view);
static void rp_hex_view_edit_paste (RPHexView *hex_view);
static void rp_hex_view_edit_delete (RPHexView *hex_view);
static void rp_hex_view_edit_select_all (RPHexView *hex_view);

G_DEFINE_TYPE_WITH_CODE (RPHexView, rp_hex_view, GTK_TYPE_WIDGET, G_ADD_PRIVATE (RPHexView)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL))


static GActionEntry context_menu_entries[] = {
	{ "cut", rp_hex_view_context_menu_cut, NULL, NULL, NULL },
	{ "copy", rp_hex_view_context_menu_copy, NULL, NULL, NULL },
	{ "paste", rp_hex_view_context_menu_paste, NULL, NULL, NULL },
	{ "delete", rp_hex_view_context_menu_delete, NULL, NULL, NULL },
	{ "select-all", rp_hex_view_context_menu_select_all, NULL, NULL, NULL }
};

static void rp_hex_view_class_init (RPHexViewClass *klass)
{
	g_message ("Widget: called Class Init");
	GObjectClass	*gobject_class	= G_OBJECT_CLASS (klass);
	GtkWidgetClass	*widget_class	= GTK_WIDGET_CLASS (klass);

	klass->byte_pos_changed				= NULL;
	klass->selection_changed			= NULL;
	klass->clipboard					= gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	klass->edit_cut						= rp_hex_view_edit_cut;
	klass->edit_copy 					= rp_hex_view_edit_copy;
	klass->edit_paste					= rp_hex_view_edit_paste;
	klass->edit_delete					= rp_hex_view_edit_delete;
	klass->edit_select_all				= rp_hex_view_edit_select_all;

	gobject_class->set_property 		= rp_hex_view_set_property;
	gobject_class->get_property 		= rp_hex_view_get_property;
	gobject_class->finalize 			= rp_hex_view_finalize;
	gobject_class->dispose 				= rp_hex_view_dispose;
	widget_class->realize 				= rp_hex_view_realize;
	widget_class->unrealize 			= rp_hex_view_unrealize;	
	widget_class->size_allocate 		= rp_hex_view_size_allocate;
	widget_class->draw 					= rp_hex_view_draw;
	widget_class->map 					= rp_hex_view_map;
	widget_class->button_press_event	= rp_hex_view_button_callback;
	widget_class->button_release_event	= rp_hex_view_button_callback;
	widget_class->motion_notify_event	= rp_hex_view_motion_callback;
	widget_class->scroll_event			= rp_hex_view_scroll_callback;
	widget_class->key_press_event		= rp_hex_view_key_press_callback;
	widget_class->key_release_event		= rp_hex_view_key_release_callback;
	widget_class->popup_menu			= rp_hex_view_context_menu;

	class_signals[BYTE_POS_CHANGED]	= g_signal_new ("byte_pos_changed",
										G_TYPE_FROM_CLASS (widget_class),
					  					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					  					G_STRUCT_OFFSET (RPHexViewClass, byte_pos_changed),
					  					NULL,
										NULL,
										NULL,
										G_TYPE_NONE,
										1,
										G_TYPE_UINT64);
	
	class_signals[SELECTION_CHANGED] = g_signal_new ("selection_changed",
										G_TYPE_FROM_CLASS (widget_class),
					  					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					  					G_STRUCT_OFFSET (RPHexViewClass, selection_changed),
					  					NULL,
										NULL,
										NULL,
										G_TYPE_NONE,
										0);

	class_signals[EDIT_CUT]	= g_signal_new ("edit_cut",
										G_TYPE_FROM_CLASS (widget_class),
					  					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					  					G_STRUCT_OFFSET (RPHexViewClass, edit_cut),
					  					NULL,
										NULL,
										NULL,
										G_TYPE_NONE,
										0);
	
	class_signals[EDIT_COPY] = g_signal_new ("edit_copy",
										G_TYPE_FROM_CLASS (widget_class),
					  					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					  					G_STRUCT_OFFSET (RPHexViewClass, edit_copy),
					  					NULL,
										NULL,
										NULL,
										G_TYPE_NONE,
										0);

	class_signals[EDIT_PASTE] = g_signal_new ("edit_paste",
										G_TYPE_FROM_CLASS (widget_class),
					  					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					  					G_STRUCT_OFFSET (RPHexViewClass, edit_paste),
					  					NULL,
										NULL,
										NULL,
										G_TYPE_NONE,
										0);

	class_signals[EDIT_DELETE] = g_signal_new ("edit_delete",
										G_TYPE_FROM_CLASS (widget_class),
					  					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					  					G_STRUCT_OFFSET (RPHexViewClass, edit_delete),
					  					NULL,
										NULL,
										NULL,
										G_TYPE_NONE,
										0);

	class_signals[EDIT_SELECT_ALL] = g_signal_new ("edit_select_all",
										G_TYPE_FROM_CLASS (widget_class),
					  					G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					  					G_STRUCT_OFFSET (RPHexViewClass, edit_select_all),
					  					NULL,
										NULL,
										NULL,
										G_TYPE_NONE,
										0);

	
	gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_TEXT);

	/* Key-Binding interface */
	GtkBindingSet *binding_set = gtk_binding_set_by_class (klass);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_x, GDK_CONTROL_MASK, "edit_cut", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_c, GDK_CONTROL_MASK, "edit_copy", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_v, GDK_CONTROL_MASK, "edit_paste", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK, "edit_select_all", 0);
	
	/* GtkScrollable interface */
	g_object_class_override_property (gobject_class, PROP_HADJUSTMENT,    "hadjustment");
	g_object_class_override_property (gobject_class, PROP_VADJUSTMENT,    "vadjustment");
	g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
	g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");	
}

static void rp_hex_view_init (RPHexView *hex_view)
{
	g_message ("Widget: called Init");
	GtkWidget 			*widget = GTK_WIDGET (hex_view);
	RPHexViewPrivate	*priv;

	hex_view->priv = rp_hex_view_get_instance_private (hex_view);
	priv = hex_view->priv;

	priv->pFontMetrics			= rp_hex_view_get_fontmetrics (DEFAULT_FONT);
	priv->pFontDescription		= pango_font_description_from_string (DEFAULT_FONT);
	priv->pPrintFontName		= g_strdup (DEFAULT_FONT);
	priv->pPrintFontDescription	= pango_font_description_from_string (DEFAULT_FONT);
	priv->pLayout				= gtk_widget_create_pango_layout (widget, "hex_Layout");
	priv->pPrintLayout 			= NULL;
	priv->pPrintFontMetrics 	= NULL;
	priv->iPrintCharHeight		= 0;
	priv->iPrintCharWidth		= 0;

	gtk_widget_set_can_focus (widget, TRUE);
	gtk_widget_set_focus_on_click (widget, TRUE);
	gtk_widget_grab_focus (widget);

	pango_layout_set_font_description (priv->pLayout, priv->pFontDescription);
	
	rp_hex_view_update_character_size(priv);

	gdk_rgba_parse(&priv->cLightGray, "#D3D3D3");
	gdk_rgba_parse(&priv->cDimGray, "#696969");
	gdk_rgba_parse(&priv->cGray, "#808080");
	gdk_rgba_parse(&priv->cAddressBg, "#1e1e1e"); // Light "#333333", dark #252526
	gdk_rgba_parse(&priv->cAddressFg, "#818181");
	gdk_rgba_parse(&priv->cCursor, "#d70c0c");

	priv->iAddressWidth			= 8;
	priv->iRows					= 0;
	priv->iPrintRows			= 0;
	priv->iTopRow				= 0;
	priv->iPrintTopRow			= 0;
	priv->iVisibleRows			= 0;
	priv->iPrintVisibleRows		= 0;
	priv->iLastRow				= 0;
	priv->iPrintLastRow			= 0;
	priv->iCols					= 0;
	priv->iLeftCol				= 0;
	priv->iVisibleCols			= 0;
	priv->iBytesPerLine			= 16;
	priv->iPrintBytesPerLine	= 16;
	priv->iMaxVisibleBytes		= 0;
	priv->iPrintMaxVisibleBytes	= 0;
	priv->iStartByte			= 0;
	priv->iPrintStartByte		= 0;
	priv->iEndByte				= 0;
	priv->iPrintEndByte			= 0;
	priv->iPrintPages			= 0;
	priv->cursorArea			= AREA_HEX;
	priv->iBytePos				= 0;
	priv->bBytePosIsNibble		= FALSE;
	
	priv->bSelecting			= FALSE;
	priv->bLButttonDown			= FALSE;
	
	priv->selection 			= g_slice_alloc (sizeof (dataSelection));
	priv->selection->startSel	= -1;
	priv->selection->endSel		= -1;
	priv->selection->clipboard_startSel	= -1;
	priv->selection->clipboard_endSel	= -1;

	priv->iFileSize     	= 0;
	priv->bDrawAddresses	= TRUE;
	priv->bAutoBytesPerRow	= TRUE;
	priv->bDrawCharacters	= TRUE;	

	priv->hex_file			= NULL;
	priv->bIsOvertype		= TRUE;
	priv->num_entered		= 0;
    priv->num_del			= 0;
    priv->num_bs			= 0;

	priv->action_group_context_menu = g_simple_action_group_new ();

	g_action_map_add_action_entries (G_ACTION_MAP (priv->action_group_context_menu), 
									context_menu_entries, 
									G_N_ELEMENTS (context_menu_entries), widget);
	gtk_widget_insert_action_group (widget, "context", 
									G_ACTION_GROUP(priv->action_group_context_menu));
}

GtkWidget* rp_hex_view_new (void)
{
  return g_object_new (RP_TYPE_HEX_VIEW, NULL);
}

GtkWidget* rp_hex_view_new_with_file (RPHexFile *hex_file)
{
	GtkWidget	*widget;
	RPHexView	*hex_view;
	
	widget = rp_hex_view_new ();
	
	hex_view = RP_HEX_VIEW (widget);
	RPHexViewPrivate *priv = hex_view->priv;
	priv->hex_file = hex_file;
	priv->iFileSize = rp_hex_file_get_size (priv->hex_file);

	return widget;
}

PangoFontMetrics* rp_hex_view_get_fontmetrics (const char *font_name)
{
	PangoContext          *context;
	PangoFont             *new_font;
	PangoFontDescription  *new_desc;
	PangoFontMetrics      *new_metrics;

	new_desc  = pango_font_description_from_string (font_name);
	context   = gdk_pango_context_get();

	pango_context_set_language (context, gtk_get_default_language());

	new_font    = pango_context_load_font (context, new_desc);
	new_metrics = pango_font_get_metrics (new_font, pango_context_get_language (context));

	pango_font_description_free (new_desc);
	g_object_unref (G_OBJECT (context));
	g_object_unref (G_OBJECT (new_font));

	return new_metrics;
}

PangoFontMetrics* rp_hex_view_get_fontmetrics_print (PangoContext *context, const char *font_name)
{
	PangoFont             *new_font;
	PangoFontDescription  *new_desc;
	PangoFontMetrics      *new_metrics;

	pango_context_set_language (context, gtk_get_default_language());
	
	new_desc  = pango_font_description_from_string (font_name);
	new_font    = pango_context_load_font (context, new_desc);
	new_metrics = pango_font_get_metrics (new_font, pango_context_get_language (context));

	pango_font_description_free (new_desc);
	g_object_unref (G_OBJECT (new_font));

	return new_metrics;
}

static void rp_hex_view_update_character_size (RPHexViewPrivate *priv)
{
	priv->iCharHeight	= 	PANGO_PIXELS (pango_font_metrics_get_ascent (priv->pFontMetrics)) + 
                    		PANGO_PIXELS (pango_font_metrics_get_descent (priv->pFontMetrics));
	priv->iCharWidth	= 	PANGO_PIXELS (pango_font_metrics_get_approximate_char_width (priv->pFontMetrics));
}

static void rp_hex_view_update_character_size_print (RPHexViewPrivate *priv)
{
	priv->iPrintCharHeight = PANGO_PIXELS (pango_font_metrics_get_ascent (priv->pPrintFontMetrics)) + 
                    		PANGO_PIXELS (pango_font_metrics_get_descent (priv->pPrintFontMetrics));
	priv->iPrintCharWidth = PANGO_PIXELS (pango_font_metrics_get_approximate_char_width (priv->pPrintFontMetrics));
}

static void rp_hex_view_dispose (GObject *object)
{
	g_message ("Widget: called Dispose");
	RPHexView 			*hex_view 	= RP_HEX_VIEW (object);
	RPHexViewPrivate 	*priv 		= hex_view->priv;

	G_OBJECT_CLASS (rp_hex_view_parent_class)->dispose (object);

	if (priv->selection)
	{
		g_slice_free1 (sizeof (dataSelection), priv->selection);
		priv->selection = NULL;
	}

	priv->hex_file = NULL;
}

static void rp_hex_view_finalize (GObject *object)
{
	RPHexView 			*hex_view;
	RPHexViewPrivate 	*priv;

	hex_view = RP_HEX_VIEW (object);
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;
  
	if (priv->pFontMetrics)
		pango_font_metrics_unref (priv->pFontMetrics);

	if (priv->pPrintFontMetrics)
		pango_font_metrics_unref (priv->pPrintFontMetrics);

  	if (priv->pFontDescription)
		pango_font_description_free (priv->pFontDescription);

	if (priv->pPrintFontDescription)
		pango_font_description_free (priv->pPrintFontDescription);

	if (priv->pPrintFontName)
		g_free (priv->pPrintFontName);

  	if (priv->pLayout)
		g_object_unref (G_OBJECT (priv->pLayout));

	if (priv->pPrintLayout)
		g_object_unref (G_OBJECT (priv->pPrintLayout));

  	if (priv->hadjustment)
    	g_object_unref (priv->hadjustment);

  	if (priv->vadjustment)
    	g_object_unref (priv->vadjustment);

	g_clear_pointer (&priv->context_menu, gtk_widget_unparent);

	G_OBJECT_CLASS (rp_hex_view_parent_class)->finalize (object);

	g_message ("Widget: called Finalize");
}

static void rp_hex_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	RPHexView 			*hex_view;
	RPHexViewPrivate 	*priv;

	hex_view = RP_HEX_VIEW (object);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;

  	switch (prop_id)
  	{
    	case PROP_HADJUSTMENT:
    		rp_hex_view_set_hadjustment (hex_view, g_value_get_object (value));
      		break;

    	case PROP_VADJUSTMENT:
      		rp_hex_view_set_vadjustment (hex_view, g_value_get_object (value));
      		break;

    	case PROP_HSCROLL_POLICY:
      		if (priv->hscroll_policy != g_value_get_enum (value))
      		{
        		priv->hscroll_policy = g_value_get_enum (value);
        		gtk_widget_queue_resize (GTK_WIDGET (hex_view));
        		g_object_notify_by_pspec (object, pspec);
      		}
      		break;

    	case PROP_VSCROLL_POLICY:
      		if (priv->vscroll_policy != g_value_get_enum (value))
      		{
        		priv->vscroll_policy = g_value_get_enum (value);
    			gtk_widget_queue_resize (GTK_WIDGET (hex_view));
    			g_object_notify_by_pspec (object, pspec);
    		}
    		break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    		break;
  	}
}

static void rp_hex_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	RPHexView 			*hex_view;
	RPHexViewPrivate 	*priv;

	hex_view = RP_HEX_VIEW (object);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;

  	switch (prop_id)
	{
    	case PROP_HADJUSTMENT:
      		g_value_set_object (value, priv->hadjustment);
      		break;

    	case PROP_VADJUSTMENT:
      		g_value_set_object (value, priv->vadjustment);
    		break;

    	case PROP_HSCROLL_POLICY:
      		g_value_set_enum (value, priv->hscroll_policy);
      		break;

    	case PROP_VSCROLL_POLICY:
      		g_value_set_enum (value, priv->vscroll_policy);
      		break;

    	default:
    		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      		break;
	}
}

static void rp_hex_view_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;
 
	hex_view = RP_HEX_VIEW (widget);
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;
  
	gtk_widget_set_allocation(widget, allocation);

	if (gtk_widget_get_realized(widget))
	{     
		gdk_window_move_resize(	priv->hex_window, allocation->x, allocation->y, 
								allocation->width, allocation->height);

		rp_hex_view_update_layout(priv);

		rp_hex_view_set_hadjustment_values (hex_view);
		rp_hex_view_set_vadjustment_values (hex_view);
  	}
}

static void rp_hex_view_update_layout (RPHexViewPrivate *priv)
{
	priv->rectClient.x		= 0;
	priv->rectClient.y		= 0;
	priv->rectClient.width	= gdk_window_get_width(priv->hex_window);
	priv->rectClient.height	= gdk_window_get_height(priv->hex_window);
		
	if (priv->bDrawAddresses)
	{
		priv->rectAddresses.x		= priv->rectClient.x - priv->iCharWidth * priv->iLeftCol;
		priv->rectAddresses.y		= priv->rectClient.y;
		priv->rectAddresses.width	= priv->iCharWidth * (priv->iAddressWidth + 1);
		priv->rectAddresses.height	= priv->rectClient.height;
	}
	else
	{
		priv->rectAddresses.x		= priv->rectClient.x - priv->iCharWidth * priv->iLeftCol;
		priv->rectAddresses.y		= 0;
		priv->rectAddresses.width	= 0;
		priv->rectAddresses.height	= 0;
	}

	priv->rectHexBytes.x		= priv->rectAddresses.x + priv->rectAddresses.width;
	priv->rectHexBytes.y		= priv->rectAddresses.y;
	priv->rectHexBytes.width	= priv->rectClient.width - priv->rectAddresses.width;
	priv->rectHexBytes.height	= priv->rectClient.height;

	if (priv->bAutoBytesPerRow)
	{
		gint hmax = (gint)floor ((double)priv->rectHexBytes.width / (double)priv->iCharWidth);

		if (priv->bDrawCharacters)
		{
			hmax -= 2;
			if (hmax > 1)
				priv->iBytesPerLine = (gint)floor ((double)hmax / 4);
			else
				priv->iBytesPerLine = 1;
		}
		else
		{
			if (hmax > 1)
				priv->iBytesPerLine = (gint)floor ((double)hmax / 3);
			else
				priv->iBytesPerLine = 1;
		}
		
		priv->rectHexBytes.width = (gint)floor (((double)priv->iBytesPerLine) * priv->iCharWidth * 3);
	}
	else
	{
		priv->rectHexBytes.width = (gint)floor (((double)priv->iBytesPerLine) * priv->iCharWidth * 3);
	}

	priv->rectCharacters.x		= priv->bDrawCharacters ? priv->rectHexBytes.x + priv->rectHexBytes.width : 0;
	priv->rectCharacters.y		= priv->bDrawCharacters ? priv->rectHexBytes.y : 0;
	priv->rectCharacters.width	= priv->bDrawCharacters ? priv->iCharWidth * priv->iBytesPerLine : 0;
	priv->rectCharacters.height	= priv->bDrawCharacters ? priv->rectHexBytes.height : 0;

	gint iMaxHexVBytes		= (gint)floor ((double)priv->rectHexBytes.height / (double)priv->iCharHeight);
	priv->iMaxVisibleBytes	= priv->iBytesPerLine * iMaxHexVBytes;

	priv->iRows	= priv->iFileSize / priv->iBytesPerLine;
		
	if (priv->iRows % priv->iBytesPerLine != 0)
    	priv->iRows++;
	
	priv->iCols = priv->iBytesPerLine * 3;
	
	if (priv->bDrawAddresses)
		priv->iCols += priv->iAddressWidth;
	
	if (priv->bDrawCharacters)
		priv->iCols += priv->iBytesPerLine;
	
	priv->iVisibleCols	= priv->rectClient.width / priv->iCharWidth;
	priv->iVisibleRows	= priv->rectClient.height / priv->iCharHeight;
	priv->iLastRow		= CLAMP (priv->iVisibleRows, 1, (priv->iRows == 0) ? priv->iRows + 1 : priv->iRows);
}

static void rp_hex_view_update_layout_print (RPHexViewPrivate *priv, GtkPrintContext *context)
{
	priv->rectPrintClient.x			= 0;
	priv->rectPrintClient.y			= 0;
	priv->rectPrintClient.width		= gtk_print_context_get_width (context);
	priv->rectPrintClient.height	= gtk_print_context_get_height (context);
		
	if (priv->bDrawAddresses)
	{
		priv->rectPrintAddresses.x		= priv->rectPrintClient.x - priv->iPrintCharWidth;
		priv->rectPrintAddresses.y		= priv->rectPrintClient.y;
		priv->rectPrintAddresses.width	= priv->iPrintCharWidth * (priv->iAddressWidth + 1);
		priv->rectPrintAddresses.height	= priv->rectPrintClient.height;
	}
	else
	{
		priv->rectPrintAddresses.x		= priv->rectPrintClient.x - priv->iPrintCharWidth;
		priv->rectPrintAddresses.y		= 0;
		priv->rectPrintAddresses.width	= 0;
		priv->rectPrintAddresses.height	= 0;
	}

	priv->rectPrintHexBytes.x		= priv->rectPrintAddresses.x + priv->rectPrintAddresses.width;
	priv->rectPrintHexBytes.y		= priv->rectPrintAddresses.y;
	priv->rectPrintHexBytes.width	= priv->rectPrintClient.width - priv->rectPrintAddresses.width;
	priv->rectPrintHexBytes.height	= priv->rectPrintClient.height;

	if (priv->bAutoBytesPerRow)
	{
		gint hmax = (gint)floor ((double)priv->rectPrintHexBytes.width / (double)priv->iPrintCharWidth);

		if (priv->bDrawCharacters)
		{
			hmax -= 2;
			if (hmax > 1)
				priv->iPrintBytesPerLine = (gint)floor ((double)hmax / 4);
			else
				priv->iPrintBytesPerLine = 1;
		}
		else
		{
			if (hmax > 1)
				priv->iPrintBytesPerLine = (gint)floor ((double)hmax / 3);
			else
				priv->iPrintBytesPerLine = 1;
		}
		
		priv->rectPrintHexBytes.width = (gint)floor (((double)priv->iPrintBytesPerLine) * priv->iPrintCharWidth * 3);
	}
	else
	{
		priv->rectPrintHexBytes.width = (gint)floor (((double)priv->iPrintBytesPerLine) * priv->iPrintCharWidth * 3);
	}

	priv->rectPrintCharacters.x			= priv->bDrawCharacters ? priv->rectPrintHexBytes.x + priv->rectPrintHexBytes.width : 0;
	priv->rectPrintCharacters.y			= priv->bDrawCharacters ? priv->rectPrintHexBytes.y : 0;
	priv->rectPrintCharacters.width		= priv->bDrawCharacters ? priv->iPrintCharWidth * priv->iPrintBytesPerLine : 0;
	priv->rectPrintCharacters.height	= priv->bDrawCharacters ? priv->rectPrintHexBytes.height : 0;

	gint iMaxHexVBytes			= (gint)floor ((double)priv->rectPrintHexBytes.height / (double)priv->iPrintCharHeight);
	priv->iPrintMaxVisibleBytes	= priv->iPrintBytesPerLine * iMaxHexVBytes;
	priv->iPrintRows			= priv->iFileSize / priv->iPrintBytesPerLine;
		
	if (priv->iPrintRows % priv->iPrintBytesPerLine != 0)
    	priv->iPrintRows++;

	priv->iPrintPages = priv->iPrintRows / iMaxHexVBytes;

	if (priv->iPrintRows % iMaxHexVBytes != 0)
		priv->iPrintPages++;
	
	priv->iPrintVisibleRows	= priv->rectPrintClient.height / priv->iPrintCharHeight;
	priv->iPrintLastRow 	= MIN (priv->iPrintVisibleRows, priv->iPrintRows - priv->iPrintTopRow);
}

static void rp_hex_view_update_byte_visibility (RPHexViewPrivate *priv)
{
	if (priv->iFileSize == 0)
    	return;

    guint32 vadjPos = (guint32)gtk_adjustment_get_value (priv->vadjustment);

	priv->iStartByte = (vadjPos + 1) * priv->iBytesPerLine - priv->iBytesPerLine;
	priv->iEndByte = (guint32)MIN (priv->iFileSize - 1, 
									priv->iStartByte + 
									priv->iMaxVisibleBytes - 1);
}

static void rp_hex_view_update_byte_visibility_print (RPHexViewPrivate *priv)
{
	if (priv->iFileSize == 0)
    	return;

	priv->iPrintStartByte = (priv->iPrintTopRow + 1) * 
							priv->iPrintBytesPerLine - 
							priv->iPrintBytesPerLine;
	priv->iPrintEndByte	= (guint32)MIN (priv->iFileSize - 1, 
							priv->iPrintStartByte + 
							priv->iPrintMaxVisibleBytes - 1);
}

static gboolean rp_hex_view_draw (GtkWidget *widget, cairo_t *cr)
{
	RPHexView         *hex_view;
	RPHexViewPrivate  *priv;

	hex_view  = RP_HEX_VIEW (widget);
	
	g_return_val_if_fail (RP_IS_HEX_VIEW (hex_view), FALSE);

	priv = hex_view->priv;

	rp_hex_view_update_byte_visibility (priv);

	// draw address data
	if (priv->bDrawAddresses)
		rp_hex_view_draw_address_lines (priv, cr);

	// draw hex / ascii data
	rp_hex_view_draw_hex_lines (priv, cr);
	
	// draw selection data
	rp_hex_view_draw_selection (priv, cr);
	
	// draw cursor data
	rp_hex_view_draw_cursor (priv, cr);

	return TRUE;
}

static void rp_hex_view_draw_address_lines (RPHexViewPrivate *priv, cairo_t *cr)
{
	gchar sAddrLine[9];

	// paint addresses background
	cairo_set_source_rgb (cr, priv->cAddressBg.red, priv->cAddressBg.green, priv->cAddressBg.blue);
	cairo_rectangle (	cr, 
						priv->rectAddresses.x, 
						priv->rectAddresses.y, 
						priv->rectAddresses.width, 
						priv->rectAddresses.height);
	cairo_fill (cr);

	// paint addresses
	cairo_set_source_rgb (cr, priv->cAddressFg.red, priv->cAddressFg.green, priv->cAddressFg.blue);

	for (gint i = 0; i < priv->iLastRow; i++)
	{
    	g_snprintf (sAddrLine, sizeof(sAddrLine), "%08X", (priv->iTopRow + i) * priv->iBytesPerLine);
		cairo_move_to (cr, priv->rectAddresses.x + priv->iCharWidth / 2, i * priv->iCharHeight);
  		pango_layout_set_text (priv->pLayout, sAddrLine, priv->iAddressWidth);
    	pango_cairo_show_layout (cr, priv->pLayout);
  	}
}

static void rp_hex_view_draw_address_lines_print (RPHexViewPrivate *priv, cairo_t *cr)
{
	gchar sAddrLine[9];

	// paint addresses background
	cairo_set_source_rgb (cr, priv->cAddressBg.red, priv->cAddressBg.green, priv->cAddressBg.blue);
	cairo_rectangle (	cr, 
						priv->rectPrintAddresses.x, 
						priv->rectPrintAddresses.y, 
						priv->rectPrintAddresses.width, 
						priv->rectPrintAddresses.height);
	cairo_fill (cr);

	// paint addresses
	cairo_set_source_rgb (cr, priv->cAddressFg.red, priv->cAddressFg.green, priv->cAddressFg.blue);

	for (gint i = 0; i < priv->iPrintLastRow; i++)
	{
    	g_snprintf (sAddrLine, sizeof(sAddrLine), "%08X", (priv->iPrintTopRow + i) * priv->iPrintBytesPerLine);
		cairo_move_to (cr, priv->rectPrintAddresses.x + priv->iPrintCharWidth / 2, i * priv->iPrintCharHeight);
  		pango_layout_set_text (priv->pPrintLayout, sAddrLine, priv->iAddressWidth);
    	pango_cairo_show_layout (cr, priv->pPrintLayout);
  	}
}

static void rp_hex_view_draw_hex_lines (RPHexViewPrivate *priv, cairo_t *cr)
{
	guchar 	hByte[3];
	guchar 	aByte[2] = "\0\0";
	gint 	row, column;
	guint32 tmpEndByte = MIN ((priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1, priv->iEndByte);
	guint32 bytesToRead = tmpEndByte - priv->iStartByte + 1;
	guint32 bytesRead = 0;
	guchar  *buffer = g_try_malloc0 (bytesToRead);

	g_assert (buffer != NULL);

	// paint hex data background
	cairo_set_source_rgb (cr, priv->cLightGray.red, priv->cLightGray.green, priv->cLightGray.blue);
	cairo_rectangle (	cr, 
						priv->rectHexBytes.x, 
						priv->rectHexBytes.y, 
						priv->rectHexBytes.width, 
						priv->rectHexBytes.height);
	cairo_fill (cr);

	// paint hex data
	cairo_set_source_rgb (cr, 0, 0, 0);
	
	bytesRead = rp_hex_file_get_data (priv->hex_file, buffer, bytesToRead, priv->iStartByte);

	g_return_if_fail (bytesRead == bytesToRead);

	for (guint32 i = priv->iStartByte; i <= tmpEndByte; i++)
	{
		row		= i / priv->iBytesPerLine - priv->iTopRow;
		column	= (i - priv->iTopRow * priv->iBytesPerLine) - row * priv->iBytesPerLine;
		g_snprintf (hByte, sizeof(hByte), "%02X", buffer[i - priv->iStartByte]);
		
		cairo_move_to (cr, priv->rectHexBytes.x + column * priv->iCharWidth * 3 , row * priv->iCharHeight);
		pango_layout_set_text (priv->pLayout, hByte, 2);
    	pango_cairo_show_layout (cr, priv->pLayout);

		if (priv->bDrawCharacters)
		{
			aByte[0] = is_ascii (buffer[i - priv->iStartByte]) ? buffer[i - priv->iStartByte] : '.';
			cairo_move_to (cr, priv->rectCharacters .x + column * priv->iCharWidth , row * priv->iCharHeight);
  			pango_layout_set_text (priv->pLayout, aByte, 1);
    		pango_cairo_show_layout (cr, priv->pLayout);
		}
	}

	g_free (buffer);
    buffer = NULL;
}

static void rp_hex_view_draw_hex_lines_print (RPHexViewPrivate *priv, cairo_t *cr)
{
	guchar 	hByte[3];
	guchar 	aByte[2] = "\0\0";
	gint 	row, column;
	guint32 tmpEndByte = MIN ((priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1, priv->iPrintEndByte);
	guint32 bytesToRead = tmpEndByte - priv->iPrintStartByte;
	guint32 bytesRead = 0;
	guchar  *buffer = g_try_malloc0 (bytesToRead);

	g_assert (buffer != NULL);

	// paint hex data background
	cairo_set_source_rgb (cr, priv->cLightGray.red, priv->cLightGray.green, priv->cLightGray.blue);
	cairo_rectangle (	cr, 
						priv->rectPrintHexBytes.x, 
						priv->rectPrintHexBytes.y, 
						priv->rectPrintHexBytes.width, 
						priv->rectPrintHexBytes.height);
	cairo_fill (cr);

	// paint hex data
	cairo_set_source_rgb (cr, 0, 0, 0);
	
	bytesRead = rp_hex_file_get_data (priv->hex_file, buffer, bytesToRead, priv->iPrintStartByte);

	g_return_if_fail (bytesRead == bytesToRead);

	for (guint32 i = priv->iPrintStartByte; i <= tmpEndByte; i++)
	{
		row		= i / priv->iPrintBytesPerLine - priv->iPrintTopRow;
		column	= (i - priv->iPrintTopRow * priv->iPrintBytesPerLine) - row * priv->iPrintBytesPerLine;
		g_snprintf (hByte, sizeof(hByte), "%02X", buffer[i - priv->iPrintStartByte]);
		
		cairo_move_to (cr, priv->rectPrintHexBytes.x + column * priv->iPrintCharWidth * 3 , row * priv->iPrintCharHeight);
		pango_layout_set_text (priv->pPrintLayout, hByte, 2);
    	pango_cairo_show_layout (cr, priv->pPrintLayout);

		if (priv->bDrawCharacters)
		{
			aByte[0] = is_ascii (buffer[i - priv->iPrintStartByte]) ? buffer[i - priv->iPrintStartByte] : '.';
			cairo_move_to (cr, priv->rectPrintCharacters .x + column * priv->iPrintCharWidth , row * priv->iPrintCharHeight);
  			pango_layout_set_text (priv->pPrintLayout, aByte, 1);
    		pango_cairo_show_layout (cr, priv->pPrintLayout);
		}
	}

	g_free (buffer);
    buffer = NULL;
}

static void rp_hex_view_draw_selection (RPHexViewPrivate *priv, cairo_t *cr)
{
	if (priv->selection->startSel < 0 || priv->selection->endSel < 0)
		return;

	guint32 selStart	= MIN (priv->selection->startSel, G_MAXUINT32);
	guint32 selEnd		= MIN (priv->selection->endSel, G_MAXUINT32);

	if (selStart > selEnd)
	{
		selStart	= selEnd;
		selEnd		= MIN (priv->selection->startSel, G_MAXUINT32);
	}

	gdk_cairo_set_source_rgba (cr, &priv->cAddressFg);
	cairo_set_operator (cr, CAIRO_OPERATOR_DARKEN);

	gint max_column 	= priv->iBytesPerLine * 3 - 1;
	gint max_column_ch	= priv->iBytesPerLine;
	gint start_row		= selStart / priv->iBytesPerLine - priv->iTopRow;
	gint start_column	= (selStart - priv->iTopRow * priv->iBytesPerLine) - start_row * priv->iBytesPerLine;
	gint end_row		= selEnd / priv->iBytesPerLine - priv->iTopRow;
	gint end_column		= (selEnd - priv->iTopRow * priv->iBytesPerLine) - (end_row * priv->iBytesPerLine);

	for (gint i = start_row; i <= end_row; i++)
	{
		if (i > start_row)	start_column = 0;		
		if (i == end_row)	max_column = (end_column + 1) * 3 - 1;
		if (i == end_row)	max_column_ch = end_column + 1;

		cairo_rectangle (	cr, 
							priv->rectHexBytes.x + start_column * priv->iCharWidth * 3, 
							priv->rectHexBytes.y + i * priv->iCharHeight,
							priv->iCharWidth * max_column - (start_column * priv->iCharWidth * 3), 
							priv->iCharHeight);
	
		cairo_fill (cr);
		
		if (priv->bDrawCharacters)
		{
			cairo_rectangle (	cr, 
								priv->rectCharacters.x + start_column * priv->iCharWidth, 
								priv->rectCharacters.y + i * priv->iCharHeight, 
								priv->iCharWidth * max_column_ch - (start_column * priv->iCharWidth), 
								priv->iCharHeight);
			cairo_fill (cr);
		}
	}
}

static void rp_hex_view_draw_cursor (RPHexViewPrivate *priv, cairo_t *cr)
{
	gchar 	hByte[3];
	guchar 	aByte[2] = "\0\0";
	guchar 	sByte[1] = "\0";
	gint 	start_row		= priv->iBytePos / priv->iBytesPerLine - priv->iTopRow;
	gint 	start_column	= (priv->iBytePos - priv->iTopRow * priv->iBytesPerLine) - start_row * priv->iBytesPerLine;

	rp_hex_file_get_data (priv->hex_file, sByte, 1, priv->iBytePos);

	gdk_cairo_set_source_rgba (cr, &priv->cCursor);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_rectangle (	cr, 
							priv->rectHexBytes.x + start_column * priv->iCharWidth * 3, 
							priv->rectHexBytes.y + start_row * priv->iCharHeight,
							priv->iCharWidth, 
							priv->iCharHeight);
	
	if (priv->cursorArea == AREA_HEX || priv->cursorArea == AREA_ADDRESS)
	{
		cairo_fill (cr);
	
		cairo_set_source_rgb (cr, 1, 1, 1);
		
		g_snprintf (hByte, sizeof(hByte), "%02X", sByte[0]);
		cairo_move_to (cr, priv->rectHexBytes.x + start_column * priv->iCharWidth * 3 , start_row * priv->iCharHeight);
		pango_layout_set_text (priv->pLayout, hByte, 1);
    	pango_cairo_show_layout (cr, priv->pLayout);
	}
	else
		cairo_stroke (cr);

	if (priv->bDrawCharacters)
	{
		gdk_cairo_set_source_rgba (cr, &priv->cCursor);
		cairo_rectangle (	cr, 
							priv->rectCharacters.x + start_column * priv->iCharWidth, 
							priv->rectCharacters.y + start_row * priv->iCharHeight, 
							priv->iCharWidth, 
							priv->iCharHeight);
		
		if (priv->cursorArea == AREA_TEXT)
		{
			aByte[0] = is_ascii (sByte[0]) ? sByte[0] : '.';
			cairo_fill (cr);
			cairo_set_source_rgb (cr, 1, 1, 1);
			cairo_move_to (cr, priv->rectCharacters .x + start_column * priv->iCharWidth , start_row * priv->iCharHeight);
  			pango_layout_set_text (priv->pLayout, aByte, 1);
    		pango_cairo_show_layout (cr, priv->pLayout);
		}
		else
			cairo_stroke (cr);
	}
}

static void rp_hex_view_realize (GtkWidget *widget)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;
	GtkAllocation		alloc;
	GdkWindowAttr 		attrs;
	guint				attrs_mask;

	hex_view = RP_HEX_VIEW (widget);
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));
	
	priv = hex_view->priv;

	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &alloc);
	gtk_widget_set_can_focus (widget, TRUE);
  
	attrs.x           	= alloc.x;
	attrs.y           	= alloc.y;
	attrs.width       	= alloc.width;
	attrs.height      	= alloc.height;
	attrs.window_type 	= GDK_WINDOW_CHILD;
	attrs.wclass      	= GDK_INPUT_OUTPUT;
	attrs.event_mask  	= gtk_widget_get_events (widget) |
							GDK_BUTTON_PRESS_MASK |
							GDK_BUTTON_RELEASE_MASK |
							GDK_BUTTON_MOTION_MASK |
							GDK_KEY_PRESS_MASK |
							GDK_KEY_RELEASE_MASK |
							GDK_SCROLL_MASK |
							GDK_ALL_EVENTS_MASK; 
	attrs_mask 			= GDK_WA_X | GDK_WA_Y;

	priv->hex_window = gdk_window_new (gtk_widget_get_parent_window (widget), &attrs, attrs_mask);

	gdk_window_set_user_data (priv->hex_window, widget);
	gtk_widget_set_window (widget, priv->hex_window);
	
	// set minimum widget height and width
	gtk_widget_set_size_request (gtk_widget_get_parent(widget), 40 * priv->iCharWidth, 10 * priv->iCharHeight);

	g_message ("Widget: called Realize");
}

static void rp_hex_view_unrealize (GtkWidget *widget)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;

	g_clear_pointer (&priv->context_menu, gtk_widget_unparent);

	GTK_WIDGET_CLASS (rp_hex_view_parent_class)->unrealize (widget);

	g_message ("Widget: called Unrealize");
}

static void rp_hex_view_map (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (rp_hex_view_parent_class)->map (widget);
}

static void rp_hex_view_set_hadjustment (RPHexView *hex_view, GtkAdjustment *adjustment)
{
  RPHexViewPrivate *priv = hex_view->priv;

  if (adjustment && priv->hadjustment == adjustment)
    return;

  if (priv->hadjustment != NULL)
  {
    g_signal_handlers_disconnect_by_func (priv->hadjustment, rp_hex_view_scroll_value_changed, hex_view);
    g_object_unref (priv->hadjustment);
  }

  if (adjustment == NULL)
    adjustment = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed", G_CALLBACK (rp_hex_view_scroll_value_changed), hex_view);
  
  priv->hadjustment = g_object_ref_sink (adjustment);
  
  rp_hex_view_set_hadjustment_values (hex_view);

  g_object_notify (G_OBJECT (hex_view), "hadjustment");
}

static void rp_hex_view_set_vadjustment (RPHexView *hex_view, GtkAdjustment *adjustment)
{
  RPHexViewPrivate *priv = hex_view->priv;

  if (adjustment && priv->vadjustment == adjustment)
    return;

  if (priv->vadjustment != NULL)
  {
    g_signal_handlers_disconnect_by_func (priv->vadjustment, rp_hex_view_scroll_value_changed, hex_view);
    g_object_unref (priv->vadjustment);
  }

  if (adjustment == NULL)
    adjustment = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  g_signal_connect (adjustment, "value-changed", G_CALLBACK (rp_hex_view_scroll_value_changed), hex_view);
  
  priv->vadjustment = g_object_ref_sink (adjustment);
  
  rp_hex_view_set_vadjustment_values (hex_view);

  g_object_notify (G_OBJECT (hex_view), "vadjustment");
}

static void rp_hex_view_set_hadjustment_values (RPHexView *hex_view)
{
	RPHexViewPrivate 	*priv;
	GtkWidget			*widget;
  
  	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv	= hex_view->priv;
  	widget	= GTK_WIDGET (hex_view);

	if (!gtk_widget_get_realized(widget))
		return;

	gtk_adjustment_configure (	priv->hadjustment,
								priv->iLeftCol,
								0,
								priv->iCols,
								1,
								priv->iVisibleCols - 1,
								priv->iVisibleCols);
}

static void rp_hex_view_set_vadjustment_values (RPHexView *hex_view)
{
	RPHexViewPrivate  *priv;
	GtkWidget         *widget;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));
	
	priv    = hex_view->priv;
	widget  = GTK_WIDGET (hex_view);

	if (!gtk_widget_get_realized(widget))
    	return;

	gtk_adjustment_configure (	priv->vadjustment,
								priv->iTopRow,
								0,
								priv->iRows,
								1,
								priv->iVisibleRows - 1,
								priv->iVisibleRows);
}

static void rp_hex_view_scroll_value_changed (GtkAdjustment *adjustment, RPHexView *hex_view)
{
	RPHexViewPrivate *priv;
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;
  
	gint iMoveDiff = 0;
	gint iNewValue = (gint)gtk_adjustment_get_value(adjustment);

	if (adjustment == priv->vadjustment)
	{
    	iMoveDiff = (priv->iTopRow - iNewValue) * priv->iCharHeight;
    	priv->iTopRow = iNewValue;
  	}

	if (adjustment == priv->hadjustment)
	{
		iMoveDiff = (priv->iLeftCol - iNewValue) * priv->iCharWidth;
    	priv->iLeftCol = iNewValue;
	}

	rp_hex_view_update_layout(priv);
}

static gboolean rp_hex_view_button_callback (GtkWidget *widget, GdkEventButton *event)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;
	guint32 			curPos = 0;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;
	
	g_return_val_if_fail (RP_IS_HEX_VIEW (hex_view), TRUE);

	if ((event->type == GDK_BUTTON_RELEASE) && (event->button == GDK_BUTTON_PRIMARY))
	{
		gtk_grab_remove(widget);
		priv->bSelecting = FALSE;
		priv->bLButttonDown = FALSE;

		return TRUE;
	}
	else 
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == GDK_BUTTON_PRIMARY)) 
	{
		priv->bLButttonDown = TRUE;
		priv->bSelecting = (event->state & GDK_SHIFT_MASK) ? TRUE : FALSE;
		
		if (!gtk_widget_has_focus (GTK_WIDGET (hex_view)))
			gtk_widget_grab_focus (GTK_WIDGET(hex_view));
		
		gtk_grab_add(widget);

		curPos = rp_hex_view_bytepos_from_point (priv, event->x, event->y, TRUE);
		rp_hex_view_set_cursor (widget, curPos);

		return TRUE;
	}
	else
	if (gdk_event_triggers_context_menu ((GdkEvent *) event) && event->type == GDK_BUTTON_PRESS)
	{
		rp_hex_view_context_menu_show (widget, event);
		return TRUE;
	}
	
	return FALSE;
}

static gboolean rp_hex_view_motion_callback	(GtkWidget *widget, GdkEventMotion *event)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;
	guint32 			curPos = 0;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;
	
	g_return_val_if_fail (RP_IS_HEX_VIEW (hex_view), TRUE);

	if (priv->bLButttonDown)
	{
		glong selStart	= priv->selection->startSel;
		glong selEnd	= priv->selection->endSel;

		priv->bSelecting = TRUE;
		curPos = rp_hex_view_bytepos_from_point (priv, event->x, event->y, TRUE);
		selEnd = curPos;

		rp_hex_view_set_cursor (widget, curPos);
	}

	return TRUE;
}

static gboolean rp_hex_view_scroll_callback	(GtkWidget *widget, GdkEventScroll *event)
{
	return FALSE;
}

static gboolean rp_hex_view_key_press_callback(GtkWidget *widget, GdkEventKey *event)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;
	gboolean 			ret = FALSE;
	guint32 			clampFileSize;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;
	
	g_return_val_if_fail (RP_IS_HEX_VIEW (hex_view), TRUE);

	/* Binding set */
	if (GTK_WIDGET_CLASS (rp_hex_view_parent_class)->key_press_event (widget, event))
	{
		ret = TRUE;
	}
	else 
	{
		priv->bSelecting = (event->state & GDK_SHIFT_MASK) ? TRUE : FALSE;

 		clampFileSize = (priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1;

		switch(event->keyval)
		{
			case GDK_KEY_Up:
				rp_hex_view_set_cursor(widget, CLAMP ((glong)priv->iBytePos - priv->iBytesPerLine, 0, clampFileSize));
				ret = TRUE;
				break;
			case GDK_KEY_Down:
				rp_hex_view_set_cursor(widget, CLAMP ((glong)priv->iBytePos + priv->iBytesPerLine, 0, clampFileSize));
				ret = TRUE;
				break;
			case GDK_KEY_Left:
				rp_hex_view_set_cursor(widget, CLAMP ((glong)priv->iBytePos - 1, 0, clampFileSize));
				ret = TRUE;
				break;
			case GDK_KEY_Right:
				rp_hex_view_set_cursor(widget, CLAMP ((glong)priv->iBytePos + 1, 0, clampFileSize));
				ret = TRUE;
				break;
			case GDK_KEY_Page_Up:
				rp_hex_view_set_cursor(widget, CLAMP ((glong)priv->iBytePos - priv->iVisibleRows * priv->iBytesPerLine, 0, clampFileSize));
				ret = TRUE;
				break;
			case GDK_KEY_Page_Down:
				rp_hex_view_set_cursor(widget, CLAMP ((glong)priv->iBytePos + priv->iVisibleRows * priv->iBytesPerLine, 0, clampFileSize));
				ret = TRUE;
				break;
			case GDK_KEY_Tab:
			case GDK_KEY_KP_Tab:
				priv->cursorArea = (priv->cursorArea == AREA_HEX || priv->cursorArea == AREA_ADDRESS) ? AREA_TEXT : AREA_HEX;
				gtk_widget_queue_draw (widget);
				ret = TRUE;
				break;
			default:
				priv->num_del = priv->num_bs = 0;
				guchar cc = '\0';

				if (priv->cursorArea == AREA_HEX && !isxdigit (event->keyval))
				{
					g_message("This is not a valid hexadecimal value.");
					gdk_display_beep (gdk_display_get_default());
					return TRUE;
				}

				if (rp_hex_file_is_read_only (priv->hex_file))
				{
					g_message("Editing not allowed in Read-Only mode.");
					gdk_display_beep (gdk_display_get_default());
					return TRUE;
				}

				if (priv->bIsOvertype && priv->selection->startSel != priv->selection->endSel)
				{
					g_message("Deleting not allowed in Overtype mode.");
					gdk_display_beep (gdk_display_get_default());
					return TRUE;
				}

				if (priv->bIsOvertype && priv->iBytePos == priv->iFileSize)
				{
					g_message("Appending not allowed in Overtype mode.");
					gdk_display_beep (gdk_display_get_default());
					return TRUE;
				}

				if (rp_hex_view_has_selection (widget))
        		{
            		rp_hex_file_change_data (priv->hex_file,
											mod_delforw, 
											priv->selection->startSel,
											priv->selection->endSel - priv->selection->startSel,
											NULL, 0);
        		}

				if (priv->cursorArea == AREA_TEXT)
				{
					cc = (guchar)event->keyval;
					rp_hex_file_change_data (priv->hex_file,
											priv->bIsOvertype ? mod_replace : mod_insert,
											priv->iBytePos, 1, 
											&cc, 
											priv->num_entered);
					
					rp_hex_view_set_cursor (widget, CLAMP ((glong)priv->iBytePos + 1, 0, clampFileSize));
					priv->num_entered += 2;
					ret = TRUE;
				}
				else
				{
					if ((priv->num_entered % 2) == 0)
					{
						//  1st nibble entered - convert to low nibble
						cc = isdigit (event->keyval) ? event->keyval - '0' : toupper (event->keyval) - 'A' + 10;
						rp_hex_file_change_data (priv->hex_file,
											priv->bIsOvertype ? mod_replace : mod_insert,
											priv->iBytePos, 1, 
											&cc, 
											priv->num_entered);
						
						priv->num_entered++;

						gtk_widget_queue_draw (widget);
					}
					else
					{
						// 2nd nibble entered - convert to higher nibble
						cc = '\0';

						if (priv->iBytePos <= clampFileSize)
						{
							guint rByte = rp_hex_file_get_data (priv->hex_file, &cc, 1, priv->iBytePos);
							g_assert (rByte == 1);
						}

						cc = (cc << 4) + (isdigit (event->keyval) ? event->keyval - '0' : toupper(event->keyval) - 'A' + 10);

						rp_hex_file_change_data (priv->hex_file,
											priv->bIsOvertype ? mod_replace : mod_insert,
											priv->iBytePos, 1, 
											&cc, 
											priv->num_entered);

						priv->num_entered++;
						rp_hex_view_set_cursor (widget, CLAMP ((glong)priv->iBytePos + 1, 0, clampFileSize));
					}

					ret = TRUE;
				}
		}
	}

	return ret;
}

static gboolean rp_hex_view_key_release_callback(GtkWidget *widget, GdkEventKey *event)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;
		
	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;
	
	g_return_val_if_fail (RP_IS_HEX_VIEW (hex_view), TRUE);

	if (event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R) {
		priv->bSelecting = FALSE;
	}

	return TRUE;
}

gboolean rp_hex_view_has_selection (GtkWidget *widget)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;

	g_return_val_if_fail (RP_IS_HEX_VIEW (hex_view), FALSE);

	return (priv->selection->startSel >= 0 && priv->selection->endSel >= 0);
}

static void rp_hex_view_remove_selection (GtkWidget *widget)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	if (rp_hex_view_has_selection (widget))
	{
		priv->selection->startSel 	= -1; 
		priv->selection->endSel		= -1;

		gtk_widget_queue_draw (widget);

		g_signal_emit_by_name (G_OBJECT(hex_view), "selection_changed");
	}
}

static void rp_hex_view_set_selection (GtkWidget *widget, glong selStartPos, glong selEndPos)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	g_assert (selEndPos >= 0);

	if (selStartPos == -1) selStartPos = selEndPos;

	guint32 selStart	= CLAMP((glong)selStartPos, 0, (priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1);
	guint32 selEnd		= CLAMP((glong)selEndPos, 0, (priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1);

	priv->iBytePos				= selEnd;
	priv->selection->startSel 	= selStart;
	priv->selection->endSel		= selEnd;
}

static void rp_hex_view_set_cursor (GtkWidget *widget, guint32 newPos)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;
	guint32				oldBytePos;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	oldBytePos		= priv->iBytePos;	// save old postion when starting selection by keyboard
	priv->iBytePos	= CLAMP (newPos, 0, (priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1);

	if (priv->bSelecting)
		rp_hex_view_set_selection (	widget, 
						(priv->selection->startSel == -1) ? oldBytePos : priv->selection->startSel, 
						priv->iBytePos);
	else
		rp_hex_view_remove_selection (widget);
	

	if (priv->iBytePos < priv->iStartByte || priv->iBytePos > priv->iEndByte)
		rp_hex_view_scroll_byte_into_view (priv, priv->iBytePos);
	
	gtk_widget_queue_draw (widget);

	if (oldBytePos != priv->iBytePos)
	{
		priv->num_entered = priv->num_del = priv->num_bs = 0;

		g_signal_emit_by_name (G_OBJECT(hex_view), "byte_pos_changed", priv->iBytePos);

		if (priv->bSelecting)
			g_signal_emit_by_name (G_OBJECT(hex_view), "selection_changed");
	}
}

static void rp_hex_view_update_cursor_area (RPHexViewPrivate *priv, gdouble posX, gdouble posY)
{
	priv->cursorArea = AREA_NONE;

	if (posX < priv->rectAddresses.width)
	{
		priv->cursorArea = AREA_ADDRESS;
	}
	if (posX > priv->rectHexBytes.x && posX < priv->rectHexBytes.x + priv->rectHexBytes.width)
	{
		priv->cursorArea = AREA_HEX;
	}
	if (posX > priv->rectCharacters.x && posX < priv->rectCharacters.x + priv->rectCharacters.width)
	{
		priv->cursorArea = AREA_TEXT;
	}
}

static void rp_hex_view_scroll_byte_into_view (RPHexViewPrivate *priv, guint32 curPos)
{
	gint	sel_row = curPos / priv->iBytesPerLine - priv->iTopRow + 1;
	gdouble value	= gtk_adjustment_get_value (priv->vadjustment);

	if (sel_row > priv->iVisibleRows)
	{
		gtk_adjustment_set_value (priv->vadjustment, value + sel_row - priv->iVisibleRows);
	}

	if (curPos < priv->iStartByte)
	{
		gtk_adjustment_set_value (priv->vadjustment, value - 1);
	}
}

static guint32 rp_hex_view_bytepos_from_point (RPHexViewPrivate *priv, 
												gdouble posX, gdouble posY, gboolean bSnap)
{
	gint xPos, yPos = 0;
	glong newPos 	= 0;

	rp_hex_view_update_cursor_area (priv, posX, posY);

	priv->bBytePosIsNibble = FALSE;

	if (priv->cursorArea == AREA_HEX)
	{
		priv->bBytePosIsNibble = (((gint)((posX - priv->rectHexBytes.x) / priv->iCharWidth)) % 3) > 0;

		xPos = (gint)(posX - priv->rectHexBytes.x) / priv->iCharWidth / 3;
		yPos = (gint)(posY - priv->rectHexBytes.y) / priv->iCharHeight;
		
		if (posY < 0) yPos -= 1;

		newPos = CLAMP ((glong)priv->iStartByte + priv->iBytesPerLine * yPos + xPos,
						0,
						(priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1);
	}

	if (priv->cursorArea == AREA_TEXT)
	{
		xPos = (gint)(posX - priv->rectCharacters.x) / priv->iCharWidth;
		yPos = (gint)(posY - priv->rectCharacters.y) / priv->iCharHeight;

		if (posY < 0) yPos -= 1;

		newPos = CLAMP ((glong)priv->iStartByte + priv->iBytesPerLine * yPos + xPos,
						0,
						(priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1);
	}

	if (priv->cursorArea == AREA_ADDRESS)
	{
		yPos = (gint)(posY - priv->rectAddresses.y) / priv->iCharHeight;

		if (posY < 0) yPos -= 1;

		newPos = CLAMP ((glong)priv->iStartByte + priv->iBytesPerLine * yPos,
						0,
						(priv->iFileSize == 0) ? priv->iFileSize : priv->iFileSize - 1);
	}
	
	if (bSnap)
		priv->bBytePosIsNibble = FALSE;

	return newPos;
}

static gboolean rp_hex_view_context_menu (GtkWidget *widget)
{
	g_message ("Widget: Show context menu without event");
	rp_hex_view_context_menu_show (widget, NULL);

	return TRUE;
}

static void rp_hex_view_context_menu_update_actions (RPHexView *hex_view)
{
	RPHexViewPrivate	*priv;
	GtkClipboard		*clipboard;
	gboolean			bCanPaste;
	
	priv = hex_view->priv;
	
	gboolean bHasSelection = rp_hex_view_has_selection (GTK_WIDGET(hex_view));

	GAction	*action_cut = g_action_map_lookup_action (G_ACTION_MAP (priv->action_group_context_menu), 
														context_menu_entries[0].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action_cut), bHasSelection);

	GAction	*action_copy = g_action_map_lookup_action (G_ACTION_MAP (priv->action_group_context_menu), 
														context_menu_entries[1].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action_copy), bHasSelection);

	clipboard	= gtk_widget_get_clipboard (GTK_WIDGET (hex_view), GDK_SELECTION_CLIPBOARD);
	bCanPaste	= gtk_clipboard_wait_is_target_available (clipboard, gdk_atom_intern_static_string ("BINARY"));

	GAction	*action_paste = g_action_map_lookup_action (G_ACTION_MAP (priv->action_group_context_menu), 
														context_menu_entries[2].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action_paste), bCanPaste);

	GAction	*action_delete = g_action_map_lookup_action (G_ACTION_MAP (priv->action_group_context_menu), 
														context_menu_entries[3].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action_delete), bHasSelection);

	GAction	*action_select_all = g_action_map_lookup_action (G_ACTION_MAP (priv->action_group_context_menu), 
														context_menu_entries[4].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action_select_all), TRUE);
}

static void rp_hex_view_context_menu_show (GtkWidget *widget, GdkEventButton *event)
{
	g_message ("Widget: Show context menu");
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;
	
	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	rp_hex_view_context_menu_update_actions (hex_view);

	if (!priv->context_menu)
    {
      GMenuModel *model = rp_hex_view_get_context_menu_model (hex_view);
      priv->context_menu = gtk_menu_new_from_model (model);
	  gtk_menu_attach_to_widget (GTK_MENU (priv->context_menu), widget, NULL);
	}

	/* FIXME check for event == NULL
	*/

	gtk_menu_popup_at_pointer (GTK_MENU (priv->context_menu), (GdkEvent *) event);
}

static GMenuModel *rp_hex_view_get_context_menu_model (RPHexView *hex_view)
{
	RPHexViewPrivate 	*priv;
	GMenu				*menu;
	GMenu 				*section;
	GMenuItem			*item;
	
	g_return_val_if_fail (RP_IS_HEX_VIEW (hex_view), NULL);

	priv = hex_view->priv;
	menu = g_menu_new ();

	section = g_menu_new ();
	item = g_menu_item_new ("Cu_t", "context.cut");
	g_menu_item_set_attribute (item, "touch-icon", "s", "edit-cut-symbolic");
	g_menu_append_item (section, item);
	g_object_unref (item);
  
	item = g_menu_item_new ("_Copy", "context.copy");
	g_menu_item_set_attribute (item, "touch-icon", "s", "edit-copy-symbolic");
	g_menu_append_item (section, item);
	g_object_unref (item);
  
	item = g_menu_item_new ("_Paste", "context.paste");
	g_menu_item_set_attribute (item, "touch-icon", "s", "edit-paste-symbolic");
	g_menu_append_item (section, item);
	g_object_unref (item);

	item = g_menu_item_new ("_Delete", "context.delete");
	g_menu_item_set_attribute (item, "touch-icon", "s", "edit-delete-symbolic");
	g_menu_append_item (section, item);
	g_object_unref (item);
  
	g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
	g_object_unref (section);

	section = g_menu_new ();
	item = g_menu_item_new ("_Undo", "context.undo");
	g_menu_item_set_attribute (item, "touch-icon", "s", "edit-undo-symbolic");
	g_menu_append_item (section, item);
	g_object_unref (item);
  
	item = g_menu_item_new ("_Redo", "context.redo");
	g_menu_item_set_attribute (item, "touch-icon", "s", "edit-redo-symbolic");
	g_menu_append_item (section, item);
	g_object_unref (item);
	g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
	g_object_unref (section);

	section = g_menu_new ();
	item = g_menu_item_new ("Select _All", "context.select-all");
	g_menu_item_set_attribute (item, "touch-icon", "s", "edit-select-all-symbolic");
	g_menu_append_item (section, item);
	g_object_unref (item);

	g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
	g_object_unref (section);

	return G_MENU_MODEL(menu);
}

static void rp_hex_view_context_menu_cut (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	g_message ("Widget: called Context menu - cut");
	RPHexView *hex_view;

	hex_view = RP_HEX_VIEW (data);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	rp_hex_view_edit_cut (hex_view);
}

static void rp_hex_view_context_menu_copy (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	g_message ("Widget: called Context menu - copy");
	RPHexView *hex_view;

	hex_view = RP_HEX_VIEW (data);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	rp_hex_view_edit_copy (hex_view);
}

static void rp_hex_view_context_menu_paste (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	g_message ("Widget: called Context menu - paste");
	RPHexView *hex_view;

	hex_view = RP_HEX_VIEW (data);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	rp_hex_view_edit_paste (hex_view);
}

static void rp_hex_view_context_menu_delete (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	g_message ("Widget: called Context menu - delete");
	RPHexView *hex_view;

	hex_view = RP_HEX_VIEW (data);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	rp_hex_view_edit_delete (hex_view);
}

static void rp_hex_view_context_menu_select_all (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	g_message ("Widget: called Context menu - select-all");
	RPHexView *hex_view;

	hex_view  = RP_HEX_VIEW (data);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	rp_hex_view_edit_select_all (hex_view);
}

static void rp_hex_view_clipboard_set_data (GtkClipboard *clipboard, GtkSelectionData *data, 
											guint info, gpointer user_data)
{
	g_message ("Widget: Clipboard set_data callback called. Info: %i", info);
	RPHexViewPrivate	*priv;
	RPHexView 			*hex_view = RP_HEX_VIEW (user_data);
	guint8 				iByte;
	gint				iLen;
	guint32 			bytesRead = 0;
	gpointer			clipdata;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;

	iLen 		= priv->selection->clipboard_endSel - priv->selection->clipboard_startSel + 1;
	clipdata	= g_try_malloc0 (iLen);
	
	if (clipdata == NULL || iLen <= 0 )
		return;

	bytesRead = rp_hex_file_get_data (priv->hex_file, clipdata, iLen, priv->selection->clipboard_startSel);

	g_return_if_fail (bytesRead == iLen);

	gtk_selection_data_set (data, gdk_atom_intern_static_string ("BINARY"), 8, clipdata, iLen);

	g_free (clipdata);
}

static void rp_hex_view_clipboard_clear_data (GtkClipboard *clipboard, gpointer user_data)
{
	g_message ("Widget: Clipboard clear callback called.");
}

static void rp_hex_view_clipboard_get_data (GtkClipboard *clipboard, GtkSelectionData *data, gpointer user_data)
{
	g_message ("Widget: Clipboard get_data callback called.");
	guint8 	iByte;
	GdkAtom atom = gtk_selection_data_get_data_type (data);
	gint len = gtk_selection_data_get_length (data);
	const guchar *rec = gtk_selection_data_get_data (data);
	
	g_message ("Received len: %i", len);
	
	for (guint i = 0; i < len; i++)
	{
		iByte = rec[i];
		g_message ("Received: %02X", iByte);
	}

	g_message ("Widget: Clipboard get-receive function got data: %s of type: %s", rec, gdk_atom_name (atom));
}

static void rp_hex_view_edit_cut (RPHexView *hex_view)
{
	g_message ("Widget: called edit - cut");
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Not implemented yet.");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void rp_hex_view_edit_copy (RPHexView *hex_view)
{
	g_message ("Widget: called edit - copy");
	RPHexViewPrivate 	*priv;
	GtkClipboard 		*clipboard;
	gboolean			result;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;

	priv->selection->clipboard_startSel = priv->selection->startSel;
	priv->selection->clipboard_endSel	= priv->selection->endSel;

	clipboard	= gtk_widget_get_clipboard (GTK_WIDGET (hex_view), GDK_SELECTION_CLIPBOARD);
	result		= gtk_clipboard_set_with_data (clipboard, clip_targets, n_clip_targets, 
											rp_hex_view_clipboard_set_data, 
											rp_hex_view_clipboard_clear_data, 
											hex_view);
	
	g_message ("Widget: Clipboard data set data result: %s", result ? "TRUE" : "FALSE");

	if (!result)
	{
		GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, 
													GTK_BUTTONS_OK, 
													"The data could not be copied to the clipboard.");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

static void rp_hex_view_edit_paste (RPHexView *hex_view)
{
	g_message ("Widget: called edit - paste");
	GtkClipboard	*clipboard;
	gboolean		result;

	clipboard	= gtk_widget_get_clipboard (GTK_WIDGET (hex_view), GDK_SELECTION_CLIPBOARD);
	result		= gtk_clipboard_wait_is_target_available (clipboard, gdk_atom_intern_static_string ("BINARY"));
	
	g_message ("Widget: Clipboard data 'BINARY' available: %s", result ? "TRUE" : "FALSE");
	
	if (!result)
		return;

	gtk_clipboard_request_contents (clipboard, 
									gdk_atom_intern_static_string ("BINARY"),
									rp_hex_view_clipboard_get_data, 
									hex_view);

	
	GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Not implemented yet.");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void rp_hex_view_edit_delete (RPHexView *hex_view)
{
	g_message ("Widget: called edit - delete");
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Not implemented yet.");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void rp_hex_view_edit_select_all (RPHexView *hex_view)
{
	g_message ("Widget: called edit - select_all");
	RPHexViewPrivate *priv;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;

	g_assert (priv->iFileSize > 0);

	if (rp_hex_view_has_selection (GTK_WIDGET(hex_view)))
		rp_hex_view_remove_selection (GTK_WIDGET(hex_view));

	rp_hex_view_set_selection (GTK_WIDGET (hex_view), 0, priv->iFileSize - 1);
	gtk_widget_queue_draw (GTK_WIDGET (hex_view));
}

void rp_hex_view_print_begin_print (GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data)
{
	RPHexView         *hex_view;
	RPHexViewPrivate  *priv;

	hex_view  = RP_HEX_VIEW (user_data);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;

	if (priv->pPrintLayout == NULL)
	{
		PangoContext *pcontext = gtk_print_context_create_pango_context (context);
		priv->pPrintLayout = gtk_print_context_create_pango_layout (context);
		pango_layout_set_font_description (priv->pPrintLayout, priv->pPrintFontDescription);
		priv->pPrintFontMetrics = rp_hex_view_get_fontmetrics_print (pcontext, priv->pPrintFontName);
		
		rp_hex_view_update_character_size_print (priv);
		
		g_object_unref (G_OBJECT (pcontext));
	}

	rp_hex_view_update_layout_print (priv, context);
	rp_hex_view_update_byte_visibility_print (priv);

	gtk_print_operation_set_n_pages (operation, priv->iPrintPages);
	
	priv->iPrintTopRow = 0;

	g_message ("Widget: Print pages: %i", priv->iPrintPages);
}

void rp_hex_view_print_page (GtkPrintOperation *operation, GtkPrintContext *context,
							gint page_nr, gpointer user_data)
{
	RPHexView 			*hex_view;
	RPHexViewPrivate	*priv;
	cairo_t  			*cr;

	hex_view  = RP_HEX_VIEW (user_data);

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv = hex_view->priv;

	cr = gtk_print_context_get_cairo_context (context);
	
	priv->iPrintTopRow = page_nr * priv->iPrintVisibleRows;
	
	rp_hex_view_update_layout_print (priv, context);
	rp_hex_view_update_byte_visibility_print (priv);

	g_message ("Widget: Print page %i, Top row: %i, Last row: %i", page_nr, priv->iPrintTopRow, priv->iPrintLastRow);

	rp_hex_view_draw_address_lines_print (priv, cr);
	rp_hex_view_draw_hex_lines_print (priv, cr);
}

gboolean rp_hex_view_print (GtkWidget *widget, GtkWindow *parent, const gchar *print_job_name)
{
	RPHexView         		*hex_view;
	GtkPrintOperation		*pOperation;
	GtkPrintSettings		*pSettings;
	GtkPaperSize			*paper_size;
	GtkPrintOperationResult	result;
	GError 					*error = NULL;
	gboolean				ret_val = TRUE;

	hex_view = RP_HEX_VIEW (widget);

	g_return_val_if_fail (RP_IS_HEX_VIEW (hex_view), FALSE);

	pOperation	= gtk_print_operation_new ();
	pSettings	= gtk_print_settings_new ();
	paper_size	= gtk_paper_size_new (gtk_paper_size_get_default());
	
	gtk_print_operation_set_job_name (pOperation, print_job_name);
	gtk_print_operation_set_show_progress (pOperation, TRUE);
	gtk_print_operation_set_embed_page_setup (pOperation, TRUE);
	gtk_print_settings_set_paper_size (pSettings, paper_size);
	gtk_print_operation_set_unit (pOperation, GTK_UNIT_POINTS);
	gtk_print_operation_set_print_settings (pOperation, pSettings);
	
	g_signal_connect (pOperation, "begin_print", G_CALLBACK (rp_hex_view_print_begin_print), hex_view);
	g_signal_connect (pOperation, "draw_page", G_CALLBACK (rp_hex_view_print_page), hex_view);

	result = gtk_print_operation_run (pOperation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, parent, &error);

	if (result == GTK_PRINT_OPERATION_RESULT_ERROR)
	{
		g_message (error->message);
		g_error_free (error);
		ret_val = FALSE;
	}

	if (paper_size)
		gtk_paper_size_free (paper_size);

	if (pSettings)
		g_object_unref (pSettings);

	if (pOperation)
		g_object_unref (pOperation);

	return ret_val;
}

void rp_hex_view_toggle_draw_addresses (GtkWidget *widget, gboolean bEnable)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv->bDrawAddresses = bEnable;
	gtk_widget_queue_resize (GTK_WIDGET (hex_view));
	gtk_widget_queue_draw (GTK_WIDGET (hex_view));
}

void rp_hex_view_toggle_draw_characters (GtkWidget *widget, gboolean bEnable)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv->bDrawCharacters = bEnable;
	gtk_widget_queue_resize (GTK_WIDGET (hex_view));
	gtk_widget_queue_draw (GTK_WIDGET (hex_view));
}

void rp_hex_view_toggle_auto_fit (GtkWidget *widget, gboolean bEnable)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	priv->bAutoBytesPerRow = bEnable;
	gtk_widget_queue_resize (GTK_WIDGET (hex_view));
	gtk_widget_queue_draw (GTK_WIDGET (hex_view));
}

gboolean isMonospaceFont (guchar *font)
{
	return TRUE;
}

void rp_hex_view_toggle_font (GtkWidget *widget, guchar *font)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

	if (priv->pFontMetrics)
		pango_font_metrics_unref (priv->pFontMetrics);

  	if (priv->pFontDescription)
		pango_font_description_free (priv->pFontDescription);

	if (isMonospaceFont (font))
	{
		priv->pFontMetrics		= rp_hex_view_get_fontmetrics (font);
		priv->pFontDescription	= pango_font_description_from_string (font);
	}
	else
	{
		priv->pFontMetrics		= rp_hex_view_get_fontmetrics (DEFAULT_FONT);
		priv->pFontDescription	= pango_font_description_from_string (DEFAULT_FONT);
	}

	pango_layout_set_font_description (priv->pLayout, priv->pFontDescription);
	
	rp_hex_view_update_character_size(priv);

	gtk_widget_queue_resize (GTK_WIDGET (hex_view));
	gtk_widget_queue_draw (GTK_WIDGET (hex_view));
}

void rp_hex_view_toggle_print_font (GtkWidget *widget, guchar *font)
{
	RPHexView			*hex_view;
	RPHexViewPrivate	*priv;

	hex_view = RP_HEX_VIEW (widget);
	priv = hex_view->priv;

	g_return_if_fail (RP_IS_HEX_VIEW (hex_view));

  	if (priv->pPrintFontDescription)
		pango_font_description_free (priv->pPrintFontDescription);

	if (isMonospaceFont (font))
	{
		priv->pPrintFontDescription	= pango_font_description_from_string (font);
		priv->pPrintFontName = g_strdup (font);
	}
	else
	{
		priv->pPrintFontDescription	= pango_font_description_from_string (DEFAULT_FONT);
		priv->pPrintFontName = DEFAULT_FONT;
	}
}
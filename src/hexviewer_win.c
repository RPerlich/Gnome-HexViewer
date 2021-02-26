/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* hexviewer_win.c
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

#include "hexviewer_win.h"
#include "rphexview.h"
#include "rphexfile.h"
#include "hexviewer_prefs.h"

typedef struct _HexViewerWindow HexViewerWindow;

struct _HexViewerWindow
{
	GtkApplicationWindow	parent;  
	GtkHeaderBar			*headerBar;
	GtkStatusbar			*statusbar;
	GtkScrolledWindow		*scrolledWindow;
	GtkButton				*btn_open;
	GtkButton				*btn_save;
	GtkWidget				*hex_view;
	RPHexFile				*hex_file;
	GSettings				*settings;
};

G_DEFINE_TYPE (HexViewerWindow, hexviewer_window, GTK_TYPE_APPLICATION_WINDOW)

static GList *window_list = NULL;

static void hexviewer_window_dispose 	(GObject *object);
static void hexviewer_window_update_file_data (HexViewerWindow *window, gboolean bChanged);
static void callback_byte_pos_changed	(RPHexView *widget, guint64 position, HexViewerWindow *window);
static void callback_selection_changed	(RPHexView *widget, HexViewerWindow *window);
static void callback_data_changed		(RPHexFile *hex_file, gboolean changed, HexViewerWindow *window);
static void action_open_file 			(GSimpleAction *action, GVariant *parameter, gpointer window);
static void action_save_file 			(GSimpleAction *action, GVariant *parameter, gpointer window);
static void action_print_print			(GSimpleAction *action, GVariant *parameter, gpointer window);
static void action_preferences 			(GSimpleAction *action, GVariant *parameter, gpointer window);
static void action_prefs				(GSettings *settings, gchar *key, gpointer user_data);

static GActionEntry win_action_entries[] = {
	{ "open_file", action_open_file, NULL, NULL, NULL },
	{ "save_file", action_save_file, NULL, NULL, NULL },
	{ "print", action_print_print, NULL, NULL, NULL },
	{ "preferences", action_preferences, NULL, NULL, NULL }
};

static void hexviewer_window_class_init (HexViewerWindowClass *klass)
{
	GtkWidgetClass *class = GTK_WIDGET_CLASS (klass);

	G_OBJECT_CLASS (class)->dispose = hexviewer_window_dispose;

	gtk_widget_class_set_template_from_resource (class, "/org/gnome/hexviewer/hexviewer_win.ui");
	gtk_widget_class_bind_template_child (class, HexViewerWindow, headerBar);
	gtk_widget_class_bind_template_child (class, HexViewerWindow, statusbar);
	gtk_widget_class_bind_template_child (class, HexViewerWindow, scrolledWindow);
	gtk_widget_class_bind_template_child (class, HexViewerWindow, btn_open);
	gtk_widget_class_bind_template_child (class, HexViewerWindow, btn_save);
}

static void hexviewer_window_init (HexViewerWindow *window)
{
	g_return_if_fail (HEXVIEWER_WINDOW_IS_WINDOW (window));

	gtk_widget_init_template (GTK_WIDGET (window));
	
	g_action_map_add_action_entries (G_ACTION_MAP (window), win_action_entries, G_N_ELEMENTS (win_action_entries), window);
	
	GAction *action_save_file = g_action_map_lookup_action (G_ACTION_MAP (window), win_action_entries[1].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action_save_file), FALSE); 

	GAction *action_print_print = g_action_map_lookup_action (G_ACTION_MAP (window), win_action_entries[2].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action_print_print), FALSE);

	window->hex_view = NULL;
	window->hex_file = NULL;

	window->settings = g_settings_new ("org.gnome.hexviewer");

	g_settings_bind (window->settings, "show-statusbar", window->statusbar, "visible", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect (G_OBJECT (window->settings), "changed", G_CALLBACK (action_prefs), window);

	window_list = g_list_prepend (window_list, window);
}

static void hexviewer_window_dispose (GObject *object)
{
	g_message ("Win: called Dispose");
	HexViewerWindow *window = HEXVIEWER_WINDOW (object);

	G_OBJECT_CLASS (hexviewer_window_parent_class)->dispose (object);

	if (window->hex_file)
	{
		g_object_unref (window->hex_file);
		window->hex_file = NULL;
	}

	g_clear_object (&window->settings);
}

HexViewerWindow *hexviewer_window_new (HexViewerApp *app)
{
	return g_object_new (TYPE_HEXVIEWER_WINDOW, "application", app, NULL);
}

gboolean hexviewer_window_open (HexViewerWindow *window, GFile *file)
{
	GError 		file_error;
	gboolean	bEnable = FALSE;
	guchar 		*font = NULL;
	
	g_return_val_if_fail (HEXVIEWER_WINDOW_IS_WINDOW (window), FALSE);

	window->hex_file = rp_hex_file_new_with_file (file, FALSE, &file_error);
	g_return_val_if_fail (window->hex_file != NULL, FALSE);
	
	window->hex_view = rp_hex_view_new_with_file (window->hex_file);
	g_return_val_if_fail (window->hex_view != NULL, FALSE);

	gtk_container_add (GTK_CONTAINER(window->scrolledWindow), window->hex_view);
	gtk_widget_show (window->hex_view);
	
	g_signal_connect (G_OBJECT(window->hex_view), "byte_pos_changed",
                     G_CALLBACK(callback_byte_pos_changed), window);

	g_signal_connect (G_OBJECT(window->hex_view), "selection_changed",
                     G_CALLBACK(callback_selection_changed), window);
	
	g_signal_connect (G_OBJECT(window->hex_file), "data_changed",
                     G_CALLBACK(callback_data_changed), window);

	hexviewer_window_update_file_data (window, FALSE);

	GAction *action_print = g_action_map_lookup_action (G_ACTION_MAP (window), 
														win_action_entries[2].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION(action_print), TRUE);

	bEnable = g_settings_get_boolean (window->settings, "show-addresses");
	rp_hex_view_toggle_draw_addresses (window->hex_view, bEnable);

	bEnable = g_settings_get_boolean (window->settings, "show-ascii");
	rp_hex_view_toggle_draw_characters (window->hex_view, bEnable);

	bEnable = g_settings_get_boolean (window->settings, "auto-fit");
	rp_hex_view_toggle_auto_fit (window->hex_view, bEnable);

	font = g_settings_get_string (window->settings, "font");
	
	if (font)
		rp_hex_view_toggle_font (window->hex_view, font); 
	g_free (font);

	font = g_settings_get_string (window->settings, "print-font");
	
	if (font)
		rp_hex_view_toggle_print_font (window->hex_view, font);
	g_free (font);

	return TRUE;
}

const GList *hexviewer_window_get_list ()
{
    return window_list;
}

static void hexviewer_window_update_file_data (HexViewerWindow *window, gboolean bChanged)
{
	gchar window_title[256];
	GFile *file = NULL;
	gchar *fileName;

	g_return_if_fail (HEXVIEWER_WINDOW_IS_WINDOW (window));
	
	file		= g_file_parse_name (window->hex_file->file_name);
	fileName	= g_file_get_basename (file);
	
	g_snprintf (window_title, sizeof(window_title), "%s%s - HexViewer", bChanged ? "*": "", fileName);
	gtk_window_set_title (GTK_WINDOW(window), window_title);
	
	g_free (fileName);

	GAction *action_save_file = g_action_map_lookup_action (G_ACTION_MAP (window), win_action_entries[1].name);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action_save_file), bChanged); 
}

static void callback_byte_pos_changed (RPHexView *widget, guint64 position, HexViewerWindow *window)
{
	g_message ("Win: Byte position changed received. %i", position);
	g_return_if_fail (RP_IS_HEX_VIEW (widget));
	g_return_if_fail (HEXVIEWER_WINDOW_IS_WINDOW (window));
}

static void callback_selection_changed (RPHexView *widget, HexViewerWindow *window)
{
	g_message ("Win: Selection changed received.");
	g_return_if_fail (RP_IS_HEX_VIEW (widget));
	g_return_if_fail (HEXVIEWER_WINDOW_IS_WINDOW (window));

	gboolean bNewState = rp_hex_view_has_selection (GTK_WIDGET (window->hex_view));
}

static void callback_data_changed (RPHexFile *hex_file, gboolean bChanged, HexViewerWindow *window)
{
	g_return_if_fail (RP_IS_HEX_FILE (hex_file));
	g_return_if_fail (HEXVIEWER_WINDOW_IS_WINDOW (window));

	hexviewer_window_update_file_data (window, bChanged);
}

static void action_open_file (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	HexViewerWindow	*window;
	GtkWidget		*dlg_openfile;
	GFile			*file;

	g_assert (data != NULL);

	window = HEXVIEWER_WINDOW (data);
	
	g_assert (window->scrolledWindow != NULL);

	dlg_openfile = gtk_file_chooser_dialog_new ("Open", NULL,
						GTK_FILE_CHOOSER_ACTION_OPEN,                                            
                    	"Cancel", GTK_RESPONSE_REJECT,
						"Open", GTK_RESPONSE_ACCEPT, NULL);

    gint id = gtk_dialog_run (GTK_DIALOG (dlg_openfile));

	switch (id)
	{
		case GTK_RESPONSE_ACCEPT:
		{
			file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dlg_openfile));

			if (window->hex_file)
			{
				g_object_unref (window->hex_file);
				window->hex_file = NULL;
				gtk_widget_destroy (window->hex_view);
			}

			hexviewer_window_open (window, file);

			if (file)
				g_object_unref (file);
			
			break;
		}
		case GTK_RESPONSE_REJECT:
            break;
	}

	gtk_widget_destroy(dlg_openfile);
}

static void action_save_file (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	gboolean 		bRet = FALSE;
	HexViewerWindow	*window;

	window = HEXVIEWER_WINDOW (data);
	
	g_return_if_fail (HEXVIEWER_WINDOW_IS_WINDOW (window));

	if (rp_hex_file_only_overtype_changes (window->hex_file))
	{
		bRet = rp_hex_file_write_in_place (window->hex_file);
		g_message ("Win: Action save successful ? %s", bRet ? "True" : "False");
	}
	else
	{
		GtkWidget *dialog = gtk_message_dialog_new (
			NULL, 
			GTK_DIALOG_MODAL, 
			GTK_MESSAGE_INFO, 
			GTK_BUTTONS_OK, 
			"Insert mode - Not implemented yet.");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		return;
	}
	
	hexviewer_window_update_file_data (window, FALSE);
}

static void action_print_print (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	g_message ("Win: Action Print called.");
	HexViewerWindow	*window;
	GtkWidget		*dialog;
	gchar			*file_name;
	gchar			print_title[256];

	g_assert (data != NULL);

	window 		= HEXVIEWER_WINDOW (data);
	file_name 	= rp_hex_file_get_file_name (window->hex_file);

	g_snprintf (print_title, sizeof(print_title), "HexViewer - %s", file_name);

	if (!rp_hex_view_print (GTK_WIDGET (window->hex_view), GTK_WINDOW (window), print_title))
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (window), 
										GTK_DIALOG_MODAL, 
										GTK_MESSAGE_ERROR, 
										GTK_BUTTONS_OK, 
										"Failed to print the current document.");

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

static void action_preferences (GSimpleAction *action, GVariant *parameter, gpointer data)
{
	HexViewerPreferences	*prefs;
	HexViewerWindow			*window;

	g_assert (data != NULL);

	window	= HEXVIEWER_WINDOW (data);
	prefs	= hexviewer_preferences_new (HEXVIEWER_WINDOW (window));
	gtk_window_present (GTK_WINDOW (prefs));
}

static void action_prefs (GSettings *settings, gchar *key, gpointer user_data)
{
	HexViewerWindow	*window;
	gboolean 		bEnable = FALSE;
	guchar 			*font = NULL;

	g_assert (user_data != NULL);

	window = HEXVIEWER_WINDOW (user_data);

	if (!window->hex_view)
		return;

	if (strcmp (key, "show-addresses") == 0)
	{
		bEnable = g_settings_get_boolean (settings, key);
		g_message ("Win: Action Prefs called. %s with %s", key, bEnable ? "True" : "False");
		rp_hex_view_toggle_draw_addresses (window->hex_view, bEnable);
	}
	else
	if (strcmp (key, "show-ascii") == 0)
	{
		bEnable = g_settings_get_boolean (settings, key);
		g_message ("Win: Action Prefs called. %s with %s", key, bEnable ? "True" : "False");
		rp_hex_view_toggle_draw_characters (window->hex_view, bEnable);
	}
	else
	if (strcmp (key, "auto-fit") == 0)
	{
		bEnable = g_settings_get_boolean (settings, key);
		g_message ("Win: Action Prefs called. %s with %s", key, bEnable ? "True" : "False");
		rp_hex_view_toggle_auto_fit (window->hex_view, bEnable);
	}
	else
	if (strcmp (key, "font") == 0)
	{
		font = g_settings_get_string (settings, key);
		g_message ("Win: Action Prefs called. %s with %s", key, font);
		if (font)
			rp_hex_view_toggle_font (window->hex_view, font);
		g_free (font);
	}
	else
	if (strcmp (key, "print-font") == 0)
	{
		font = g_settings_get_string (settings, key);
		g_message ("Win: Action Prefs called. %s with %s", key, font);
		if (font)
			rp_hex_view_toggle_print_font (window->hex_view, font);
		g_free (font);
	}
}
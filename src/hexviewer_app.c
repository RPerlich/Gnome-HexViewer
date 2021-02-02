/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* hexviewer_app.c
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "hexviewer_app.h"
#include "hexviewer_win.h"

struct _HexViewerApp
{
  GtkApplication parent;
};

G_DEFINE_TYPE(HexViewerApp, hexviewer_app, GTK_TYPE_APPLICATION);

static void sys_menu_about (GSimpleAction *action, GVariant *parameter, gpointer app)
{
	GtkWindow *window;

	window = gtk_application_get_active_window (GTK_APPLICATION (app));

	const gchar *authors[] = {
		"Ralph Perlich",
		NULL
	};

	const gchar *documentors[] = {
		"Ralph Perlich",
		NULL
	};

	gtk_show_about_dialog (GTK_WINDOW (window),
			"program-name", PACKAGE_NAME,
			"version", g_strdup_printf ("%s\nRunning against GTK+ %d.%d.%d",
			PACKAGE_VERSION,
			gtk_get_major_version (),
			gtk_get_minor_version (),
			gtk_get_micro_version ()),
			"copyright", "(C) 2020 Ralph Perlich",
			"license-type", GTK_LICENSE_GPL_3_0,
			"website", "https://github.com/RPerlich/gnome-hexviewer",
			"website-label", "github.com/RPerlich/gnome-hexviewer",
			"comments", "Show files in a hexadecimal form.",
			"authors", authors,
			"documenters", documentors,
			"logo-icon-name", "application-x-firmware",
			"title", "About HexViewer",
			NULL);
}

static void sys_menu_quit (GSimpleAction *action, GVariant *parameter,	gpointer app)
{
	g_application_quit (G_APPLICATION (app));
}

static GActionEntry sys_menu_entries[] = {
	{ "about", sys_menu_about, NULL, NULL, NULL },
	{ "quit", sys_menu_quit, NULL, NULL, NULL }
};

static void app_startup (GApplication *app)
{
	GtkBuilder	*builder;
	GMenuModel	*sys_menu;
	const gchar	*quit_accels[2] = { "<Ctrl>Q", NULL };

	G_APPLICATION_CLASS (hexviewer_app_parent_class)->startup (app);

	g_action_map_add_action_entries (G_ACTION_MAP (app), sys_menu_entries, G_N_ELEMENTS (sys_menu_entries), app);
	gtk_application_set_accels_for_action (GTK_APPLICATION (app), "app.quit", quit_accels);

	builder = gtk_builder_new_from_resource ("/org/gnome/hexviewer/app_sys_menu.ui");
	sys_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "sysmenu"));
	gtk_application_set_app_menu (GTK_APPLICATION (app), sys_menu);
	
	g_object_unref (builder);

	g_message("App: called Startup");
}

static void app_activate (GApplication *app)
{
	HexViewerWindow *window;

	g_assert (GTK_IS_APPLICATION (app));

	window = hexviewer_window_new (HEXVIEWER_APP (app));
	
	if (window)
		gtk_window_present (GTK_WINDOW (window));

	g_message("App: called Activate");	
}

static void app_open(GApplication *app, GFile **files, gint n_files, const gchar *hint)
{
	HexViewerWindow *window;
	gchar			*fileName;

	for (int i = 0; i < n_files; i++)
	{
		fileName = g_file_get_parse_name (files[i]);
		
		g_message ("App: Try to open following file: %s", fileName);
		
		if (g_file_test (fileName, G_FILE_TEST_EXISTS))
		{
			window = hexviewer_window_new (HEXVIEWER_APP (app));
			hexviewer_window_open (window, files[i]);
			
			if (window)
				gtk_window_present (GTK_WINDOW (window));
		}
		
		g_free(fileName);
	}

	if (hexviewer_window_get_list () == NULL)
	{
		g_message ("App: No files to open, create new window.");
		window = hexviewer_window_new (HEXVIEWER_APP (app));
		
		if (window)
			gtk_window_present (GTK_WINDOW (window));
	}
}

static void hexviewer_app_init (HexViewerApp *app)
{
}

static void hexviewer_app_class_init (HexViewerAppClass *class)
{
	G_APPLICATION_CLASS (class)->startup	= app_startup;
	G_APPLICATION_CLASS (class)->activate	= app_activate;
	G_APPLICATION_CLASS (class)->open		= app_open;
}

HexViewerApp *hexviewer_app_new (void)
{
	return g_object_new (HEXVIEWER_APP_TYPE,
                       "application-id", "org.gnome.hexviewer",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

int main (int argc, char *argv[])
{
	return g_application_run (G_APPLICATION (hexviewer_app_new ()), argc, argv);
}

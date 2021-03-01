/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* hexviewer_prefs.c
 *
 * Copyright 2021 Ralph Perlich
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

#include "hexviewer_prefs.h"

struct _HexViewerPreferences
{
	GtkDialog parent;
};

typedef struct _HexViewerPreferencesPrivate HexViewerPreferencesPrivate;

struct _HexViewerPreferencesPrivate
{
	GSettings	*settings;
	GtkWidget	*chk_show_ascii;
	GtkWidget	*chk_show_adresses;
	GtkWidget	*chk_auto_fit;
	GtkWidget	*chk_show_statusbar;
	GtkWidget	*font;
	GtkWidget	*print_font;
};

G_DEFINE_TYPE_WITH_PRIVATE (HexViewerPreferences, hexviewer_preferences, GTK_TYPE_DIALOG)

gboolean filterfunc (PangoFontFamily *family, const PangoFontFace *face, gpointer data)
{
const gchar *name = pango_font_family_get_name (family);

	return pango_font_family_is_monospace (family) ||
			(g_strrstr (name, "Mono") != NULL);
}

void destroyfunc (gpointer data)
{
	// nothing to free
}

static void hexviewer_preferences_init (HexViewerPreferences *prefs)
{
	HexViewerPreferencesPrivate *priv;

	priv = hexviewer_preferences_get_instance_private (prefs);
	gtk_widget_init_template (GTK_WIDGET (prefs));
	
	priv->settings = g_settings_new ("org.gnome.hexviewer");
	
	g_settings_bind (priv->settings, "show-ascii", priv->chk_show_ascii, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->settings, "show-addresses", priv->chk_show_adresses, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->settings, "auto-fit", priv->chk_auto_fit, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->settings, "show-statusbar", priv->chk_show_statusbar, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->settings, "font", priv->font, "font", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->settings, "print-font", priv->print_font, "font", G_SETTINGS_BIND_DEFAULT);

	gtk_font_chooser_set_filter_func (GTK_FONT_CHOOSER(priv->font), 
									(GtkFontFilterFunc)filterfunc, 
									NULL, (GDestroyNotify) destroyfunc);

	gtk_font_chooser_set_filter_func (GTK_FONT_CHOOSER(priv->print_font), 
									(GtkFontFilterFunc)filterfunc, 
									NULL, (GDestroyNotify) destroyfunc);
}

static void hexviewer_preferences_dispose (GObject *object)
{
  HexViewerPreferencesPrivate *priv;

  priv = hexviewer_preferences_get_instance_private (HEXVIEWER_PREFERENCES (object));
  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (hexviewer_preferences_parent_class)->dispose (object);
}

static void hexviewer_preferences_class_init (HexViewerPreferencesClass *class)
{
  G_OBJECT_CLASS (class)->dispose = hexviewer_preferences_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                            	"/org/gnome/hexviewer/hexviewer_prefs.ui");
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), HexViewerPreferences, chk_show_ascii);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), HexViewerPreferences, chk_show_adresses);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), HexViewerPreferences, chk_auto_fit);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), HexViewerPreferences, chk_show_statusbar);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), HexViewerPreferences, font);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), HexViewerPreferences, print_font);
}

HexViewerPreferences *hexviewer_preferences_new (HexViewerWindow *window)
{
  return g_object_new (TYPE_HEXVIEWER_PREFERENCES, "transient-for", window, "use-header-bar", TRUE, NULL);
}
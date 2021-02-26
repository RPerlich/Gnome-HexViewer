/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* hexviewer_prefs.h
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

#pragma once

#include <gtk/gtk.h>
#include "hexviewer_win.h"

G_BEGIN_DECLS

#define TYPE_HEXVIEWER_PREFERENCES	(hexviewer_preferences_get_type())
#define HEXVIEWER_PREFERENCES_IS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_HEXVIEWER_PREFERENCES))

G_DECLARE_FINAL_TYPE (HexViewerPreferences, hexviewer_preferences, HEXVIEWER, PREFERENCES, GtkDialog)

HexViewerPreferences *hexviewer_preferences_new (HexViewerWindow *window);

G_END_DECLS
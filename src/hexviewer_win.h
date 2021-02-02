/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* hexviewer_win.h
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

#pragma once

#include <gtk/gtk.h>
#include "hexviewer_app.h"

G_BEGIN_DECLS

#define TYPE_HEXVIEWER_WINDOW           (hexviewer_window_get_type())
#define HEXVIEWER_WINDOW_IS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_HEXVIEWER_WINDOW))

G_DECLARE_FINAL_TYPE (HexViewerWindow, hexviewer_window, HEXVIEWER, WINDOW, GtkApplicationWindow)

HexViewerWindow *hexviewer_window_new (HexViewerApp *app);
gboolean hexviewer_window_open (HexViewerWindow *window, GFile *file);
const GList *hexviewer_window_get_list (void);

G_END_DECLS

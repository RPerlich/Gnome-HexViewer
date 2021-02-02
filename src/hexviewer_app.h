/* -*- Mode: C; c-file-style: "gnu"; tab-width: 4 -*- */
/* hexviewer_app.h
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

#ifndef __HEXVIEWERAPP_H
#define __HEXVIEWERAPP_H

#include <gtk/gtk.h>


#define HEXVIEWER_APP_TYPE (hexviewer_app_get_type ())
G_DECLARE_FINAL_TYPE (HexViewerApp, hexviewer_app, HEXVIEWER, APP, GtkApplication)

HexViewerApp *hexviewer_app_new (void);

#endif /* __HEXVIEWERAPP_H */

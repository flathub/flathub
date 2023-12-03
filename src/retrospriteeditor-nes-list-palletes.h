/* retrospriteeditor-nes-list-palletes.h
 *
 * Copyright 2023 vi
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
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RETROSPRITEEDITOR_TYPE_NES_LIST_PALLETE (retrospriteeditor_nes_list_pallete_get_type())

G_DECLARE_FINAL_TYPE (RetrospriteeditorNesListPallete, retrospriteeditor_nes_list_pallete, RETROSPRITEEDITOR, NES_LIST_PALLETE, GtkFrame)

G_END_DECLS

void palette_nes_redraw (void);
GtkWidget **nes_list_pallete_get_items (void);

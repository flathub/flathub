/* retrospriteeditor-nes-current-pallete.h
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

#define RETROSPRITEEDITOR_TYPE_NES_CURRENT_PALLETE (retrospriteeditor_nes_current_pallete_get_type())

G_DECLARE_FINAL_TYPE (RetrospriteeditorNesCurrentPallete, retrospriteeditor_nes_current_pallete, RETROSPRITEEDITOR, NES_CURRENT_PALLETE, GtkFrame)

G_END_DECLS

void nes_current_pallete (RetrospriteeditorNesCurrentPallete *self,
                          guint32                            *colours,
                          guint32                            *index_colour);

RetrospriteeditorNesCurrentPallete *
nes_current_get_widget (void);
void nes_current_pallete_redraw (void);

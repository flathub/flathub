/* retrospriteeditor-nes-palette.h
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

#define RETROSPRITEEDITOR_TYPE_NES_PALETTE (retrospriteeditor_nes_palette_get_type())

G_DECLARE_FINAL_TYPE (RetrospriteeditorNesPalette, retrospriteeditor_nes_palette, RETROSPRITEEDITOR, NES_PALETTE, GtkBox)

G_END_DECLS

#include "nes_params.h"


NesTilePoint *nes_get_map (guint32 index);
NesTilePoint *nes_get_block (guint32 w, guint32 h, guint32 x, guint32 y, guint32 index_map);

RetrospriteeditorNesPalette *get_nes (void);
NesTilePoint *nes_get_color (RetrospriteeditorNesPalette *self,
                       guint32 x, guint32 y);

void nes_set_color (RetrospriteeditorNesPalette *self,
                    NesParamPoint               *params,
                    guint32                      index);

void nes_set_color_with_map (RetrospriteeditorNesPalette *self,
                    NesParamPoint               *params,
                    guint32                      index,
										guint32                      map);

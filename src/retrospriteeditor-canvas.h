/* retrospriteeditor-canvas.h
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

#include <adwaita.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RETROSPRITEEDITOR_TYPE_CANVAS (retrospriteeditor_canvas_get_type())

G_DECLARE_FINAL_TYPE (RetrospriteeditorCanvas, retrospriteeditor_canvas, RETROSPRITEEDITOR, CANVAS, GtkDrawingArea)

G_END_DECLS

typedef struct _CanvasSettings {
  gint32 type_canvas;
  gint32 canvas_width;
  gint32 canvas_height;
  gint32 palette_type;
  gint32 width_rect;
  gint32 height_rect;
  gint32 scale;
  gint32 count_x;
  gint32 count_y;
  gboolean left_top;
} CanvasSettings;

enum {
  TYPE_CANVAS_TILESET,
  TYPE_CANVAS_COLOUR_GRID,
  TYPE_CANVAS_COLOUR_PALLETE,
  N_TYPE_CANVAS
};



enum {
  NES_SPRITE,
  NES_BACKGROUND,
  N_NES
};


void canvas_set_type_palette (RetrospriteeditorCanvas *self,
                                                guint32                  type,
                                                guint32                  count);

void canvas_shut_on_event_click (RetrospriteeditorCanvas *self);
void canvas_shut_on_events (RetrospriteeditorCanvas *self);

RetrospriteeditorCanvas * canvas_get_drawing_canvas (void);
RetrospriteeditorCanvas * canvas_get_tileset (void);

void canvas_set_drawing_canvas (RetrospriteeditorCanvas *);
void canvas_set_tileset (RetrospriteeditorCanvas *);


void canvas_set_tool (RetrospriteeditorCanvas *,
                      guint32                  tool);

guint32 *
canvas_pallete_get_ptr_index_colours (RetrospriteeditorCanvas *self,
		guint32 x, guint32 y);

void
canvas_set_xy_click (RetrospriteeditorCanvas *self);

void
canvas_set_child_color (RetrospriteeditorCanvas *self,
		RetrospriteeditorCanvas *child);

void
canvas_set_index_colours (RetrospriteeditorCanvas *self,
                          guint32                 *index_color);
void
canvas_set_colours (RetrospriteeditorCanvas *self,
                    guint32                 *colours,
                    guint32                  count);

void
canvas_set_show_hex (RetrospriteeditorCanvas *self,
                     gboolean                 val);
void
canvas_set_index_colours_by_index (RetrospriteeditorCanvas * self, guint32 index_id, guint32 index_color);

void
canvas_set_colours_by_index (RetrospriteeditorCanvas * self, guint32 index_id, guint32 index_color);

void
canvas_set_index_id (RetrospriteeditorCanvas *self, guint32 index);

void
canvas_set_index (RetrospriteeditorCanvas *self, guint32 index);

/* retrospriteeditor-nes-item-pallete.c
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

#include "config.h"

#include "retrospriteeditor-nes-item-pallete.h"
#include "retrospriteeditor-canvas.h"
#include "type-pallete.h"

struct _RetrospriteeditorNesItemPallete
{
  GtkBox  parent_instance;
  GtkWidget *radio_btn;
  GtkWidget *canvas;

  guint32 *colour;
  guint32 *index_color;
};

#define DEFAULT_COLOUR_CANVAS_WIDTH             128
#define DEFAULT_COLOUR_CANVAS_HEIGHT             32
#define DEFAULT_COLOUR_CANVAS_SCALE               1
#define DEFAULT_COLOUR_PALETTE_TYPE               0

G_DEFINE_FINAL_TYPE (RetrospriteeditorNesItemPallete, retrospriteeditor_nes_item_pallete, GTK_TYPE_BOX)

static void
retrospriteeditor_nes_item_pallete_class_init (RetrospriteeditorNesItemPalleteClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

static void
retrospriteeditor_nes_item_pallete_init (RetrospriteeditorNesItemPallete *self)
{
  self->index_color = NULL;

  self->radio_btn = gtk_check_button_new ();
  self->canvas = g_object_new (RETROSPRITEEDITOR_TYPE_CANVAS,
                               NULL);

  gtk_widget_set_size_request (self->canvas, DEFAULT_COLOUR_CANVAS_WIDTH, DEFAULT_COLOUR_CANVAS_HEIGHT);
  CanvasSettings cs_palette;
  cs_palette.type_canvas    = TYPE_CANVAS_COLOUR_PALLETE;
  cs_palette.canvas_width   = DEFAULT_COLOUR_CANVAS_WIDTH;
  cs_palette.canvas_height  = DEFAULT_COLOUR_CANVAS_HEIGHT;
  cs_palette.palette_type   = DEFAULT_COLOUR_PALETTE_TYPE;
  cs_palette.scale          = DEFAULT_COLOUR_CANVAS_SCALE;
  cs_palette.width_rect     = 32;
  cs_palette.height_rect    = 32;
  cs_palette.count_x        = 4;
  cs_palette.count_y        = 1;
  cs_palette.left_top       = TRUE;
  g_object_set (self->canvas, "settings", &cs_palette, NULL);

  gtk_box_append (GTK_BOX (self), self->radio_btn);
  gtk_box_append (GTK_BOX (self), self->canvas);
}

guint32 *item_nes_pallete_get_index_colours (RetrospriteeditorNesItemPallete *self)
{
  return self->index_color;
}

void item_nes_pallete_set_index_colours (RetrospriteeditorNesItemPallete *self,
                                   guint32                         *index)
{
  self->index_color = index;
}

void item_nes_pallete_set_colours (RetrospriteeditorNesItemPallete *self,
                               guint32                         *colour,
                               guint32                          count)
{
  self->colour = colour;

  canvas_set_colours (RETROSPRITEEDITOR_CANVAS (self->canvas),
                      colour,
                      count);
}

void item_nes_palette_redraw (RetrospriteeditorNesItemPallete *self)
{
	gtk_widget_queue_draw (GTK_WIDGET (self->canvas));
}

GtkWidget *
item_nes_pallete_get_radio (RetrospriteeditorNesItemPallete *self)
{
  return self->radio_btn;
}

guint32 *
item_nes_pallete_get_colour (RetrospriteeditorNesItemPallete *self)
{
  return self->colour;
}

guint32 *
item_nes_pallete_get_colour_index (RetrospriteeditorNesItemPallete *self)
{
  return self->index_color;
}

void item_nes_pallete_get_color_from_index (RetrospriteeditorNesItemPallete *self)
{
	guint32 *colours = global_type_pallete_get_cur_ptr_pallete (0);
	self->colour[0] = colours[self->index_color[0]];
	self->colour[1] = colours[self->index_color[1]];
	self->colour[2] = colours[self->index_color[2]];
	self->colour[3] = colours[self->index_color[3]];
}

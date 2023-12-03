/* retrospriteeditor-nes-current-pallete.c
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

#include "retrospriteeditor-nes-current-pallete.h"
#include "retrospriteeditor-canvas.h"

struct _RetrospriteeditorNesCurrentPallete
{
  GtkFrame  parent_instance;

  GtkWidget *pallete;
  GtkWidget *check_hex;
  GtkWidget *box_main;
  guint32 *colours;
  guint32 *index_colour;
};


#define DEFAULT_COLOUR_CANVAS_WIDTH             128
#define DEFAULT_COLOUR_CANVAS_HEIGHT             32
#define DEFAULT_COLOUR_CANVAS_SCALE               1
#define DEFAULT_COLOUR_PALETTE_TYPE               0

static RetrospriteeditorNesCurrentPallete *global_self;

G_DEFINE_FINAL_TYPE (RetrospriteeditorNesCurrentPallete, retrospriteeditor_nes_current_pallete, GTK_TYPE_FRAME)

static void
retrospriteeditor_nes_current_pallete_class_init (RetrospriteeditorNesCurrentPalleteClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

void
nes_current_pallete_redraw ()
{
  gtk_widget_queue_draw (GTK_WIDGET (global_self->pallete));
}

void
show_hex (GtkCheckButton *btn,
         gpointer        user_data)
{
  RetrospriteeditorNesCurrentPallete *self = RETROSPRITEEDITOR_NES_CURRENT_PALLETE (user_data);

  canvas_set_show_hex (RETROSPRITEEDITOR_CANVAS (self->pallete),
                       gtk_check_button_get_active (GTK_CHECK_BUTTON (self->check_hex)));

  gtk_widget_queue_draw (GTK_WIDGET (self->pallete));
}

static void
retrospriteeditor_nes_current_pallete_init (RetrospriteeditorNesCurrentPallete *self)
{
  global_self = self;

  self->colours = NULL;

  self->box_main = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);

  self->check_hex = gtk_check_button_new_with_label ("HEX");

  self->pallete = g_object_new (RETROSPRITEEDITOR_TYPE_CANVAS,
                               NULL);

  canvas_shut_on_event_click (RETROSPRITEEDITOR_CANVAS (self->pallete));

  canvas_set_show_hex (RETROSPRITEEDITOR_CANVAS (self->pallete), FALSE);

  gtk_widget_set_size_request (self->pallete, DEFAULT_COLOUR_CANVAS_WIDTH, DEFAULT_COLOUR_CANVAS_HEIGHT);
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
  g_object_set (self->pallete, "settings", &cs_palette, NULL);

  gtk_frame_set_child (GTK_FRAME (self), self->box_main);

  gtk_box_append (GTK_BOX (self->box_main), self->pallete);
  gtk_box_append (GTK_BOX (self->box_main), self->check_hex);


  gtk_widget_set_hexpand_set (GTK_WIDGET (self->pallete), TRUE);
  gtk_widget_set_hexpand_set (GTK_WIDGET (self->check_hex), TRUE);

  gtk_widget_set_halign (GTK_WIDGET (self->check_hex), GTK_ALIGN_END);

  g_signal_connect (self->check_hex, "toggled",
                    G_CALLBACK (show_hex),
                    self);
}

void
nes_current_pallete (RetrospriteeditorNesCurrentPallete *self,
                     guint32                            *colours,
                     guint32                            *index_colour)
{
  self->colours = colours;
  self->index_colour =index_colour;

  canvas_set_colours (RETROSPRITEEDITOR_CANVAS (self->pallete), colours, 4);
  canvas_set_index_colours (RETROSPRITEEDITOR_CANVAS (self->pallete), index_colour);

  RetrospriteeditorCanvas *dr_canvas = canvas_get_drawing_canvas ();

  canvas_set_colours (RETROSPRITEEDITOR_CANVAS (dr_canvas), colours, 4);
  canvas_set_index_colours (RETROSPRITEEDITOR_CANVAS (dr_canvas), index_colour);

  RetrospriteeditorCanvas *tileset = canvas_get_tileset ();

  canvas_set_colours (RETROSPRITEEDITOR_CANVAS (tileset), colours, 4);
  canvas_set_index_colours (RETROSPRITEEDITOR_CANVAS (tileset), index_colour);

  gtk_widget_queue_draw (self->pallete);
  gtk_widget_queue_draw (GTK_WIDGET (dr_canvas));
  gtk_widget_queue_draw (GTK_WIDGET (tileset));
}

RetrospriteeditorNesCurrentPallete *
nes_current_get_widget (void)
{
  return global_self;
}

/* retrospriteeditor-tool-button.c
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

#include "retrospriteeditor-nes-list-palletes.h"
#include "retrospriteeditor-nes-item-pallete.h"
#include "retrospriteeditor-nes-current-pallete.h"
#include "retrospriteeditor-setup-pallete-nes.h"
#include "retrospriteeditor-canvas.h"
#include "type-pallete.h"

struct _RetrospriteeditorNesListPallete
{
  GtkFrame  parent_instance;
  GtkWidget *list_pallete;
  GtkWidget *btn_pallete_setup;
  GtkWidget *box_main;
	GtkWidget *setup_pallete_window;
  GtkWidget *items[4];
};

static RetrospriteeditorNesListPallete *global;

G_DEFINE_FINAL_TYPE (RetrospriteeditorNesListPallete, retrospriteeditor_nes_list_pallete, GTK_TYPE_FRAME)

static void
retrospriteeditor_nes_list_pallete_class_init (RetrospriteeditorNesListPalleteClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

GtkWidget **nes_list_pallete_get_items ()
{
	return global->items;
}

void
select_pallete (GtkCheckButton *btn,
         gpointer        user_data)
{

  RetrospriteeditorNesItemPallete *self = RETROSPRITEEDITOR_NES_ITEM_PALLETE (user_data);

  RetrospriteeditorNesCurrentPallete *cur_pallete = nes_current_get_widget ();

  guint32 *colour = item_nes_pallete_get_colour (self);
  guint32 *index_color = item_nes_pallete_get_index_colours (self);

  nes_current_pallete (cur_pallete, colour, index_color);
}

void
pallete_setup (
  GtkButton* btn,
  gpointer user_data
	)
{
	RetrospriteeditorNesListPallete *self = RETROSPRITEEDITOR_NES_LIST_PALLETE (user_data);
	gtk_window_present (GTK_WINDOW (self->setup_pallete_window));
	setup_pallete_nes_draw (RETROSPRITEEDITOR_SETUP_PALLETE_NES (self->setup_pallete_window));
}


void
palette_nes_redraw ()
{
	for (guint i = 0; i < 4; i++) {
		item_nes_palette_redraw (RETROSPRITEEDITOR_NES_ITEM_PALLETE (global->items[i]));
	}
}

static void
retrospriteeditor_nes_list_pallete_init (RetrospriteeditorNesListPallete *self)
{
	global = self;
  self->btn_pallete_setup = gtk_button_new_with_label ("PALLETE SETUP");
	g_signal_connect (self->btn_pallete_setup, "clicked", G_CALLBACK (pallete_setup), self);

	self->setup_pallete_window = g_object_new (RETROSPRITEEDITOR_TYPE_SETUP_PALLETE_NES,
			NULL);

	gtk_window_set_hide_on_close (GTK_WINDOW (self->setup_pallete_window), TRUE);

  self->list_pallete = gtk_grid_new ();

  GtkWidget *radiobuttons[4];
  GtkWidget *items[4];

  guint32 index = 0;
  guint32 i = 0;
	guint32 nindx = 0;
	guint32 nmax = 4;
  for (guint32 y = 0; y < 2; y++) {
    for (guint32 x = 0; x < 2; x++) {
      GtkWidget *item = g_object_new (RETROSPRITEEDITOR_TYPE_NES_ITEM_PALLETE,
                                      "orientation", GTK_ORIENTATION_HORIZONTAL,
                                      NULL);

			self->items[i] = item;

      radiobuttons[i] = item_nes_pallete_get_radio (RETROSPRITEEDITOR_NES_ITEM_PALLETE (item));
      items[i] = item;
      i++;

      gtk_grid_attach (GTK_GRID (self->list_pallete),
                       item,
                       x,
                       y,
                       1,
                       1);

      guint32 *pcolours = global_type_pallete_get_cur_ptr_pallete (index);
      guint32 *colours = g_malloc0 (sizeof (guint32) * 4);
      guint32 *index_colours = g_malloc0 (sizeof (guint32) * 4);
      *(colours + 0) = pcolours[0];
      *(colours + 1) = pcolours[1];
      *(colours + 2) = pcolours[2];
      *(colours + 3) = pcolours[3];
      *(index_colours + 0) = index + 0;
      *(index_colours + 1) = index + 1;
      *(index_colours + 2) = index + 2;
      *(index_colours + 3) = index + 3;
      item_nes_pallete_set_index_colours (RETROSPRITEEDITOR_NES_ITEM_PALLETE (item), index_colours);
      item_nes_pallete_set_colours (RETROSPRITEEDITOR_NES_ITEM_PALLETE (item), colours, 4);
			for (; nindx < nmax; nindx++) {
				RetrospriteeditorCanvas *c = setup_palette_window_get_palette (RETROSPRITEEDITOR_SETUP_PALLETE_NES (self->setup_pallete_window), nindx);
				canvas_set_index_colours (RETROSPRITEEDITOR_CANVAS (c), (index_colours + nindx % 4));
				canvas_set_colours (RETROSPRITEEDITOR_CANVAS (c), (colours + nindx % 4), 1);
			}
			nmax += 4;
      index += 16;
    }
  }

  self->box_main = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_append (GTK_BOX (self->box_main) , self->btn_pallete_setup);
  gtk_box_append (GTK_BOX (self->box_main), self->list_pallete);

  gtk_frame_set_child (GTK_FRAME (self), self->box_main);

  gtk_check_button_set_group (GTK_CHECK_BUTTON (radiobuttons[1]), GTK_CHECK_BUTTON (radiobuttons[0]));
  gtk_check_button_set_group (GTK_CHECK_BUTTON (radiobuttons[2]), GTK_CHECK_BUTTON (radiobuttons[0]));
  gtk_check_button_set_group (GTK_CHECK_BUTTON (radiobuttons[3]), GTK_CHECK_BUTTON (radiobuttons[0]));

  for (i = 0; i < 4; i++) {
    g_signal_connect (radiobuttons[i], "toggled",
                      G_CALLBACK (select_pallete),
                      items[i]);
  }

  gtk_check_button_set_active (GTK_CHECK_BUTTON (radiobuttons[0]), TRUE);
}

/* retrospriteeditor-frame-palette.c
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

#include "retrospriteeditor-frame-palette.h"
#include "retrospriteeditor-nes-palette.h"

struct _RetrospriteeditorFramePalette
{
  GtkFrame  parent_instance;

  GtkWidget *box_palette;
};

G_DEFINE_FINAL_TYPE (RetrospriteeditorFramePalette, retrospriteeditor_frame_palette, GTK_TYPE_FRAME)

enum {
  PROP_PLATFORM = 1,
  N_PROPERTIES
};

static void
set_platform (RetrospriteeditorFramePalette *self,
              guint32                        id)
{
  switch (id)
    {
    case PLATFORM_NES:
      self->box_palette = g_object_new (RETROSPRITEEDITOR_TYPE_NES_PALETTE,
                                        "orientation",
                                        GTK_ORIENTATION_VERTICAL,
                                        NULL);

      break;
    default:
      break;
    }

  gtk_widget_set_margin_top (self->box_palette, 10);
  gtk_widget_set_margin_start (self->box_palette, 10);
  gtk_widget_set_margin_end (self->box_palette, 10);
  gtk_widget_set_margin_bottom (self->box_palette, 10);
  gtk_box_set_spacing (GTK_BOX (self->box_palette), 10);
  gtk_frame_set_child (GTK_FRAME (self), self->box_palette);
}

static void
frame_palette_set_property (GObject      *object,
                     guint         property_id,
                     const GValue *value,
                     GParamSpec   *spec)
{
  RetrospriteeditorFramePalette *self = RETROSPRITEEDITOR_FRAME_PALETTE (object);

  switch (property_id)
    {
    case PROP_PLATFORM:
      set_platform (self, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
      break;
    }
}

static void
frame_palette_get_property (GObject    *object,
                     guint       property_id,
                     GValue     *value,
                     GParamSpec *spec)
{
  RetrospriteeditorFramePalette *self = RETROSPRITEEDITOR_FRAME_PALETTE (object);

  switch (property_id)
    {
    case PROP_PLATFORM:
      break;
    }
}

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void
retrospriteeditor_frame_palette_class_init (RetrospriteeditorFramePaletteClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = frame_palette_set_property;
  object_class->get_property = frame_palette_get_property;

  obj_properties[PROP_PLATFORM] =
      g_param_spec_uint ("platform",
                            "Platform",
                            "Set platform's type.",
                            0,
                            N_PLATFORMS,
                            0,
                            G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}

static void
retrospriteeditor_frame_palette_init (RetrospriteeditorFramePalette *self)
{
  gtk_widget_set_size_request (GTK_WIDGET (self), -1, -1);
}

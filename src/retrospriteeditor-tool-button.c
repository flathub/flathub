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

#include "retrospriteeditor-tool-button.h"

typedef struct _RetrospriteeditorToolButtonPrivate RetrospriteeditorToolButtonPrivate;

struct _RetrospriteeditorToolButtonPrivate
{
  guint32          tool_index;
};

G_DEFINE_TYPE_WITH_PRIVATE (RetrospriteeditorToolButton, retrospriteeditor_tool_button, GTK_TYPE_TOGGLE_BUTTON)

static void
retrospriteeditor_tool_button_class_init (RetrospriteeditorToolButtonClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

static void
retrospriteeditor_tool_button_init (RetrospriteeditorToolButton *self)
{
  RetrospriteeditorToolButtonClass *klass = G_TYPE_INSTANCE_GET_CLASS (self, RETROSPRITEEDITOR_TYPE_TOOL_BUTTON, RetrospriteeditorToolButtonClass);
  g_type_add_class_private (RETROSPRITEEDITOR_TYPE_TOOL_BUTTON, sizeof (RetrospriteeditorToolButtonPrivate));
  RetrospriteeditorToolButtonPrivate *priv = G_TYPE_CLASS_GET_PRIVATE (klass, RETROSPRITEEDITOR_TYPE_TOOL_BUTTON, RetrospriteeditorToolButtonPrivate);
  priv->tool_index = 0;
}

void tool_button_set_index (RetrospriteeditorToolButton *self,
                            guint32                      index)
{
  RetrospriteeditorToolButtonClass *klass = G_TYPE_INSTANCE_GET_CLASS (self, RETROSPRITEEDITOR_TYPE_TOOL_BUTTON, RetrospriteeditorToolButtonClass);
  RetrospriteeditorToolButtonPrivate *priv = G_TYPE_CLASS_GET_PRIVATE (klass, RETROSPRITEEDITOR_TYPE_TOOL_BUTTON, RetrospriteeditorToolButtonPrivate);
  priv->tool_index = index;
}

guint32 tool_button_get_type_index (RetrospriteeditorToolButton *self)
{
  RetrospriteeditorToolButtonClass *klass = G_TYPE_INSTANCE_GET_CLASS (self, RETROSPRITEEDITOR_TYPE_TOOL_BUTTON, RetrospriteeditorToolButtonClass);
  RetrospriteeditorToolButtonPrivate *priv = G_TYPE_CLASS_GET_PRIVATE (klass, RETROSPRITEEDITOR_TYPE_TOOL_BUTTON, RetrospriteeditorToolButtonPrivate);
  return priv->tool_index;
}

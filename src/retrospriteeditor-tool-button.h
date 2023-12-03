/* retrospriteeditor-tool-button.h
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
#include "tools.h"

G_BEGIN_DECLS

#define RETROSPRITEEDITOR_TYPE_TOOL_BUTTON (retrospriteeditor_tool_button_get_type())

G_DECLARE_DERIVABLE_TYPE (RetrospriteeditorToolButton, retrospriteeditor_tool_button, RETROSPRITEEDITOR, TOOL_BUTTON, GtkToggleButton)

struct _RetrospriteeditorToolButtonClass {
  GtkToggleButtonClass parent_instance;

  gpointer padding[12];
};

void tool_button_set_index (RetrospriteeditorToolButton *self,
                            guint32                      index);

guint32 tool_button_get_type_index (RetrospriteeditorToolButton *self);
G_END_DECLS

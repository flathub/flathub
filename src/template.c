/* retrospriteeditor-template.c
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

#include "retrospriteeditor-template.h"

struct _RetrospriteeditorTemplate
{
  GtkWidget  parent_instance;
};

G_DEFINE_FINAL_TYPE (RetrospriteeditorTemplate, retrospriteeditor_template, GTK_TYPE_WIDGET)

static void
retrospriteeditor_template_class_init (RetrospriteeditorTemplateClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

static void
retrospriteeditor_template_init (RetrospriteeditorTemplate *self)
{
}

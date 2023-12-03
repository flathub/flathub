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

#include "retrospriteeditor-new-project-nes.h"
#include "retrospriteeditor-window.h"
#include "project.h"

struct _RetrospriteeditorNewProjectNes
{
  GtkWindow  parent_instance;

	GtkWidget  *vert_box;
	GtkWidget  *frame_location;
	GtkWidget  *entry_location;
	GtkWidget  *button_location;
	GtkWidget  *frame_tileset;
	GtkWidget  *dropdown_tileset;
	GtkWidget  *frame_project_name;
	GtkWidget  *entry_project_name;
	GtkWidget  *button_create_project;
};

G_DEFINE_FINAL_TYPE (RetrospriteeditorNewProjectNes, retrospriteeditor_new_project_nes, GTK_TYPE_WINDOW)

static void
retrospriteeditor_new_project_nes_class_init (RetrospriteeditorNewProjectNesClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

static void
async_selected_folder (GObject *source_object,
		GAsyncResult *res,
		gpointer user_data)
{
	GtkFileDialog *dia = GTK_FILE_DIALOG (source_object);
	GFile *folder_project = gtk_file_dialog_select_folder_finish (
			dia,
			res,
			NULL);
	if (!folder_project)
		return;

	RetrospriteeditorNewProjectNes *self = RETROSPRITEEDITOR_NEW_PROJECT_NES (user_data);

	GtkEntryBuffer *buf = gtk_entry_get_buffer (GTK_ENTRY (self->entry_location));
	char *folder = g_file_get_path (folder_project);
	gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (buf), folder, -1);
}

static void
click_location (GtkButton *btn, gpointer user_data)
{
	RetrospriteeditorNewProjectNes *self = RETROSPRITEEDITOR_NEW_PROJECT_NES (user_data);

	GtkFileDialog *dia = gtk_file_dialog_new ();
	gtk_file_dialog_select_folder (dia,
			GTK_WINDOW (self),
			NULL,
			async_selected_folder,
			self);

}

static void
click_create_project (GtkButton *btn, gpointer user_data)
{
	RetrospriteeditorNewProjectNes *self = RETROSPRITEEDITOR_NEW_PROJECT_NES (user_data);

	GtkEntryBuffer *buf_location = gtk_entry_get_buffer (GTK_ENTRY (self->entry_location));
	GtkEntryBuffer *buf_project_name = gtk_entry_get_buffer (GTK_ENTRY (self->entry_project_name));

	const char *location = gtk_entry_buffer_get_text (buf_location);
	const char *project_name = gtk_entry_buffer_get_text (buf_project_name);

	project_set_folder_and_name (location, project_name);

	RetrospriteeditorWindow *win = main_window_get ();
	create_nes_widgets (win);
	gtk_window_close (GTK_WINDOW (self));
}

static void
retrospriteeditor_new_project_nes_init (RetrospriteeditorNewProjectNes *self)
{
	gtk_window_set_hide_on_close (GTK_WINDOW (self), TRUE);
	self->vert_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
	self->frame_location = g_object_new (GTK_TYPE_FRAME,
			"label", "Project Location", NULL);
	self->entry_location = gtk_entry_new ();
	self->button_location = gtk_button_new_with_label ("SET LOCATION");

	g_signal_connect (self->button_location, "clicked", G_CALLBACK (click_location), self);

	GtkWidget *box_location = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_append (GTK_BOX (box_location), self->entry_location);
	gtk_box_append (GTK_BOX (box_location), self->button_location);

	gtk_frame_set_child (GTK_FRAME (self->frame_location), box_location);

	gtk_box_append (GTK_BOX (self->vert_box), self->frame_location);

	gtk_window_set_child (GTK_WINDOW (self), self->vert_box);

	self->frame_tileset = g_object_new (GTK_TYPE_FRAME,
			"label", "Tileset map", NULL);

#if 0
	char *tile8x8 = g_strdup_printf ("8x8");
	GListStore *model_tileset = g_list_store_new (G_TYPE_POINTER);
	g_list_store_append (model_tileset, tile8x8);
	self->dropdown_tileset = gtk_drop_down_new (G_LIST_MODEL (model_tileset), NULL); // GListModel; GtkExpression; 
	
	GtkWidget *box_tileset = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_append (GTK_BOX (box_tileset), self->dropdown_tileset);
	gtk_frame_set_child (GTK_FRAME (self->frame_tileset), box_tileset);

	gtk_box_append (GTK_BOX (self->vert_box), self->frame_tileset);
#endif
	self->frame_project_name = g_object_new (GTK_TYPE_FRAME,
			"label", "Project Name", NULL);
	self->entry_project_name = gtk_entry_new ();

	GtkWidget *box_project_name = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_box_append (GTK_BOX (box_project_name), self->entry_project_name);

	gtk_frame_set_child (GTK_FRAME (self->frame_project_name), box_project_name);

	gtk_box_append (GTK_BOX (self->vert_box), self->frame_project_name);

	self->button_create_project = gtk_button_new_with_label ("CREATE PROJECT");
	
	gtk_box_append (GTK_BOX (self->vert_box), self->button_create_project);

	g_signal_connect (self->button_create_project, "clicked", G_CALLBACK (click_create_project), self);
}

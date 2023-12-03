/* retrospriteeditor-window.c
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

#include "retrospriteeditor-window.h"
#include "retrospriteeditor-canvas.h"
#include "retrospriteeditor-frame-palette.h"
#include "retrospriteeditor-tool-pencil.h"
#include "retrospriteeditor-new-project-nes.h"
#include "tools.h"
#include "project.h"
#include "type-pallete.h"
#include <libxml/parser.h>

#define DEFAULT_CANVAS_WIDTH             128
#define DEFAULT_CANVAS_HEIGHT            128
#define DEFAULT_CANVAS_SCALE               3
#define DEFAULT_PALETTE_TYPE               0
#define DEFAULT_WINDOW_WIDTH            1280
#define DEFAULT_WINDOW_HEIGHT            720
#define DEFAULT_SCROLL_AREA_WIDTH        512
#define DEFAULT_SCROLL_AREA_HEIGHT       512

struct _RetrospriteeditorWindow
{
  AdwApplicationWindow  parent_instance;

  GtkWidget           *header_bar;
  GtkWidget           *general_layout;
  GtkWidget           *menu_button;
  GtkWidget           *vert_layout;
  GtkWidget           *palette;
  GtkWidget           *canvas;
  GtkWidget           *scroll_window_canvas;
  GtkWidget           *frame_tools;
  GtkWidget           *box_tools;
  GtkIconTheme        *icon_theme;
  GtkWidget           *tool_button_pencil;
	GtkWidget  					*new_project_nes_window;
};

G_DEFINE_FINAL_TYPE (RetrospriteeditorWindow, retrospriteeditor_window, ADW_TYPE_APPLICATION_WINDOW)

static void
retrospriteeditor_window_class_init (RetrospriteeditorWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
}

static void
tool_toggled (GtkToggleButton *btn_base,
         gpointer         user_data)
{
  RetrospriteeditorWindow *self = RETROSPRITEEDITOR_WINDOW (user_data);
  RetrospriteeditorToolButton *btn = RETROSPRITEEDITOR_TOOL_BUTTON (btn_base);

  canvas_set_tool (RETROSPRITEEDITOR_CANVAS (self->canvas),
                   gtk_toggle_button_get_active (btn_base)? tool_button_get_type_index (btn): 0);

}

static void
action_menu (GSimpleAction *simple,
		GVariant *parameter,
		gpointer user_data)
{
	RetrospriteeditorWindow *self = RETROSPRITEEDITOR_WINDOW (user_data);

}

static void
async_selected_project (GObject *source_object,
		GAsyncResult *res,
		gpointer user_data)
{
	RetrospriteeditorWindow *self = RETROSPRITEEDITOR_WINDOW (user_data);

	GtkFileDialog *dia = GTK_FILE_DIALOG (source_object);
	GFile *project = gtk_file_dialog_open_finish (
			dia,
			res,
			NULL);

	if (!project)
		return;

	char *c_project = g_strdup (g_file_get_path (project));

	create_nes_widgets (self);
	project_open_nes (c_project);
}

static void
action_open_project_nes (GSimpleAction *simple,
		GVariant *parameter,
		gpointer user_data)
{
	RetrospriteeditorWindow *self = RETROSPRITEEDITOR_WINDOW (user_data);
	GtkFileDialog *dia = gtk_file_dialog_new ();
	gtk_file_dialog_open (dia,
			GTK_WINDOW (self),
			NULL,
			async_selected_project,
			self);

}

static void
export_to_file (RetrospriteeditorWindow *self)
{
	char *filepath = project_get_filepath_to_export ();
	if (!filepath)
		return;

	GFile *file = g_file_new_for_path (filepath);
	GFileOutputStream *out = g_file_replace (file, 
			NULL,
			FALSE,
			G_FILE_CREATE_REPLACE_DESTINATION,
			NULL,
			NULL);

	DataForOutput dt;
	global_get_data_for_output (&dt);

	g_output_stream_write (G_OUTPUT_STREAM (out), dt.data, dt.size, NULL, NULL);
	g_output_stream_close (G_OUTPUT_STREAM (out), NULL, NULL);

	g_free (dt.data);
}

static void
action_save_project (GSimpleAction *simple,
		GVariant *parameter,
		gpointer user_data)
{
	RetrospriteeditorWindow *self = RETROSPRITEEDITOR_WINDOW (user_data);

	export_to_file (self);
	project_save_palettes ();
}

void
create_nes_widgets (RetrospriteeditorWindow *self)
{
  self->canvas = g_object_new(RETROSPRITEEDITOR_TYPE_CANVAS, NULL);
  canvas_set_drawing_canvas (RETROSPRITEEDITOR_CANVAS (self->canvas));


  self->scroll_window_canvas = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->scroll_window_canvas),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (self->scroll_window_canvas),
    self->canvas);

  self->palette = g_object_new (RETROSPRITEEDITOR_TYPE_FRAME_PALETTE, NULL);

  self->frame_tools = gtk_frame_new ("Tools");


  gtk_box_append (GTK_BOX (self->general_layout), self->frame_tools);
  gtk_box_append (GTK_BOX (self->general_layout), self->scroll_window_canvas);
  gtk_box_append (GTK_BOX (self->general_layout), self->palette);


  self->box_tools = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  gtk_frame_set_child (GTK_FRAME (self->frame_tools), self->box_tools);

  self->icon_theme = gtk_icon_theme_get_for_display (gdk_display_get_default ());
  gtk_icon_theme_add_resource_path (self->icon_theme,
                                     "/org/xverizex/RetroSpriteEditor/icons");

  gtk_widget_set_hexpand (self->scroll_window_canvas, TRUE);
  gtk_widget_set_vexpand (self->scroll_window_canvas, TRUE);

  g_object_set (self->palette, "platform", PLATFORM_NES, NULL);
  gtk_widget_set_halign (self->palette, GTK_ALIGN_END);
  gtk_widget_set_hexpand (self->palette, FALSE);
  gtk_widget_set_vexpand (self->palette, FALSE);
  gtk_widget_set_hexpand (self->canvas, TRUE);
  gtk_widget_set_vexpand (self->canvas, TRUE);
  gtk_widget_set_vexpand (self->frame_tools, TRUE);

  CanvasSettings cs;
  cs.type_canvas    = TYPE_CANVAS_TILESET;
  cs.canvas_width   = DEFAULT_CANVAS_WIDTH;
  cs.canvas_height  = DEFAULT_CANVAS_HEIGHT;
  cs.palette_type   = DEFAULT_PALETTE_TYPE;
  cs.scale          = DEFAULT_CANVAS_SCALE;
  cs.width_rect    = 8;
  cs.height_rect   = 8;
  cs.left_top       = FALSE;
  g_object_set (self->canvas, "settings", &cs, NULL);

  canvas_shut_on_events (RETROSPRITEEDITOR_CANVAS (self->canvas));


  self->tool_button_pencil = g_object_new (RETROSPRITEEDITOR_TYPE_TOOL_PENCIL,
                                        "icon-name", "pencil",
                                        "has-frame", FALSE,
                                        NULL);


  gtk_box_append (GTK_BOX (self->box_tools), self->tool_button_pencil);

  g_signal_connect (self->tool_button_pencil, "toggled", G_CALLBACK (tool_toggled),
                    self);
}

static void
action_new_project_nes (GSimpleAction *simple,
		GVariant *parameter,
		gpointer user_data)
{
	RetrospriteeditorWindow *self = RETROSPRITEEDITOR_WINDOW (user_data);
	gtk_window_present (GTK_WINDOW (self->new_project_nes_window));
}

static void
action_export (GSimpleAction *simple,
		GVariant *parameter,
		gpointer user_data)
{
	RetrospriteeditorWindow *self = RETROSPRITEEDITOR_WINDOW (user_data);
	export_to_file (self);
}

static RetrospriteeditorWindow *global;

RetrospriteeditorWindow *
main_window_get ()
{
	return global;
}

static void
retrospriteeditor_window_init (RetrospriteeditorWindow *self)
{
	global = self;

	xmlInitParser ();

	self->menu_button = g_object_new (GTK_TYPE_MENU_BUTTON,
			"icon-name", "open-menu-symbolic",
			NULL);

	const GActionEntry entries[] = {
		{"export", 		action_export},
		{"new_project_nes", action_new_project_nes},
		{"save_project", action_save_project},
		{"open_project_nes", action_open_project_nes}
	};

	g_action_map_add_action_entries (G_ACTION_MAP (self), entries, G_N_ELEMENTS (entries), self);

	GMenu *menu_open = g_menu_new ();
	g_menu_append (menu_open, "Open Project NES", "win.open_project_nes");
	GMenu *menu_project = g_menu_new ();
	g_menu_append (menu_project, "New NES Project", "win.new_project_nes");
	GMenu *menu_root = g_menu_new ();
	g_menu_append_submenu (menu_root, "New Project", G_MENU_MODEL (menu_project));
	g_menu_append_submenu (menu_root, "Open", G_MENU_MODEL (menu_open));
	g_menu_append (menu_root, "Save", "win.save_project");
	g_menu_append (menu_root, "Export", "win.export");

	self->new_project_nes_window = g_object_new (RETROSPRITEEDITOR_TYPE_NEW_PROJECT_NES,
			NULL);

	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (self->menu_button), G_MENU_MODEL (menu_root));

  self->header_bar = adw_header_bar_new ();
  adw_header_bar_set_decoration_layout (ADW_HEADER_BAR (self->header_bar), "icon:minimize,maximize,close");
  adw_header_bar_set_show_end_title_buttons (ADW_HEADER_BAR (self->header_bar), TRUE);
	adw_header_bar_pack_end (ADW_HEADER_BAR (self->header_bar), self->menu_button);

  self->vert_layout = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  self->general_layout = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), self->vert_layout);
  gtk_window_set_default_size (GTK_WINDOW (self), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

  gtk_box_append (GTK_BOX (self->vert_layout), self->header_bar);
  gtk_box_append (GTK_BOX (self->vert_layout), self->general_layout);

	gtk_window_set_title (GTK_WINDOW (self), "Retro Sprite Editor");
}

/* main.c
 *
 * Copyright 2021 cf
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
 */

#include <glib/gi18n.h>

#include "nem-config.h"
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "main-window.h"
#include "register-window.h"

static GtkWidget *register_window;
char *root_app;
char *root_sounds;

static gboolean windows_close_request_cb (GtkWindow *widget,
		gpointer user_data)
{
	gtk_widget_hide (GTK_WIDGET (widget));
	return TRUE;
}

static void activate_register_cb (GSimpleAction *simple,
		GVariant *parameter,
		gpointer user_data)
{
	g_object_set (register_window,
			"title", "register account",
			"type", "register",
			NULL
		     );
	gtk_widget_set_visible (register_window, TRUE);
}

static void activate_login_cb (GSimpleAction *simple,
		GVariant *parameter,
		gpointer user_data)
{
	g_object_set (register_window,
			"title", "log in",
			"type", "login",
			NULL
		     );
	gtk_widget_set_visible (register_window, TRUE);
}

static void activate_quit_cb (GSimpleAction *simple,
		GVariant *parameter,
		gpointer user_data)
{
	g_application_quit (G_APPLICATION (user_data));
	exit (0);
}

static GActionGroup *create_action_group (GtkApplication *self) {
	const GActionEntry entries[] = {
		{ "register", activate_register_cb, NULL, NULL, NULL },
		{ "login", activate_login_cb, NULL, NULL, NULL },
		{ "quit", activate_quit_cb, NULL, NULL, NULL }
	};

	g_action_map_add_action_entries (G_ACTION_MAP (self), entries, G_N_ELEMENTS (entries), self);
}

static void app_activate_cb (GtkApplication *app, gpointer user_data)
{

	register_window = g_object_new (REGISTER_TYPE_WINDOW,
			"default-width", 300,
			"default-height", 450,
			NULL);


	g_signal_connect (register_window, "close-request", G_CALLBACK (windows_close_request_cb), NULL);

	GtkWidget *main_window = g_object_new (MAIN_TYPE_WINDOW,
			"default-width", 1024,
			"default-height", 600,
			"app", app,
			NULL);

	g_object_set (register_window,
			"app", app,
			"main_window", main_window,
			NULL);

	gtk_window_set_transient_for (GTK_WINDOW (register_window), GTK_WINDOW (main_window));

	gtk_application_add_window (GTK_APPLICATION (app), GTK_WINDOW (main_window));

	gtk_widget_set_visible (main_window, TRUE);

	create_action_group (app);

	const char *quit_accels[2] = { "<Ctrl>Q", NULL };
	gtk_application_set_accels_for_action (GTK_APPLICATION (app),
			"app.quit",
			quit_accels);

}


static void get_root_app ()
{
  char root[256];
  char root_keys[256];

  const char *home = getenv ("HOME");

  snprintf (root, 256, "%s/.nem", home);
	if (access (root, F_OK) == -1) {
		mkdir (root, S_IRWXU | S_IRWXG | S_IRWXO);
	}

  snprintf (root, 256, "%s/.nem/sounds", home);
	if (access (root, F_OK) == -1) {
		mkdir (root, S_IRWXU | S_IRWXG | S_IRWXO);
	}

  root_sounds = strdup (root);

  snprintf (root_keys, 256, "%s/keys", root);
	if (access (root_keys, F_OK) == -1) {
		mkdir (root_keys, S_IRWXU | S_IRWXG | S_IRWXO);
	}
  root_app = strdup (root_keys);
}

int main (int argc, char **argv)
{
  get_root_app ();
	gst_init (NULL, NULL);
	GtkApplication *app = gtk_application_new ("org.xverizex.nem-desktop", G_APPLICATION_FLAGS_NONE);
	g_application_register (G_APPLICATION (app), NULL, NULL);
	g_signal_connect (app, "activate", G_CALLBACK (app_activate_cb), NULL);
	return g_application_run (G_APPLICATION (app), argc, argv);
}

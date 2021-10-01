/* main-window.c
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

#include "main-window.h"
#include "user-item.h"
#include <json-glib/json-glib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "cert.h"
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <gst/gst.h>

extern char *root_app;
extern char *root_sounds;

struct _MainWindow {
	GtkWindow parent_instance;

	GtkApplication *app;

	GdkDisplay *display;
	GtkCssProvider *provider;
	GSocketConnection *conn;
	GNotification *notification;

	GtkWidget *header_bar;
	GtkWidget *main_pane;
	GtkWidget *frame_list;
	GtkWidget *frame_chat;
	GtkWidget *scroll_list;
	GtkWidget *list_users;
	GtkWidget *left_pane_button;
	GtkWidget *image_header_left_pane;
	GtkWidget *image_handshake;
	GtkWidget *image_header_menu;
	GtkWidget *menu_button;
	GtkWidget *handshake_button;
	GMenu *menu;

	GIOStream *gio;
	GInputStream *igio;
	GOutputStream *ogio;
	char *buf;

  JsonReader *reader;
	GstElement *new_message;

};

G_DEFINE_TYPE (MainWindow, main_window, GTK_TYPE_WINDOW)

#define TOTAL_SIZE      1024 * 1024 * 30

static void user_item_row_selected_cb (GtkListBox *box,
		GtkListBoxRow *row,
		gpointer user_data)
{
	MainWindow *self = MAIN_WINDOW (user_data);
  (void) box;
	GtkWidget *child = gtk_list_box_row_get_child (row);
	int handshaking;
	g_object_get (child,
			"handshaking", &handshaking,
			NULL);
	int blink_handshaking;
	g_object_get (child,
			"blink_handshake", &blink_handshaking,
			NULL);

	if (!blink_handshaking) {
		g_object_set (child,
			"blink", 0,
			NULL);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->handshake_button), handshaking);
	user_item_set_chat (USER_ITEM (child));
}

static void fill_arrays (MainWindow *self)
{


  json_reader_is_object (self->reader);


  int count = json_reader_count_members (self->reader);

  json_reader_read_member (self->reader, "users");
  size_t length = json_reader_count_elements (self->reader);

	for (size_t i = 0; i < length; i++) {
		json_reader_read_element (self->reader, i);

    json_reader_read_member (self->reader, "name");
    JsonNode *jname = json_reader_get_value (self->reader);
    json_reader_end_member (self->reader);
    const char *name = json_node_get_string (jname);

		json_reader_read_member (self->reader, "status");
    int status = json_reader_get_int_value (self->reader);
    json_reader_end_member (self->reader);


    GtkWidget *user_item = g_object_new (USER_TYPE_ITEM,
                                         "handbutton", self->handshake_button,
                                         "name", name,
                                         "status", status,
                                         "blink", 0,
                                         "frame_chat", self->frame_chat,
                                         "app", self->app,
                                         "notification", self->notification,
                                         "ogio", self->ogio,
                                         "main_window", self,
                                         NULL);


		gtk_list_box_append (GTK_LIST_BOX (self->list_users), user_item);
    json_reader_end_element (self->reader);
  }
  json_reader_end_member (self->reader);

	gtk_list_box_invalidate_sort (GTK_LIST_BOX (self->list_users));


}

static void fill_arrays_new_status (MainWindow *self)
{

	json_reader_read_member (self->reader, "status");
  int status = json_reader_get_int_value (self->reader);
  json_reader_end_member (self->reader);

  json_reader_read_member (self->reader, "name");
  JsonNode *jname = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);

  const char *name = json_node_get_string (jname);


	for (int i = 0; 1; i++) {
		GtkListBoxRow *row = gtk_list_box_get_row_at_index (
				GTK_LIST_BOX (self->list_users),
				i);
		if (row == NULL) break;

		GtkWidget *item = gtk_list_box_row_get_child (row);
		const char *n = user_item_get_name (USER_ITEM (item));
		if (!strncmp (n, name, strlen (name) + 1)) {
			user_item_set_status (USER_ITEM (item), status);
      json_reader_end_member (self->reader);

			gtk_list_box_invalidate_sort (GTK_LIST_BOX (self->list_users));

			return;
		}
	}

  GtkWidget *user_item = user_item_new ();

	g_object_set (user_item,
                "name", name,
                "handbutton", self->handshake_button,
                "status", status,
                "blink", 0,
                "frame_chat", self->frame_chat,
                "app", self->app,
                "notification", self->notification,
                "ogio", self->ogio,
                "main_window", self,
                NULL);

	gtk_list_box_append (GTK_LIST_BOX (self->list_users), user_item);
	gtk_list_box_invalidate_sort (GTK_LIST_BOX (self->list_users));

  return;
}

static char *escape_n_from_temp_key (const char *temp_key)
{
	int len = strlen (temp_key);
	char *key = malloc (len + 1);
	int index = 0;
	for (int i = 0; i < len; i++) {
		if (temp_key[i + 0] == '\\' && temp_key[i + 1] == 'n') {
			i++;
			key[index++] = '\n';
			continue;
		}
		key[index++] = temp_key[i];
	}
	key[index] = 0;

	return key;
}

static GtkWidget *get_child_by_name (GtkWidget *, const char *from);

static void handshake_key_save (MainWindow *self)
{
  json_reader_read_member (self->reader, "from");
  JsonNode *jfrom = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);
  json_reader_read_member (self->reader, "key");
  JsonNode *jkey = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);
	const char *from = json_node_get_string (jfrom);
	const char *temp_key = json_node_get_string (jkey);
	char *key = escape_n_from_temp_key (temp_key);

	GtkWidget *child = get_child_by_name (self->list_users, from);
	if (child) {
		g_object_set (child,
			"blink_handshake", 0,
			"handshaking", 0,
			NULL);
		GtkListBoxRow *row_sel_child = gtk_list_box_get_selected_row (GTK_LIST_BOX (self->list_users));
		GtkWidget *sel_child = gtk_list_box_row_get_child (row_sel_child);
		if (child == sel_child) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->handshake_button), 0);
		}
	}


	char path[256];
	snprintf (path, 256, "%s/%s/crypto.pem", root_app, from);
	GFile *file = g_file_new_for_path (path);
	if (!file) {
		free (key);

		return;
	}

	GFileOutputStream *out = g_file_create (file,
			G_FILE_CREATE_REPLACE_DESTINATION,
			NULL,
			NULL);
	if (!out) {
		free (key);
		return;
	}

	g_output_stream_write (G_OUTPUT_STREAM (out),
			key,
			strlen (key),
			NULL,
			NULL
			);

	g_output_stream_close (G_OUTPUT_STREAM (out),
			NULL,
			NULL);

	free (key);

	return;
}

static void handshake_answer_status (MainWindow *self)
{


  json_reader_read_member (self->reader, "status_handshake");
  JsonNode *jstatus = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);
  json_reader_read_member (self->reader, "to_name");
  JsonNode *jto_name = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);

	int status = json_node_get_int (jstatus);
	const char *name = json_node_get_string (jto_name);

	for (int i = 0; 1; i++) {
		GtkListBoxRow *row = gtk_list_box_get_row_at_index (
				GTK_LIST_BOX (self->list_users),
				i);
		if (row == NULL) break;

		GtkWidget *item = gtk_list_box_row_get_child (row);
		const char *n = user_item_get_name (USER_ITEM (item));
		if (!strncmp (n, name, strlen (name) + 1)) {
			g_object_set (item,
					"handshaking", status,
					NULL
				     );

			return;
		}
	}

	return;
}

#define FROM_TO_ME             0
#define FROM_ME_TO             1

static void read_message_from (GtkWidget *child,
		const char *name,
		char *path,
		const unsigned char *buffer,
		MainWindow *self,
		size_t buffer_len,
		const int show_notification)
{
  (void) self;
	unsigned char *to = calloc (1024 * 1024 * 30, 1);
	if (!to) return;

	FILE *fp = fopen (path, "rb");
	if (!fp) {
		free (to);
		return;
	}

	int padding = RSA_PKCS1_PADDING;

	RSA *rsa = PEM_read_RSAPrivateKey (fp, NULL, NULL, NULL);

	int encrypted_length = RSA_private_decrypt (buffer_len, buffer, to, rsa, padding);
  (void) encrypted_length;
	RSA_free (rsa);

	user_item_add_message (USER_ITEM (child), to, FROM_TO_ME, name, show_notification);

	free (to);
	fclose (fp);
}

static unsigned char get_hex (char d, int cq)
{
	switch (d) {
		case '0'...'9':
			return (d - 48) * cq;
		case 'a'...'f':
			return (d - 97 + 10) * cq;
		default:
			return 0;
	}
}

static unsigned char *convert_data_to_hex (const char *data, size_t *ll)
{
	*ll = 0;
	int len = strlen (data);
	unsigned char *dt = calloc (len, 1);

	unsigned char *d = dt;
	for (int i = 0; i < len; i += 2) {
		*d = get_hex (data[i + 0], 16) + get_hex (data[i + 1], 1);
		d++;
		(*ll)++;
	}

	return dt;
}
static GtkWidget *get_child_by_name (GtkWidget *list_users, const char *from)
{
	for (int i = 0; 1; i++) {
		GtkListBoxRow *row = gtk_list_box_get_row_at_index (
				GTK_LIST_BOX (list_users),
				i);
		if (row == NULL) break;

		GtkWidget *item = gtk_list_box_row_get_child (row);
		const char *n = user_item_get_name (USER_ITEM (item));
		if (!strncmp (n, from, strlen (from) + 1)) {
			return item;
		}
	}
	return NULL;
}

static void handshake_notice (MainWindow *self)
{

  json_reader_read_member (self->reader, "from");
  JsonNode *jfrom = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);
  json_reader_read_member (self->reader, "status");
  JsonNode *jstatus = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);
  const char *from = json_node_get_string (jfrom);
  const int status = json_node_get_int (jstatus);

	GtkWidget *child = get_child_by_name (self->list_users, from);
	if (!child) {
		return;
	}

	g_object_set (child,
			"blink_handshake", status,
			NULL);

	return;
}

static void got_message (MainWindow *self)
{

  json_reader_read_member (self->reader, "from");
  JsonNode *jfrom = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);
  json_reader_read_member (self->reader, "data");
  JsonNode *jdata = json_reader_get_value (self->reader);
  json_reader_end_member (self->reader);

	const char *from = json_node_get_string (jfrom);
	const char *data = json_node_get_string (jdata);

	size_t len;
	unsigned char *dt = convert_data_to_hex (data, &len);

	GtkWidget *child = get_child_by_name (self->list_users, from);
	if (!child) {
		free (dt);
		return;
	}
	GtkListBoxRow *row = gtk_list_box_get_selected_row (GTK_LIST_BOX (self->list_users));
	GtkWidget *row_child = NULL;
	if (row)
		row_child = gtk_list_box_row_get_child (row);

	char path[256];
	snprintf (path, 256, "%s/%s/key.pem", root_app, from);

	int show_notification = 0;
	if (child != row_child) {
		show_notification = 1;
	}
	read_message_from (child, from, path, dt, self, len, show_notification);

	free (dt);

	if (child != row_child) {
		g_object_set (child,
				"blink", 1,
				NULL);

	}

	main_window_play_new_message (self);

	return;
}

static void receive_handler_cb (GObject *source_object,
                                GAsyncResult *res,
                                gpointer user_data)
{
	MainWindow *self = MAIN_WINDOW (user_data);

	GError *error = NULL;
  gssize readed = g_input_stream_read_finish (G_INPUT_STREAM (self->igio),
                                              res,
                                              &error);
  if (error) {
    g_print ("error result read: %s\n", error->message);
    return;
  }
  self->buf[readed] = 0;


  g_autoptr(JsonParser) parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, self->buf, -1, &error)) {
    if (error) {
      g_print ("load from data: %s\n", error->message);
      g_error_free (error);
      return;
    }
  }



  g_autoptr(JsonReader) reader = json_reader_new (json_parser_get_root (parser));
  json_reader_read_member (reader, "type");
  JsonNode *jtype = json_reader_get_value (reader);
  json_reader_end_member (reader);
  const char *type = json_node_get_string (jtype);

	if (!strncmp (type, "all_users", 10)) {
    self->reader = reader;
		fill_arrays (self);
    goto end;
	}
	if (!strncmp (type, "status_online", 14)) {
    self->reader = reader;
		fill_arrays_new_status (self);
    goto end;
	}
	if (!strncmp (type, "handshake_answer", 17)) {
	  self->reader = reader;
	  handshake_answer_status (self);
    goto end;
	}
	if (!strncmp (type, "handshake_key", 14)) {
		self->reader = reader;
		handshake_key_save (self);
    goto end;
	}
	if (!strncmp (type, "message", 8)) {
    self->reader = reader;
		got_message (self);
    goto end;
	}
	if (!strncmp (type, "handshake_notice", 17)) {
    self->reader = reader;
		handshake_notice (self);
    goto end;
	}

end:

  g_input_stream_read_async (self->igio,
                                     self->buf,
                                     TOTAL_SIZE,
                                     G_PRIORITY_DEFAULT,
                                     NULL,
                                     receive_handler_cb,
                                     self);
}

static JsonNode *build_json_get_list () {
  g_autoptr(JsonBuilder) builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "type");
  json_builder_add_string_value (builder, "get_list");
  json_builder_end_object (builder);

  JsonNode *node = json_builder_get_root (builder);
	return node;
}

static JsonNode *build_json_feed () {
  g_autoptr(JsonBuilder) builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "type");
  json_builder_add_string_value (builder, "feed");
  json_builder_end_object (builder);

  JsonNode *node = json_builder_get_root (builder);
	return node;
}

void main_window_feed (MainWindow *self)
{
	self->ogio = g_io_stream_get_output_stream (G_IO_STREAM (self->conn));

	JsonNode *node = build_json_feed ();

	g_autoptr(JsonGenerator) gen = json_generator_new ();
  json_generator_set_root (gen, node);
  gsize length;
  g_autofree char *buffer = json_generator_to_data (gen, &length);

	GError *error = NULL;

	g_output_stream_write (G_OUTPUT_STREAM (self->ogio),
			buffer,
			length,
			NULL,
			&error);
	if (error) {
		g_print ("error send: %s\n", error->message);
	}

  //g_object_unref (node);
}

void main_window_get_list_users (MainWindow *self)
{
	self->ogio = g_io_stream_get_output_stream (G_IO_STREAM (self->conn));

	JsonNode *node = build_json_get_list ();

	g_autoptr(JsonGenerator) gen = json_generator_new ();
  json_generator_set_root (gen, node);
  gsize length;
  g_autofree char *buffer = json_generator_to_data (gen, &length);

	GError *error = NULL;
	g_output_stream_write (G_OUTPUT_STREAM (self->ogio),
			buffer,
			length,
			NULL,
			&error);
	if (error) {
		g_print ("error send: %s\n", error->message);
	}

	//g_object_unref (node);
}

typedef enum {
	PROP_CONN = 1,
	PROP_NOTIFICATION,
	PROP_APPLICATION,
	N_PROPERTIES
} MainWindowProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void main_window_set_property (GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec)
{
	MainWindow *self = MAIN_WINDOW (object);

	switch ((MainWindowProperty) property_id) {
		case PROP_CONN:
      if (self->conn) g_object_unref (self->conn);
      if (self->igio) g_io_stream_close (self->gio, NULL, NULL);

			self->conn = g_value_get_object (value);
			self->igio = g_io_stream_get_input_stream (G_IO_STREAM (self->conn));
			self->ogio = g_io_stream_get_output_stream (G_IO_STREAM (self->conn));
      g_input_stream_read_async (self->igio,
                                     self->buf,
                                     TOTAL_SIZE,
                                     G_PRIORITY_DEFAULT,
                                     NULL,
                                     receive_handler_cb,
                                     self);

			break;
		case PROP_NOTIFICATION:
			self->notification = g_value_get_object (value);
			break;
		case PROP_APPLICATION:
			self->app = g_value_get_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static const char *styles =
"window#main { background-color: #fafafa; }"
"headerbar#main { background: #fcfcfc; }"
"frame#list { background-color: #000000; }"
"list#list_users { background-color: #d9d9d9; font-size: 16px; }"
"list#list_users row:selected { background-color: #9ecaff; min-height: 64px; font-size: 16px; }"
"image#header { min-width: 32px; min-height: 32px; }"
;

static void main_window_class_init (MainWindowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = main_window_set_property;

	obj_properties[PROP_CONN] = g_param_spec_object (
			"conn",
			"Conn",
			"Connection",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);
	obj_properties[PROP_NOTIFICATION] = g_param_spec_object (
			"notification",
			"Notification",
			"Notification",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);
	obj_properties[PROP_APPLICATION] = g_param_spec_object (
			"app",
			"app",
			"app",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);

	g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}

static int switcher_dec;
static int switcher_inc;

static gboolean handler_left_pane_dec_cb (gpointer user_data)
{
	if (switcher_inc) {
		switcher_dec = 0;
		return G_SOURCE_REMOVE;
	}

	MainWindow *self = (MainWindow *) user_data;

	int pos = gtk_paned_get_position (GTK_PANED (self->main_pane));
	if ((pos - 30) <= 0) {
		gtk_paned_set_position (GTK_PANED (self->main_pane), 0);
		switcher_dec = 0;
		return G_SOURCE_REMOVE;
	} else {
		pos -= 30;
		gtk_paned_set_position (GTK_PANED (self->main_pane), pos);
		return G_SOURCE_CONTINUE;
	}
}

#define WIDTH_USER_LIST        342

static gboolean handler_left_pane_inc_cb (gpointer user_data)
{
	if (switcher_dec) {
		switcher_inc = 0;
		return G_SOURCE_REMOVE;
	}

	MainWindow *self = (MainWindow *) user_data;

	int pos = gtk_paned_get_position (GTK_PANED (self->main_pane));
	if ((pos + 30) >= WIDTH_USER_LIST) {
		gtk_paned_set_position (GTK_PANED (self->main_pane), WIDTH_USER_LIST);
		switcher_inc = 0;
		return G_SOURCE_REMOVE;
	} else {
		pos += 30;
		gtk_paned_set_position (GTK_PANED (self->main_pane), pos);
		return G_SOURCE_CONTINUE;
	}
}

#define OFF       0
#define ON        1

static JsonNode *build_json_handshake (const char *name, const int status)
{
  g_autoptr(JsonBuilder) builder = json_builder_new ();

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "type");
  json_builder_add_string_value (builder, "handshake");

  json_builder_set_member_name (builder, "to_name");
  json_builder_add_string_value (builder, name);

  json_builder_set_member_name (builder, "status");
  json_builder_add_int_value (builder, status);



	if (status) {
		generate_keys_for (name);
		char path[256];
		snprintf (path, 256, "%s/%s/pub.pem", root_app, name);
		GFile *file = g_file_new_for_path (path);
		char *content = NULL;
		gsize length = 0;
		GError *error = NULL;
		g_file_load_contents (file,
				NULL,
				&content,
				&length,
				NULL,
				&error);
		if (error) {
			g_print ("read key pub content: %s\n", error->message);
			g_error_free (error);
		}

		if (content) {
      json_builder_set_member_name (builder, "key");
      json_builder_add_string_value (builder, content);
			g_free (content);
		}
	}

  json_builder_end_object (builder);

  JsonNode *node = json_builder_get_root (builder);
	return node;
}

static void send_status_handshake (MainWindow *self, const char *name, const int status)
{
  (void) self;
	JsonNode *node = build_json_handshake (name, status);

  g_autoptr(JsonGenerator) gen = json_generator_new ();
  json_generator_set_root (gen, node);

  gsize length;
	g_autofree char *data = json_generator_to_data (gen, &length);

	g_output_stream_write (
			G_OUTPUT_STREAM (self->ogio),
			data,
			length,
			NULL,
			NULL
			);

}

static void handshake_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	MainWindow *self = (MainWindow *) user_data;

	int status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

	GtkListBoxRow *row = gtk_list_box_get_selected_row (GTK_LIST_BOX (self->list_users));
	if (!row) return;

	GtkWidget *child = gtk_list_box_row_get_child (row);
	const char *name = user_item_get_name (USER_ITEM (child));

	if (!status) {
		send_status_handshake (self, name, OFF);
	} else {
		send_status_handshake (self, name, ON);
	}
}

static void left_pane_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	MainWindow *self = (MainWindow *) user_data;

	int status = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

	if (status) {
		switcher_inc = 1;
		g_timeout_add (30, handler_left_pane_inc_cb, self);
	} else {
		switcher_dec = 1;
		g_timeout_add (30, handler_left_pane_dec_cb, self);
	}
}

int list_users_sort_func_cb (
		GtkListBoxRow *row1,
		GtkListBoxRow *row2,
		gpointer user_data)
{
  (void) user_data;

	GtkWidget *child1 = gtk_list_box_row_get_child (row1);
	GtkWidget *child2 = gtk_list_box_row_get_child (row2);

	int status1 = user_item_get_status (USER_ITEM (child1));
	int status2 = user_item_get_status (USER_ITEM (child2));

	if (status1 > status2) return -1;
	if (status1 < status2) return 1;
	return 0;
}


void main_window_play_new_message (MainWindow *self)
{
	gst_element_set_state (self->new_message, GST_STATE_READY);
	gst_element_seek_simple (self->new_message, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, 0L);
	gst_element_set_state (self->new_message, GST_STATE_PLAYING);
}

static void main_window_init (MainWindow *self)
{
  GFile *ff = g_file_new_for_uri ("resource:///org/xverizex/nem-desktop/in_message.mp3");

	GError *error = NULL;
  char ppath[512];
  snprintf (ppath, 512, "%s/in_message.mp3", root_sounds);
  if (access (ppath, F_OK)) {
    g_autofree char *content = NULL;
    gsize length;
    if (!g_file_load_contents (ff,
                               NULL,
                               &content,
                               &length,
                               NULL,
                               &error))
      {
        if (error)
          {
            g_print ("error resource load in_message: %s\n", error->message);
            g_error_free (error);
            error = NULL;
          }
      } else {
        GFile *file = g_file_new_for_path (ppath);
        GFileOutputStream *out = g_file_create (file,
                                                G_FILE_CREATE_NONE,
                                                NULL,
                                                &error);
        if (error) {
          g_print ("create file in_message.: %s\n", error->message);
          g_error_free (error);
          error = NULL;
        }
               g_output_stream_write (G_OUTPUT_STREAM (out),
                                      content,
                                      length,
                                      NULL,
                                      &error);
        if (error) {
          g_print ("error write in_message: %s\n", error->message);
          g_error_free (error);
          error = NULL;
        }
        g_output_stream_close (G_OUTPUT_STREAM (out), NULL, NULL);

      }

  }
  snprintf (ppath, 512,"filesrc location=%s/in_message.mp3 "
			"! mpegaudioparse ! mpg123audiodec ! audioconvert ! audioresample ! autoaudiosink",
            root_sounds);
	self->new_message = gst_parse_launch ( ppath, &error);
	if (error) {
		g_print ("error parse gst file in_message: %s\n", error->message);
		g_error_free (error);
	}
	self->buf = malloc (TOTAL_SIZE);

	self->display = gdk_display_get_default ();
	self->provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_data (self->provider, styles, -1);
	gtk_style_context_add_provider_for_display (self->display, (GtkStyleProvider *) self->provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_widget_set_name ((GtkWidget *) self, "main");
	gtk_window_set_title (GTK_WINDOW (self), "SECURE CHAT");

	self->header_bar = gtk_header_bar_new ();
	gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (self->header_bar), ":minimize,maximize,close");
	gtk_header_bar_set_show_title_buttons (GTK_HEADER_BAR (self->header_bar), TRUE);
	gtk_window_set_titlebar (GTK_WINDOW (self), self->header_bar);
	gtk_widget_set_name (self->header_bar, "main");

	self->main_pane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	self->frame_list = gtk_frame_new (NULL);
	self->frame_chat = gtk_frame_new (NULL);
	g_object_set (self->main_pane, "vexpand", TRUE, "hexpand", TRUE, NULL);
	g_object_set (self->frame_chat, "vexpand", TRUE, "hexpand", TRUE, NULL);

	gtk_widget_set_size_request (self->frame_list, WIDTH_USER_LIST, -1);
	gtk_widget_set_size_request (self->frame_chat, 1024 - WIDTH_USER_LIST, -1);
	gtk_widget_set_visible (self->main_pane, TRUE);
	gtk_widget_set_visible (self->frame_list, TRUE);
	gtk_widget_set_visible (self->frame_chat, TRUE);
	gtk_widget_set_name (self->frame_list, "list");
	gtk_paned_set_start_child (GTK_PANED (self->main_pane), self->frame_list);
	gtk_paned_set_resize_start_child (GTK_PANED (self->main_pane), FALSE);
	gtk_paned_set_shrink_start_child (GTK_PANED (self->main_pane), TRUE);
	gtk_paned_set_end_child (GTK_PANED (self->main_pane), self->frame_chat);
	gtk_paned_set_resize_end_child (GTK_PANED (self->main_pane), TRUE);
	gtk_paned_set_shrink_end_child (GTK_PANED (self->main_pane), TRUE);
	gtk_window_set_child (GTK_WINDOW (self), self->main_pane);

	self->scroll_list = gtk_scrolled_window_new ();
	gtk_frame_set_child (GTK_FRAME (self->frame_list), self->scroll_list);

	self->list_users = gtk_list_box_new ();
	g_signal_connect (self->list_users, "row-selected", G_CALLBACK (user_item_row_selected_cb), self);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (self->scroll_list), self->list_users);
	gtk_widget_set_name (GTK_WIDGET (self->list_users), "list_users");
	gtk_list_box_set_sort_func (GTK_LIST_BOX (self->list_users),
			list_users_sort_func_cb,
			NULL,
			NULL
			);

	self->left_pane_button = gtk_toggle_button_new ();
	self->image_header_left_pane = g_object_new (GTK_TYPE_IMAGE,
                                               "resource", "/org/xverizex/nem-desktop/left-pane.svg",
                                               NULL);

	gtk_button_set_child (GTK_BUTTON (self->left_pane_button), self->image_header_left_pane);
	gtk_widget_set_name (self->image_header_left_pane, "header");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->left_pane_button), TRUE);
	g_signal_connect (self->left_pane_button, "clicked", G_CALLBACK (left_pane_button_clicked_cb), self);

	self->handshake_button = gtk_toggle_button_new ();
  self->image_handshake = g_object_new (GTK_TYPE_IMAGE,
                                        "resource", "/org/xverizex/nem-desktop/handshake.svg",
                                        "icon-size", GTK_ICON_SIZE_NORMAL,
                                        NULL);

	gtk_button_set_child (GTK_BUTTON (self->handshake_button), self->image_handshake);
	gtk_widget_set_name (self->image_handshake, "header");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->handshake_button), FALSE);
	g_signal_connect (self->handshake_button, "clicked", G_CALLBACK (handshake_button_clicked_cb), self);


	self->menu_button = g_object_new (GTK_TYPE_MENU_BUTTON,
                                    "icon-name", "open-menu",
                                    NULL);

	self->menu = g_menu_new ();
	g_menu_append (self->menu, "REGISTER", "app.register");
	g_menu_append (self->menu, "LOGIN", "app.login");
	g_menu_append (self->menu, "QUIT", "app.quit");

	gtk_menu_button_set_menu_model ( GTK_MENU_BUTTON (self->menu_button), (GMenuModel *) self->menu);

	gtk_header_bar_pack_end (GTK_HEADER_BAR (self->header_bar), self->menu_button);

	gtk_header_bar_pack_start (GTK_HEADER_BAR (self->header_bar), self->left_pane_button);
	gtk_header_bar_pack_end (GTK_HEADER_BAR (self->header_bar), self->handshake_button);

#if 0
	for (int i = 0; i < 3; i++) {
		GtkWidget *item = user_item_new ();
		gtk_list_box_append (GTK_LIST_BOX (self->list_users), item);
	}

	gtk_list_box_unselect_all (GTK_LIST_BOX (self->list_users));
#endif
}

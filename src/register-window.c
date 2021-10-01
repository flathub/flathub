/* register-window.c
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

#include "register-window.h"
#include "main-window.h"
#include <json-glib/json-glib.h>


struct _RegisterWindow {
	GtkWindow parent_instance;

	GdkDisplay *display;
	GtkCssProvider *provider;
	GSocketClient *socket_client;

	GtkApplication *app;

	GtkWidget *main_window;

	GtkWidget *box;
	GtkWidget *frame;
	GtkWidget *box_frame;
	GtkWidget *entry_login;
	GtkWidget *entry_password;
	GtkWidget *entry_ip;
	GtkWidget *entry_port;
	GtkWidget *button_register;
	GSocketConnection *conn;
	GNotification *notification;

	char *type;
	char *title;
};

G_DEFINE_TYPE (RegisterWindow, register_window, GTK_TYPE_WINDOW)

typedef enum {
	PROP_SOCKET = 1,
	PROP_APP,
	PROP_MAIN_WINDOW,
	PROP_TYPE,
	PROP_TITLE,
	N_PROPERTIES
} RegisterWindowProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static const char *styles =
"window { background-color: #fcfcfc; }"
"frame#frame { background-color: #e1e1e1; margin-top: 64px; margin-left: 16px; margin-right: 16px; margin-bottom: 32px; }"
"entry#entry_start { margin-top: 16px; margin-left: 16px; margin-right: 16px; }"
"entry#entry_end { margin-bottom: 16px; margin-left: 16px; margin-right: 16px; margin-top: 16px; }"
"button#button_register { border-radius: 8px; }"
;

static gboolean accept_certificate (GTlsConnection *conn,
		GTlsCertificate *peer_cert,
		GTlsCertificateFlags flags,
		gpointer user_data)
{
  (void) user_data;
  (void) conn;
  (void) peer_cert;
  (void) flags;
	return TRUE;
}

static void button_register_clicked_cb (GtkButton *button, gpointer user_data) {
	RegisterWindow *self = REGISTER_WINDOW (user_data);

	gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);

	GtkEntryBuffer *buffer_login = gtk_entry_get_buffer (GTK_ENTRY (self->entry_login));
	GtkEntryBuffer *buffer_password = gtk_entry_get_buffer (GTK_ENTRY (self->entry_password));
	GtkEntryBuffer *buffer_ip = gtk_entry_get_buffer (GTK_ENTRY (self->entry_ip));
	GtkEntryBuffer *buffer_port = gtk_entry_get_buffer (GTK_ENTRY (self->entry_port));

	const char *login = gtk_entry_buffer_get_text (buffer_login);
	const char *password = gtk_entry_buffer_get_text (buffer_password);
	const char *ip = gtk_entry_buffer_get_text (buffer_ip);
	const unsigned short port = atoi (gtk_entry_buffer_get_text (buffer_port));

	if (g_utf8_strlen (login, 34L) < 4) {
		if (self->app) {
			g_notification_set_body (self->notification,
					"Auth error: login must be contains at least 4 symbols."
					);
			g_application_send_notification (G_APPLICATION (self->app),
					NULL,
					self->notification
					);
		}
		gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
		return;
	}
	if (g_utf8_strlen (login, 34L) >= 33) {
		if (self->app) {
			g_notification_set_body (self->notification,
					"Auth error: login must be contains max symbols."
					);
			g_application_send_notification (G_APPLICATION (self->app),
					NULL,
					self->notification
					);
		}
		gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
		return;
	}
	if (g_utf8_strlen (password, 34L) < 4) {
		if (self->app) {
			g_notification_set_body (self->notification,
					"Auth error: password must be contains at least 4 symbols."
					);
			g_application_send_notification (G_APPLICATION (self->app),
					NULL,
					self->notification
					);
		}
		gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
		return;
	}
	if (g_utf8_strlen (password, 34L) >= 33) {
		if (self->app) {
			g_notification_set_body (self->notification,
					"Auth error: password must be contains max symbols."
					);
			g_application_send_notification (G_APPLICATION (self->app),
					NULL,
					self->notification
					);
		}
		gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
		return;
	}

  g_autoptr(JsonBuilder) builder = json_builder_new ();

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "type");
  json_builder_add_string_value (builder, self->type);

  json_builder_set_member_name (builder, "login");
  json_builder_add_string_value (builder, login);

  json_builder_set_member_name (builder, "password");
  json_builder_add_string_value (builder, password);

  json_builder_end_object (builder);

  g_autoptr(JsonNode) root = json_builder_get_root (builder);
  g_autoptr(JsonGenerator) gen = json_generator_new ();

  json_generator_set_root (gen, root);
  gsize length_buffer;
  g_autofree char *buffer = json_generator_to_data (gen, &length_buffer);

	g_socket_client_set_tls (self->socket_client, TRUE);

	GError *error = NULL;

	g_socket_client_set_tls_validation_flags(self->socket_client,
			G_TLS_CERTIFICATE_VALIDATE_ALL &
			G_TLS_CERTIFICATE_BAD_IDENTITY &
			G_TLS_CERTIFICATE_UNKNOWN_CA
			);

	self->conn = g_socket_client_connect_to_host (
			self->socket_client,
			ip,
			port,
			NULL,
			&error
			);
	if (error) {
		g_print ("connect_to_host: %s\n", error->message);
		g_error_free (error);
		error = NULL;
		gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
		return;
	}

	GInputStream *igio = g_io_stream_get_input_stream (G_IO_STREAM (self->conn));
	GOutputStream *ogio = g_io_stream_get_output_stream (G_IO_STREAM (self->conn));

	g_output_stream_write (G_OUTPUT_STREAM (ogio),
			buffer,
			length_buffer,
			NULL,
			&error);
	if (error) {
		g_print ("register send: %s\n", error->message);
		g_error_free (error);
		error = NULL;
		gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
		return;
	}

	gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);


	g_autofree char *b = g_malloc (512);
	int re = g_input_stream_read (G_INPUT_STREAM (igio),
			b,
			512,
			NULL,
			&error);
	if (error) {
		g_print ("register answer: %s\n", error->message);
		g_error_free (error);
		error = NULL;
		gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
		return;
	}
	b[re] = 0;

  g_autoptr(JsonParser) parser = json_parser_new ();
  json_parser_load_from_data (parser, b, -1, &error);
  if (error) {
    g_print ("%s: %s\n", self->type, error->message);
    g_error_free (error);
    return;
  }
  JsonNode *root_answer = json_parser_get_root (parser);
  JsonReader *reader_answer = json_reader_new (root_answer);

  json_reader_read_member (reader_answer, "status");
  JsonNode *jstatus = json_reader_get_value (reader_answer);
  json_reader_end_member (reader_answer);


	g_autofree char *status = json_node_get_string (jstatus);


	if (!strncmp (status, "false", 6)) {
		if (self->app) {
			g_notification_set_body (self->notification,
					"Auth error: server has been returned error."
					);
			g_application_send_notification (G_APPLICATION (self->app),
					NULL,
					self->notification
					);
		}
		return;
	}
	if (!strncmp (status, "ok", 6)) {
		if (self->app) {
			g_notification_set_body (self->notification,
					"Auth ok. welcome."
					);
			g_application_send_notification (G_APPLICATION (self->app),
					"org.xverizex.nem-desktop",
					self->notification
					);
		}
		g_object_set (self->main_window,
				"conn", self->conn,
				"notification", self->notification,
				NULL);
		gtk_widget_hide (GTK_WIDGET (self));
		main_window_get_list_users (MAIN_WINDOW (self->main_window));
		main_window_feed (MAIN_WINDOW (self->main_window));
		return;
	}

}

static void register_window_init (RegisterWindow *self) {
	self->display = gdk_display_get_default ();
	self->provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_data (self->provider, styles, -1);
	gtk_style_context_add_provider_for_display (self->display, (GtkStyleProvider *) self->provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

	self->notification = g_notification_new ("SECURE CHAT");

	gtk_window_set_title (GTK_WINDOW (self), self->title);

	self->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	self->frame = gtk_frame_new (NULL);
	self->entry_login = gtk_entry_new ();
	self->entry_password = gtk_entry_new ();
	self->entry_ip = gtk_entry_new ();
	self->entry_port = gtk_entry_new ();
	self->box_frame = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	gtk_box_append (GTK_BOX (self->box), self->frame);

	gtk_window_set_child (GTK_WINDOW (self), self->box);

	gtk_frame_set_child (GTK_FRAME (self->frame), self->box_frame);

	gtk_box_append (GTK_BOX (self->box_frame), self->entry_login);
	gtk_box_append (GTK_BOX (self->box_frame), self->entry_password);
	gtk_box_append (GTK_BOX (self->box_frame), self->entry_ip);
	gtk_box_append (GTK_BOX (self->box_frame), self->entry_port);

	gtk_widget_set_name (self->frame, "frame");
	gtk_widget_set_name (self->entry_login, "entry_start");
	gtk_widget_set_name (self->entry_password, "entry_start");
	gtk_widget_set_name (self->entry_ip, "entry_start");
	gtk_widget_set_name (self->entry_port, "entry_end");

	gtk_entry_set_placeholder_text (GTK_ENTRY (self->entry_login), "input login");
	gtk_entry_set_placeholder_text (GTK_ENTRY (self->entry_password), "input password");
	gtk_entry_set_placeholder_text (GTK_ENTRY (self->entry_ip), "input ip");
	gtk_entry_set_placeholder_text (GTK_ENTRY (self->entry_port), "input port");

	self->button_register = gtk_button_new_with_label ("REGISTER");
	gtk_widget_set_name (self->button_register, "button_register");
	g_object_set (self->button_register,
			"valign", GTK_ALIGN_END,
			"halign", GTK_ALIGN_CENTER,
			NULL);

	gtk_box_append (GTK_BOX (self->box), self->button_register);

	g_signal_connect (self->button_register, "clicked", G_CALLBACK (button_register_clicked_cb), self);

	self->socket_client = g_socket_client_new ();
	g_socket_client_set_family (self->socket_client, G_SOCKET_FAMILY_IPV4);
	g_socket_client_set_protocol (self->socket_client, G_SOCKET_PROTOCOL_TCP);
}

static void register_window_set_property (GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec) {
	RegisterWindow *self = REGISTER_WINDOW (object);

	switch ((RegisterWindowProperty) property_id) {
		case PROP_APP:
			self->app = g_value_get_object (value);
			break;
		case PROP_MAIN_WINDOW:
			self->main_window = g_value_get_object (value);
			break;
		case PROP_TITLE:
			if (self->title) g_free (self->title);
			self->title = g_value_dup_string (value);
			gtk_window_set_title (GTK_WINDOW (self), self->title);
			break;
		case PROP_TYPE:
			if (self->type) g_free (self->type);
			self->type = g_value_dup_string (value);
			gtk_button_set_label (GTK_BUTTON (self->button_register), self->type);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void register_window_get_property (GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec) {
	RegisterWindow *self = REGISTER_WINDOW (object);

	switch ((RegisterWindowProperty) property_id) {
		case PROP_SOCKET:
			g_value_set_object (value, self->socket_client);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}


static void register_window_class_init (RegisterWindowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = register_window_set_property;
	object_class->get_property = register_window_get_property;

	obj_properties[PROP_SOCKET] = g_param_spec_object (
			"socket_client",
			"Socket client",
			"socket client of connection",
			G_TYPE_OBJECT,
			G_PARAM_READABLE
			);

	obj_properties[PROP_APP] = g_param_spec_object (
			"app",
			"App",
			"app",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);

	obj_properties[PROP_MAIN_WINDOW] = g_param_spec_object (
			"main_window",
			"Main window",
			"main_window",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);

	obj_properties[PROP_TITLE] = g_param_spec_string (
			"title",
			"Title",
			"title",
			"default",
			G_PARAM_WRITABLE
			);

	obj_properties[PROP_TYPE] = g_param_spec_string (
			"type",
			"Type",
			"type",
			"default",
			G_PARAM_WRITABLE
			);

	g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}

/* user-item.c
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

#include "user-item.h"
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <json-glib/json-glib.h>
#include "message-item.h"
#include "main-window.h"

extern char *root_app;

struct _UserItem {
	GtkFrame parent_instance;

	char *icon;
	char *name;
	int status;
	int blink;
	int blink_handshake;

	GdkDisplay *display;
	GtkCssProvider *provider;

	GtkWidget *box;
	GtkWidget *iconw;
	GtkWidget *namew;
	GtkWidget *statusw;
	GtkWidget *handbutton;
	GtkWidget *frame_chat;
	GtkWidget *box_frame_chat;
	GtkWidget *scroll;
	GtkWidget *box_scroll;
	GtkWidget *entry_input_text;
	GOutputStream *ogio;
	GNotification *notification;
	GtkApplication *app;
	gboolean handshaked;
	GtkWidget *main_window;
};

G_DEFINE_TYPE (UserItem, user_item, GTK_TYPE_FRAME)

typedef enum {
	PROP_ICON = 1,
	PROP_NAME,
	PROP_STATUS,
	PROP_BLINK,
	PROP_BLINK_HANDSHAKE,
	PROP_HANDSHAKED,
	PROP_HANDBUTTON,
	PROP_FRAME_CHAT,
	PROP_OGIO,
	PROP_NOTIFICATION,
	PROP_APP,
	PROP_MAIN_WINDOW,
	N_PROPERTIES
} UserItemProperty;

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static const char *styles =
"@keyframes blinks { "
"	from { "
"		background-color: #b0bfd1; "
"	}"
"	50% {"
"		background-color: #11bfc2; "
"	}"
"	to {"
"		background-color: #b0bfd1; "
"	}"
"}"

"@keyframes blinks_handshake { "
"	from { "
"		background-color: #b0bfd1; "
"	}"
"	50% {"
"		background-color: #01944d; "
"	}"
"	to {"
"		background-color: #b0bfd1; "
"	}"
"}"
"frame#noblink { background-color: #b0bfd1; }"
"frame#blink { background-color: #b0bfd1; animation-name: blinks; "
"		animation-play-state: running; "
"		animation-duration: 1s;"
"		animation-iteration-count: infinite;"
"		animation-timing-function: linear; }"

"frame#noblink_handshake { background-color: #b0bfd1; }"
"frame#blink_handshake { background-color: #b0bfd1; animation-name: blinks_handshake; "
"		animation-play-state: running; "
"		animation-duration: 1s;"
"		animation-iteration-count: infinite;"
"		animation-timing-function: linear; }"
""
"label#item { color: #000000; font-size: 21px; }"
"image#item { min-width: 48px; min-height: 48px; margin: 8px; }"
"frame#msg { background-color: #b0bfd1; margin-top: 8px; margin-bottom: 8px; margin-left: 16px; margin-right: 16px; }"
;

void user_item_set_chat (UserItem *self) {
	GtkWidget *child = gtk_frame_get_child (GTK_FRAME (self->frame_chat));
	if (child) g_object_ref (child);
	gtk_frame_set_child (GTK_FRAME (self->frame_chat), self->box_frame_chat);
}

void user_item_add_message (UserItem *self, const char *msg, int me, const char *from, const int show_notification) {
	GtkWidget *frame = gtk_frame_new (NULL);
	GtkWidget *text_view = g_object_new (MESSAGE_TYPE_ITEM, NULL);

	g_object_set (frame,
			"width-request", 400,
			"halign", me ? GTK_ALIGN_START: GTK_ALIGN_END,
			NULL);
	g_object_set (text_view,
			"text", msg,
			"max_width", 400,
			NULL);

	gtk_widget_set_name (frame, "msg");
	gtk_frame_set_child (GTK_FRAME (frame), text_view);

	gtk_box_append ( GTK_BOX (self->box_scroll), frame);

	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->scroll));
	gtk_adjustment_set_value ( vadj, gtk_adjustment_get_upper ( vadj ) );

	if (me == 0 && show_notification) {
		g_notification_set_title (G_NOTIFICATION (self->notification),
				from
				);
		g_notification_set_body (G_NOTIFICATION (self->notification),
				msg
				);
		g_application_send_notification (G_APPLICATION (self->app),
				NULL,
				G_NOTIFICATION (self->notification)
				);
	}
}

static void send_message_to ( const char *name, char *path, const char *buffer, UserItem *self) {
	unsigned char *to = calloc (1024 * 1024 * 30, 1);
	if (!to) return;

	FILE *fp = fopen (path, "rb");
	if (!fp) {
		free (to);
		return;
	}

	int padding = RSA_PKCS1_PADDING;

	int buffer_len = strlen (buffer);
	RSA *rsa = PEM_read_RSA_PUBKEY (fp, NULL, NULL, NULL);

	int encrypted_length = RSA_public_encrypt (buffer_len, (const unsigned char *) buffer, to, rsa, padding);
	char *buf = calloc (encrypted_length * 2 + 1, 1);
	if (!buf) {
		fclose (fp);
		free (to);
		return;
	}
	int n = 0;
	char *s = buf;
	for (int i = 0; i < encrypted_length; i++) {
		sprintf (s, "%02x%n", to[i], &n);
		s += n;
	}

  JsonBuilder *builder = json_builder_new ();
  json_builder_begin_object (builder);
  json_builder_set_member_name (builder, "type");
  json_builder_add_string_value (builder, "message");
  json_builder_set_member_name (builder, "to");
  json_builder_add_string_value (builder, name);
  json_builder_set_member_name (builder, "data");
  json_builder_add_string_value (builder, buf);
  json_builder_end_object (builder);

  JsonNode *node = json_builder_get_root (builder);
  JsonGenerator *gen = json_generator_new ();
  json_generator_set_root (gen, node);
  gsize length = 0;
  char *data = json_generator_to_data (gen, &length);

	g_output_stream_write (G_OUTPUT_STREAM (self->ogio),
			data,
			length,
			NULL,
			NULL
			);

  g_object_unref (builder);
  g_object_unref (gen);
  free (data);
	RSA_free (rsa);
	free (to);
	free (buf);
}

static void entry_input_text_cb (GtkEntry *entry, gpointer user_data) {
	UserItem *self = USER_ITEM (user_data);

	GtkEntryBuffer *buffer = gtk_entry_get_buffer (entry);
	const char *text = gtk_entry_buffer_get_text (buffer);
	if (strlen (text) == 0) return;

	const char *name = user_item_get_name (USER_ITEM (self));

	char path[256];
	snprintf (path, 256, "%s/%s/crypto.pem", root_app, name);

	if (access (path, F_OK)) {
		g_notification_set_body (G_NOTIFICATION (self->notification),
				"Not found public key from user."
				);
		g_application_send_notification (G_APPLICATION (self->app),
				NULL,
				G_NOTIFICATION (self->notification)
				);
		return;
	}

	send_message_to (name, path, text, self);
	user_item_add_message (self, text, 1, NULL, 0);
	gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER (buffer), "", -1);

	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (self->scroll));
	gtk_adjustment_set_value ( vadj, gtk_adjustment_get_upper ( vadj ) );

//	main_window_play_new_message (MAIN_WINDOW (self->main_window));
#if 0
	if (scroll_event == 0) {
		gtk_adjustment_set_value (vadj, gtk_adjustment_get_upper (vadj));
	} else {
		gtk_adjustment_set_value (vadj, gtk_adjustment_get_value (vadj));
	}
#endif
}

static void user_item_init (UserItem *self) {
	self->display = gdk_display_get_default ();
	self->provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_data (self->provider, styles, -1);
	gtk_style_context_add_provider_for_display (self->display, (GtkStyleProvider *) self->provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_widget_set_name (GTK_WIDGET (self), "noblink");

	self->box_frame_chat = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	self->box_scroll = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	self->entry_input_text = gtk_entry_new ();
	g_signal_connect (self->entry_input_text, "activate", G_CALLBACK (entry_input_text_cb), self);
	self->scroll = gtk_scrolled_window_new ();
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (self->scroll), self->box_scroll);
	gtk_box_append (GTK_BOX (self->box_frame_chat), self->scroll);
	gtk_box_append (GTK_BOX (self->box_frame_chat), self->entry_input_text);
	g_object_set (self->box_scroll,
			"vexpand", TRUE,
			"hexpand", TRUE,
			NULL);
	g_object_set (self->scroll,
			"vexpand", TRUE,
			"hexpand", TRUE,
			NULL);
	g_object_set (self->entry_input_text,
			"hexpand", TRUE,
			NULL);

	self->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	self->namew = gtk_label_new ("aaaaaaaaaaaaaaaa");
	gtk_widget_set_name (self->namew, "item");
	g_object_set (self->namew,
			"overflow", TRUE,
			NULL);

	gtk_frame_set_child (GTK_FRAME (self), self->box);

	self->iconw = gtk_image_new_from_resource ("/org/xverizex/nem-desktop/human_default.svg");
	gtk_widget_set_name (self->iconw, "item");

	self->statusw = gtk_image_new_from_resource ("/org/xverizex/nem-desktop/status_offline.svg");
	g_object_set (self->statusw, "halign", GTK_ALIGN_END, "hexpand", TRUE, NULL);
	gtk_widget_set_name (self->statusw, "item");


	gtk_box_append (GTK_BOX (self->box), self->iconw);

	gtk_box_append (GTK_BOX (self->box), self->namew);

	gtk_box_append (GTK_BOX (self->box), self->statusw);


	g_object_set (self, "hexpand", TRUE, NULL);
	g_object_set (self, "height-request", 64, NULL);
}

static void user_item_set_property (GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec) {
	UserItem *self = USER_ITEM (object);

	switch ((UserItemProperty) property_id) {
		case PROP_ICON:
			self->icon = g_value_dup_string (value);
			break;
		case PROP_NAME:
			self->name = g_value_dup_string (value);
			gtk_label_set_label (GTK_LABEL (self->namew), self->name);
			break;
		case PROP_STATUS:
			self->status = g_value_get_int (value);
			if (self->status) {
				if (self->statusw) {
					gtk_box_remove (GTK_BOX (self->box), self->statusw);
					self->statusw = gtk_image_new_from_resource ("/org/xverizex/nem-desktop/status_online.svg");
					g_object_set (self->statusw, "halign", GTK_ALIGN_END, "hexpand", TRUE, NULL);
					gtk_widget_set_name (self->statusw, "item");
				}
				gtk_box_append (GTK_BOX (self->box), self->statusw);
			} else {
				if (self->statusw) {
					gtk_box_remove (GTK_BOX (self->box), self->statusw);
					self->statusw = gtk_image_new_from_resource ("/org/xverizex/nem-desktop/status_offline.svg");
					g_object_set (self->statusw, "halign", GTK_ALIGN_END, "hexpand", TRUE, NULL);
					gtk_widget_set_name (self->statusw, "item");
				}
				gtk_box_append (GTK_BOX (self->box), self->statusw);
			}
			break;
		case PROP_BLINK:
			self->blink = g_value_get_int (value);
			if (self->blink) gtk_widget_set_name (GTK_WIDGET (self), "blink");
			else gtk_widget_set_name (GTK_WIDGET (self), "noblink");
			break;
		case PROP_BLINK_HANDSHAKE:
			self->blink_handshake = g_value_get_int (value);
			if (self->blink_handshake) gtk_widget_set_name (GTK_WIDGET (self), "blink_handshake");
			else gtk_widget_set_name (GTK_WIDGET (self), "noblink_handshake");
			break;
		case PROP_HANDSHAKED:
			self->handshaked = g_value_get_boolean (value);
			break;
		case PROP_HANDBUTTON:
			self->handbutton = g_value_get_object (value);
			break;
		case PROP_FRAME_CHAT:
			self->frame_chat = g_value_get_object (value);
			break;
		case PROP_OGIO:
			self->ogio = g_value_get_object (value);
			break;
		case PROP_NOTIFICATION:
			self->notification = g_value_get_object (value);
			break;
		case PROP_APP:
			self->app = g_value_get_object (value);
			break;
		case PROP_MAIN_WINDOW:
			self->main_window = g_value_get_object (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void user_item_get_property (GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec) {
	UserItem *self = USER_ITEM (object);

	switch ((UserItemProperty) property_id) {
		case PROP_ICON:
			g_value_set_string (value, self->icon);
			break;
		case PROP_NAME:
			g_value_set_string (value, self->name);
			break;
		case PROP_BLINK_HANDSHAKE:
			g_value_set_int (value, self->blink_handshake);
			break;
		case PROP_STATUS:
			g_value_set_int (value, self->status);
			break;
		case PROP_BLINK:
			g_value_set_int (value, self->blink);
			break;
		case PROP_HANDSHAKED:
			g_value_set_boolean (value, self->handshaked);
			break;
		case PROP_FRAME_CHAT:
			g_value_set_object (value, self->frame_chat);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}


static void user_item_class_init (UserItemClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = user_item_set_property;
	object_class->get_property = user_item_get_property;

	obj_properties[PROP_ICON] = g_param_spec_string (
			"icon",
			"Icon",
			"icon data in a json format",
			NULL,
			G_PARAM_READWRITE
			);

	obj_properties[PROP_NAME] = g_param_spec_string (
			"name",
			"Name",
			"name of user",
			NULL,
			G_PARAM_READWRITE
			);

	obj_properties[PROP_STATUS] = g_param_spec_int (
			"status",
			"Status",
			"status online",
			0,
			1,
			0,
			G_PARAM_READWRITE
			);

	obj_properties[PROP_BLINK] = g_param_spec_int (
			"blink",
			"Blink",
			"blink item",
			0,
			1,
			0,
			G_PARAM_READWRITE
			);

	obj_properties[PROP_BLINK_HANDSHAKE] = g_param_spec_int (
			"blink_handshake",
			"Blink",
			"blink item",
			0,
			1,
			0,
			G_PARAM_READWRITE
			);

	obj_properties[PROP_HANDSHAKED] = g_param_spec_boolean (
			"handshaking",
			"Handshaking",
			"handshaking",
			FALSE,
			G_PARAM_READWRITE
			);

	obj_properties[PROP_HANDBUTTON] = g_param_spec_object (
			"handbutton",
			"Handbutton",
			"handbutton",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);

	obj_properties[PROP_FRAME_CHAT] = g_param_spec_object (
			"frame_chat",
			"Frame chat",
			"frame_chat",
			G_TYPE_OBJECT,
			G_PARAM_READWRITE
			);

	obj_properties[PROP_OGIO] = g_param_spec_object (
			"ogio",
			"ogio",
			"ogio",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);
	obj_properties[PROP_NOTIFICATION] = g_param_spec_object (
			"notification",
			"notification",
			"notification",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);
	obj_properties[PROP_APP] = g_param_spec_object (
			"app",
			"app",
			"app",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);
	obj_properties[PROP_MAIN_WINDOW] = g_param_spec_object (
			"main_window",
			"main window",
			"main window",
			G_TYPE_OBJECT,
			G_PARAM_WRITABLE
			);

	g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}


void user_item_set_icon (UserItem *item, const char *data) {
	g_object_set (item, "icon", data, NULL);
}

void user_item_set_name (UserItem *item, const char *name) {
	g_object_set (item, "name", name, NULL);
}

void user_item_set_status (UserItem *item, const int status) {
	g_object_set (item, "status", status, NULL);
}

void user_item_set_blink (UserItem *item, const int blink) {
	g_object_set (item, "blink", blink, NULL);
}

const char *user_item_get_icon (UserItem *item) {
	const char *icon;
	g_object_get (item, "icon", &icon, NULL);
	return icon;
}

const char *user_item_get_name (UserItem *item) {
	const char *name;
	g_object_get (item, "name", &name, NULL);
	return name;
}

int user_item_get_status (UserItem *item) {
	int status;
	g_object_get (item, "status", &status, NULL);
	return status;
}

int user_item_get_blink (UserItem *item) {
	int blink;
	g_object_get (item, "blink", &blink, NULL);
	return blink;
}

GtkWidget *user_item_new () {
	return g_object_new (USER_TYPE_ITEM, NULL);
}

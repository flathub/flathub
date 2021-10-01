/* message-item.c
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

#include "message-item.h"

struct _MessageItem {
	GtkDrawingArea parent_instance;

	char *text;
	int max_width;
};

G_DEFINE_TYPE (MessageItem, message_item, GTK_TYPE_DRAWING_AREA)

typedef enum {
	PROP_TEXT = 1,
	PROP_MAX_WIDTH,
	N_PROPERTIES
} MessageItemProperties;

#define OFFSET            20

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void draw_function (GtkDrawingArea *area,
		cairo_t *cr,
		int width,
		int height,
		gpointer data)
{
	MessageItem *self = MESSAGE_ITEM (area);

  GDateTime *datet = g_date_time_new_now_local ();
  int hour = g_date_time_get_hour (datet);
  int minute = g_date_time_get_minute (datet);
  int second = g_date_time_get_second (datet);
  g_date_time_unref (datet);
  char buf_time[255];
  snprintf (buf_time, 255, "%02d:%02d:%02d", hour, minute, second);

	int total_w = self->max_width;
	int total_h = 0;
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_set_font_size (cr, 14);
	cairo_text_extents_t sz;
	cairo_text_extents (cr, buf_time, &sz);
	total_h = sz.height + OFFSET;
  int x = self->max_width / 2 - sz.width / 2 - OFFSET;
  cairo_save (cr);
  cairo_set_source_rgb (cr, 0.4, 0.4, 0.4);
  cairo_move_to (cr, x - 10, 0);
  cairo_line_to (cr, x, sz.height + 8);
  cairo_line_to (cr, x + sz.width, sz.height + 8);
  cairo_line_to (cr, x + sz.width + 10, 0);
  cairo_line_to (cr, x - 10, 0);
  cairo_move_to (cr, x, total_h);
  cairo_fill (cr);
  cairo_restore (cr);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_move_to (cr, x, sz.height + 4);
	cairo_show_text (cr, buf_time);

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_set_font_size (cr, 18);
 	cairo_text_extents (cr, self->text, &sz);


	if ((sz.width + OFFSET * 2) > self->max_width) {
		int index = 0;
		int len = 0;
		const char *loc = NULL;
		char *s = strdup (self->text);
		char *st = s;
		int llen = strlen (self->text);
		int total_index = 0;
		int te = 0;
		while (1) {
			if (total_index >= llen) break;
			s[index] = self->text[te];
			index++;
			te++;
			total_index++;
			s[index] = 0;
			gboolean valid = g_utf8_validate (s, -1, &loc);
			if (valid) {
				cairo_text_extents (cr, s, &sz);
				if ((sz.width + OFFSET * 2) < self->max_width) {
					continue;
				}
				index--;
				te--;
				s[index] = 0;
				while (!g_utf8_validate (s, -1, &loc)) {
					index--;
					te--;
					total_index--;
					s[index] = 0;
				}
				cairo_move_to (cr, OFFSET, total_h);
				total_h += sz.height + 10;
				cairo_show_text (cr, s);
				s += index;
				index = 0;
			}
		}
		cairo_move_to (cr, OFFSET, total_h);
		total_h += OFFSET;
		gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (self), total_w);
		gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (self), total_h);
		cairo_show_text (cr, s);
		free (st);
		return;
	}

	gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (self), sz.width + OFFSET * 2);
	gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (self), total_h + sz.height + OFFSET * 2);
	cairo_move_to (cr, OFFSET, total_h + sz.height + OFFSET);
	cairo_show_text (cr, self->text);
}

static void assembly_width_and_height (MessageItem *self) {
}

static void message_item_set_property (GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec) {
	MessageItem *self = MESSAGE_ITEM (object);

	switch ((MessageItemProperties) property_id) {
		case PROP_TEXT:
			if (self->text) g_free (self->text);
			self->text = g_value_dup_string (value);
			assembly_width_and_height (self);
			break;
		case PROP_MAX_WIDTH:
			self->max_width = g_value_get_int (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void message_item_class_init (MessageItemClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = message_item_set_property;

	obj_properties[PROP_TEXT] = g_param_spec_string (
			"text",
			"text",
			"text",
			NULL,
			G_PARAM_WRITABLE
			);
	obj_properties[PROP_MAX_WIDTH] = g_param_spec_int (
			"max_width",
			"max width",
			"set max width for frame",
			100,
			1080,
			400,
			G_PARAM_WRITABLE
			);

	g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}

static void message_item_init (MessageItem *self) {
	gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (self), 10);
	gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (self), 10);
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (self), draw_function, NULL, NULL);
}

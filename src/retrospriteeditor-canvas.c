/* retrospriteeditor-canvas.c
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

#include "retrospriteeditor-canvas.h"
#include "retrospriteeditor-colours.h"
#include "custom_math.h"
#include "nes_params.h"
#include "retrospriteeditor-nes-palette.h"
#include "retrospriteeditor-nes-list-palletes.h"
#include "retrospriteeditor-nes-current-pallete.h"
#include "tools.h"
#include "type-pallete.h"

struct _RetrospriteeditorCanvas
{
  GtkDrawingArea  parent_instance;

  guint32 width;
  guint32 height;
  guint32 orig_width;
  guint32 orig_height;
  guint32 palette_type;
  guint32 width_rect;
  guint32 height_rect;
  gint32 scale;
  guint32 type_canvas;
  guint32 count_x;
  guint32 count_y;
  guint32 *colours;
  guint32 *index_color;
  guint32 count_colours;
  gboolean left_top;
  gint32 px;
  gint32 py;
  gint32 w;
  gint32 h;
  gint32 xw;
  gint32 yh;
  guint32 selected_index_color;

  gboolean is_2_btn_pressed;
  gboolean is_0_btn_pressed;
  gint32 mx;
  gint32 my;
  gint32 ox;
  gint32 oy;
  guint32 last_pointx;
  guint32 last_pointy;
  guint32 last_blockx;
  guint32 last_blocky;
  guint32 colour_block_size;
  gboolean show_hex_index;


  guint32 tool;
  gboolean drawing_tool;
	guint32 index_id;

  GtkEventController *event_zoom;
  GtkEventController *event_motion;
  GtkGesture         *gesture_click_move_canvas;
  GtkGesture         *gesture_click_tool_press;
  GtkGesture         *gesture_click_select_index_color;
  GtkGesture         *gesture_click_select_one_color;
	RetrospriteeditorCanvas  *child_one_color;
};


G_DEFINE_FINAL_TYPE (RetrospriteeditorCanvas, retrospriteeditor_canvas, GTK_TYPE_DRAWING_AREA)

enum {
  PROP_SETTINGS = 1,
  N_PROPERTIES
};

static void
colour_rgb_get_double_color (guint32 color,
                             gdouble *r,
                             gdouble *g,
                             gdouble *b)
{
  guint8 red =   (color >>  0) & 0xff;
  guint8 green = (color >>  8) & 0xff;
  guint8 blue =  (color >> 16) & 0xff;

  *r = red / 255.0;
  *g = green / 255.0;
  *b = blue / 255.0;
}

static void
draw_grid (cairo_t                 *cr,
           int                      width,
           int                      height,
           RetrospriteeditorCanvas *self)
{


  double line_width = 1.0;

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_set_line_width (cr, line_width);

  int x = self->px + 1;
  int y = self->py + 1;

  if (self->left_top) {
    x = y = 0;
  }

  guint32 count_rect_w = self->orig_width / self->width_rect;
  guint32 count_rect_h = self->orig_height / self->height_rect;

  guint32 rect_w_result_size = c_pow (self->width_rect, self->scale);
  guint32 rect_h_result_size = c_pow (self->height_rect, self->scale);


  int xx = 0;
  int yy = 0;


  for (int cyy = 0; cyy < count_rect_h; cyy++) {
    for (int cxx = 0; cxx < count_rect_w; cxx++) {
      cairo_rectangle (cr, x + xx, y + yy,
                       rect_w_result_size,
                       rect_h_result_size);
      xx += rect_w_result_size;
      xx++;
    }
    yy += rect_h_result_size;
    yy++;
    xx = 0;
  }

  cairo_stroke (cr);
}

static void
draw_tool (cairo_t                 *cr,
                int                      width,
                int                      height,
                RetrospriteeditorCanvas *self)
{
  if (!self->drawing_tool)
    return;

  int posx = self->mx - self->px;
  int posy = self->my - self->py;

  guint32 y = 0;
  guint32 x = 0;

  int found = 0;

  guint32 count_rect_w = self->orig_width / self->width_rect;
  guint32 count_rect_h = self->orig_height / self->height_rect;

  guint32 rect_w_result_size = c_pow (self->width_rect, self->scale);
  guint32 rect_h_result_size = c_pow (self->height_rect, self->scale);

  int xx = 1;
  int yy = 1;
  int cyy = 0;
  int cxx = 0;
  int pointx = 0;
  int pointy = 0;
  for (cyy = 0; cyy < count_rect_h; cyy++) {
    for (cxx = 0; cxx < count_rect_w; cxx++) {
      int ex = xx + rect_w_result_size;
      int ey = yy + rect_h_result_size;

      if ((posx >= xx) && (posx <= ex)) {
        if ((posy >= yy) && (posy <= ey)) {
          found = 1;
          break;
        }
      }
      xx += rect_w_result_size;
      xx++;
    }
    if (found)
      break;
    yy += rect_h_result_size;
    yy++;
    xx = 1;
  }

  if (found == 0)
    return;

  found = 0;

  for (y = yy; y < (yy + rect_h_result_size);) {
    for (x = xx; x < (xx + rect_w_result_size);) {
      int ex = x + c_pow (1, self->scale);
      int ey = y + c_pow (1, self->scale);
      if ((posx >= x) && (posx <= ex)) {
        if ((posy >= y) && (posy <= ey)) {
          found = 1;
          break;
        }
      }

      x += c_pow (1, self->scale);
      pointx++;
    }
    if (found)
      break;
    y += c_pow (1, self->scale);
    pointy++;
    pointx = 0;
  }

  if (found == 0)
    return;

  self->last_pointx = pointx;
  self->last_pointy = pointy;
  self->last_blockx = cxx;
  self->last_blocky = cyy;
  int psize = 1;

  if (self->is_0_btn_pressed && (self->tool == TOOL_PENCIL)) {
    NesParamPoint n;
    n.blockx = self->last_blockx;
    n.blocky = self->last_blocky;
    n.x = self->last_pointx;
    n.y = self->last_pointy;
    RetrospriteeditorNesPalette *nes = get_nes ();
    nes_set_color (nes, &n, self->selected_index_color + 1);
  }

  double r, g, b;
  colour_rgb_get_double_color (self->colours[self->selected_index_color], &r, &g, &b);
  cairo_set_source_rgb (cr, r, g, b);

  switch (self->tool)
    {
    case TOOL_PENCIL:
      psize = c_pow (1, self->scale);
      cairo_rectangle (cr, self->px + x, self->py + y, psize, psize);
      cairo_fill (cr);
      break;
    }
}

void
canvas_set_index (RetrospriteeditorCanvas *self,
		guint32 index)
{
	self->selected_index_color = index;
}
static void
draw_rectangle (cairo_t                 *cr,
                int                      width,
                int                      height,
                RetrospriteeditorCanvas *self)
{


  int x = self->px;
  int y = self->py;

  if (self->left_top) {
    x = y = 0;
  }

  cairo_set_source_rgb (cr, 0x4c / 255.0, 0x4c / 255.0, 0x4c / 255.0);
  cairo_paint (cr);

  guint32 count_rect_w = self->orig_width / self->width_rect;
  guint32 count_rect_h = self->orig_height / self->height_rect;
  guint32 rect_w_result_size = c_pow (self->width_rect, self->scale);
  guint32 rect_h_result_size = c_pow (self->height_rect, self->scale);

  /*
   * Calculate full size of rectangle with white lines as grid.
   */
  guint32 w = count_rect_w * rect_w_result_size + count_rect_w + 1;
  guint32 h = count_rect_h * rect_h_result_size + count_rect_h + 1;

  self->w = w;
  self->h = h;
  self->xw = self->px + w;
  self->yh = self->py + h;

  cairo_rectangle (cr, x, y, w, h);

  double r, g, b;
  colour_rgb_get_double_color (self->colours[0], &r, &g, &b);
  cairo_set_source_rgb (cr, r, g, b);
  cairo_fill (cr);
}

void
canvas_set_index_colours (RetrospriteeditorCanvas *self,
                          guint32                 *index_color)
{
  self->index_color = index_color;
}

void
canvas_set_colours (RetrospriteeditorCanvas *self,
                    guint32                 *colours,
                    guint32                  count)
{
  self->colours = colours;
  self->count_colours = count;
}

guint32 *
canvas_pallete_get_ptr_index_colours (RetrospriteeditorCanvas *self,
		guint32 x,
		guint32 y
		)
{
	return NULL;
}

void
canvas_set_type_palette (RetrospriteeditorCanvas *self,
                                                guint32                  type,
                                                guint32                  count)
{
  self->colours = global_type_pallete_get_cur_ptr_pallete (0);
  self->count_colours = count;
}

static void
draw_pixels (cairo_t                 *cr,
                    int                      width,
                    int                      height,
                    RetrospriteeditorCanvas *self)
{
  int xx = self->px + 1;
  int yy = self->py + 1;

  int nx = 0;
  int ny = 0;
  RetrospriteeditorNesPalette *nes = get_nes ();
  for (guint32 y = 0; y < self->orig_height; y++) {
    for (guint32 x = 0; x < self->orig_width; x++) {

      double r, g, b;
      NesTilePoint *p = nes_get_color (nes, x, y);
      if (p->index > 0) {
        colour_rgb_get_double_color (self->colours[p->index - 1], &r, &g, &b);
        cairo_set_source_rgb (cr, r, g, b);
        int psize = c_pow (1, self->scale);
        cairo_rectangle (cr, xx, yy, psize, psize);
        cairo_fill (cr);
      }
      nx++;
      xx += c_pow (1, self->scale);
      if (nx == 8) {
        xx++;
        nx = 0;
      }
    }
    xx = self->px + 1;
    nx = 0;
    ny++;
    yy += c_pow (1, self->scale);
    if (ny == 8) {
      yy++;
      ny = 0;
    }
  }
}

static void
draw_colour_blocks (cairo_t                 *cr,
                    int                      width,
                    int                      height,
                    RetrospriteeditorCanvas *self)
{
  guint32 indx = 0;

  cairo_set_font_size (cr, 16.0);

  guint32 offsetx = 0;
  guint32 offsety = 0;
  for (guint32 y = 0; y < self->count_y; y++) {
    for (guint32 x = 0; x < self->count_x; x++) {

      double r, g, b;
      if (self->colours) {
        colour_rgb_get_double_color (self->colours[indx], &r, &g, &b);
        cairo_set_source_rgb (cr, r, g, b);

        cairo_rectangle (cr,
                      x * self->colour_block_size * self->scale,
                      y * self->colour_block_size * self->scale,
                      self->colour_block_size * self->scale,
                      self->colour_block_size * self->scale);

        cairo_fill (cr);

        if (self->show_hex_index) {
	  offsetx = x * self->colour_block_size * self->scale;
	  offsety = y * self->colour_block_size * self->scale;
          int px = offsetx + self->colour_block_size / 2 - 10;
          int py = y + self->colour_block_size - 4;
          cairo_move_to (cr, px, py);
          char hex_index[32] = "\0";
	  if (self->colours[indx] == 0x0) {
		  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	  } else {
		  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	  }

          snprintf (hex_index, 32, "%X", self->index_color[indx]);
          cairo_show_text (cr, hex_index);
        }

	cairo_move_to (cr, x, y);
      }

      indx++;
    }
  }
}

void
canvas_set_show_hex (RetrospriteeditorCanvas *self,
                     gboolean                 val)
{
  self->show_hex_index = val;
}

static void
draw_canvas (GtkDrawingArea *area,
             cairo_t        *cr,
             int             width,
             int             height,
             gpointer        data)
{
  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (area);



  switch (self->type_canvas)
    {
    case TYPE_CANVAS_TILESET:
      draw_rectangle (cr, width, height, self);
      draw_grid (cr, width, height, self);
      draw_pixels (cr, width, height, self);
      draw_tool (cr, width, height, self);
      break;
    case TYPE_CANVAS_COLOUR_GRID:
    case TYPE_CANVAS_COLOUR_PALLETE:
      draw_colour_blocks (cr, width, height, self);
      break;
    }

}

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void
canvas_set_settings (RetrospriteeditorCanvas *self,
                     CanvasSettings          *stgs)
{
  self->type_canvas = stgs->type_canvas >= 0? stgs->type_canvas: self->type_canvas;
  self->width = self->orig_width = stgs->canvas_width >= 0? stgs->canvas_width: self->width;
  self->height = self->orig_height = stgs->canvas_height >= 0? stgs->canvas_height: self->height;
  self->palette_type = stgs->palette_type >= 0? stgs->palette_type: self->palette_type;
  self->width_rect = stgs->width_rect >= 0? stgs->width_rect: self->width_rect;
  self->height_rect = stgs->height_rect >= 0? stgs->height_rect: self->height_rect;
  self->scale = stgs->scale >= 0? stgs->scale: self->scale;
  self->count_x = stgs->count_x >= 0? stgs->count_x: self->count_x;
  self->count_y = stgs->count_y >= 0? stgs->count_y:self->count_y;
  self->left_top = stgs->left_top;
  self->px = 0;
  self->py = 0;


  switch (self->type_canvas)
    {
    case TYPE_CANVAS_COLOUR_GRID:
      self->colour_block_size = 16;
      break;
    case TYPE_CANVAS_COLOUR_PALLETE:
      self->colour_block_size = 32;
      break;
    default:
      break;
    }

}

static void
canvas_set_property (GObject      *object,
                     guint         property_id,
                     const GValue *value,
                     GParamSpec   *spec)
{
  RetrospriteeditorCanvas *cnvs = RETROSPRITEEDITOR_CANVAS (object);

  switch (property_id)
    {
    case PROP_SETTINGS:
      canvas_set_settings (cnvs, g_value_get_pointer (value));
      guint32 width =  cnvs->width + cnvs->width_rect == 1? 0: cnvs->width_rect;
      guint32 height = cnvs->height + cnvs->height_rect == 1? 0: cnvs->height_rect;
      gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (cnvs), width);
      gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (cnvs), height);
      gtk_widget_queue_draw (GTK_WIDGET (cnvs));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, spec);
      break;
    }
}

static void
canvas_get_property (GObject    *object,
                     guint       property_id,
                     GValue     *value,
                     GParamSpec *spec)
{
  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (object);

  switch (property_id)
    {
    case PROP_SETTINGS:
      CanvasSettings *settings = (CanvasSettings *)
        g_value_get_pointer (value);
      settings->canvas_width = self->width;
      settings->canvas_height = self->height;
      settings->palette_type = self->palette_type;
      settings->scale = self->scale;
      break;
    }
}

static void
retrospriteeditor_canvas_class_init (RetrospriteeditorCanvasClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = canvas_set_property;
  object_class->get_property = canvas_get_property;

  obj_properties[PROP_SETTINGS] =
      g_param_spec_pointer ("settings",
                            "Settings",
                            "Set settings for canvas",
                            G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}


static gboolean
zoom_changed (
  GtkEventControllerScroll* evt,
  gdouble dx,
  gdouble dy,
  gpointer user_data
)
{

  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (user_data);


  int posx = (self->mx - self->px);
  int posy = (self->my - self->py);

  if (posx < 0 || posy < 0)
    return TRUE;

  if (posx > (self->xw - self->px) ||
      posy > (self->yh - self->py))
    return TRUE;


  if (dy < 0.0) {
    self->scale++;
    if (self->scale > 5) {
      self->scale = 5;
      goto end;
    }

    int rect_cx = posx / c_pow (self->width_rect, (self->scale - 1));
    int rect_cy = posy / c_pow (self->height_rect, (self->scale - 1));

    posx += posx - rect_cx - 1;
    posy += posy - rect_cy - 1;

    self->px = self->mx - posx;
    self->py = self->my - posy;

  } else {
    self->scale--;
    if (self->scale < 1) {
      self->scale = 1;
      goto end;
    }

    int rect_cx = posx / c_pow (self->width_rect, (self->scale + 1));
    int rect_cy = posy / c_pow (self->height_rect, (self->scale + 1));

    posx = (posx + rect_cx + 1) / 2;
    posy = (posy + rect_cy + 1) / 2;

    self->px = self->mx - posx;
    self->py = self->my - posy;
  }

end:
  gtk_widget_queue_draw (GTK_WIDGET (self));

  return TRUE;
}

static void
moving_canvas (RetrospriteeditorCanvas *self,
               gdouble                  x,
               gdouble                  y)
{
    self->px += self->mx - self->ox;
    self->py += self->my - self->oy;
    self->ox = self->mx;
    self->oy = self->my;

    gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
event_motion (GtkEventControllerMotion *evt,
              gdouble                   x,
              gdouble                   y,
              gpointer                  user_data)
{
  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (user_data);
  self->mx = x;
  self->my = y;

  if (self->is_2_btn_pressed) {
    moving_canvas (self, x, y);
  }

  if (x >= self->px && x <= self->xw) {
    if (y >= self->py && y <= self->yh) {
      if (self->tool > 0) {
        self->drawing_tool = TRUE;
        gtk_widget_queue_draw (GTK_WIDGET (self));
      } else {
        self->drawing_tool = FALSE;
      }
    } else {
      self->drawing_tool = FALSE;
    }

  } else {
    self->drawing_tool = FALSE;
  }

}

static void
mouse_mov_canvas_press (GtkGestureClick *evt,
         gint             n_press,
         gdouble          x,
         gdouble          y,
         gpointer         user_data)
{
  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (user_data);

  self->ox = x;
  self->oy = y;
  self->is_2_btn_pressed = TRUE;
}

static void
mouse_mov_canvas_release (GtkGestureClick *evt,
         gint             n_press,
         gdouble          x,
         gdouble          y,
         gpointer         user_data)
{
  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (user_data);

  self->is_2_btn_pressed = FALSE;
}

static void
mouse_hit_canvas_release (GtkGestureClick *evt,
         gint             n_press,
         gdouble          x,
         gdouble          y,
         gpointer         user_data)
{
   RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (user_data);
  self->is_0_btn_pressed = FALSE;
}

static void
select_index_color (GtkGestureClick *evt,
         gint             n_press,
         gdouble          x,
         gdouble          y,
         gpointer         user_data)
{
  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (user_data);
  guint32 xx = 0;
  guint32 max_index_color = global_get_max_index ();

  for (guint32 index_color = 0; index_color < max_index_color; index_color++) {
	  if (x >= xx && x <= (xx + 32)) {
		  self->selected_index_color = index_color;
		  RetrospriteeditorCanvas *main_canvas = canvas_get_drawing_canvas ();
		  canvas_set_index (RETROSPRITEEDITOR_CANVAS (main_canvas), index_color);
		  break;
	  }
	  xx += 32;
  }
}
void
canvas_set_index_colours_by_index (RetrospriteeditorCanvas * self, guint32 index_id, guint32 index_color)
{
	self->index_color[index_id] = index_color;	
}

void
canvas_set_colours_by_index (RetrospriteeditorCanvas * self, guint32 index_id, guint32 index_color)
{
  guint32 *pcolours = global_type_pallete_get_cur_ptr_pallete (0);
	self->colours[index_id] = pcolours[index_color];	
}

void
canvas_set_index_id (RetrospriteeditorCanvas *self, guint32 index)
{
	self->index_id = index;
}

static void
select_index_color_for_palette (GtkGestureClick *evt,
         gint             n_press,
         gdouble          x,
         gdouble          y,
         gpointer         user_data)
{
  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (user_data);
  guint32 xx = 0;
  guint32 yy = 0;
	guint32 w = self->width_rect * self->scale * 16;
	guint32 h = self->height_rect * self->scale * 4;
	guint32 index_color = 0;

	int found = 0;
	for (; yy < h; yy += self->height_rect * self->scale) {
		xx = 0;
		for (; xx < w; xx += self->width_rect * self->scale) {
	  	if ((x >= xx) && (x <= (xx + self->width_rect * self->scale))) {
				if ((y >= yy) && (y <= (yy + self->height_rect * self->scale))) {
					found = 1;
			  	self->selected_index_color = index_color;
					canvas_set_index_colours_by_index (RETROSPRITEEDITOR_CANVAS (self->child_one_color), self->index_id, index_color);
					canvas_set_colours_by_index (RETROSPRITEEDITOR_CANVAS (self->child_one_color), self->index_id, self->selected_index_color);
					gtk_widget_queue_draw (GTK_WIDGET (self->child_one_color));
					palette_nes_redraw ();
					nes_current_pallete_redraw ();
		  		break;
				}
	  	}
			index_color++;
		}
		if (found)
			break;
  }
}

void
canvas_set_xy_click (RetrospriteeditorCanvas *self)
{
	gtk_widget_add_controller (GTK_WIDGET (self),
			GTK_EVENT_CONTROLLER (self->gesture_click_select_one_color));
}

void
canvas_set_child_color (RetrospriteeditorCanvas *self,
		RetrospriteeditorCanvas *child)
{
	self->child_one_color = child;
}

static void
mouse_hit_canvas_press (GtkGestureClick *evt,
         gint             n_press,
         gdouble          x,
         gdouble          y,
         gpointer         user_data)
{
  RetrospriteeditorCanvas *self = RETROSPRITEEDITOR_CANVAS (user_data);

  self->is_0_btn_pressed = TRUE;

  if (self->tool > 0) {

    NesParamPoint n;
    n.blockx = self->last_blockx;
    n.blocky = self->last_blocky;
    n.x = self->last_pointx;
    n.y = self->last_pointy;
    RetrospriteeditorNesPalette *nes = get_nes ();

    nes_set_color (nes, &n, self->selected_index_color + 1);
    gtk_widget_queue_draw (GTK_WIDGET (self));
  }

}


static void
retrospriteeditor_canvas_init (RetrospriteeditorCanvas *self)
{
	self->child_one_color = NULL;
  self->colours = NULL;
  self->count_colours = 0;
  self->left_top = FALSE;
  self->mx = 0;
  self->my = 0;
  self->px = 0;
  self->py = 0;
  self->is_2_btn_pressed = FALSE;
  self->is_0_btn_pressed = FALSE;
  self->tool = 0;
  self->drawing_tool = FALSE;
  self->show_hex_index = FALSE;



  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (self), draw_canvas,
                                  NULL, NULL);

  self->event_zoom = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  g_signal_connect (self->event_zoom, "scroll",
                    G_CALLBACK (zoom_changed),
                    self);

  self->event_motion = gtk_event_controller_motion_new ();

  g_signal_connect (self->event_motion,
                   "motion",
                   G_CALLBACK (event_motion),
                   self);

  self->gesture_click_move_canvas = gtk_gesture_click_new ();
  self->gesture_click_tool_press  = gtk_gesture_click_new ();
  self->gesture_click_select_index_color = gtk_gesture_click_new ();
  self->gesture_click_select_one_color = gtk_gesture_click_new ();

  g_signal_connect (self->gesture_click_move_canvas, "pressed",
                    G_CALLBACK (mouse_mov_canvas_press),
                    self);

  g_signal_connect (self->gesture_click_move_canvas, "released",
                    G_CALLBACK (mouse_mov_canvas_release),
                    self);

  g_signal_connect (self->gesture_click_tool_press, "pressed",
                    G_CALLBACK (mouse_hit_canvas_press),
                    self);

  g_signal_connect (self->gesture_click_tool_press, "released",
                    G_CALLBACK (mouse_hit_canvas_release),
                    self);

  g_signal_connect (self->gesture_click_select_index_color, "pressed",
		  G_CALLBACK (select_index_color),
		  self);

  g_signal_connect (self->gesture_click_select_one_color, "pressed",
		  G_CALLBACK (select_index_color_for_palette),
		  self);

  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->gesture_click_select_one_color), 1);
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->gesture_click_select_index_color), 1);
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->gesture_click_tool_press), 1);
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (self->gesture_click_move_canvas), 2);
}

void canvas_shut_on_event_click (RetrospriteeditorCanvas *self)
{
	gtk_widget_add_controller (GTK_WIDGET (self),
			GTK_EVENT_CONTROLLER (self->gesture_click_select_index_color));
}

void canvas_shut_on_events (RetrospriteeditorCanvas *self)
{
  gtk_widget_add_controller (GTK_WIDGET (self),
                             GTK_EVENT_CONTROLLER (self->event_motion));

  gtk_widget_add_controller (GTK_WIDGET (self),
                             GTK_EVENT_CONTROLLER (self->event_zoom));

  gtk_widget_add_controller (GTK_WIDGET (self),
                             GTK_EVENT_CONTROLLER (self->gesture_click_move_canvas));

  gtk_widget_add_controller (GTK_WIDGET (self),
                             GTK_EVENT_CONTROLLER (self->gesture_click_tool_press));
}

static RetrospriteeditorCanvas *global_drawing_canvas;
static RetrospriteeditorCanvas *global_drawing_canvas_tileset;

RetrospriteeditorCanvas *canvas_get_tileset (void)
{
  return global_drawing_canvas_tileset;
}

void canvas_set_tileset (RetrospriteeditorCanvas *self)
{
  global_drawing_canvas_tileset = self;
}

RetrospriteeditorCanvas *canvas_get_drawing_canvas (void)
{
  return global_drawing_canvas;
}

void canvas_set_drawing_canvas (RetrospriteeditorCanvas *self)
{
  global_drawing_canvas = self;
}

void canvas_set_tool (RetrospriteeditorCanvas *self,
                      guint32                  tool)
{
  self->tool = tool;
}

#pragma once

#include <gtk/gtk.h>

#define MESSAGE_TYPE_ITEM          message_item_get_type ()
G_DECLARE_FINAL_TYPE (MessageItem, message_item, MESSAGE, ITEM, GtkDrawingArea)

G_END_DECLS

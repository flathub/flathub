#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define USER_TYPE_ITEM       user_item_get_type ()
G_DECLARE_FINAL_TYPE (UserItem, user_item, USER, ITEM, GtkFrame)

GtkWidget *user_item_new ();
void user_item_set_icon (UserItem *item, const char *data);
void user_item_set_name (UserItem *item, const char *name);
void user_item_set_status (UserItem *item, const int status);
void user_item_set_blink (UserItem *item, const int blink);
const char *user_item_get_icon (UserItem *item);
const char *user_item_get_name (UserItem *item);
int user_item_get_status (UserItem *item);
int user_item_get_blink (UserItem *item);
void user_item_set_chat (UserItem *item);
void user_item_add_message (UserItem *item, const char *message, const int me, const char *name, const int show_notification);

G_END_DECLS

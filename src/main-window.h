#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MAIN_TYPE_WINDOW             main_window_get_type ()
G_DECLARE_FINAL_TYPE (MainWindow, main_window, MAIN, WINDOW, GtkWindow)

void main_window_get_list_users (MainWindow *w);
void main_window_feed (MainWindow *w);
void main_window_play_new_message (MainWindow *self);

G_END_DECLS

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REGISTER_TYPE_WINDOW            register_window_get_type ()
G_DECLARE_FINAL_TYPE (RegisterWindow, register_window, REGISTER, WINDOW, GtkWindow)

G_END_DECLS

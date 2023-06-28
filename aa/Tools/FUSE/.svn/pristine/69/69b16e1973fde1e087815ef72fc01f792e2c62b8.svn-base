#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <gtk/gtk.h>
#include <glade/glade.h>

//#define OFFLINE  // Use captured UDP packets offline - faster for debug
#define TITLE    "FUSE Server Browser"
#define VERSION  "0.9.0"

extern GladeXML *xml;

/* Macro to provide quick popup - useful for debug */
#define POPUP(...) \
{                                                                           \
    extern GtkWindow *window;                                               \
    GtkWidget *dialog;                                                      \
    dialog = gtk_message_dialog_new (window,                                \
                                  GTK_DIALOG_DESTROY_WITH_PARENT,           \
                                  GTK_MESSAGE_INFO,                         \
                                  GTK_BUTTONS_OK,                           \
                                  __VA_ARGS__);                             \
    gtk_dialog_run (GTK_DIALOG (dialog));                                   \
    gtk_widget_destroy (dialog);                                            \
}

#define POPUP_ERROR(...) \
{                                                                           \
    extern GtkWindow *window;                                               \
    GtkWidget *dialog;                                                      \
    dialog = gtk_message_dialog_new (window,                                \
                                  GTK_DIALOG_MODAL,           \
                                  GTK_MESSAGE_ERROR,                        \
                                  GTK_BUTTONS_CLOSE,                        \
                                  __VA_ARGS__);                             \
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));                        \
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);\
    gtk_dialog_run (GTK_DIALOG (dialog));                                   \
    gtk_widget_destroy (dialog);                                            \
}


//gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
//gtk_window_set_keep_above(GTK_WINDOW(dialog), FALSE);
#endif

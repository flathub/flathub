#pragma once
#include <gtk/gtk.h>
#include <libxml/parser.h>

void project_set_folder_and_name (const char *folder, const char *name);
char * project_get_filepath_to_export (void);
void project_save_palettes (void);
void project_open_nes (char *filepath);

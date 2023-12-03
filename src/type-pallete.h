#pragma once
#include <gtk/gtk.h>

enum {
  PLATFORM_PALLETE_NES,
  N_PLATFORM_PALLETE
};

enum {
  NES_TYPE_PALETTE_2C02,
  N_NES_TYPE_PALETTE
};

typedef struct _DataForOutput {
	guint8 *data;
	gsize size;
} DataForOutput;

void global_get_data_for_output (DataForOutput *st);
guint32 global_get_max_index (void);
guint32 global_type_pallete_get_cur_platform (void);
guint32 global_type_pallete_get_cur_pallete (void);
guint32 *global_type_pallete_get_cur_ptr_pallete (guint32 index);
void global_type_pallete_set_cur (guint32 index, guint32 typen);

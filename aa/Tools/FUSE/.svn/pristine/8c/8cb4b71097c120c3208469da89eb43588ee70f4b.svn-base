#ifndef _MODELS_H_
#define _MODELS_H_

enum {
  SV_COLUMN_IP_PORT,
  SV_COLUMN_SERVERNAME,
  SV_COLUMN_MAPNAME,
  SV_COLUMN_PLAYERS,
  SV_COLUMN_PING,
  SV_COLUMNS
};

enum {
  PL_COLUMN_NAME,
  PL_COLUMN_SCORE,
  PL_COLUMN_PING,
  PL_COLUMNS
};

void AddServerModelToView(GtkWidget *sv_view);
void AddPlayerModelToView(GtkWidget *pl_view);
void AddServerDataToView(GtkTreeView *sv_view, GData **serverdata, gboolean countbots);
void AddPlayerDataToView(GtkTreeView *pl_view, tServer *server);

#endif

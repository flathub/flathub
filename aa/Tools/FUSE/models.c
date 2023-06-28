/*
FUSE FPS server browser
Copyright (C) 2008  Tony Jackson

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <gtk/gtk.h>

#include "slink.h"
#include "models.h"
#include "global.h"

GtkListStore *serverlist;

void AddServerModelToView(GtkWidget *sv_view)
{
    GtkCellRenderer *cell_renderer;
    GtkCellRenderer *cell_renderer_limited;

    GtkTreeViewColumn *col_ip, *col_server, *col_map, *col_players, *col_ping;

    GtkTreeSelection *tree_selection;
    gint width;
    gint height;

    /* IP/port, Name, mapname, version, website, admin, players, ping */
    serverlist = gtk_list_store_new(SV_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);

    gtk_tree_view_set_model(GTK_TREE_VIEW(sv_view), GTK_TREE_MODEL(serverlist));

    /* GtkListStore implements GtkTreeSortable */
    /* Default sort is by ascending ping */
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (serverlist),
                                        SV_COLUMN_PING, GTK_SORT_ASCENDING);

/*    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (serverlist),
                                        SV_COLUMN_SERVERNAME, GTK_SORT_ASCENDING);
*/

    cell_renderer = gtk_cell_renderer_text_new();
    cell_renderer_limited = gtk_cell_renderer_text_new();

    /* Change default cell renderer width to 100, but leave height as-is */
    gtk_cell_renderer_get_fixed_size(cell_renderer_limited, &width, &height);
    /* This renderer is primarily used to limit the width of the "Server" column */
    gtk_cell_renderer_set_fixed_size(cell_renderer_limited, 200, height);

    /* Create columns */
    col_ip      = gtk_tree_view_column_new_with_attributes("IP:Port", cell_renderer,         "text", SV_COLUMN_IP_PORT,    NULL);
    col_server  = gtk_tree_view_column_new_with_attributes("Server",  cell_renderer_limited, "text", SV_COLUMN_SERVERNAME, NULL);
    col_map     = gtk_tree_view_column_new_with_attributes("Map",     cell_renderer,         "text", SV_COLUMN_MAPNAME,    NULL);
    col_players = gtk_tree_view_column_new_with_attributes("Players", cell_renderer,         "text", SV_COLUMN_PLAYERS,    NULL);
    col_ping    = gtk_tree_view_column_new_with_attributes("Ping",    cell_renderer,         "text", SV_COLUMN_PING,       NULL);

    /* Allow columns to be resized */
    gtk_tree_view_column_set_resizable(col_ip,      1);
    gtk_tree_view_column_set_resizable(col_server,  1);
    gtk_tree_view_column_set_resizable(col_map,     1);
    gtk_tree_view_column_set_resizable(col_players, 1);
    gtk_tree_view_column_set_resizable(col_ping,    1);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(sv_view), TRUE);

    /* Typed searchs search by servername */
    gtk_tree_view_set_search_column (GTK_TREE_VIEW(sv_view), SV_COLUMN_SERVERNAME);


    gtk_tree_view_append_column (GTK_TREE_VIEW(sv_view), col_ip     );
    gtk_tree_view_append_column (GTK_TREE_VIEW(sv_view), col_server );
    gtk_tree_view_append_column (GTK_TREE_VIEW(sv_view), col_map    );
    gtk_tree_view_append_column (GTK_TREE_VIEW(sv_view), col_players);
    gtk_tree_view_append_column (GTK_TREE_VIEW(sv_view), col_ping   );

    gtk_tree_view_column_set_reorderable (col_ip,      TRUE);
    gtk_tree_view_column_set_reorderable (col_server,  TRUE);
    gtk_tree_view_column_set_reorderable (col_map,     TRUE);
    gtk_tree_view_column_set_reorderable (col_players, TRUE);
    gtk_tree_view_column_set_reorderable (col_ping,    TRUE);

    gtk_tree_view_column_set_sort_column_id (col_ip,      SV_COLUMN_IP_PORT);
    gtk_tree_view_column_set_sort_column_id (col_server,  SV_COLUMN_SERVERNAME);
    gtk_tree_view_column_set_sort_column_id (col_map,     SV_COLUMN_MAPNAME);
    gtk_tree_view_column_set_sort_column_id (col_players, SV_COLUMN_PLAYERS);
    gtk_tree_view_column_set_sort_column_id (col_ping,    SV_COLUMN_PING);

  /* This doesn't seem to work - would have to set it after the column is selected */
/*    gtk_tree_view_column_set_sort_order(col_players, GTK_SORT_DESCENDING); */

    /* Set selection mode so that only a single server can be selected at once */
    tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sv_view));
    gtk_tree_selection_set_mode(tree_selection, GTK_SELECTION_SINGLE);
}

void AddPlayerModelToView(GtkWidget *pl_view)
{

    GtkListStore *playerlist;
    GtkCellRenderer *cell_renderer;
    GtkTreeSelection *tree_selection;

    GtkTreeViewColumn *col_name, *col_score, *col_ping;

	/* Name, score, ping */
    playerlist = gtk_list_store_new(PL_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);

    gtk_tree_view_set_model(GTK_TREE_VIEW(pl_view), GTK_TREE_MODEL(playerlist));

    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (playerlist),
                                        PL_COLUMN_SCORE, GTK_SORT_DESCENDING);

    /* Players are not selectable */
    tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl_view));
    gtk_tree_selection_set_mode(tree_selection, GTK_SELECTION_NONE);

    /* Create new cell renderer (strictly, we could re-use the server view one
       but this keeps the code tidy) */
    cell_renderer = gtk_cell_renderer_text_new();

    /* Create columns */
    col_name  = gtk_tree_view_column_new_with_attributes("Name",  cell_renderer, "text", PL_COLUMN_NAME,  NULL); /* Could use "markup" instead of "text" to bold real players? */
    col_score = gtk_tree_view_column_new_with_attributes("Score", cell_renderer, "text", PL_COLUMN_SCORE, NULL);
    col_ping  = gtk_tree_view_column_new_with_attributes("Ping",  cell_renderer, "text", PL_COLUMN_PING,  NULL);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pl_view), TRUE);

    gtk_tree_view_append_column (GTK_TREE_VIEW(pl_view), col_name );
    gtk_tree_view_append_column (GTK_TREE_VIEW(pl_view), col_score);
    gtk_tree_view_append_column (GTK_TREE_VIEW(pl_view), col_ping );

    gtk_tree_view_column_set_resizable(col_name,  1);
    gtk_tree_view_column_set_resizable(col_score, 1);
    gtk_tree_view_column_set_resizable(col_ping,  1);

    gtk_tree_view_column_set_reorderable (col_name,   TRUE);
    gtk_tree_view_column_set_reorderable (col_score,  TRUE);
    gtk_tree_view_column_set_reorderable (col_ping,   TRUE);

    gtk_tree_view_column_set_sort_column_id (col_name,   PL_COLUMN_NAME);
    gtk_tree_view_column_set_sort_column_id (col_score,  PL_COLUMN_SCORE);
    gtk_tree_view_column_set_sort_column_id (col_ping,   PL_COLUMN_PING);

    /* Typed searchs search by servername */
    gtk_tree_view_set_search_column (GTK_TREE_VIEW(pl_view), PL_COLUMN_NAME);

}

/* Private structure declaration */
typedef struct _tModelUserData
{
    GtkListStore   *model;
    gboolean        countbots; /* If true, bots and players are counted together */
} tModelUserData;

void MODEL_AddServerData(GQuark key_id, gpointer data, gpointer user_data)
{
    tModelUserData *ud     = (tModelUserData *)user_data;
    tServer        *server = (tServer *)data;
    GtkTreeIter iter;
    guint i, players;

    if(server->player == NULL)
        players = 0;
    else
    {
        if(ud->countbots)
            players = g_strv_length((gchar **)(server->player));
        else /* Don't count players with 0ms ping - assumed to be bots */
            for(i=0, players=0; server->player[i] != NULL; i++)
                if(server->player[i]->ping != 0)
                    players++;
    }
/*    POPUP("Hostname = %s, Mapname = %s", sv_info[i].szHostName, sv_info[i].szMapName);*/
    gtk_list_store_append(ud->model, &iter);
    /* Note that GetServerInfo() returning NULL adds an empty entry to the list store */
    gtk_list_store_set(ud->model, &iter,
                      SV_COLUMN_IP_PORT,    g_quark_to_string(key_id),
                      SV_COLUMN_SERVERNAME, GetServerInfo(server, "hostname"),
                      SV_COLUMN_MAPNAME,    GetServerInfo(server, "mapname"),
                      SV_COLUMN_PLAYERS,    players,
                      SV_COLUMN_PING,       server->ping,
                      -1);
}

void AddServerDataToView(GtkTreeView *sv_view, GData **serverdata, gboolean countbots)
{
    tModelUserData ud;

    ud.model = GTK_LIST_STORE(gtk_tree_view_get_model(sv_view));
    ud.countbots = countbots;
    gtk_list_store_clear(ud.model);

    g_datalist_foreach(serverdata, MODEL_AddServerData, &ud);
}

void AddPlayerDataToView(GtkTreeView *pl_view, tServer *server)
{
    guint i;
    GtkTreeIter iter;
    GtkListStore *model;

    model = GTK_LIST_STORE(gtk_tree_view_get_model(pl_view));
    gtk_list_store_clear(model);

    if(server == NULL)
        return;

    if(server->player == NULL)
        return;

    for(i = 0; server->player[i] != NULL; i++)
    {
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter,
                              PL_COLUMN_NAME,  server->player[i]->playername,
                              PL_COLUMN_SCORE, server->player[i]->score,
                              PL_COLUMN_PING,  server->player[i]->ping,
                              -1);
    }
}

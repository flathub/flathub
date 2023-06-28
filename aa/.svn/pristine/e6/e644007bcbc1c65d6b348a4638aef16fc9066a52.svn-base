/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "g_local.h"
#include "m_player.h"


char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if(g_tactical->value)
	{
		if(ent1->ctype == ent2->ctype)
			return true;
	}

	if (!((dmflags->integer & DF_SKINTEAMS) || ctf->value))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;

	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}

void DrawChatBubble (edict_t *ent)
{
	if (!ent->client || !player_participating (ent))
		return;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SAYICON);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
}

//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		safe_cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (Q_strcasecmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_strcasecmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000, true, true);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		//don't give jetpacks or flags
		it = FindItem("Jetpack");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;
		it = FindItem("Blue Flag");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;
		it = FindItem("Red Flag");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;
		return;
	}
	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			safe_cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		safe_cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}

}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		safe_cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	safe_cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		safe_cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	safe_cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		safe_cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	safe_cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();

	if(ent->ctype == 0)
	{
		if(strcmp (s, "Blaster") == 0)
			strcpy(s, "Alien Blaster");
	}

	it = FindItem (s);
	if (!it)
	{
		safe_cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		safe_cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		if ((it->flags & IT_BUYABLE) && ent->client->resp.powered)
		{	// obtainable through the rewards system.
			ent->client->resp.powered = false;
			ent->client->resp.reward_pts = 0;
			ent->client->pers.inventory[index] = 1;
			it->use (ent, it);
			return;
		}
		safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		if (it->flags & IT_WEAPON) {
		    ent->client->pers.lastfailedswitch = it;
		    ent->client->pers.failedswitch_framenum = level.framenum;
		}
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		safe_cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		safe_cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		safe_cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		safe_cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		safe_cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		safe_cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		safe_cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) <= 0)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < g_maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags ascending
	qsort (index, count, sizeof(index[0]), G_PlayerSortAscending);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{

		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);

		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	safe_cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		safe_cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		safe_cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		safe_cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		safe_cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		safe_cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (strlen(gi.args()) < 3) //no text, don't send
		return;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SAYICON);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	if (!((dmflags->integer & DF_SKINTEAMS) || ctf->value))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "[TEAM] %s: ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			safe_cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] &&
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			safe_cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (g_dedicated->value)
		safe_cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if(other->is_bot) //JD - security fix
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		//safe_cprintf(other, PRINT_CHAT, "%s", text);
		//JD - security fix
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}

}

static const char *participation_descriptions[participation_numstates] = 
{
	"", " (spectator)", " (picking team)", " (waiting for turn)"
};

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < g_maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		Com_sprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
			((int)(level.time - e2->client->resp.entertime) / 60),
			((int)(level.time - e2->client->resp.entertime) % 60)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			participation_descriptions[e2->client->resp.participation]);
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			safe_cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	safe_cprintf(ent, PRINT_HIGH, "%s", text);
}

void Cmd_TeamSelect_f (edict_t *ent)
{
	if (!TEAM_GAME)
	{
		safe_cprintf (ent, PRINT_HIGH, "You do not need to pick a team on "
		              "this server.\n");
		return;
	}
	if (ent->client->resp.participation == participation_spectating)
	{
		// TODO: should we just stuff "set spectator 0" instead?
		safe_cprintf (ent, PRINT_HIGH, "Exit spectator mode (set spectator 0) "
		              "to choose a team.\n");
		return;
	}
	ent->dmteam = NO_TEAM;
}

/*
=================
Cmd_CallVote_f
=================
*/
void Cmd_CallVote_f (edict_t *ent)
{

	if(level.time <= warmuptime->value) {
		safe_bprintf(PRINT_HIGH, "Cannot call a vote during warmup!\n");
		return;
	}

	if(playervote.called) {
		safe_bprintf(PRINT_HIGH, "Vote already in progress, please wait.\n");
		return;
	}

	//start a vote
	playervote.called = true;
	playervote.yay = playervote.nay = 0;
	playervote.starttime = level.time;
	if(strlen(gi.args()) < 128) {
		strcpy(playervote.command, gi.args());
		safe_bprintf(PRINT_HIGH, "%s called a vote: %s\n", ent->client->pers.netname, playervote.command);
	}
}

/*
=================
Cmd_Vote_f
=================
*/
void Cmd_Vote_f (edict_t *ent)
{

	int	i, j, mostvotes, winner;
	int	n_candidates;
	int	candidates[4];
	char	buffer[512];
	char	buffer2[60];
	edict_t *cl_ent;

	i = atoi (gi.argv(1));

	if(g_callvote->value && playervote.called) {

		switch(i) { //to do - move "voted" to persistant data
			case 1:
				if(!ent->client->resp.voted) {
					ent->client->resp.voted = true;
					playervote.yay++;
					safe_bprintf(PRINT_HIGH, "%s voted ^2YES\n", ent->client->pers.netname);
				}
				break;
			case 2:
				if(!ent->client->resp.voted) {
					ent->client->resp.voted = true;
					playervote.nay++;
					safe_bprintf(PRINT_HIGH, "%s voted ^1NO\n", ent->client->pers.netname);
				}
				break;
		}
	}

	if (!level.intermissiontime || ! (g_mapvote && g_mapvote->value && !g_mapvote->modified) )
		return;

	ent->client->mapvote = i;
	safe_bprintf(PRINT_HIGH, "%s voted for map %i\n", ent->client->pers.netname, i);

	//update scoreboard
	mostvotes = 0;
	winner = 1; //next map in line
	for(i = 0; i < 4; i++)
		votedmap[i].tally = 0;
	for (i=0 ; i<g_maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->is_bot)
			continue;
		for(j = 0; j < 4; j++) {
			if(cl_ent->client->mapvote-1 == j)
				votedmap[j].tally++;
			if(votedmap[j].tally > mostvotes){
				mostvotes = votedmap[j].tally;
			}
		}
	}

	if ( g_voterand && g_voterand->value )
	{
		// random tie resolution
		n_candidates = 0;
		for (j = 0; j < 4; j ++) {
			if ( votedmap[j].tally < mostvotes )
				continue;
			candidates[n_candidates ++] = j;
		}

		if ( n_candidates == 1 )
		{
			winner = candidates[0];
			sprintf( buffer, "Map %s leads with %i vote%s!", votedmap[winner].mapname, votedmap[winner].tally, (mostvotes > 1) ? "s" : ""  );
		}
		else
		{
			strcpy( buffer, "It's a tie!\nMaps ");
			for ( i = 0 ; i < n_candidates ; i ++ ) {
				j = candidates[i];
				if ( i > 0 )
				{
					if ( i == n_candidates - 1 )
						strcat( buffer, " and " );
					else
						strcat( buffer, ", " );
				}
				strcat( buffer, votedmap[j].mapname );
			}
			sprintf( buffer2, "\nlead with %i vote%s!" , mostvotes , (mostvotes > 1) ? "s" : "" );
			strcat( buffer, buffer2 );
		}
	}
	else
	{
		// "old" voting system, leading map is the first one with enough votes
		for (j = 0; j < 4; j ++) {
			i = (j + 1) % 4;
			if ( votedmap[i].tally < mostvotes )
				continue;
			winner = i;
			break;
		}
		sprintf( buffer, "Map %s leads with %i vote%s!", votedmap[winner].mapname, votedmap[winner].tally, (mostvotes > 1) ? "s" : "" );
	}

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->is_bot)
			continue;
		safe_centerprintf(cl_ent, "%s", buffer );
	}
}

void Cmd_VoiceTaunt_f (edict_t *ent)
{
	int			index, i;
	qboolean	done;
	char		name[32];
	char		string[256]; //a small string to send
	char		playermodel[MAX_OSPATH], tauntsound[MAX_OSPATH];
	char		*info;

	index = atoi (gi.argv(1));

	if(index > 5 || index < 1 || ent->is_bot) {
		index = (int)(1 + random() * 5);
		if(index > 5)
			index = 5;
	}

	//get info about this client
	if(ent->inuse && ent->client) {

		if((level.time - ent->client->lasttaunttime) > 2) { //prevent flooding

			ent->client->lasttaunttime = level.time;

			strcpy(name, ent->client->pers.netname);
			info = Info_ValueForKey (ent->client->pers.userinfo, "skin");

			if ( *info == '\0' )
			{ /* could not find skin. probable programming error. */
				gi.dprintf("Cmd_VoiceTaunt_f: skin not found in userinfo\n");
				return;
			}

			info[96] = 0; //truncate to prevent bad people from harming the server

			i = 0;
			done = false;
			while(!done)
			{
				if((info[i] == '/') || (info[i] == '\\'))
					done = true;
				playermodel[i] = info[i];
				if(i > 62)
					done = true;
				i++;
			}
			playermodel[i-1] = 0;

			sprintf(tauntsound, "taunts/%s/taunt%i.wav", playermodel, index);

			Com_sprintf(string, sizeof(string),
				"%s %s %s ", info, tauntsound, name);

			//send to all clients as a general config string
			gi.configstring (CS_GENERAL, string);
		}
	}
}

/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t *ent)
{
	char	*cmd;

	if (!ent->client)
		return;		// not fully in game yet

// ACEBOT_ADD
	if(ACECM_Commands(ent))
		return;
// ACEBOT_END

	cmd = gi.argv(0);

	if (Q_strcasecmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_strcasecmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_strcasecmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_strcasecmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_strcasecmp (cmd, "callvote") == 0)
	{
		Cmd_CallVote_f(ent);
		return;
	}
	if (Q_strcasecmp (cmd, "vote") == 0)
	{
		Cmd_Vote_f(ent);
		return;
	}
	if	(Q_strcasecmp (cmd, "vtaunt") == 0)
	{
		Cmd_VoiceTaunt_f(ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_strcasecmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_strcasecmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_strcasecmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_strcasecmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_strcasecmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_strcasecmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_strcasecmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_strcasecmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_strcasecmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_strcasecmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_strcasecmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_strcasecmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_strcasecmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_strcasecmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_strcasecmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_strcasecmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_strcasecmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_strcasecmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_strcasecmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_strcasecmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_strcasecmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_strcasecmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	else if (Q_strcasecmp(cmd, "teamselect") == 0)
		Cmd_TeamSelect_f (ent);
	else if (Q_strcasecmp (cmd, "chatbubble") == 0)
		DrawChatBubble(ent);
	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);

}

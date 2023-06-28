/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2010 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "g_local.h"
#include "m_player.h"

/* Number of gibs to throw on death with lots of damage (including Client Head, where applicable) */
#define DEATH_GIBS_TO_THROW 5
void ClientUserinfoChanged (edict_t *ent, char *userinfo, int whereFrom);
void ClientDisconnect (edict_t *ent);
void SP_misc_teleporter_dest (edict_t *ent);

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
	return;
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	//SP_misc_teleporter_dest (self);
}
void SP_info_player_red(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	//SP_misc_teleporter_dest (self);
}
void SP_info_player_blue(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	//SP_misc_teleporter_dest (self);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(edict_t *ent)
{
}


//=======================================================================


void player_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
	if(self->is_bot)
		self->oldenemy = other;
}


static qboolean IsFemale (edict_t *ent)
{
	char		*info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "skin");
	if (info[0] == 'f' || info[0] == 'F')
		return true;
	return false;
}


void ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	int		mod=0, msg;
	char		*message;
	char		*message2;
	qboolean	ff;
	char		*chatmsg;
	char		*tauntmsg;
	char cleanname[PLAYERNAME_SIZE];
	char cleanname2[PLAYERNAME_SIZE];
	int			i, pos, total, place;
	edict_t		*cl_ent;

	if (deathmatch->value)
	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		chatmsg = NULL;
		tauntmsg = NULL;
		message2 = "";

		switch (mod)
		{
		case MOD_SUICIDE:
			message = "suicides";
			break;
		case MOD_FALLING:
			message = "cratered";
			break;
		case MOD_CRUSH:
			message = "was squished";
			break;
		case MOD_WATER:
			message = "sank like a rock";
			break;
		case MOD_SLIME:
			message = "melted";
			break;
		case MOD_LAVA:
			message = "does a back flip into the lava";
			break;
		case MOD_EXPLOSIVE:
		case MOD_BARREL:
			message = "blew up";
			break;
		case MOD_EXIT:
			message = "found a way out";
			break;
		case MOD_TARGET_LASER:
			message = "saw the light";
			break;
		case MOD_TARGET_BLASTER:
			message = "got blasted";
			break;
		case MOD_BOMB:
		case MOD_SPLASH:
		case MOD_TRIGGER_HURT:
			message = "was in the wrong place";
			break;
		}
		if (attacker == self)
		{
			switch (mod)
			{
			case MOD_CAMPING:
				message = "was killed for camping";
				break;
			case MOD_PLASMA_SPLASH:
				if (IsFemale(self))
					message = "melted herself";
				else
					message = "melted himself";
				break;
			case MOD_R_SPLASH:
				if (IsFemale(self))
					message = "blew herself up";
				else
					message = "blew himself up";
				break;
			case MOD_VAPORIZER:
				message = "should have used a smaller gun";
				break;
			default:
				if (IsFemale(self))
					message = "killed herself";
				else
					message = "killed himself";
				break;
			}
		}
		if (message)
		{
			safe_bprintf (PRINT_MEDIUM, "%s %s.\n", self->client->pers.netname, message);
			if (deathmatch->value) {
				self->client->resp.score--;
				self->client->resp.deaths++;
			}
			self->enemy = NULL;
			self->client->kill_streak = 0; //reset, you are dead
			return;
		}

		self->enemy = attacker;
		if (attacker && attacker->client)
		{
			//clean up names, get rid of escape chars
			G_CleanPlayerName ( self->client->pers.netname , cleanname );
			G_CleanPlayerName ( attacker->client->pers.netname , cleanname2 );

			if(!attacker->is_bot) 
			{
				pos = 0;
				total = 0;
				for (i=0 ; i<game.maxclients ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || !player_participating (cl_ent))
						continue;

					if(attacker->client->resp.score+1 >= game.clients[i].resp.score)
						pos++;

					total++;
				}
				place = total - pos;
				switch(place)
				{
				case 0:
					safe_centerprintf(attacker, "You fragged %s\n1st place with %i frags\n", cleanname, attacker->client->resp.score+1);
					break;
				case 1:
					safe_centerprintf(attacker, "You fragged %s\n2nd place with %i frags\n", cleanname, attacker->client->resp.score+1);
					break;
				case 2:
					safe_centerprintf(attacker, "You fragged %s\n3rd place with %i frags\n", cleanname, attacker->client->resp.score+1);
					break;
				default:
					safe_centerprintf(attacker, "You fragged %s\n", cleanname);
					break;
				}

				// Send Steam stats
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte(TE_KILL);
				gi.unicast (attacker, false);
			}

			switch (mod)
			{
			case MOD_BLASTER:
				message = "was blasted by";
				break;
			case MOD_VIOLATOR:
				message = "was probed by";
				break;
			case MOD_CGALTFIRE:
				message = "was blown away by";
				message2 = "'s chaingun burst";
				break;
			case MOD_CHAINGUN:
				message = "was cut in half by";
				message2 = "'s chaingun";
				break;
			case MOD_FLAME:
				message = "was burned by";
				message2 = "'s napalm";
				break;
			case MOD_ROCKET:
				message = "ate";
				message2 = "'s rocket";
				break;
			case MOD_R_SPLASH:
				message = "almost dodged";
				message2 = "'s rocket";
				break;
			case MOD_BEAMGUN:
				message = "was melted by";
				message2 = "'s beamgun";
				break;
			case MOD_DISRUPTOR:
				message = "was disrupted by";
				break;
			case MOD_SMARTGUN:
				message = "saw the pretty lights from";
				message2 = "'s smartgun";
				break;
			case MOD_VAPORIZER:
				message = "was disintegrated by";
				message2 = "'s vaporizer blast";
				break;
			case MOD_VAPORALTFIRE:
				message = "couldn't hide from";
				message2 = "'s vaporizer";
				break;
			case MOD_MINDERASER:
				message = "had its mind erased by";
				message2 = "'s alien seeker";
				break;
			case MOD_PLASMA_SPLASH: //blaster splash damage
				message = "was melted";
				message2 = "'s plasma";
				break;
			case MOD_TELEFRAG:
				message = "tried to invade";
				message2 = "'s personal space";
				break;
			case MOD_GRAPPLE:
				message = "was caught by";
				message2 = "'s grapple";
				break;
			case MOD_HEADSHOT:
				message = "had its head blown off by";
			}
			//here is where the bot chat features will be added.
			//default is on.  Setting to 1 turns it off.

			if ( !(dmflags->integer & DF_BOTCHAT) && self->is_bot)
			{
				// FIXME: strange way to generate a random integer
				msg = random() * 9;
				if (msg > 0 && msg < 8)
					chatmsg = self->chatmsg[msg-1];
				else
					chatmsg = "%s: Stop it %s, you punk!";
				
				if(chatmsg) {
					safe_bprintf (PRINT_CHAT, chatmsg, self->client->pers.netname, attacker->client->pers.netname);
					safe_bprintf (PRINT_CHAT, "\n");

					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (TE_SAYICON);
					gi.WritePosition (self->s.origin);
					gi.multicast (self->s.origin, MULTICAST_PVS);
				}
			}

			//bot taunts
			if(!(dmflags->integer & DF_BOTCHAT) && attacker->is_bot) {

				if(!(attacker->client->ps.pmove.pm_flags & PMF_DUCKED)) {
					attacker->state = STATE_STAND;
					attacker->client->anim_priority = ANIM_WAVE;
					attacker->s.frame = FRAME_taunt01-1;
					attacker->client->anim_end = FRAME_taunt17;

					//print a taunt, or send taunt sound
					msg = random() * 24;
					switch(msg){
					case 1:
						tauntmsg = "%s: You should have used a bigger gun %s.\n";
						break;
					case 2:
						tauntmsg = "%s: You fight like your mom %s.\n";
						break;
					case 3:
						tauntmsg = "%s: And stay down %s!\n";
						break;
					case 4:
						tauntmsg = "%s: %s = pwned!\n";
						break;
					case 5:
						tauntmsg = "%s: All too easy, %s, all too easy.\n";
						break;
					case 6:
						tauntmsg = "%s: Ack! %s Ack! Ack!\n";
						break;
					case 7:
						tauntmsg = "%s: What a loser you are %s!\n";
						break;
					case 8:
						tauntmsg = "%s: %s, could you BE any more dead?\n";
						break;
					case 9:
					case 10:
					case 11:
					case 12:
					case 13:
					case 14:
					case 15:
					case 16:
					case 17:
					case 18:
					case 19:
					case 20:
					case 21:
					case 22:
					case 23:
					case 24:
						Cmd_VoiceTaunt_f(attacker);
						break;
					default:
						tauntmsg = "%s: You are useless to me, %s\n";
						break;
					}
					if(tauntmsg) {
						safe_bprintf (PRINT_CHAT, tauntmsg, attacker->client->pers.netname, self->client->pers.netname);
						//send an effect to show that the bot is taunting
						gi.WriteByte (svc_temp_entity);
						gi.WriteByte (TE_SAYICON);
						gi.WritePosition (attacker->s.origin);
						gi.multicast (attacker->s.origin, MULTICAST_PVS);
					}
				}
			}

			if (message)
			{
				safe_bprintf (PRINT_MEDIUM,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);

				if (deathmatch->value)
				{
					if(mod == MOD_MINDERASER)
					{
						self->client->resp.reward_pts = 0; 
						self->client->resp.powered = false;
						if(!attacker->is_bot) 
						{
							// Send Steam stats
							gi.WriteByte (svc_temp_entity);
							gi.WriteByte(TE_MINDERASED);
							gi.unicast (attacker, false);
						}
					}

					if(mod == MOD_VIOLATOR)
					{
						if(!attacker->is_bot) 
						{
							// Send Steam stats
							gi.WriteByte (svc_temp_entity);
							gi.WriteByte(TE_VIOLATED);
							gi.unicast (attacker, false);
						}
					}

					if (ff) 
					{
						attacker->client->resp.score--;
						attacker->client->resp.deaths++;
						if ((dmflags->integer & DF_SKINTEAMS) && !ctf->value) {
							if(attacker->dmteam == RED_TEAM)
								red_team_score--;
							else
								blue_team_score--;
						}
					}
					else 
					{
						attacker->client->resp.score++;

						if(!self->groundentity) 
						{
							PlayerGrantRewardPoints (attacker, 3);
							safe_centerprintf(attacker, "Midair shot!\n");
							if(!attacker->is_bot) 
							{
								// Send Steam stats
								gi.WriteByte (svc_temp_entity);
								gi.WriteByte(TE_MIDAIRSHOT);
								gi.unicast (attacker, false);
							}
						}
						else
							PlayerGrantRewardPoints (attacker, 1);

						if(mod == MOD_HEADSHOT) 
						{	
							//3 more pts for a headshot
							PlayerGrantRewardPoints (attacker, 3);
							safe_centerprintf(attacker, "HEADSHOT!\n");
							gi.sound(attacker, CHAN_AUTO, gi.soundindex("misc/headshot.wav"), 1, ATTN_STATIC, 0);

							if(!attacker->is_bot) 
							{
								// Send Steam stats
								gi.WriteByte (svc_temp_entity);
								gi.WriteByte(TE_HEADSHOT);
								gi.unicast (attacker, false);
							}
						}

						//mutators
						if(vampire->value) 
						{
							attacker->health+=20;
							if(attacker->health > attacker->max_health)
								attacker->health = attacker->max_health;
						}
						self->client->resp.deaths++;

						if ((dmflags->integer & DF_SKINTEAMS)  && !ctf->value) 
						{
							if(attacker->dmteam == RED_TEAM)
							{
								red_team_score++;
								safe_bprintf(PRINT_MEDIUM, "Red Team scores!\n");
								gi.sound (self, CHAN_AUTO, gi.soundindex("misc/red_scores.wav"), 1, ATTN_NONE, 0);
							}
							else 
							{
								blue_team_score++;
								safe_bprintf(PRINT_MEDIUM, "Blue Team scores!\n");
								gi.sound (self, CHAN_AUTO, gi.soundindex("misc/blue_scores.wav"), 1, ATTN_NONE, 0);

							}
						}
						//kill streaks
						attacker->client->kill_streak++;
						switch(attacker->client->kill_streak) 
						{
							case 3:
								for (i=0 ; i<g_maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s is on a killing spree!\n", cleanname2);
								}
								if(!attacker->is_bot) 
								{
									// Send Steam stats
									gi.WriteByte (svc_temp_entity);
									gi.WriteByte(TE_KILLSTREAK);
									gi.unicast (attacker, false);
								}
								break;
							case 5:
								for (i=0 ; i<g_maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s is on a rampage!\n", cleanname2);
								}
								gi.sound (self, CHAN_AUTO, gi.soundindex("misc/rampage.wav"), 1, ATTN_NONE, 0);
								PlayerGrantRewardPoints (attacker, 10);
								if(!attacker->is_bot) 
								{
									// Send Steam stats
									gi.WriteByte (svc_temp_entity);
									gi.WriteByte(TE_RAMPAGE);
									gi.unicast (attacker, false);
								}
								break;
							case 8:
								for (i=0 ; i<g_maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s is unstoppable!\n", cleanname2);
								}
								if(!attacker->is_bot) 
								{
									// Send Steam stats
									gi.WriteByte (svc_temp_entity);
									gi.WriteByte(TE_UNSTOPPABLE);
									gi.unicast (attacker, false);
								}
								break;
							case 10:
								for (i=0 ; i<g_maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s is a god!\n", cleanname2);
								}
								gi.sound (self, CHAN_AUTO, gi.soundindex("misc/godlike.wav"), 1, ATTN_NONE, 0);
								PlayerGrantRewardPoints (attacker, 20);
								if(!attacker->is_bot) 
								{
									// Send Steam stats
									gi.WriteByte (svc_temp_entity);
									gi.WriteByte(TE_GODLIKE);
									gi.unicast (attacker, false);
								}
								break;
							default:
								break;
						}
						if(self->client->kill_streak >=3) 
						{
							for (i=0 ; i<g_maxclients->value ; i++)
							{
								cl_ent = g_edicts + 1 + i;
								if (!cl_ent->inuse || cl_ent->is_bot)
									continue;
								safe_centerprintf(cl_ent, "%s's killing spree\nended by %s!\n", cleanname, cleanname2);
							}
						}
					}

				}
				self->client->kill_streak = 0; //reset, you are dead
				return;
			}
		}

	}

	if(mod == MOD_DEATHRAY) 
	{
		safe_bprintf(PRINT_MEDIUM, "%s killed by Deathray!\n", self->client->pers.netname);

		//immune player (activator) gets score increase
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			if(cl_ent->client)
				if(cl_ent->client->rayImmunity)
					cl_ent->client->resp.score++;
		}
		return;
	}

	if(mod == MOD_SPIDER) 
	{
		safe_bprintf(PRINT_MEDIUM, "%s killed by Spiderbot!\n", self->client->pers.netname);

		self->client->resp.reward_pts = 0; 
		self->client->resp.powered = false;

		if(inflictor->owner->client)
			inflictor->owner->client->resp.score++;
		return;
	}
	
	safe_bprintf (PRINT_MEDIUM,"%s died.\n", self->client->pers.netname);
	if (deathmatch->value) 
	{
		self->client->resp.score--;
		self->client->resp.deaths++;
	}

}


void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void TossClientWeapon (edict_t *self)
{
	gitem_t		*item;
	edict_t		*drop;
	qboolean	quad;
	qboolean	sproing;
	qboolean	haste;
	float		spread;

	if (!deathmatch->value)
	{
		return;
	}

	item = self->client->pers.weapon;
	if (!self->client->pers.inventory[self->client->ammo_index] )
		item = NULL;

#ifdef ALTERIA
	if (item && (strcmp (item->pickup_name, "Warriorpunch") == 0))
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Wizardpunch") == 0))
		item = NULL;
#else
	if (item && (strcmp (item->pickup_name, "Blaster") == 0))
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Alien Blaster") == 0))
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Violator") == 0))
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Minderaser") == 0))
		item = NULL;
	if (g_tactical->integer && item && (strcmp (item->pickup_name, "Alien Vaporizer") == 0))
		item = NULL;
#endif

	if(g_tactical->integer)
	{
		//always drop your weapon on death, even if it isn't the current held item.
		if(self->client->pers.inventory[ITEM_INDEX(FindItem("Flame Thrower"))] == 1)
		{
			item = FindItem( "Flame Thrower" );
			if(item)
			{
				spread = 0;
				self->client->v_angle[YAW] -= spread;
				drop = Drop_Item (self, item);
				self->client->v_angle[YAW] += spread;
				drop->spawnflags = DROPPED_PLAYER_ITEM;
			}
		}
		if(self->client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] == 1)
		{	
			item = FindItem( "Rocket Launcher" );
			if(item)
			{
				spread = 35;
				self->client->v_angle[YAW] -= spread;
				drop = Drop_Item (self, item);
				self->client->v_angle[YAW] += spread;
				drop->spawnflags = DROPPED_PLAYER_ITEM;
			}
		}
		if(self->client->pers.inventory[ITEM_INDEX(FindItem("Chaingun"))] == 1)
		{	
			item = FindItem( "Chaingun" );
			if(item)
			{
				spread = 70;
				self->client->v_angle[YAW] -= spread;
				drop = Drop_Item (self, item);
				self->client->v_angle[YAW] += spread;
				drop->spawnflags = DROPPED_PLAYER_ITEM;
			}
		}
		if(self->client->pers.inventory[ITEM_INDEX(FindItem("Disruptor"))] == 1)
		{	
			item = FindItem( "Disruptor" );
			if(item)
			{
				spread = 105;
				self->client->v_angle[YAW] -= spread;
				drop = Drop_Item (self, item);
				self->client->v_angle[YAW] += spread;
				drop->spawnflags = DROPPED_PLAYER_ITEM;
			}
		}
		if(self->client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] == 1)
		{
			item = FindItem( "Alien Disruptor" );
			if(item)
			{
				spread = 140;
				self->client->v_angle[YAW] -= spread;
				drop = Drop_Item (self, item);
				self->client->v_angle[YAW] += spread;
				drop->spawnflags = DROPPED_PLAYER_ITEM;
			}
		}
		if(self->client->pers.inventory[ITEM_INDEX(FindItem("Alien Smartgun"))] == 1)
		{	
			item = FindItem( "Alien Smartgun" );
			if(item)
			{
				spread = 175;
				self->client->v_angle[YAW] -= spread;
				drop = Drop_Item (self, item);
				self->client->v_angle[YAW] += spread;
				drop->spawnflags = DROPPED_PLAYER_ITEM;
			}
		}
	}
	else
	{
		if (!(dmflags->integer & DF_QUAD_DROP))
			quad = false;
		else
			quad = self->client->doubledamage_expiretime > level.time + 1.0f;

		sproing = self->client->sproing_expiretime > level.time + 1.0f;
		haste = self->client->haste_expiretime > level.time + 1.0f;

		if ((item && quad) || (item && haste) || (item && sproing))
			spread = 22.5;
		else
			spread = 0.0;

		if (item)
		{
			self->client->v_angle[YAW] -= spread;
			drop = Drop_Item (self, item);
			self->client->v_angle[YAW] += spread;
			drop->spawnflags = DROPPED_PLAYER_ITEM;
		}

		if (quad)
		{
			self->client->v_angle[YAW] += spread;
			drop = Drop_Item (self, FindItemByClassname ("item_quad"));
			self->client->v_angle[YAW] -= spread;
			drop->spawnflags |= DROPPED_PLAYER_ITEM;

			drop->touch = Touch_Item;
			drop->nextthink = self->client->doubledamage_expiretime;
			drop->think = G_FreeEdict;
		}
		if (sproing && !self->client->resp.powered)
		{
			self->client->v_angle[YAW] += spread;
			drop = Drop_Item (self, FindItemByClassname ("item_sproing"));
			self->client->v_angle[YAW] -= spread;
			drop->spawnflags |= DROPPED_PLAYER_ITEM;

			drop->touch = Touch_Item;
			drop->nextthink = self->client->sproing_expiretime;
			drop->think = G_FreeEdict;
		}
		if (haste && !self->client->resp.powered)
		{
			self->client->v_angle[YAW] += spread;
			drop = Drop_Item (self, FindItemByClassname ("item_haste"));
			self->client->v_angle[YAW] -= spread;
			drop->spawnflags |= DROPPED_PLAYER_ITEM;

			drop->touch = Touch_Item;
			drop->nextthink = self->client->haste_expiretime;
			drop->think = G_FreeEdict;
		}	
	}
}

void Player_ResetPowerups (edict_t *ent)
{
	ent->client->doubledamage_expiretime = 0;
	ent->client->alienforce_expiretime = 0;
	ent->client->haste_expiretime = 0;
	ent->client->sproing_expiretime = 0;
	ent->client->invis_expiretime = 0;
	ent->client->next_regen_time = 0;
}


/*
==================
player_die
==================
*/
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;
	char	*info;
	int	got_vehicle = 0;
	int	number_of_gibs = 0;
	int	gib_effect = EF_GREENGIB;
	int hasFlag = false;
	gitem_t *flag1_item, *flag2_item;
	int mod;

	mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;

	if (self->in_vehicle) 
	{
		Reset_player(self);	//get the player out of the vehicle
		Jet_Explosion(self); //blow that bitch up!
		got_vehicle = 1; //so we know how to handle dropping it
	}

	VectorClear (self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	info = Info_ValueForKey (self->client->pers.userinfo, "skin");

	self->s.modelindex2 = 0;	// remove linked weapon model

	if(ctf->value)
		self->s.modelindex4 = 0;	// remove linked ctf flag

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = level.time + 3.8;

		//go into 3rd person view
		if (deathmatch->value)
			if(!self->is_bot)
				DeathcamStart(self);

		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary (self, inflictor, attacker);
		if(got_vehicle) //special for vehicles
			VehicleDeadDrop(self);
		else 
		{
			if(!excessive->value && !instagib->value && !rocket_arena->value && !insta_rockets->value)
			{
				if(mod == MOD_VIOLATOR)
				{
					int i;
					gitem_t		*it;
					//spew entire inventory out
					for (i=0 ; i<MAX_ITEMS ; i++)
					{
						if (!self->client->pers.inventory[i])
							continue;
						it = GetItemByIndex (i);

						//do not toss things that are not tossable
						if (strcmp (it->pickup_name, "Blaster") == 0)
							continue;
						if (strcmp (it->pickup_name, "Alien Blaster") == 0)
							continue;
						if (strcmp (it->pickup_name, "Violator") == 0)
							continue;
						if (strcmp (it->pickup_name, "Red Flag") == 0)
							continue;
						if (strcmp (it->pickup_name, "Blue Flag") == 0)
							continue;
						Throw_Item (self, it);
					}
					//perhaps throw out some armor and health if the player has a lot of it
				}
				else
					TossClientWeapon (self);
			}
		}

		if(ctf->value) 
		{
			//check to see if they had a flag
			flag1_item = flag2_item = NULL;

			flag1_item = FindItemByClassname("item_flag_red");
			flag2_item = FindItemByClassname("item_flag_blue");

			if (self->client->pers.inventory[ITEM_INDEX(flag1_item)] || self->client->pers.inventory[ITEM_INDEX(flag1_item)])
				hasFlag = true;

			CTFDeadDropFlag(self, attacker);
			if(anticamp->value && meansOfDeath == MOD_SUICIDE && hasFlag) 
			{
				//make campers really pay for hiding flags
				if(self->dmteam == BLUE_TEAM)
					CTFResetFlag(RED_TEAM);
				else
					CTFResetFlag(BLUE_TEAM);
			}
		}

		CTFPlayerResetGrapple(self);

		if (deathmatch->integer && !self->is_bot)
		{
			ACESP_UpdateBots();
			self->client->showscores = false; // override toggle
			Cmd_Score_f( self );
		}

		if (self->health < -40 && attacker &&  attacker->client) 
			PlayerGrantRewardPoints (attacker, 1);
	}

	Player_ResetPowerups (self);

	// clear inventory
	memset(self->client->pers.inventory, 0, sizeof(self->client->pers.inventory));

	if (self->health < -40)
	{	// gib
		self->takedamage	= DAMAGE_NO;
		self->s.modelindex3	= 0;    //remove helmet, if a martian

		if(self->client->chasetoggle == 1)
		{
			/* If deathcam is active, switch client model to nothing */
			self->s.modelindex = 0;
			self->s.effects = 0; 
			self->solid = SOLID_NOT;

			number_of_gibs = DEATH_GIBS_TO_THROW;
		}
		else
		{
			/* No deathcam, handle player's view and model with ThrowClientHead() */
			ThrowClientHead (self, damage);
			number_of_gibs = DEATH_GIBS_TO_THROW - 1;
		}

		if(attacker != NULL)
		{
			if(!instagib->integer && attacker->client)
			{
				if(!attacker->is_bot) 
				{
					// Send Steam stats
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte(TE_DISINTEGRATED);
					gi.unicast (attacker, false);
				}
			}
		}

		if(self->ctype == 0) 
		{	//alien
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEATHFIELD);
			gi.WritePosition (self->s.origin);
			gi.multicast (self->s.origin, MULTICAST_PVS);

			for (n= 0; n < number_of_gibs; n++) {
				if(mod == MOD_R_SPLASH || mod == MOD_ROCKET)
					ThrowGib (self, "models/objects/gibs/mart_gut/tris.iqm", damage, GIB_METALLIC, EF_SHIPEXHAUST);
				else
					ThrowGib (self, "models/objects/gibs/mart_gut/tris.iqm", damage, GIB_METALLIC, EF_GREENGIB);
				ThrowGib (self, "models/objects/debris2/tris.iqm", damage, GIB_METALLIC, 0);
			}
		}
		else if(self->ctype == 2) 
		{	//robot
			gib_effect = 0;
			for (n= 0; n < number_of_gibs; n++) 
			{
				ThrowGib (self, "models/objects/debris3/tris.iqm", damage, GIB_METALLIC, 0);
				ThrowGib (self, "models/objects/debris1/tris.iqm", damage, GIB_METALLIC, 0);
			}
			//blow up too :)
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_ROCKET_EXPLOSION);
			gi.WritePosition (self->s.origin);
			gi.multicast (self->s.origin, MULTICAST_PHS);
		}
		else 
		{	//human
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEATHFIELD2);
			gi.WritePosition (self->s.origin);
			gi.WriteDir (self->s.angles);
			gi.multicast (self->s.origin, MULTICAST_PVS);

			gib_effect = EF_GIB;
			for (n= 0; n < number_of_gibs; n++) 
			{
				if(mod == MOD_R_SPLASH || mod == MOD_ROCKET)
					ThrowGib (self, "models/objects/gibs/sm_meat/tris.iqm", damage, GIB_METALLIC, EF_SHIPEXHAUST);
				else
					ThrowGib (self, "models/objects/gibs/sm_meat/tris.iqm", damage, GIB_METALLIC, EF_GIB);
			}
		}
	}
	else
	{	// normal death
		if (!self->deadflag)
		{
			static int i;
			
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;

			//if a violator death, let's actually use a death frame that spawns a ragoll at the end of the sequence inst
			if(mod == MOD_VIOLATOR)
				i = 2;
			else
			{
				i = (i+1)%3;
				if(i == 2)
					i = 0; //only do this for violator deaths
			}

			switch (i)
			{
			default:
			case 0:
				self->s.frame = FRAME_death501-1;
				self->client->anim_end = FRAME_death518;
				break;
			case 1:
				self->s.frame = FRAME_death601-1;
				self->client->anim_end = FRAME_death620;
				break;
			case 2:
				self->s.frame = FRAME_death401-1;
				self->client->anim_end = FRAME_death422;
				break;

			}
			gi.sound (self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand()%4)+1)), 1, ATTN_NORM, 0);
		}
	}

	gi.sound (self, CHAN_VOICE, gi.soundindex("misc/death.wav"), 1, ATTN_STATIC, 0);

	self->deadflag = DEAD_DEAD;
	self->takedamage	= DAMAGE_NO;

	gi.linkentity (self);
}

//=======================================================================

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant (gclient_t *client)
{
	gitem_t		*item;
	int			queue=0;
	
	if(g_duel->integer) //need to save this off in duel mode.  Potentially dangerous?
		queue = client->pers.queue;

	memset (&client->pers, 0, sizeof(client->pers));

	if(g_duel->integer)
		client->pers.queue = queue;
	
#ifdef ALTERIA
	item = FindItem("Warriorpunch");
#else
	if(!rocket_arena->integer && !g_tactical->integer) 
	{	//gets a violator, unless RA or Tactical
		item = FindItem("Violator");
		client->pers.selected_item = ITEM_INDEX(item);
		client->pers.inventory[client->pers.selected_item] = 1;
		client->pers.weapon = item;
	}

	//mutator - will need to have item
	if(instagib->integer) 
	{
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("cells"))] = g_maxcells->value;
		item = FindItem("Alien Disruptor");
	}
	else if(rocket_arena->integer) 
	{
		client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("rockets"))] = g_maxrockets->value;
		item = FindItem("Rocket Launcher");
	}
	else if (insta_rockets->integer )
	{
		client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("rockets"))] = g_maxrockets->value;
		item = FindItem("Rocket Launcher");
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("cells"))] = g_maxcells->value;
		item = FindItem("Alien Disruptor");
	}
	else 
		item = FindItem("Blaster");
#endif
	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[client->pers.selected_item] = 1;

	client->pers.weapon = item;

	if(excessive->value) 
	{
		//Allow custom health, even in excessive.
		client->pers.health 		= g_spawnhealth->value * 3;
		
#define X(name,itname,base,max,excessivemult)					\
		client->pers.inventory[ITEM_INDEX(FindItem(itname))] = g_max##name->value * excessivemult;
		
		AMMO_TYPES
	
	#undef X

		client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("Chaingun"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("Disruptor"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Smartgun"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Vaporizer"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("Flame Thrower"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("Minderaser"))] = 1;
	} 
	else 
	{
		client->pers.health 		= g_spawnhealth->value;
	}

	if(vampire->value)
		client->pers.max_health = g_maxhealth->value * 2;
	else if(excessive->value)
		client->pers.max_health = g_maxhealth->value * 3;
	else
		client->pers.max_health = g_maxhealth->value;

	if(grapple->value) 
	{
		item = FindItem("Grapple");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
	}

	client->pers.connected = true;
}


void InitClientResp (gclient_t *client)
{
	memset (&client->resp, 0, sizeof(client->resp));
	client->resp.entertime = level.time;
}

/*
==================
SaveClientData

Some information that should be persistant, like health,
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData (void)
{
	int		i;
	edict_t	*ent;

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->inuse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
	}
}

void FetchClientEntData (edict_t *ent)
{
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
static float	PlayersRangeFromSpot (edict_t *spot)
{
	edict_t	*player;
	float	bestplayerdistance;
	vec3_t	v;
	int		n;
	float	playerdistance;


	bestplayerdistance = 9999999;

	for (n = 1; n <= g_maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;


		if (player->health <= 0)
			continue;

		VectorSubtract (spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength (v);

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
static edict_t *SelectRandomDeathmatchSpawnPoint (void)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
	{
		if ( spot1 ) {
			count--;
		}
		if ( spot2 ) {
			count--;
		}
	}

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), "info_player_deathmatch");
		if (spot == spot1 || spot == spot2)
			selection++;
	} while(selection--);

	return spot;
}
/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
static edict_t *SelectRandomCTFSpawnPoint (void)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;
	char	whichteam[32];

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	if(random() < 0.5)
		strcpy(whichteam, "info_player_red");
	else
		strcpy(whichteam, "info_player_blue");

	while ((spot = G_Find (spot, FOFS(classname), whichteam)) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
	{
		if ( spot1 ) {
			count--;
		}
		if ( spot2 ) {
			count--;
		}
	}

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), whichteam);
		if (spot == spot1 || spot == spot2)
			selection++;
	} while(selection--);

	return spot;
}
/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
static edict_t *SelectFarthestDeathmatchSpawnPoint (void)
{
	edict_t	*bestspot;
	float	bestdistance, bestplayerdistance;
	edict_t	*spot;


	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		bestplayerdistance = PlayersRangeFromSpot (spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
	{
		return bestspot;
	}

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = G_Find (NULL, FOFS(classname), "info_player_deathmatch");

	return spot;
}

static edict_t *SelectDeathmatchSpawnPoint (void)
{
	if ( dmflags->integer & DF_SPAWN_FARTHEST)
		return SelectFarthestDeathmatchSpawnPoint ();
	else
		return SelectRandomDeathmatchSpawnPoint ();
}

/*
================
SelectCTFSpawnPoint

go to a ctf point, but NOT the two points closest
to other players
================
*/
static edict_t *SelectCTFSpawnPoint (edict_t *ent)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;
	char	*cname;

	if(g_tactical->value)
	{
		if(ent->ctype == 1)
			cname = "info_player_red";
		else
			cname = "info_player_blue";
	}
	else
	{
		switch (ent->dmteam) 
		{
			case RED_TEAM:
				cname = "info_player_red";
				break;
			case BLUE_TEAM:
				cname = "info_player_blue";
				break;
			case NO_TEAM:
			default:
				return SelectRandomCTFSpawnPoint();
		}
	}

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find (spot, FOFS(classname), cname)) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return SelectRandomDeathmatchSpawnPoint();

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
		count -= 2;

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), cname);
		if (spot == spot1 || spot == spot2)
			selection++;
	} while(selection--);

	return spot;
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
void	SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles)
{
	edict_t	*spot = NULL;

	if (deathmatch->value) 
	{
		if (g_tactical->value || ctf->value || (dmflags->integer & DF_SKINTEAMS)) 
		{			
			spot = SelectCTFSpawnPoint(ent);
			if(!spot)
				spot = SelectDeathmatchSpawnPoint ();
		}
		else 
		{
			spot = SelectDeathmatchSpawnPoint ();
			if(!spot)
				spot = SelectCTFSpawnPoint(ent); //dm on team based maps
		}
	}

	// find a single player start spot
	if (!spot)
	{
		spot = G_Find (spot, FOFS(classname), "info_player_start");
		if (!spot)
			gi.error ("Couldn't find spawn point!");
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);
}

//======================================================================


void InitBodyQue (void)
{
	int		i;
	edict_t	*ent;

	level.body_que = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++)
	{
		ent = G_Spawn();
		ent->classname = "bodyque";
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
static void BodySink (edict_t *ent)
{
	if ( level.time - ent->timestamp > 10.5 ) {
		// the body ques are never actually freed, they are just unlinked
		gi.unlinkentity( ent );
		ent->s.modelindex = 0; //for good measure
		ent->s.modelindex2 = 0;
		ent->s.modelindex3 = 0;
		ent->s.modelindex4 = 0;
		ent->s.effects = 0;
		return;
	}
	ent->nextthink = level.time + .1;
	ent->s.origin[2] -= 1;
	ent->s.effects |= EF_COLOR_SHELL;
	ent->s.renderfx |= RF_SHELL_GREEN;
	ent->solid = SOLID_NOT; //don't gib sinking bodies
}

static void body_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->s.modelindex3 = 0;
	self->s.modelindex4 = 0;

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->s.effects = EF_GIB;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;

	if (self->client)	// bodies in the queue don't have a client anymore
	{
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else
	{
		self->think = NULL;
		self->nextthink = 0;
	}

	gi.linkentity (self);

  }


void CopyToBodyQue (edict_t *ent)
{
	edict_t		*body;

	// grab a body que and cycle to the next one
	body = &g_edicts[g_maxclients->integer + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	gi.unlinkentity (ent);

	gi.unlinkentity (body);
	body->s = ent->s;
	body->s.number = body - g_edicts;

	body->svflags = ent->svflags;

	VectorCopy (ent->mins, body->mins);
	VectorCopy (ent->maxs, body->maxs);
	VectorCopy (ent->absmin, body->absmin);
	VectorCopy (ent->absmax, body->absmax);
	VectorCopy (ent->size, body->size);
	body->solid = SOLID_NOT;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;
	body->die = body_die;
	body->takedamage = DAMAGE_YES;
	body->ctype = ent->ctype;
	body->timestamp = level.time;
	body->nextthink = level.time + 5;
	body->think = BodySink;

	gi.linkentity (body);
}


void respawn (edict_t *self)
{
	if (deathmatch->value)
	{
		//spectator mode
		// spectator's don't leave bodies
		if (self->movetype != MOVETYPE_NOCLIP)
			CopyToBodyQue (self);
		//end spectator mode
		self->svflags &= ~SVF_NOCLIENT;
		
		PutClientInServer (self);

		// add a teleportation effect
		self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		self->client->respawn_time = level.time;

		return;
	}

	// restart the entire server
	gi.AddCommandString ("menu_loadgame\n");
}

//spectator mode

static qboolean veto_spectator_state (edict_t *ent, const char *userinfo, const char **rejmsg)
{
	char	*value;
	int		i, numspec;
	
	value = Info_ValueForKey (userinfo, "spectator");
	
	if (deathmatch->value && *value && strcmp(value, "0")) {

		value = Info_ValueForKey( userinfo, "spectator_password" );

		if (*spectator_password->string &&
			strcmp (spectator_password->string, "none") &&
			strcmp (spectator_password->string, value)) {
			*rejmsg = "Spectator password required or incorrect.";
			return true;
		}

		// count spectators
		for (i = numspec = 0; i < g_maxclients->value; i++)
			if (g_edicts[i+1].inuse && g_edicts[i+1].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			*rejmsg = "Server spectator limit is full.";
			return true;
		}
	} else if (!ent->is_bot) {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if (*password->string && strcmp (password->string, "none") &&
			strcmp (password->string, value)) {
			*rejmsg = "Password required or incorrect.";
			return true;
		}
	}
	
	return false;
}

qboolean player_participating (const edict_t *ent)
{
	return	ent->inuse && ent->client != NULL &&
			ent->client->resp.participation == participation_playing;
}

participation_t player_desired_participation (const edict_t *ent)
{
	if (ent->client->pers.spectator)
		return participation_spectating;
	// in ctf, initially start in chase mode, and allow them to choose a team
	if (TEAM_GAME && ent->dmteam == NO_TEAM)
		return participation_pickingteam;
	if (g_duel->integer && ent->client->pers.queue > 2)
		return participation_duelwaiting;
	return participation_playing;
}

void spectator_respawn (edict_t *ent)
{
	const char *msg;
	
	/* Remove deathcam if changed to spectator after death */
	if (!player_participating (ent) && ent->deadflag)
		DeathcamRemove (ent, "off");

	// clear client on respawn
	ent->client->resp.score = 0;

	ent->svflags &= ~SVF_NOCLIENT;
	ent->is_bot = false;
	PutClientInServer (ent);
	
	switch (ent->client->resp.participation)
	{
		case participation_playing:
			// add a teleportation effect
			gi.WriteByte (svc_muzzleflash);
			gi.WriteShort (ent-g_edicts);
			gi.WriteByte (MZ_LOGIN);
			gi.multicast (ent->s.origin, MULTICAST_PVS);

			// hold in place briefly
			ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
			ent->client->ps.pmove.pm_time = 14;

			msg = g_duel->integer ? "has joined the duel" : "has joined the game";
			break;
		
		case participation_pickingteam:
			msg = "is picking a team";
			break;
		
		case participation_duelwaiting:
			msg = "has joined the duel queue";
			break;
		
		default:
			msg = "has moved to the sidelines";
			break;
	}
	
	safe_bprintf (PRINT_HIGH, "%s %s.\n", ent->client->pers.netname, msg);

	if (sv_botkickthreshold && sv_botkickthreshold->integer || g_duel->integer)
	{ // player entered or left playing field, update for auto botkick
		ACESP_LoadBots( ent );
	}


}
//end spectator mode

//==============================================================

/*
===========
ParseClassFile

Used in tactical mode for setting class items
===========
*/

void ParseClassFile( char *config_file, edict_t *ent )
{
	char full_path[MAX_OSPATH];
	FILE *fp;
	int length;
	char a_string[128];
	char *buffer;
	const char *s;
	size_t result;	

	if ( gi.FullPath( full_path, sizeof(full_path), config_file ) )
	{

		if((fp = fopen(full_path, "rb" )) == NULL)
		{
			return;
		}
		if ( fseek(fp, 0, SEEK_END) )
		{ // seek error
			fclose( fp );
			return;
		}
		if ( (length = ftell(fp)) == (size_t)-1L )
		{ // tell error
			fclose( fp );
			return;
		}
		if ( fseek(fp, 0, SEEK_SET) )
		{ // seek error
			fclose( fp );
			return;
		}
	
		buffer = malloc( length + 1 );
		if ( buffer != NULL )
		{
			buffer[length] = 0;
			result = fread( buffer, length, 1, fp );
			if ( result == 1 )
			{
				s = buffer;

				strcpy( a_string, COM_Parse( &s ) );
				ent->max_health = atoi(a_string);
				strcpy( a_string, COM_Parse( &s ) );
				ent->armor_type = atoi(a_string);
				strcpy( a_string, COM_Parse( &s ) );
				ent->has_bomb = atoi(a_string);
				strcpy( a_string, COM_Parse( &s ) );
				ent->has_detonator = atoi(a_string);
				strcpy( a_string, COM_Parse( &s ) );
				ent->has_minderaser = atoi(a_string);
				strcpy( a_string, COM_Parse( &s ) );
				ent->has_vaporizer = atoi(a_string);

				//note - we may or may not need the ent vars, for now keep them
				if(ent->max_health > 0)
					ent->health = ent->client->pers.max_health = ent->client->pers.health = ent->max_health;
				else
					ent->max_health = 100;

				switch(ent->armor_type)
				{
					default:
					case 0:
						break;
					case 1:
						ent->client->pers.inventory[ITEM_INDEX(FindItem("Jacket Armor"))] += 30;
						break;
					case 2:
						ent->client->pers.inventory[ITEM_INDEX(FindItem("Body Armor"))] += 50;
						break;
					case 3:
						ent->client->pers.inventory[ITEM_INDEX(FindItem("Combat Armor"))] += 100;
						break;
				}

				if(ent->has_minderaser)
				{
					ent->client->pers.inventory[ITEM_INDEX(FindItem("Minderaser"))] = 1;
					ent->client->pers.inventory[ITEM_INDEX(FindItem("seekers"))] = 1;
				}

				if(ent->has_vaporizer)
				{
					ent->client->pers.inventory[ITEM_INDEX(FindItem("Alien Vaporizer"))] = 1;
					ent->client->pers.inventory[ITEM_INDEX(FindItem("slugs"))] = 4;
				}
			}
		
			free( buffer );
		}
		fclose( fp );
	}
	
	return;
}

/*
===========
Respawn_ClassSpecific

Called when a player connects to a server or respawns in
a deathmatch: give the player class-specific items
============
*/
void Respawn_ClassSpecific (edict_t *ent, gclient_t *client)
{
	gitem_t	*item;
	FILE	*file;
	char	modelpath[MAX_OSPATH] = " ";
	int		armor_index;
	
	//check for class file
#ifdef ALTERIA
	ent->ctype = 0; //wizard is default
	sprintf(modelpath, "players/%s/human", ent->charModel);
	Q2_FindFile (modelpath, &file);
	if(file) 
	{	//warrior
		ent->ctype = 1;
		
		ent->health = ent->max_health = client->pers.max_health = client->pers.health = 100;
		armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
		client->pers.inventory[armor_index] += 30;
		client->pers.inventory[ITEM_INDEX(FindItem("Warriorpunch"))] = 1;
		item = FindItem("Warriorpunch");
		client->pers.selected_item = ITEM_INDEX(item);
		client->pers.inventory[client->pers.selected_item] = 1;
		client->pers.weapon = item;
		
		fclose(file);
	}
	else 
	{	//wizard
		
		ent->health = ent->max_health = client->pers.max_health = client->pers.health = 150;
		client->pers.inventory[ITEM_INDEX(FindItem("Wizardpunch"))] = 1;
		//client->pers.inventory[ITEM_INDEX(FindItem("cells"))] = 100; //to to - blue or yellow mana
		item = FindItem("Wizardpunch");
		client->pers.selected_item = ITEM_INDEX(item);
		client->pers.inventory[client->pers.selected_item] = 1;
		client->pers.weapon = item;
	}
#else
	ent->ctype = 0; //alien is default
	sprintf(modelpath, "players/%s/human", ent->charModel);
	Q2_FindFile (modelpath, &file);
	if(file) 
	{ 
		fclose(file);

		//human
		ent->ctype = 1;
		if(g_tactical->integer || (classbased->value && !(rocket_arena->integer || instagib->integer || insta_rockets->value || excessive->value)))
		{				
			if(g_tactical->integer)
			{
				//read class file(tactical only)
				//example:
				//100-150 (health)
				//0-3 (armor type)
				//0-1 (has bomb)
				//0-1 (has detonator)
				//0-1 (has mind eraser)
				//0-1 (has vaporizer)
			
				ParseClassFile(modelpath, ent); 
				if(ent->has_bomb)
				{
					ent->client->pers.inventory[ITEM_INDEX(FindItem("Human Bomb"))] = 1;
					ent->client->pers.inventory[ITEM_INDEX(FindItem("bombs"))] = 1; //tactical note - humans will use same ammo, etc, just different weapons
				}				
				item = FindItem("Blaster");
			}
			else
			{
				ent->health = ent->max_health = client->pers.max_health = client->pers.health = 100;
				armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
				client->pers.inventory[armor_index] += 30;
				
				client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] = 1;
				client->pers.inventory[ITEM_INDEX(FindItem("rockets"))] = 10;

				item = FindItem("Rocket Launcher");
			}
			client->pers.selected_item = ITEM_INDEX(item);
			client->pers.inventory[client->pers.selected_item] = 1;
			if(ent->has_bomb)
			{
				item = FindItem("Human Bomb");
				client->pers.selected_item = ITEM_INDEX(item);
			}
			client->pers.weapon = item;
		}		
		else if(!(rocket_arena->integer || instagib->integer || insta_rockets->value))
		{
			item = FindItem("Blaster");
			client->pers.selected_item = ITEM_INDEX(item);
			client->pers.inventory[client->pers.selected_item] = 1;
			client->pers.weapon = item;
		}
	}
	else 
	{ 
		//robot - not used in tactical - should we kick them out at this point, or just let them continue on the alien team?  
		sprintf(modelpath, "players/%s/robot", ent->charModel);
		Q2_FindFile (modelpath, &file);
		if(file && !g_tactical->integer) 
		{
			ent->ctype = 2;
			if(classbased->value && !(rocket_arena->integer || instagib->integer || insta_rockets->value || excessive->value))
			{
				ent->health = ent->max_health = client->pers.max_health = client->pers.health = 85;
				armor_index = ITEM_INDEX(FindItem("Combat Armor"));
				client->pers.inventory[armor_index] += 175;
			}
			fclose(file);
		
			if(!(rocket_arena->integer || instagib->integer || insta_rockets->value))
			{
				item = FindItem("Blaster");
				client->pers.selected_item = ITEM_INDEX(item);
				client->pers.inventory[client->pers.selected_item] = 1;
				client->pers.weapon = item;
			}
		}
		else
		{ 
			//alien
			ent->ctype = 0;
			if(g_tactical->integer || (classbased->value && !(rocket_arena->integer || instagib->integer || insta_rockets->value || excessive->value)))
			{
				ent->health = ent->max_health = client->pers.max_health = client->pers.health = 150;
				if(g_tactical->integer)
				{
					sprintf(modelpath, "players/%s/alien", ent->charModel);
					Q2_FindFile (modelpath, &file);
					if(file)
					{
						ParseClassFile(modelpath, ent); 		
						if(ent->has_bomb)
						{
							ent->client->pers.inventory[ITEM_INDEX(FindItem("Alien Bomb"))] = 1;
							ent->client->pers.inventory[ITEM_INDEX(FindItem("bombs"))] = 1; //tactical note - humans will use same ammo, etc, just different weapons
						}
					}
					item = FindItem("Blaster");
					client->pers.selected_item = ITEM_INDEX(item);
					client->pers.inventory[client->pers.selected_item] = 0;					
				
					item = FindItem("Alien Blaster");
				}
				else
				{
					client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] = 1;
					client->pers.inventory[ITEM_INDEX(FindItem("cells"))] = 100;
					item = FindItem("Alien Disruptor");
				}
				client->pers.selected_item = ITEM_INDEX(item);
				client->pers.inventory[client->pers.selected_item] = 1;
				if(ent->has_bomb)
				{
						item = FindItem("Alien Bomb");
						client->pers.selected_item = ITEM_INDEX(item);
				}
				client->pers.weapon = item;				
			}
			else if(!(rocket_arena->integer || instagib->integer || insta_rockets->value))
			{
				item = FindItem("Blaster");
				client->pers.selected_item = ITEM_INDEX(item);
				client->pers.inventory[client->pers.selected_item] = 0;	

				item = FindItem("Alien Blaster");
				client->pers.selected_item = ITEM_INDEX(item);
				client->pers.inventory[client->pers.selected_item] = 1;
				client->pers.weapon = item;
			}
		}
	}

	if(g_tactical->integer)
	{
		// let player know what they need to do
		if(ent->has_bomb && !ent->is_bot)
		{
			safe_centerprintf(ent, "Place bomb near enemy base!");
		}
		else if(ent->has_detonator && !ent->is_bot)
			safe_centerprintf(ent, "Walk over bombs to detonate them!");
	}
#endif
}

/*
===========
Respawn_Player_ClearEnt

Called when a player connects to a server or respawns in
a deathmatch: clear entity values
============
*/
void Respawn_Player_ClearEnt (edict_t *ent)
{
	// copy some data from the client to the entity
	FetchClientEntData (ent);

	// clear entity values
	ent->groundentity = NULL;
	ent->client = &game.clients[ent - g_edicts - 1];
	if(g_spawnprotect->value)
		ent->client->spawnprotected = true;
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/martianenforcer/tris.iqm";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->svflags &= ~SVF_DEADMONSTER;

	//vehicles
	ent->in_vehicle = false;
}

/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer (edict_t *ent)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 32};
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i, done;
	client_persistant_t	saved;
	client_respawn_t	resp;
	char	*info;
	char modelpath[MAX_OSPATH] = " ";
	FILE *file;
	char userinfo[MAX_INFO_STRING];

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if(!g_tactical->integer)
		SelectSpawnPoint (ent, spawn_origin, spawn_angles);

	client = ent->client;

	// deathmatch wipes most client data every spawn.
	// NOTE: to implement a single-player game, look at how the code was prior
	// to SVN rev 125.
	// init pers.* variables, save and restore userinfo variables (name, skin)
	resp = client->resp;
	memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
	InitClientPersistant (client);
	if (ent->is_bot)
	{
		// TODO: make ClientUserinfoChanged handle bots correctly.
		int index = ent - g_edicts - 1;
		
		memcpy(client->pers.userinfo, userinfo, MAX_INFO_STRING );

		// set netname from userinfo
		strncpy( client->pers.netname,
				 (Info_ValueForKey( client->pers.userinfo, "name")),
				 sizeof(client->pers.netname)-1);

		// combine name and skin into a configstring
		gi.configstring( CS_PLAYERSKINS+index, va("%s\\%s",
				client->pers.netname,
				(Info_ValueForKey( client->pers.userinfo, "skin"))));

		if (ent->skill < 0)
			ent->client->pers.max_health = ent->client->pers.health = ent->client->pers.max_health / 2;
	}
	else
	{
		ClientUserinfoChanged (ent, userinfo, SPAWN);
	}
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	client->resp = resp;

	Respawn_Player_ClearEnt (ent);
	
	ent->classname = ent->is_bot ? "bot" : "player";

	if (!ent->is_bot)
	{
		// anti-camp (bots use this mechanism for something else)
		ent->suicide_timeout = level.time + 10.0;
		VectorClear (ent->velocity_accum);
		ent->old_velocities_current = -1;
		ent->old_velocities_count = 0;
	}

	VectorCopy (mins, ent->mins);
	VectorCopy (maxs, ent->maxs);
	VectorClear (ent->velocity);
	
	// init playerstate values
	client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
	if (client->ps.fov < 1)
		client->ps.fov = 90;
	else if (client->ps.fov > 160)
		client->ps.fov = 160;

	// clear entity state values
	ent->s.effects = 0;
	ent->s.skinnum = ent - g_edicts - 1;
	ent->s.modelindex = 255;		// will use the skin specified model
	ent->s.modelindex2 = 255;		// custom gun model
	
	info = Info_ValueForKey (client->pers.userinfo, "skin");
	i = 0;
	done = false;
	strcpy (ent->charModel, " ");
	while (!done)
	{
		if ((info[i] == '/') || (info[i] == '\\'))
			done = true;
		ent->charModel[i] = info[i];
		if(i > 62)
			done = true;
		i++;
	}
	ent->charModel[i-1] = 0;

	sprintf(modelpath, "players/%s/helmet.iqm", ent->charModel);
	Q2_FindFile (modelpath, &file); //does a helmet exist?
	if(file) 
	{
		sprintf(modelpath, "players/%s/helmet.iqm", ent->charModel);
		ent->s.modelindex3 = gi.modelindex(modelpath);
		fclose(file);
	}
	else
		ent->s.modelindex3 = 0;

	ent->s.modelindex4 = 0;

	Respawn_ClassSpecific (ent, client);

	// has to be done after determining the class/team - note - we don't care about spawn distances in tactical
	if(g_tactical->integer)
	{
		// note - these teams are currently only used for server info passed - and is redundant to ctype in tactical code for now
		if(ent->ctype == 0)
			ent->dmteam = ALIEN_TEAM;
		if(ent->ctype == 1)
			ent->dmteam = HUMAN_TEAM;

		SelectSpawnPoint (ent, spawn_origin, spawn_angles);	
	}
	
	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

	ent->s.frame = 0;
	VectorCopy (spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy (ent->s.origin, ent->s.old_origin);

	//clear lean		
	client->lean = 0;

	//clear sneaking
	client->sneaking = false;

	// set the delta angle
	for (i=0 ; i<3 ; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy (ent->s.angles, client->ps.viewangles);
	VectorCopy (ent->s.angles, client->v_angle);

	if (ent->is_bot)
		ACESP_SpawnInitializeAI (ent);
	
	// spectator mode (non-bot players only)
	// this should be the only place to modify this variable.
	client->resp.participation = player_desired_participation (ent);
	if (!player_participating (ent))
	{
		// spawn a "ghost"
		client->chase_target = NULL;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		client->ps.gunindex = 0;
		gi.linkentity (ent);

		// clear team affiliation
		ent->dmteam = NO_TEAM;

		// if in team select mode, show scoreboard
		if (client->resp.participation == participation_pickingteam) 
		{
			CTFScoreboardMessage (ent, NULL, false);
			gi.unicast (ent, true);
			ent->teamset = client->showscores = true;
		}

		return;
	}

	if (!KillBox (ent))
	{	// could't spawn in?
	}
	ent->s.event = EV_OTHER_TELEPORT; //to fix "player flash" bug
	gi.linkentity (ent);

	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon (ent);

	client->spawnprotecttime = level.time;

	//unlagged
	G_ResetHistory (ent);
	// and this is as good a time as any to clear the saved state
	client->saved.leveltime = 0;

	//client->ps.pmove.pm_time = 60;

	// set animation to spawn sequence
	ent->state = STATE_STAND;
	ent->s.frame = FRAME_rise1;
	ent->client->anim_end = FRAME_rise16;

	if (!ent->is_bot && skill->integer < 0)
		ent->client->spawnprotecttime += g_spawnprotect->integer * 2;
}

//DUEL MODE
void ClientPlaceInQueue (edict_t *ent)
{
	int		i;
	int		highestpos = 0;

	for (i = 0; i < g_maxclients->value; i++) 
	{
		edict_t *cl_ent = &g_edicts[i+1];
		if (!cl_ent->inuse || !cl_ent->client)
			continue;
		if (game.clients[i].resp.participation == participation_playing ||
			game.clients[i].resp.participation == participation_duelwaiting)
		{
			if (game.clients[i].pers.queue > highestpos) //only count players that are actually in
				highestpos = game.clients[i].pers.queue;
		}
	}
	
	ent->client->pers.queue = highestpos+1;
}
void MoveClientsDownQueue (edict_t *ent)
{
	int i;
	
	if(ent->client)
	{
		if (ent->client->pers.queue == 0)
			return;
	}
	else
		return;

	for (i = 0; i < g_maxclients->value; i++) 
	{ 
		edict_t *cl_ent = &g_edicts[i+1];
		if (!cl_ent->inuse || !cl_ent->client)
			continue;
		if (game.clients[i].resp.participation == participation_playing ||
			game.clients[i].resp.participation == participation_duelwaiting)
		{
			if (game.clients[i].pers.queue > ent->client->pers.queue)
				game.clients[i].pers.queue--;
		}
	}
	
	ent->client->pers.queue = 0;
}
void DemoteDuelLoser (void)
{
	int	i;
	int lowscore;
	int loser = -1;
	
	for (i = 0; i < g_maxclients->integer; i++)
	{
		edict_t *ent = &g_edicts[i+1];
		if (ent->inuse && ent->client && player_participating (ent) &&
			(loser == -1 || game.clients[i].resp.score < lowscore))
		{
			loser = i;
			lowscore = game.clients[i].resp.score;
		}
	}
	
	if(loser != -1)
	{
		MoveClientsDownQueue (&g_edicts[loser+1]);
		ClientPlaceInQueue (&g_edicts[loser+1]);
	}
}
//END DUEL MODE

void PlayerGrantRewardPoints (edict_t *ent, int points_granted)
{
	ent->client->resp.reward_pts += points_granted;

	if (ent->client->resp.reward_pts >= g_reward->integer && !ent->client->resp.powered)
	{
		ent->client->resp.powered = true;
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
	}
}

#define MAX_MOTD_SIZE	500

/** @brief Read the MOTD file and send it to the client
 *
 * Try to open the MOTD file (first from the CVar setting, then from the
 * default location). If it is found, read it and send it to the client.
 *
 * @param ent the entity to send to
 *
 * @return true if the message was found and sent
 */
static qboolean SendMessageOfTheDay( edict_t * ent )
{
	FILE		*file;
	char		name[ MAX_OSPATH ];
	qboolean	found;
	int		size;
	char		motd[ MAX_MOTD_SIZE ];

	found = false;
	if ( motdfile && motdfile->string && motdfile->string[0] ) {
		// look for custom message of the day file
		found = gi.FullPath( name, sizeof( name ),
				motdfile->string );
	}
	if ( !found ) {
		// look for default message of the day file
		found = gi.FullPath( name, sizeof( name ), "motd.txt" );
	}
	if ( !found || (file = fopen( name, "rb" )) == NULL ) {
		// No MOTD at all, or we can't read it
		return false;
	}

	// We successfully opened the file "motd.txt" - read it and close it
	size = fread( motd , 1 , MAX_MOTD_SIZE - 1 , file );
	fclose( file );

	// Make sure what we read is NULL-terminated
	motd[ size ] = 0;

	// If the file did contain data, print to the client
	if ( size ) {
		gi.centerprintf( ent , "%s" , motd );
	}
	return ( size > 0 );
}


/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
deathmatch mode, so clear everything out before starting them.
=====================
*/
static void ClientBeginDeathmatch (edict_t *ent)
{
	G_InitEdict (ent);

	InitClientResp (ent->client);

	ent->dmteam = NO_TEAM;

	// locate ent at a spawn point
	if (player_participating (ent)) //fixes invisible player bugs caused by leftover svf_noclients
		ent->svflags &= ~SVF_NOCLIENT;
	ent->is_bot = false;
	PutClientInServer (ent);

	//kick and blackhole a player in tactical that is not using an authorized character!
	if(g_tactical->integer)
	{
		//we want to actually check their model to be one of the valid ones we use
		if(strcmp("martianenforcer", ent->charModel) && strcmp("martianwarrior", ent->charModel) && strcmp("martianoverlord", ent->charModel)
			&& strcmp("femborg", ent->charModel) && strcmp("enforcer", ent->charModel) && strcmp("commander", ent->charModel))
		{
			if ( ent->is_bot )
			{
				ACESP_KickBot( ent );
			}
			else
			{
				safe_bprintf(PRINT_HIGH, "%s was kicked for using invalid character class!\n", ent->client->pers.netname);
				ClientDisconnect (ent);
			}
		}
	}

	//if duel mode, then check number of existing players.  If more there are already two in the game, force
	//this player to spectator mode, and assign a queue position(we can use the spectator cvar for this)
	else if (g_duel->integer) 
		ClientPlaceInQueue (ent);

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_LOGIN);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	safe_bprintf (PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

	// Send MOTD and enable MOTD protection if necessary
	if ( SendMessageOfTheDay( ent ) ) {
		ent->client->motd_frames = motdforce->integer;
		if ( ent->client->motd_frames < 0 ) {
			ent->client->motd_frames = 0;
		}
	} else {
		ent->client->motd_frames = 0;
	}

	if(g_callvote->value)
		safe_cprintf(ent, PRINT_HIGH, "Call voting is ^2ENABLED\n");
	else
		safe_cprintf(ent, PRINT_HIGH, "Call voting is ^1DISABLED\n");

	if (g_antilagprojectiles->integer)
		safe_cprintf (ent, PRINT_HIGH, "Antilagged projectiles are ^2ENABLED\n");
	else
		safe_cprintf (ent, PRINT_HIGH, "Antilagged projectiles are ^1DISABLED\n");

	//check bots with each player connect
	ACESP_LoadBots( ent );

	// make sure all view stuff is valid
	ClientEndServerFrame (ent);
}


/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin (edict_t *ent)
{
	int		i;

	ent->client = game.clients + (ent - g_edicts - 1);

	for(i = 0; i < 9; i++) {
		ent->client->resp.weapon_shots[i] = 0;
		ent->client->resp.weapon_hits[i] = 0;
	}
	ent->client->kill_streak = 0;

	ent->client->homing_shots = 0;

	ClientBeginDeathmatch (ent);

}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged (edict_t *ent, char *userinfo, int whereFrom)
{
	char	*s;
	edict_t *cl_ent;
	int		playernum;
	int		i, j, k;
	qboolean duplicate, done, copychar;
	char playermodel[MAX_OSPATH] = " ";
	char playerskin[MAX_INFO_STRING] = " ";
	char modelpath[MAX_OSPATH] = " ";
	char slot[4];
	char tmp_playername[PLAYERNAME_SIZE];
	FILE *file;
	teamcensus_t teamcensus;
	const char *rejmsg;
	qboolean desired_spectator;
	
	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		if(ent->dmteam == RED_TEAM)
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/red");
		else if(ent->dmteam == BLUE_TEAM)
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/blue");
		else
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/default");

		ent->s.modelindex3 = gi.modelindex("players/martianenforcer/helmet.iqm");
	}

	if(whereFrom != SPAWN && whereFrom != CONNECT)
		whereFrom = INGAME;

	if(playervote.called && whereFrom == INGAME)
		return; //do not allow people to change info during votes

	// validate and set name
	s = Info_ValueForKey (userinfo, "name");
	if ( s == NULL || *s == 0 )
		s = "Player";
	Q_strncpyz2( tmp_playername, s , sizeof(tmp_playername) );
	ValidatePlayerName( tmp_playername, sizeof(tmp_playername) );
	Q_strncpyz2( ent->client->pers.netname, tmp_playername,
			sizeof(ent->client->pers.netname));
	// end name validate

	// set spectator	
	
	s = Info_ValueForKey (userinfo, "spectator");
	desired_spectator = atoi (s) != 0;
	
	if (veto_spectator_state (ent, userinfo, &rejmsg))
		gi.cprintf (ent, PRINT_HIGH, "%s\n", rejmsg);
	else if (deathmatch->value)
		ent->client->pers.spectator = desired_spectator;
	else
		ent->client->pers.spectator = 0;
	
	if (ent->client->pers.spectator != desired_spectator)
	{
		gi.WriteByte (svc_stufftext);
		gi.WriteString (va ("spectator %d\n", ent->client->pers.spectator));
		gi.unicast(ent, true);
	}
	
	// set skin
	s = Info_ValueForKey (userinfo, "skin");

	// do the team skin check
	if (TEAM_GAME && player_participating (ent))
	{
		copychar = false;
		strcpy( playerskin, " " );
		strcpy( playermodel, " " );
		j = k = 0;
		for ( i = 0; i <= strlen( s ) && i < MAX_OSPATH; i++ )
		{
				if(copychar){
				playerskin[k] = s[i];
				k++;
			}
				else {
				playermodel[j] = s[i];
				j++;
			}
			if ( s[i] == '/' )
				copychar = true;
		}
		playermodel[j] = 0;

		if ( whereFrom != SPAWN && whereFrom != CONNECT && ent->teamset )
		{ // ingame and team is supposed to be set, error check skin
			if ( strcmp( playerskin, "red" ) && strcmp( playerskin, "blue" ) )
			{ // skin error, fix team assignment
				ent->dmteam = NO_TEAM;
				TeamCensus( &teamcensus ); // apply team balancing rules
				ent->dmteam = teamcensus.team_for_real;
				safe_bprintf( PRINT_MEDIUM,
						"Invalid Team Skin!  Assigning to %s Team...\n",
						(ent->dmteam == RED_TEAM ? "Red" : "Blue") );
				ClientBegin( ent ); // this might work
			}
		}
		strcpy( playerskin, (ent->dmteam == RED_TEAM ? "red" : "blue") );

		if(strlen(playermodel) > 32) //something went wrong, or somebody is being malicious
			strcpy(playermodel, "martianenforcer/");
		strcpy(s, playermodel);
		strcat(s, playerskin);
		Info_SetValueForKey (userinfo, "skin", s);
	}
	// end skin check

	// Do tactical checks.
	if(g_tactical->integer)
	{
		copychar = false;
		strcpy( playermodel, " " );
		j = k = 0;
		for ( i = 0; i <= strlen( s ) && i < MAX_OSPATH; i++ )
		{
				if(copychar){
				playerskin[k] = s[i];
				k++;
			}
				else {
				playermodel[j] = s[i];
				j++;
			}
			if ( s[i] == '/' )
				copychar = true;
		}
		playermodel[j] = 0;

		if(ent->ctype == 0)
		{ // alien
			if ( Q_strcasecmp(playermodel, "martianenforcer/")
				 && Q_strcasecmp(playermodel, "martianwarrior/")
				 && Q_strcasecmp(playermodel, "martianoverlord/") )
			{
				//safe_bprintf( PRINT_MEDIUM, "Invalid Character!\n");
				return;
			}
		}
		else if(ent->ctype == 1)
		{ // human
			if ( Q_strcasecmp(playermodel, "commander/")
				&& Q_strcasecmp(playermodel, "enforcer/")
				&& Q_strcasecmp(playermodel, "femborg/") )
			{
				//safe_bprintf( PRINT_MEDIUM, "Invalid Character!\n");
				return;
			}
		}
	}

	playernum = ent-g_edicts-1;

	//check for duplicates(but not on respawns)
	duplicate = false;
	if(whereFrom != SPAWN) 
	{
		for (j=0; j<g_maxclients->value ; j++) 
		{
			cl_ent = g_edicts + 1 + j;
			if (!cl_ent->inuse)
				continue;

			if(!strcmp(ent->client->pers.netname, cl_ent->client->pers.netname)) 
			{
				if(playernum != j)
				{
					duplicate = true;
					break;
				}
			}
		}

		if(duplicate && playernum < 100) 
		{	
			//just paranoia, should never be more than 64

#if defined WIN32_VARIANT
			i = sprintf_s(slot, sizeof(slot), "%i", playernum);
#else
			i = snprintf(slot, sizeof(slot), "%i", playernum);
#endif
			if ( strlen(ent->client->pers.netname) < (PLAYERNAME_SIZE - i) )
			{ //small enough, just add to end
				strcat(ent->client->pers.netname, slot);
			}
			else
			{ //need to lop off end first  TODO: technically, should look for color escapes
				ent->client->pers.netname[ (PLAYERNAME_SIZE-1) - i ] = 0;
				strcat(ent->client->pers.netname, slot);
			}

			Info_SetValueForKey (userinfo, "name", ent->client->pers.netname);
			safe_bprintf(PRINT_HIGH, "Was a duplicate, changing name to %s\n", ent->client->pers.netname);
		}
	}
	// end duplicate check

	// combine name and skin into a configstring
	gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, s) );

	s = Info_ValueForKey (userinfo, "skin");

	i = 0;
	done = false;
	strcpy(playermodel, " ");
	while(!done)
	{
		if((s[i] == '/') || (s[i] == '\\'))
			done = true;
		playermodel[i] = s[i];
		if(i > 62)
			done = true;
		i++;
	}
	playermodel[i-1] = 0;

	sprintf(modelpath, "players/%s/helmet.iqm", playermodel);
	Q2_FindFile (modelpath, &file); //does a helmet exist?
	if(file) 
	{
		sprintf(modelpath, "players/%s/helmet.iqm", playermodel);
		ent->s.modelindex3 = gi.modelindex(modelpath);
		fclose(file);
	}
	else
		ent->s.modelindex3 = 0;

	ent->s.modelindex4 = 0;
	
	// fov
	ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
	if (ent->client->ps.fov < 1)
		ent->client->ps.fov = 90;
	else if (ent->client->ps.fov > 160)
		ent->client->ps.fov = 160;

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (strlen(s))
	{
		ent->client->pers.hand = atoi(s);
	}

	// save off the userinfo in case we want to check something later
	Q_strncpyz2( ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo) );

	if(g_tactical->integer && player_participating(ent) && whereFrom == INGAME)
	{
		PutClientInServer(ent);
	}
}

static void ClientChangeSkin (edict_t *ent)
{
	char *s;
	int  playernum;
	int  i, j, k, copychar;
	char playermodel[MAX_OSPATH] = " ";
	char playerskin[MAX_INFO_STRING] = " ";
	char userinfo[MAX_INFO_STRING];
	char tmp_playername[PLAYERNAME_SIZE];

	//get the userinfo
	memcpy (userinfo, ent->client->pers.userinfo, sizeof(userinfo));

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		if(ent->dmteam == RED_TEAM)
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/red");
		else if(ent->dmteam == BLUE_TEAM)
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/blue");
		else
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/default");

		ent->s.modelindex3 = gi.modelindex("players/martianenforcer/helmet.iqm");
	}

	// set name
	s = Info_ValueForKey (userinfo, "name");

	// fix player name if corrupted
	if ( s != NULL && s[0] )
	{

		Q_strncpyz2( tmp_playername, s, sizeof(tmp_playername ) );
		ValidatePlayerName( tmp_playername, sizeof(tmp_playername ) );
		Q_strncpyz2(ent->client->pers.netname, tmp_playername,
				sizeof(ent->client->pers.netname) );
	}

	// set skin
	s = Info_ValueForKey (userinfo, "skin");

	copychar = false;
	strcpy(playerskin, " ");
	strcpy(playermodel, " ");
	j = k = 0;
	for(i = 0; i <= strlen(s) && i < MAX_OSPATH; i++)
	{
		if(copychar){
			playerskin[k] = s[i];
			k++;
		}
		else {
			playermodel[j] = s[i];
			j++;
		}
		if(s[i] == '/')
			copychar = true;

	}
	playermodel[j] = 0;


	if( ent->dmteam == BLUE_TEAM)
	{
		if ( !ent->is_bot )
			safe_bprintf (PRINT_MEDIUM, "Joined Blue Team...\n");
		strcpy(playerskin, "blue");
	}
	else
	{
		if ( !ent->is_bot )
			safe_bprintf (PRINT_MEDIUM, "Joined Red Team...\n");
		strcpy(playerskin, "red");
	}
	if(strlen(playermodel) > 32) //something went wrong, or somebody is being malicious
		strcpy(playermodel, "martianenforcer/");
	strcpy(s, playermodel);
	strcat(s, playerskin);
	Info_SetValueForKey (userinfo, "skin", s);

	playernum = ent-g_edicts-1;

	// combine name and skin into a configstring
	gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, s) );

	// fov
	ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
	if (ent->client->ps.fov < 1)
		ent->client->ps.fov = 90;
	else if (ent->client->ps.fov > 160)
		ent->client->ps.fov = 160;

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (strlen(s))
	{
		ent->client->pers.hand = atoi(s);
	}

	// save off the userinfo in case we want to check something later
	strncpy (ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo)-1);

}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/

qboolean ClientConnect (edict_t *ent, char *userinfo)
{
	char	*value;
	const char *rejmsg;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if (SV_FilterPacket(value)) {
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	if (veto_spectator_state (ent, userinfo, &rejmsg))
	{
		Info_SetValueForKey(userinfo, "rejmsg", rejmsg);
		return false;
	}

	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == false)
	{
		// clear the respawning variables
		InitClientResp (ent->client);
		if (!game.autosaved || !ent->client->pers.weapon)
			InitClientPersistant (ent->client);
	}

	// for real players in team games, team is to be selected
	ent->dmteam = NO_TEAM;
	ent->teamset = false;

	ClientUserinfoChanged (ent, userinfo, CONNECT);

	if (game.maxclients > 1)
		gi.dprintf ("%s connected\n", ent->client->pers.netname);

	ent->client->pers.connected = true;

	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect (edict_t *ent)
{
	int	playernum, i;

	if (!ent->client)
		return;

	safe_bprintf (PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

    if(ctf->value)
    { //if carrying flag, don't take it with you! no attacker points.
		CTFDeadDropFlag(ent, NULL);
    }

	if(ent->deadflag && ent->client->chasetoggle == 1)
		DeathcamRemove(ent, "off");

	//if in duel mode, we need to bump people down the queue if its the player in game leaving
	if(g_duel->integer) {
		MoveClientsDownQueue(ent);
		if (player_participating (ent)) {
			for (i = 0; i < g_maxclients->value; i++)  //clear scores if player was in duel
				if(g_edicts[i+1].inuse && g_edicts[i+1].client && !g_edicts[i+1].is_bot)
					g_edicts[i+1].client->resp.score = 0;
		}
	}
	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_LOGOUT);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	gi.unlinkentity (ent);
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;

	playernum = ent-g_edicts-1;
	gi.configstring (CS_PLAYERSKINS+playernum, "");

	// if using bot thresholds, put the bot back in(duel always uses them)
	if ( sv_botkickthreshold->integer || g_duel->integer )
	{
		ACESP_LoadBots( ent );
	}

}


//==============================================================


edict_t	*pm_passent;

// pmove doesn't need to know about passent and contentmask
static trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent->health > 0)
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

static unsigned CheckBlock (void *b, int c)
{
	int	v,i;
	v = 0;
	for (i=0 ; i<c ; i++)
		v+= ((byte *)b)[i];
	return v;
}
static void PrintPmove (pmove_t *pm)
{
	unsigned	c1, c2;

	c1 = CheckBlock (&pm->s, sizeof(pm->s));
	c2 = CheckBlock (&pm->cmd, sizeof(pm->cmd));
	Com_Printf ("sv %3i:%i %i\n", pm->cmd.impulse, c1, c2);
}

/*
======
 TeamCensus

 tallies up players and bots in game
 used by auto bot kick, and auto team selection
 implements team balancing rules
======
 */
void TeamCensus( teamcensus_t *teamcensus )
{
	int i;
	int diff;
	int dmteam;
	int red_sum;
	int blue_sum;

	int real_red = 0;
	int real_blue = 0;
	int bots_red = 0;
	int bots_blue = 0;
	int team_for_real = NO_TEAM;
	int team_for_bot = NO_TEAM;

	for ( i = 1; i <= game.maxclients; i++ )
	{ // count only clients that have joined teams
		if ( g_edicts[i].inuse )
		{
			dmteam = g_edicts[i].dmteam;
			if ( g_edicts[i].is_bot )
			{
				if ( dmteam == RED_TEAM )
				{
					++bots_red;
				}
				else if ( dmteam == BLUE_TEAM )
				{
					++bots_blue;
				}
			}
			else
			{
				if ( dmteam == RED_TEAM )
				{
					++real_red;
				}
				else if ( dmteam == BLUE_TEAM )
				{
					++real_blue;
				}
			}
		}
	}
	red_sum = real_red + bots_red;
	blue_sum = real_blue + bots_blue;

	// team balancing rules
	if ( red_sum == blue_sum )
	{ // teams are of equal size
		if ( bots_blue == bots_red )
		{ // with equal number of both real players and bots
			// assign by scoring or random selection
			if ( red_team_score > blue_team_score )
			{ // leader gets the bot
				// real player being a good sport joins the team that is behind
				team_for_bot = RED_TEAM;
				team_for_real = BLUE_TEAM;
			}
			else if ( blue_team_score > red_team_score )
			{
				team_for_bot = BLUE_TEAM;
				team_for_real = RED_TEAM;
			}
			else if ( rand() & 1 )
			{
				team_for_real = team_for_bot = RED_TEAM;
			}
			else
			{
				team_for_real = team_for_bot = BLUE_TEAM;
			}
		}
		else if ( bots_blue > bots_red )
		{ // with more blue bots than red
			// put bot on team with fewer bots (red)
			// real player on team with fewer real players (blue)
			team_for_bot = RED_TEAM;
			team_for_real = BLUE_TEAM;
		}
		else
		{ // with more red bots than blue
			// put bot on team with fewer bots (blue)
			// real player on team with fewer real players (red)
			team_for_bot = BLUE_TEAM;
			team_for_real = RED_TEAM;
		}
	}
	else
	{ // teams are of unequal size
		diff = red_sum - blue_sum;
		if ( diff > 1 )
		{ // red is 2 or more larger
			team_for_real = team_for_bot = BLUE_TEAM;
		}
		else if ( diff < -1 )
		{ // blue is 2 or more larger
			team_for_real = team_for_bot = RED_TEAM;
		}
		else if ( real_blue == real_red )
		{ // with equal numbers of real players
			if ( bots_blue > bots_red )
			{ // blue team is larger by 1 bot
				team_for_real = team_for_bot = RED_TEAM;
			}
			else
			{ // red_team is larger by 1 bot
				team_for_real = team_for_bot = BLUE_TEAM;
			}
		}
		else
		{ // with equal numbers of bots
			if ( real_blue > real_red )
			{ // blue team is larger by 1 real player
				team_for_real = team_for_bot = RED_TEAM;
			}
			else
			{ // red team is larger by 1 real player
				team_for_real = team_for_bot = BLUE_TEAM;
			}
		}
	}

	teamcensus->total = red_sum + blue_sum; //   sum of all
	teamcensus->real = real_red + real_blue; //  sum of real players
	teamcensus->bots = bots_red + bots_blue; //  sum of bots
	teamcensus->red = real_red + bots_red; //    sum of red team members
	teamcensus->blue = real_blue + bots_blue; // sum of blue team members
	teamcensus->real_red = real_red; //   red team players in game
	teamcensus->real_blue = real_blue; // blue team players in game
	teamcensus->bots_red = bots_red; //   red team bots in game
	teamcensus->bots_blue = bots_blue; // blue team bots in game
	teamcensus->team_for_real = team_for_real; // team for real player to join
	teamcensus->team_for_bot = team_for_bot; //   team for bot to join
}


/** @brief Handle clients that are in "team selection" mode
 *
 * @todo this function needs cleaning up - if we set spectator mode to 0, we
 *	can return from the function; if that is done, then we no longer need
 *	to make sure that the client is not in a team in the rest of the
 *	function.
 *
 * @param ent entity of the client
 * @param client the client itself
 * @param ucmd user command from the client
 */
static inline void ClientTeamSelection( edict_t * ent ,
		gclient_t * client ,
		usercmd_t * ucmd )
{
	teamcensus_t teamcensus;

	if (level.time / 2 == ceil (level.time / 2))
	{
		// periodically send "how to join" message
		if ( g_autobalance->integer )
		{
			safe_centerprintf (ent,
				"\n\n\nPress <fire> to join\n"
				"autobalanced team\n");
		}
		else
		{
			safe_centerprintf (ent,
				"\n\n\nPress <fire> to autojoin\n"
				"or <jump> to join BLUE\n"
				"or <crouch> to join RED\n");
		}
	}

	if ( client->latched_buttons & BUTTON_ATTACK )
	{ // <fire> to auto join
		client->latched_buttons = 0;
		if (ent->dmteam == NO_TEAM)
		{
			TeamCensus( &teamcensus ); // apply team balance rules
			ent->dmteam = teamcensus.team_for_real;
			ClientChangeSkin( ent );
		}
	}

	if ( ucmd->upmove >= 10 && ent->dmteam == NO_TEAM )
	{
		if ( !g_autobalance->integer )
		{ // jump to join blue
			ent->dmteam = BLUE_TEAM;
			ClientChangeSkin( ent );
		}
	}
	else if ( ucmd->upmove < 0 && ent->dmteam == NO_TEAM )
	{
		if ( !g_autobalance->integer )
		{ // crouch to join red
			ent->dmteam = RED_TEAM;
			ClientChangeSkin( ent );
		}
	}
	else
	{
		client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}
}


/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client;
	edict_t	*other;
	int		i, j, mostvotes, n_candidates;
	int		map_candidates[4];
	pmove_t	pm;
	qboolean sproing, haste;
	vec3_t addspeed, forward, up, right;

	level.current_entity = ent;
	client = ent->client;

	// If the MOTD is being forced on, decrease frame counter
	// and re-send the file if necessary
	if ( client->motd_frames ) 
	{
		if ( level.time - floor( level.time ) != 0 ) 
		{
			SendMessageOfTheDay( ent );
		}
		client->motd_frames --;
	}

	//unlagged
	client->attackTime = gi.Sys_Milliseconds();

	if (level.intermissiontime)
	{
		client->ps.pmove.pm_type = PM_FREEZE;

		// can exit intermission after 10 seconds, or 20 if map voting enables
		// (voting will only work if g_mapvote wasn't modified during intermission)
		if (g_mapvote->value && ! g_mapvote->modified) 
		{
			//print out results, track winning map
			mostvotes = 0;

			for (j = 0; j < 4; j++) 
			{
				if (votedmap[j].tally > mostvotes)
					mostvotes = votedmap[j].tally;
			}

			if ( g_voterand && g_voterand->value )
			{
				// we're using a random value for the next map
				// if a choice needs to be done
				n_candidates = 0;
				for ( j = 0 ; j < 4 ; j ++ ) 
				{
					if ( votedmap[j].tally < mostvotes )
						continue;
					map_candidates[n_candidates ++] = j;
				}

				j = random() * (n_candidates - 1);
				level.changemap = votedmap[map_candidates[j]].mapname; 
			}
			else
			{
				// "old" voting system, take the first map that
				// has enough votes
				for ( j = 0 ; j < 4 ; j ++ ) 
				{
					i = (j + 1) % 4;
					if ( votedmap[i].tally < mostvotes )
						continue;
				    level.changemap = votedmap[i].mapname;
					break;
				}
			}

			if (level.time > level.intermissiontime + 20.0
				&& (ucmd->buttons & BUTTON_ANY) )
				level.exitintermission = true;
		}
		else 
		{
			if (level.time > level.intermissiontime + 10.0
				&& (ucmd->buttons & BUTTON_ANY) )
				level.exitintermission = true;
		}
		return;
	}
	else if ( g_mapvote && g_mapvote->modified )
	{
		g_mapvote->modified = false;
	}

	pm_passent = ent;

	if (ent->client->chase_target) 
	{
		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	} else 
	{
		// set up for pmove
		memset (&pm, 0, sizeof(pm));

		if (ent->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->s.modelindex != 255 && !(ent->in_vehicle) && !(client->chasetoggle)) //for vehicles or deathcam
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		if(!client->chasetoggle)
		{
			client->ps.pmove.gravity = sv_gravity->value;
		}
		else
		{	/* No gravity to move the deathcam */
			client->ps.pmove.gravity = 0;
		}

		//vehicles
		if ( Jet_Active(ent) )
			Jet_ApplyJet( ent, ucmd );

		pm.s = client->ps.pmove;

		for (i=0 ; i<3 ; i++)
		{
			pm.s.origin[i] = ent->s.origin[i]*8;
			pm.s.velocity[i] = ent->velocity[i]*8;
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
		{
			pm.snapinitial = true;
		}

		if(g_tactical->integer)
		{
			//limit acceleration
			if(ucmd->forwardmove > 200)
				ucmd->forwardmove = 200;
			if(ucmd->forwardmove < -200)
				ucmd->forwardmove = -200;
			if(ucmd->sidemove > 200)
				ucmd->sidemove = 200;
			if(ucmd->sidemove < -200)
				ucmd->sidemove = -200;

			Tactical_tutorial_think(ent);
		}

		//various running movement types
		ent->last_sidemove = ent->sidemove;
		ent->sidemove = 0;
		if(abs(ucmd->sidemove) > abs(ucmd->forwardmove))
		{
			if(ucmd->sidemove > 0)
				ent->sidemove = 1;
			else
				ent->sidemove = -1;
		}
		if(ucmd->forwardmove < 0)
			ent->backpedal = true;
		else
			ent->backpedal = false;

		ucmd->forwardmove *= 1.3;		

		//leaning
		if (ucmd->buttons & BUTTON_LEANRIGHT)
		{
			AngleVectors (client->v_angle, NULL, right, NULL);
			
			if(client->lean < 28)
				client->lean += 2;
			client->lean_angles[ROLL] = client->lean;
			VectorScale (right, client->lean, ent->client->lean_origin);
		}		
		else if (ucmd->buttons & BUTTON_LEANLEFT)
		{
			AngleVectors (client->v_angle, NULL, right, NULL);
			
			if(client->lean > -28)
				client->lean -= 2;
			client->lean_angles[ROLL] = client->lean;
			VectorScale (right, client->lean, ent->client->lean_origin);
		}
		else
		{			
			AngleVectors (client->v_angle, NULL, right, NULL);

			if(client->lean < 0)
				client->lean += 2;
			if(client->lean > 0)
				client->lean -= 2;
			client->lean_angles[ROLL] = client->lean;
			VectorScale (right, client->lean, ent->client->lean_origin);
		}

		//sneaking
		if(ucmd->buttons & BUTTON_SNEAK)
		{
			pm.s.pm_flags |= PMF_SNEAKING;
			ent->s.effects |= EF_SILENT;
		}
		else if(!ent->is_bot)
		{
			pm.s.pm_flags &= ~ PMF_SNEAKING;
			ent->s.effects &= ~ EF_SILENT;
		}
		
		//zooming
		if (ucmd->buttons & BUTTON_ZOOM)
		{
			if(level.time - client->zoomtime > 1.0) //1 second delay between 
			{
				client->zoomed = !client->zoomed;
				client->zoomtime = level.time;
			}
			if(client->zoomed)
			{
				client->ps.fov = 20;
				ent->client->ps.stats[STAT_ZOOMED] = 1;
			}
			else
			{
				client->ps.fov = atoi(Info_ValueForKey(ent->client->pers.userinfo, "fov"));
				ent->client->ps.stats[STAT_ZOOMED] = 0;
				ent->client->kick_angles[0] = -3; //just a little kick to refresh the draw
			}
		}			

		//dodging
		client->dodge = false;	
		if((level.time - client->lastdodge) > 1.0 && ent->groundentity && ucmd->forwardmove == 0 && ucmd->sidemove != 0 && client->moved == false
			&& client->keydown < (10 * 1) && ((level.time - client->lastmovetime) < .15))
		{
			if((ucmd->sidemove < 0 && client->lastsidemove < 0) || (ucmd->sidemove > 0 && client->lastsidemove > 0)) 
			{
				if(ucmd->sidemove > 0)
					client->dodge = 1;
				else
					client->dodge = -1;
				ucmd->upmove += 100;
			}
		}
		if((level.time - client->lastdodge) > 1.0 && ent->groundentity && ucmd->forwardmove != 0 && ucmd->sidemove == 0 && client->moved == false
			&& client->keydown < (10 * 1) && ((level.time - client->lastmovetime) < .15))
		{
			if((ucmd->forwardmove < 0 && client->lastforwardmove < 0) || (ucmd->forwardmove > 0 && client->lastforwardmove > 0)) 
			{
				if(ucmd->forwardmove > 0)
					client->dodge = 2;
				else
					client->dodge = -2;
				ucmd->upmove += 100;
			}
		}			
				
		//checking previous frame's movement
		if(client->moved == true && (ucmd->buttons & BUTTON_ANY))
		{
			client->keydown++;
		}
		else if (ucmd->sidemove != 0 || ucmd->forwardmove != 0)
		{
			client->keydown = 0;
		}

		if(ucmd->sidemove != 0 || ucmd->forwardmove != 0) 
		{
			client->lastmovetime = level.time;
			client->lastsidemove = ucmd->sidemove;
			client->lastforwardmove = ucmd->forwardmove;
			client->moved = true;		
		}
		else //we had a frame with no movement
			client->moved = false;

		pm.cmd = *ucmd;

		pm.trace = PM_trace;	// adds default parms
		pm.pointcontents = gi.pointcontents;

		//joust mode
		if(joustmode->value) 
		{
			if(ent->groundentity)
				client->joustattempts = 0;
			if(pm.cmd.upmove >= 10) 
			{
				client->joustattempts++;
				pm.joustattempts = client->joustattempts;
				if(pm.joustattempts == 10 || pm.joustattempts == 20) 
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
					PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
				}
			}
		}

		// perform a pmove
		gi.Pmove (&pm);

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		for (i=0 ; i<3 ; i++)
		{
			ent->s.origin[i] = pm.s.origin[i]*0.125;
			ent->velocity[i] = pm.s.velocity[i]*0.125;
		}

		//check for a dodge, and peform if true
		if(client->dodge != 0) 
		{
			//check for dodge direction
			if(client->dodge == 2 || client->dodge == -2)
			{
				//was forward or backward
				AngleVectors (ent->s.angles, addspeed, right, up);
				client->dodge /= 2;
			}
			else
			{
				//was sideways
				AngleVectors (ent->s.angles, forward, addspeed, up);
			}

			addspeed[0] *= 300*client->dodge;
			addspeed[1] *= 300*client->dodge;
			
			VectorAdd(ent->velocity, addspeed, ent->velocity);

			//check velocity
			SV_CheckVelocity(ent);

			client->dodge = false;
			client->lastdodge = client->lastmovetime = level.time;
		}

		VectorCopy (pm.mins, ent->mins);
		VectorCopy (pm.maxs, ent->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

		//vehicles
		if ( Jet_Active(ent) )
			if( pm.groundentity ) 		/*are we on ground*/
				if ( Jet_AvoidGround(ent) )	/*then lift us if possible*/
					pm.groundentity = NULL;		/*now we are no longer on ground*/

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
		{
			sproing = client->sproing_expiretime > level.time;
			haste = client->haste_expiretime > level.time;
			if(sproing) 
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("items/sproing.wav"), 1, ATTN_NORM, 0);
				ent->velocity[2] += 400;
			}
			if(haste && ucmd->forwardmove > 0) 
			{
				AngleVectors (ent->s.angles, addspeed, right, up);
				addspeed[0] *= 400;
				addspeed[1] *= 400;
				for(i = 0; i < 2; i++) 
				{
					if(addspeed[i] > 200)
						addspeed[i] = 200;
				}
				VectorAdd(ent->velocity, addspeed, ent->velocity);

				gi.sound(ent, CHAN_VOICE, gi.soundindex("items/haste.wav"), 1, ATTN_NORM, 0);
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_EXPLOSION2);
				gi.WritePosition (ent->s.origin);
				gi.multicast (ent->s.origin, MULTICAST_PVS);
			}
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		}

		ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;
		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->linkcount;

		if (ent->deadflag)
		{
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else
		{
			VectorCopy (pm.viewangles, client->v_angle);
			VectorCopy (pm.viewangles, client->ps.viewangles);
		}		

		if (client->ctf_grapple)
			CTFGrapplePull(client->ctf_grapple);

		gi.linkentity (ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers (ent);

		// touch other objects
		for (i=0 ; i<pm.numtouch ; i++)
		{
			other = pm.touchents[i];
			for (j=0 ; j<i ; j++)
				if (pm.touchents[j] == other)
					break;
			if (j != i)
				continue;	// duplicated
			if (!other->touch)
				continue;
			other->touch (other, ent, NULL, NULL);
		}
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;

	if (player_participating (ent))
	{ // regular (non-spectator) mode
		if (client->latched_buttons & BUTTON_ATTACK)
		{
			if (!client->weapon_thunk)
			{
				client->weapon_thunk = true;
				Think_Weapon (ent);
			}
		}
	}
	else
	{ // spectator mode
		if (TEAM_GAME && client->resp.participation == participation_pickingteam)
		{ // team selection state
			ClientTeamSelection( ent , client , ucmd );
		} /* team selection state */
		else
		{ // regular spectator
			if ( client->latched_buttons & BUTTON_ATTACK )
			{
				client->latched_buttons = 0;
				if ( client->chase_target )
				{
					client->chase_target = NULL;
					client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
				}
				else
				{
					GetChaseTarget( ent );
				}
			}
			if ( ucmd->upmove >= 10 )
			{
				if ( !(client->ps.pmove.pm_flags & PMF_JUMP_HELD) )
				{
					client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
					if ( client->chase_target )
						ChaseNext( ent );
					else
						GetChaseTarget( ent );
				}
			}
			else
			{
				client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
			}
		} /* regular spectator */
	} /* spectator mode */

	// update chase cam if being followed
	for (i = 1; i <= g_maxclients->value; i++) 
	{
		other = g_edicts + i;
		if (other->inuse && other->client->chase_target == ent)
			UpdateChaseCam(other);
	}

	//mutators
	if ((regeneration->integer || excessive->integer) && !ent->deadflag)
	{
		if (ent->health < ent->max_health && client->next_regen_time < level.time)
		{
			// 2 HP every half second until max health
			client->next_regen_time = level.time + 0.5f;
			ent->health += 2;
			if (ent->health > ent->max_health)
				ent->health = ent->max_health;
		}
	}

	//spawn protection has run out
	if (level.time > ent->client->spawnprotecttime + g_spawnprotect->integer)
		ent->client->spawnprotected = false;

	//lose one health every second
	if(g_losehealth->value && !ent->deadflag) 
	{
		if(regeneration->value || excessive->value || vampire->value)
			return;
		if((ent->health > g_losehealth_num->value) && (client->losehealth_frametime < level.time)) 
		{
			client->losehealth_frametime = level.time + 1.0f;
			ent->health-=1;
		}
	}
}


/** @brief Update the anti-camp timeout
 *
 * The anti-camp computation accumulates players' velocities, averages them
 * and updates the "suicide_timeout" field if the length of that average
 * vector is above some threshold.
 *
 * It is controlled by the camptime, ac_frames and ac_threshold CVars.
 */
static inline void _UpdateAntiCamp( edict_t * ent )
{
	int n_frames = ac_frames->integer;
	float thresh;
	vec3_t avg;

	// Make sure we have a valid ac_frames
	if ( n_frames < 1 || n_frames > 100 ) {
		n_frames = G_ANTICAMP_FRAMES;
	}

	if ( ent->old_velocities_count < n_frames ) {
		// Not enough frames yet
		ent->old_velocities_count ++;
		ent->old_velocities_current ++;
	} else {
		// Enough frames - remove oldest known velocity from
		// accumulator
		ent->old_velocities_current = ( ent->old_velocities_current
				+ 1 ) % n_frames;
		VectorSubtract( ent->velocity_accum ,
			ent->old_velocities[ ent->old_velocities_current ] ,
			ent->velocity_accum );
	}

	// Store current velocity into history and add its value to the
	// accumulator
	VectorCopy( ent->velocity ,
			ent->old_velocities[ ent->old_velocities_current ] );
	VectorAdd( ent->velocity_accum , ent->velocity , ent->velocity_accum );
	if ( ent->old_velocities_count < n_frames ) {
		return;
	}

	// Get and adjust speed threshold
	thresh = ac_threshold->value;
	if ( thresh <= 0 || thresh > 500 ) {
		thresh = G_ANTICAMP_THRESHOLD;
	}
	if ( excessive->integer ) {
		thresh *= 1.5;
	}

	// Check average velocity lengths against threshold
	VectorCopy( ent->velocity_accum , avg );
	avg[0] /= n_frames; avg[1] /= n_frames; avg[2] /= n_frames;
	if ( VectorLength( avg ) > thresh ) {
		ent->suicide_timeout = level.time + camptime->integer;
	}

	// Inflict anti-camp damage
	if (ent->suicide_timeout < level.time && ent->takedamage == DAMAGE_AIM
			&& player_participating (ent)) {
		T_Damage (ent, world, world, vec3_origin, ent->s.origin,
				vec3_origin, ent->dmg, 0, DAMAGE_NO_ARMOR,
				MOD_SUICIDE);
		safe_centerprintf(ent, "Anticamp: move or die!\n");
	}
}


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame (edict_t *ent)
{
	gclient_t	*client;
	int			buttonMask;

	if (level.intermissiontime)
		return;

	client = ent->client;

	if (deathmatch->value && !ent->is_bot
			&& player_desired_participation (ent) != client->resp.participation
			&& (level.time - client->respawn_time) >= 5 )
	{
		// enter or exit spectator mode
		spectator_respawn (ent);
	}

	//anti-camp
	// do not apply to godmode cheat or bots.
	// bots have other suicidal tendencies which may (or may not) conflict.
	if ( anticamp->integer && !(ent->flags & FL_GODMODE) && !(ent->is_bot) )
	{
		_UpdateAntiCamp( ent );
	}

	if (!client->weapon_thunk && player_participating (ent))
		Think_Weapon (ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag)
	{
		// wait for any button just going down
		if ( level.time > client->respawn_time)
		{
			// in deathmatch, only wait for attack button
			if (deathmatch->value)
				buttonMask = BUTTON_ATTACK | BUTTON_ATTACK2;
			else
				buttonMask = -1;

			//should probably add in a force respawn option
			if (( client->latched_buttons & buttonMask ) ||
				(deathmatch->value && (dmflags->integer & DF_FORCE_RESPAWN) ) )
			{

				if(!ent->is_bot)
					DeathcamRemove (ent, "off");

				respawn(ent);
				client->latched_buttons = 0;
			}
		}
		return;
	}

	// add player trail so monsters can follow
	if (!deathmatch->value)
		if (!visible (ent, PlayerTrail_LastSpot() ) )
			PlayerTrail_Add (ent->s.old_origin);

	client->latched_buttons = 0;
}

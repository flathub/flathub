/*
Copyright (C) 20?? COR Entertainment, LLC.

Copyright (C) 1998 by "Attila." Based on the email address provided, Attila is
likely the pseudonym of Jens Bohnwagner, but this isn't certain. Original
code: http://www.quakewiki.net/archives/qdevels/quake2/1_2_98.html

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


/*we get silly velocity-effects when we are on ground and try to
  accelerate, so lift us a little bit if possible*/
qboolean Jet_AvoidGround( edict_t *ent )
{
	vec3_t		new_origin;
	trace_t	trace;
	qboolean	success;

	/*Check if there is enough room above us before we change origin[2]*/
	new_origin[0] = ent->s.origin[0];
	new_origin[1] = ent->s.origin[1];
	new_origin[2] = ent->s.origin[2] + 0.5;
	trace = gi.trace( ent->s.origin, ent->mins, ent->maxs, new_origin, ent, MASK_PLAYERSOLID );

	if ( (success=(trace.plane.normal[2]) == 0 ) )	/*no ceiling?*/
		ent->s.origin[2] += 0.5;			/*then make sure off ground*/

	return success;
}

/*This function returns true if the jet is activated
  (surprise, surprise)*/
qboolean Jet_Active( edict_t *ent )
{
	return ( ent->client->Jet_framenum >= level.framenum );
}

/*If a player dies with activated jetpack this function will be called
  and produces a little explosion*/
void Jet_Explosion( edict_t *ent )
{
	gi.WriteByte( svc_temp_entity );
	gi.WriteByte( TE_EXPLOSION1 );   /*TE_EXPLOSION2 is possible too*/
	gi.WritePosition( ent->s.origin );
	gi.multicast( ent->s.origin, MULTICAST_PVS );
}

/*The lifting effect is done through changing the origin, it
  gives the best results. Of course its a little dangerous because
  if we dont take care, we can move into solid*/
void Jet_ApplyLifting( edict_t *ent )
{
	float		delta;
	vec3_t	new_origin;
	trace_t	trace;
	int 		time = 24*TENFPS/FRAMETIME;     /*must be >0, time/10 = time in sec for a
                                 complete cycle (up/down)*/
	float		amplitude = 2.0/(TENFPS/FRAMETIME);

	/*calculate the z-distance to lift in this step*/
	delta = sin( (float)((level.framenum%time)*(360/time))/180*M_PI ) * amplitude;
	delta = (float)((int)(delta*8))/8; /*round to multiples of 0.125*/

	VectorCopy( ent->s.origin, new_origin );
	new_origin[2] += delta;

	if( VectorLength(ent->velocity) == 0 )
	{
		/*i dont know the reason yet, but there is some floating so we
		have to compensate that here (only if there is no velocity left)*/
		new_origin[0] -= 0.125;
		new_origin[1] -= 0.125;
		new_origin[2] -= 0.125;
	}

	/*before we change origin, its important to check that we dont go
	into solid*/
	trace = gi.trace( ent->s.origin, ent->mins, ent->maxs, new_origin, ent, MASK_PLAYERSOLID );
	if ( trace.plane.normal[2] == 0 )
		VectorCopy( new_origin, ent->s.origin );
}

void Jet_ApplyEffects( edict_t *ent, vec3_t forward, vec3_t right )
{
	vec3_t exhaust, distance, dir;

	VectorSet(dir, 0, 0, -1);

	gi.sound (ent, CHAN_AUTO, gi.soundindex("weapons/grenlx1a.wav"), 0.9, ATTN_NORM, 0);

	//add smoke effect from jets
	VectorSet(distance, -16, -32, 0);
	G_ProjectSource (ent->s.origin, distance, forward, right, exhaust);
			
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_JETEXHAUST);
	gi.WritePosition (exhaust);
	gi.WriteDir (dir);
	gi.multicast (exhaust, MULTICAST_PHS);

	VectorSet(distance, -32, 0, 0);
	G_ProjectSource (ent->s.origin, distance, forward, right, exhaust);
			
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_JETEXHAUST);
	gi.WritePosition (exhaust);
	gi.WriteDir (dir);
	gi.multicast (exhaust, MULTICAST_PHS);
}

/*if the angle of the velocity vector is different to the viewing
	angle (flying curves or stepping left/right) we get a dotproduct
	which is here used for rolling*/
void Jet_ApplyRolling( edict_t *ent, vec3_t right )
{
	float roll,
        value = 0.05,
        sign = -1;    /*set this to +1 if you want to roll contrariwise*/

	roll = DotProduct( ent->velocity, right ) * value * sign;

	ent->client->jet_angles[ROLL] = roll;
}


/*Now for the main movement code. The steering is a lot like in water, that
  means your viewing direction is your moving direction. You have three
  direction Boosters: the big Main Booster and the smaller up-down and
  left-right Boosters.
  There are only 2 adds to the code of the first tutorial: the Jet_next_think
  and the rolling.
  The other modifications results in the use of the built-in quake functions,
  there is no change in moving behavior (reinventing the wheel is a lot of
  "fun" and a BIG waste of time ;-))*/
void Jet_ApplyJet( edict_t *ent, usercmd_t *ucmd )
{
	float	direction;
	vec3_t acc;
	vec3_t forward, right;
	int    i;
  
	/*clear gravity so we dont have to compensate it with the Boosters*/
	ent->client->ps.pmove.gravity = 0;
 
	/*calculate the direction vectors dependent on viewing direction
    (length of the vectors forward/right is always 1, the coordinates of
    the vectors are values of how much youre looking in a specific direction
    [if youre looking up to the top, the x/y values are nearly 0 the
    z value is nearly 1])*/
	AngleVectors( ent->client->v_angle, forward, right, NULL );

	/*Run jet only 10 times a second so movement dont depends on fps
    because ClientThink is called as often as possible
    (fps<10 still is a problem ?)*/
	if ( ent->client->Jet_next_think <= level.framenum )
	{
		ent->client->Jet_next_think = level.framenum + 1;

		/*clear acceleration-vector*/
		VectorClear( acc );

		/*if we are moving forward or backward add MainBooster acceleration
		(60)*/
		if ( ucmd->forwardmove )
		{
			/*are we accelerating backward or forward?*/
			direction = (ucmd->forwardmove<0) ? -1.0 : 1.0;

			/*add the acceleration for each direction*/
			acc[0] += direction * forward[0] * 60;
			acc[1] += direction * forward[1] * 60;
			acc[2] += direction * forward[2] * 60;
		}

		/*if we sidestep add Left-Right-Booster acceleration (40)*/
		if ( ucmd->sidemove )
		{
			/*are we accelerating left or right*/
			direction = (ucmd->sidemove<0) ? -1.0 : 1.0;

			/*add only to x and y acceleration*/
			acc[0] += right[0] * direction * 40;
			acc[1] += right[1] * direction * 40;
		}

		/*if we crouch or jump add Up-Down-Booster acceleration (30)*/
		if ( ucmd->upmove )
		{
			acc[2] += ucmd->upmove > 0 ? 30 : -30;
			if(ucmd->upmove > 0 && !ent->deadflag)
			{
				int nPeriod = 6*TENFPS/FRAMETIME;
				if ( ((int)ent->client->Jet_remaining % nPeriod) == 0 ) 
				{					
					Jet_ApplyEffects( ent, forward, right );
				}
			}
		}

		/*now apply some friction dependent on velocity (higher velocity results
		in higher friction), without acceleration this will reduce the velocity
		to 0 in a few steps*/
		ent->velocity[0] += -(ent->velocity[0]/6.0);
		ent->velocity[1] += -(ent->velocity[1]/6.0);
		ent->velocity[2] += -(ent->velocity[2]/7.0);

		/*then accelerate with the calculated values. If the new acceleration for
		a direction is smaller than an earlier, the friction will reduce the speed
		in that direction to the new value in a few steps, so if youre flying
		curves or around corners youre floating a little bit in the old direction*/
		VectorAdd( ent->velocity, acc, ent->velocity );

		/*round velocitys (is this necessary?)*/
		ent->velocity[0] = (float)((int)(ent->velocity[0]*8))/8;
		ent->velocity[1] = (float)((int)(ent->velocity[1]*8))/8;
		ent->velocity[2] = (float)((int)(ent->velocity[2]*8))/8;

		/*Bound velocitys so that friction and acceleration dont need to be
		synced on maxvelocitys*/
		for ( i=0 ; i<2 ; i++) /*allow z-velocity to be greater*/
		{
			if (ent->velocity[i] > 300)
				ent->velocity[i] = 300;
			else if (ent->velocity[i] < -300)
				ent->velocity[i] = -300;
		}

		/*add some gentle up and down when idle (not accelerating)*/
		if( VectorLength(acc) == 0 )
			Jet_ApplyLifting( ent );
	}

	/*add rolling when we fly curves or boost left/right*/
	Jet_ApplyRolling( ent, right );
}

void Use_Jet ( edict_t *ent)
{
    /*jetpack in inventory but no fuel time? must be one of the
      give all/give jetpack cheats, so put fuel in*/
    if ( ent->client->Jet_remaining == 0 )
      ent->client->Jet_remaining = 500*TENFPS/FRAMETIME;

    if ( Jet_Active(ent) )
      ent->client->Jet_framenum = 0;
    else
      ent->client->Jet_framenum = level.framenum + ent->client->Jet_remaining;

    gi.sound( ent, CHAN_ITEM, gi.soundindex("vehicles/got_in.wav"), 0.8, ATTN_NORM, 0 );
}

static void VehicleThink(edict_t *ent)
{
	ent->nextthink = level.time + TENFPS;
}

float jetpackTime;

void SpawnJetpack(edict_t *ent)
{
	edict_t *jetpack, *cl_ent;
	int i;

	for (i = 0; i < g_maxclients->integer; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->is_bot)
			continue;
		safe_centerprintf(cl_ent, "A Jetpack has spawned!\n");
	}

	//to do - play level wide klaxxon 
	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "misc/jetpackspawn.wav" ), 1, ATTN_NONE, 0 );

	jetpack = G_Spawn();
	VectorCopy(ent->s.origin, jetpack->s.origin);
	jetpack->spawnflags = DROPPED_PLAYER_ITEM;
	jetpack->model = "vehicles/jetpack/tris.iqm";
	jetpack->classname = "item_jetpack";
	jetpack->item = FindItem ("Jetpack");
	jetpack->s.effects = jetpack->item->world_model_flags;
	jetpack->s.effects |= EF_ROTATE;
	jetpack->s.renderfx = RF_GLOW;
	VectorSet (jetpack->mins, -15, -15, -15);
	VectorSet (jetpack->maxs, 15, 15, 15);
	gi.setmodel (jetpack, jetpack->item->world_model);
	jetpack->solid = SOLID_TRIGGER;
	jetpack->health = 100;
	jetpack->movetype = MOVETYPE_TOSS;
	jetpack->touch = Touch_Item;
	jetpack->owner = NULL;

	SetRespawn (ent, 1000000); //huge delay until jetpack is picked up from pad.			
	jetpack->replaced_weapon = ent; //remember this entity

	jetpack->nextthink = level.time + TENFPS;
	jetpack->think = VehicleThink;

	jetpackTime = level.time;
}

void VehicleSetup (edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	//no jetpacks in CTF(conflicts with carrying a flag model)
	if(ctf->integer)
		return;

	VectorSet (ent->mins, -15, -15, -15);
	VectorSet (ent->maxs, 15, 15, 15);

	if (ent->model)
		gi.setmodel (ent, ent->model);
	else
		gi.setmodel (ent, ent->item->world_model);

	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->s.effects |= EF_ROTATE;
	ent->touch = Touch_Item;

	v = tv(0,0,-128);
	VectorAdd (ent->s.origin, v, dest);

	tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.dprintf ("VehicleSetup: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	gi.linkentity (ent);

	ent->nextthink = level.time + TENFPS;
	ent->think = VehicleThink;
}

static void VehicleDropTouch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	//owner (who dropped us) can't touch for two secs
	if (other == ent->owner &&
		ent->nextthink - level.time > 2)
		return;

	Touch_Item (ent, other, plane, surf);
}

static void VehicleDropThink(edict_t *ent)
{
	//let it hang around for awhile
	ent->nextthink = level.time + 20;
	ent->think = G_FreeEdict;
}

void VehicleDeadDrop(edict_t *self)
{
	edict_t *dropped;
	gitem_t *vehicle;

	dropped = NULL;

	vehicle = FindItemByClassname("item_jetpack");

	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) {
		dropped = Drop_Item(self, vehicle);
		self->client->pers.inventory[ITEM_INDEX(vehicle)] = 0;
		safe_bprintf(PRINT_HIGH, "Jetpack is abandoned!\n");
	}

	if (dropped) {
		dropped->think = VehicleDropThink;
		dropped->nextthink = level.time + 5;
		dropped->touch = VehicleDropTouch;
		dropped->s.frame = 0;
		dropped->s.effects |= EF_ROTATE;
		return;
	}
}
void Reset_player(edict_t *ent)
{
	//set everything back
	ent->client->Jet_remaining = 0;
	ent->client->Jet_framenum = 0;
	VectorClear(ent->client->jet_angles);
	ent->s.modelindex4 = 0;
	ent->in_vehicle = false;
}

void Leave_vehicle(edict_t *ent, gitem_t *item)
{
	gi.sound( ent, CHAN_ITEM, gi.soundindex("vehicles/got_in.wav"), 0.8, ATTN_NORM, 0 );	

	Reset_player(ent);
	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;
	
	Drop_Item (ent, item);

	safe_bprintf(PRINT_HIGH, "Jetpack has been dropped!\n");
}

qboolean Get_in_vehicle (edict_t *ent, edict_t *other)
{
	gitem_t *vehicle;

	vehicle = NULL;

	if(other->in_vehicle) //already in a vehicle
		return false;

	vehicle = FindItemByClassname(ent->classname);

	//put him in the vehicle
	if(!strcmp(ent->classname, "item_jetpack")) {
		other->s.modelindex4 = gi.modelindex("vehicles/jetpack/tris.iqm");
	}
	
	other->in_vehicle = true;
	other->client->Jet_remaining = 500*TENFPS/FRAMETIME;

	other->s.origin[2] +=24;

	other->client->pers.inventory[ITEM_INDEX(vehicle)] = 1;

	if (!(ent->spawnflags & DROPPED_ITEM)) {
			//if aoa, free this edict
		if(all_out_assault->integer)
		{
			ent->think = G_FreeEdict;
		}
		else
			SetRespawn (ent, 60);
	}

	Use_Jet(other);

	ent->owner = other;

	return true;
}

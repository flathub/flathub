/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 1998 Steve Yeager
Copyright (C) 2010 COR Entertainment, LLC.

See below for Steve Yeager's original copyright notice.
Modified to GPL in 2002.

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
///////////////////////////////////////////////////////////////////////
//
//  ACE - Quake II Bot Base Code
//
//  Version 1.0
//
//  This file is Copyright(c), Steve Yeager 1998, All Rights Reserved
//
//
//	All other files are Copyright(c) Id Software, Inc.
//
//	Please see liscense.txt in the source directory for the copyright
//	information regarding those files belonging to Id Software, Inc.
//
//	Should you decide to release a modified version of ACE, you MUST
//	include the following text (minus the BEGIN and END lines) in the
//	documentation for your modification.
//
//	--- BEGIN ---
//
//	The ACE Bot is a product of Steve Yeager, and is available from
//	the ACE Bot homepage, at http://www.axionfx.com/ace.
//
//	This program is a modification of the ACE Bot, and is therefore
//	in NO WAY supported by Steve Yeager.

//	This program MUST NOT be sold in ANY form. If you have paid for
//	this product, you should contact Steve Yeager immediately, via
//	the ACE Bot homepage.
//
//	--- END ---
//
//	I, Steve Yeager, hold no responsibility for any harm caused by the
//	use of this source code, especially to small children and animals.
//  It is provided as-is with no implied warranty or support.
//
//  I also wish to thank and acknowledge the great work of others
//  that has helped me to develop this code.
//
//  John Cricket    - For ideas and swapping code.
//  Ryan Feltrin    - For ideas and swapping code.
//  SABIN           - For showing how to do true client based movement.
//  BotEpidemic     - For keeping us up to date.
//  Telefragged.com - For giving ACE a home.
//  Microsoft       - For giving us such a wonderful crash free OS.
//  id              - Need I say more.
//
//  And to all the other testers, pathers, and players and people
//  who I can't remember who the heck they were, but helped out.
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//  acebot_movement.c - This file contains all of the
//                      movement routines for the ACE bot
//
///////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game/g_local.h"
#include "acebot.h"

///////////////////////////////////////////////////////////////////////
// Checks if bot can move (really just checking the ground)
// Also, this is not a real accurate check, but does a
// pretty good job and looks for lava/slime.
///////////////////////////////////////////////////////////////////////
qboolean ACEMV_CanMove(edict_t *self, int direction)
{
	vec3_t forward, right;
	vec3_t offset,start,end;
	vec3_t angles;
	trace_t tr;
	gitem_t *vehicle;

	vehicle = FindItemByClassname("item_jetpack");

	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) {
		return true; // yup, can move, we are in an air vehicle
	}
	
	// Now check to see if move will move us off an edge
	VectorCopy(self->s.angles,angles);

	if(direction == MOVE_LEFT)
		angles[1] += 90;
	else if(direction == MOVE_RIGHT)
		angles[1] -= 90;
	else if(direction == MOVE_BACK)
		angles[1] -=180;

	// Set up the vectors
	AngleVectors (angles, forward, right, NULL);

	VectorSet(offset, 36, 0, 24);
	G_ProjectSource (self->s.origin, offset, forward, right, start);

	VectorSet(offset, 36, 0, -400);
	G_ProjectSource (self->s.origin, offset, forward, right, end);

	tr = gi.trace(start, NULL, NULL, end, self,
			(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_WINDOW));
	if(tr.fraction > 0.3 || (tr.contents & (CONTENTS_LAVA|CONTENTS_SLIME)) )
	{
		if(debug_mode)
			debug_printf("%s: move blocked\n",self->client->pers.netname);
		if(self->groundentity)
			self->s.angles[YAW] += random() * 180 - 90; //blocked, so try something else
		return false;
	}

	return true; // yup, can move
}

static int ACEMV_GetWalkSpeed (edict_t *self)
{
	if (self->skill < 0)
		return 150;
	if (g_tactical->integer)
		return 200;
	return 400;
}

///////////////////////////////////////////////////////////////////////
// Handle special cases of crouch/jump
//
// If the move is resolved here, this function returns
// true.
///////////////////////////////////////////////////////////////////////
qboolean ACEMV_SpecialMove(edict_t *self, usercmd_t *ucmd)
{
	vec3_t dir,forward,right,start,end,offset;
	vec3_t top;
	trace_t tr;
	short mSpeed = ACEMV_GetWalkSpeed (self);

	// Get current direction
	VectorCopy(self->client->ps.viewangles,dir);
	dir[YAW] = self->s.angles[YAW];
	AngleVectors (dir, forward, right, NULL);

	VectorSet(offset, 18, 0, 0);
	G_ProjectSource (self->s.origin, offset, forward, right, start);
	offset[0] += 18;
	G_ProjectSource (self->s.origin, offset, forward, right, end);

	// trace it
	start[2] += 18; // so they are not jumping all the time
	end[2] += 18;
	tr = gi.trace (start, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);

	if(tr.allsolid)
	{
		// Check for crouching
		start[2] -= 14;
		end[2] -= 14;

		// Set up for crouching check
		VectorCopy(self->maxs,top);
		top[2] = 0.0; // crouching height
		tr = gi.trace (start, self->mins, top, end, self, MASK_PLAYERSOLID);

		// Crouch
		if(!tr.allsolid)
		{
			if(ACEMV_CanMove(self, MOVE_FORWARD))
			{
				ucmd->forwardmove = mSpeed;
				self->backpedal = false;
			}
			else if(ACEMV_CanMove(self, MOVE_BACK))
			{
				ucmd->forwardmove = -mSpeed;
			}

			ucmd->upmove = -400;
			return true;
		}

		// Check for jump
		start[2] += 32;
		end[2] += 32;
		tr = gi.trace (start, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);

		if(!tr.allsolid)
		{
			if(ACEMV_CanMove(self, MOVE_FORWARD))
			{
				ucmd->forwardmove = mSpeed;
				self->backpedal = false;
			}
			else if(ACEMV_CanMove(self, MOVE_BACK))
			{
				ucmd->forwardmove = -mSpeed;
			}
			ucmd->upmove = 400;
			return true;
		}
	}

	return false; // We did not resolve a move here
}


///////////////////////////////////////////////////////////////////////
// Checks for obstructions in front of bot
//
// This is a function I created origianlly for ACE that
// tries to help steer the bot around obstructions.
//
// If the move is resolved here, this function returns true.
///////////////////////////////////////////////////////////////////////
qboolean ACEMV_CheckEyes(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  forward, right;
	vec3_t  leftstart, rightstart,focalpoint;
	vec3_t  upstart,upend;
	vec3_t  dir,offset;
	trace_t traceRight,traceLeft,traceUp, traceFront; // for eyesight
	short mSpeed = ACEMV_GetWalkSpeed (self);
	
	// Get current angle and set up "eyes"

/* make sure bot's "eyes" are straight ahead
 * something is going wrong after rocket jump, and "eyes" are always down
 * and bot runs in circle.
 */
	self->s.angles[PITCH] = 0.0f;

	VectorCopy(self->s.angles,dir);
	AngleVectors (dir, forward, right, NULL);

	// Let them move to targets by walls
	if(!self->movetarget)
		VectorSet(offset,200,0,4); // focalpoint
	else
		VectorSet(offset,36,0,4); // focalpoint

	G_ProjectSource (self->s.origin, offset, forward, right, focalpoint);

	// Check from self to focalpoint
	// Ladder code
	VectorSet(offset,36,0,0); // set as high as possible
	G_ProjectSource (self->s.origin, offset, forward, right, upend);
	traceFront = gi.trace(self->s.origin, self->mins, self->maxs, upend, self, BOTMASK_OPAQUE);

	if ( traceFront.contents & CONTENTS_LADDER )
	{
		ucmd->upmove = 400;
		if(ACEMV_CanMove(self, MOVE_FORWARD))
		{
			ucmd->forwardmove = mSpeed;
			self->backpedal = false;
		}
		return true;
	}

	// If this check fails we need to continue on with more detailed checks
	if ( traceFront.fraction >= 1.0f )
	{
		if ( ACEMV_CanMove( self, MOVE_FORWARD ) )
		{
			ucmd->forwardmove = mSpeed; //only try forward, bot should be looking to move in direction of eyes
			self->backpedal = false;
		}
		return true;
	}

	VectorSet(offset, 0, 18, 4);
	G_ProjectSource (self->s.origin, offset, forward, right, leftstart);

	offset[1] -= 36; // want to make sure this is correct
	//VectorSet(offset, 0, -18, 4);
	G_ProjectSource (self->s.origin, offset, forward, right, rightstart);

	traceRight = gi.trace(rightstart, NULL, NULL, focalpoint, self, BOTMASK_OPAQUE);
	traceLeft = gi.trace(leftstart, NULL, NULL, focalpoint, self, BOTMASK_OPAQUE);

	// Wall checking code, this will degenerate progressivly so the least cost
	// check will be done first.

	// If open space move ok
	if(traceRight.fraction != 1 || traceLeft.fraction != 1 || strcmp(traceLeft.ent->classname,"func_door")!=0)
	{
		// Special uppoint logic to check for slopes/stairs/jumping etc.
		VectorSet(offset, 0, 18, 24);
		G_ProjectSource (self->s.origin, offset, forward, right, upstart);

		VectorSet(offset,0,0,200); // scan for height above head
		G_ProjectSource (self->s.origin, offset, forward, right, upend);
		traceUp = gi.trace(upstart, NULL, NULL, upend, self, BOTMASK_OPAQUE);

		VectorSet(offset,200,0,200*traceUp.fraction-5); // set as high as possible
		G_ProjectSource (self->s.origin, offset, forward, right, upend);
		traceUp = gi.trace(upstart, NULL, NULL, upend, self, BOTMASK_OPAQUE);

		// If the upper trace is not open, we need to turn.
		if(traceUp.fraction < 1.0f )
		{
			if(traceRight.fraction > traceLeft.fraction)
				self->s.angles[YAW] += (1.0 - traceLeft.fraction) * 45.0;
			else
				self->s.angles[YAW] += -(1.0 - traceRight.fraction) * 45.0;
			if(ACEMV_CanMove(self, MOVE_FORWARD))
			{
				ucmd->forwardmove = mSpeed;
				self->backpedal = false;
			}
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////
// Make the change in angles a little more gradual, not so snappy
// Subtle, but noticeable.
//
// Modified from the original id ChangeYaw code...
///////////////////////////////////////////////////////////////////////
void ACEMV_ChangeBotAngle (edict_t *ent)
{
	float	ideal_yaw;
	float   ideal_pitch;
	float	current_yaw;
	float   current_pitch;
	float	move;
	float	speed;
	vec3_t  ideal_angle;

	// Normalize the move angle first
	VectorNormalize(ent->move_vector);

	current_yaw = anglemod(ent->s.angles[YAW]);
	current_pitch = anglemod(ent->s.angles[PITCH]);

	vectoangles (ent->move_vector, ideal_angle);

	ideal_yaw = anglemod(ideal_angle[YAW]);
	ideal_pitch = anglemod(ideal_angle[PITCH]);

	// Yaw
	if (current_yaw != ideal_yaw)
	{
		move = ideal_yaw - current_yaw;
		speed = ent->yaw_speed;
		if (ideal_yaw > current_yaw)
		{
			if (move >= 180.0f)
				move = move - 360.0f;
		}
		else
		{
			if (move <= -180.0f)
				move = move + 360.0f;
		}
		if (move > 0.0f)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent->s.angles[YAW] = anglemod (current_yaw + move);
	}

	// Pitch
	if (current_pitch != ideal_pitch)
	{
		move = ideal_pitch - current_pitch;
		speed = ent->yaw_speed;
		if (ideal_pitch > current_pitch)
		{
			if (move >= 180.0f)
				move = move - 360.0f;
		}
		else
		{
			if (move <= -180.0f)
				move = move + 360.0f;
		}
		if (move > 0.0f)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent->s.angles[PITCH] = anglemod (current_pitch + move);
	}
}

///////////////////////////////////////////////////////////////////////
// Set bot to move to it's movetarget. (following node path)
///////////////////////////////////////////////////////////////////////
void ACEMV_MoveToGoal(edict_t *self, usercmd_t *ucmd)
{
	short mSpeed = ACEMV_GetWalkSpeed (self);

	//not going to use sidestepping animations
	self->sidemove = 0;

	// If a rocket or grenade is around deal with it
	// Simple, but effective (could be rewritten to be more accurate)
	if(strcmp(self->movetarget->classname,"rocket") == 0 ||
	   strcmp(self->movetarget->classname,"grenade") == 0 ||
	   strcmp(self->movetarget->classname,"seeker") == 0 )
	{		
		if(debug_mode)
			debug_printf("%s: Oh crap a rocket!\n",self->client->pers.netname);

		if(strcmp(self->movetarget->classname,"seeker") == 0)
		{
			VectorSubtract (self->s.origin, self->movetarget->s.origin, self->move_vector);
			ACEMV_ChangeBotAngle(self);

			//turn and run like hell away from it
			//to do - needs improvement - bot should run when getting hit by seeker as well
			if(ACEMV_CanMove(self, MOVE_FORWARD))
			{
				ucmd->forwardmove = mSpeed;
				self->backpedal = false;
			}
		}
		else
		{
			VectorSubtract (self->movetarget->s.origin, self->s.origin, self->move_vector);
			ACEMV_ChangeBotAngle(self);

			// move left/right
			if(rand()%1 && ACEMV_CanMove(self, MOVE_LEFT))
					ucmd->sidemove = -mSpeed;
			else if(ACEMV_CanMove(self, MOVE_RIGHT))
					ucmd->sidemove = mSpeed;
		}
		return;

	}
	else
	{
		// Set bot's movement direction
		VectorSubtract (self->movetarget->s.origin, self->s.origin, self->move_vector);
		ACEMV_ChangeBotAngle(self);

		//try moving forward, if blocked, strafe around it if possible.
		self->backpedal = false;
		if(ACEMV_CanMove(self, MOVE_FORWARD))
		{
			ucmd->forwardmove = mSpeed;
		}
		else if(ACEMV_CanMove(self, MOVE_BACK))
		{
				ucmd->forwardmove = -mSpeed;
				self->backpedal = true;
		}
		else if(ACEMV_CanMove(self, MOVE_RIGHT))
		{
				ucmd->sidemove = mSpeed;
		}
		else if(ACEMV_CanMove(self, MOVE_LEFT))
		{
				ucmd->sidemove = -mSpeed;
		}
		return;
	}
}

///////////////////////////////////////////////////////////////////////
// Main movement code. (following node path)
///////////////////////////////////////////////////////////////////////
void ACEMV_Move(edict_t *self, usercmd_t *ucmd)
{
	vec3_t dist;
	int current_node_type=-1;
	int next_node_type=-1;
	int i;
	float c;
	short mSpeed = ACEMV_GetWalkSpeed (self);

	self->sidemove = 0;
	self->backpedal = false;

	// Get current and next node back from nav code.
	if(!ACEND_FollowPath(self))
	{
		self->state = STATE_WANDER;
		self->wander_timeout = level.time + 1.0;
		return;
	}

	if(!self->groundentity)
		return;

	current_node_type = nodes[self->current_node].type;
	next_node_type = nodes[self->next_node].type;

	///////////////////////////
	// Move To Goal
	///////////////////////////
	if (self->movetarget)
		ACEMV_MoveToGoal(self,ucmd);

	////////////////////////////////////////////////////////
	// Platforms
	///////////////////////////////////////////////////////
	if(current_node_type != NODE_PLATFORM && next_node_type == NODE_PLATFORM)
	{
		// check to see if lift is down?
		for(i=0;i<num_items;i++)
			if(item_table[i].node == self->next_node)
				if(item_table[i].ent->moveinfo.state != STATE_BOTTOM)
				    return; // Wait for elevator
	}
	if(current_node_type == NODE_PLATFORM && next_node_type == NODE_PLATFORM)
	{
		// Move to the center
		self->move_vector[2] = 0; // kill z movement
		if(VectorLength(self->move_vector) > 10)
			ucmd->forwardmove = 200; // walk to center
	
		ACEMV_ChangeBotAngle(self);

		return; // No move, riding elevator
	}

	////////////////////////////////////////////////////////
	// Jumpto Nodes
	///////////////////////////////////////////////////////
	if(next_node_type == NODE_JUMP ||
	  (current_node_type == NODE_JUMP && next_node_type != NODE_ITEM && nodes[self->next_node].origin[2] > self->s.origin[2]))
	{
		// Set up a jump move
		ucmd->forwardmove = mSpeed;
		ucmd->upmove = 400;

		ACEMV_ChangeBotAngle(self);

		VectorCopy(self->move_vector, dist);
		VectorScale(dist, 440, self->velocity);

		return;
	}

	////////////////////////////////////////////////////////
	// Ladder Nodes
	///////////////////////////////////////////////////////
	if(next_node_type == NODE_LADDER && nodes[self->next_node].origin[2] > self->s.origin[2])
	{
		// Otherwise move as fast as we can
		ucmd->forwardmove = mSpeed;
		self->velocity[2] = 320;

		ACEMV_ChangeBotAngle(self);

		return;

	}
	// If getting off the ladder
	if(current_node_type == NODE_LADDER && next_node_type != NODE_LADDER &&
	   nodes[self->next_node].origin[2] > self->s.origin[2])
	{
		ucmd->forwardmove = mSpeed;
		ucmd->upmove = 200;
		self->velocity[2] = 200;
		ACEMV_ChangeBotAngle(self);
		return;
	}

	////////////////////////////////////////////////////////
	// Water Nodes
	///////////////////////////////////////////////////////
	if(current_node_type == NODE_WATER)
	{
		// We need to be pointed up/down
		ACEMV_ChangeBotAngle(self);

		// If the next node is not in the water, then move up to get out.
		if(next_node_type != NODE_WATER && !(gi.pointcontents(nodes[self->next_node].origin) & MASK_WATER)) // Exit water
			ucmd->upmove = 400;

		ucmd->forwardmove = 300;

		return;
	}

	// Falling off ledge?
	if(!self->groundentity)
	{
		ACEMV_ChangeBotAngle(self);

		self->velocity[0] = self->move_vector[0] * 360;
		self->velocity[1] = self->move_vector[1] * 360;

		return;
	}

	// Check to see if stuck, and if so try to free us
	// Also handles crouching
 	if(VectorLength(self->velocity) < 37)
	{
		// Keep a random factor just in case....
		if (self->skill >= 0 && random() > 0.1 && ACEMV_SpecialMove(self, ucmd))
			return;

		self->s.angles[YAW] += random() * 180 - 90;
			
		// Try moving foward, if not, try to strafe around obstacle
		if(ACEMV_CanMove(self, MOVE_FORWARD))
		{
			ucmd->forwardmove = mSpeed;
		}
		else if(ACEMV_CanMove(self, MOVE_BACK))
		{
			ucmd->forwardmove = -mSpeed;
			self->backpedal = true;
		}
		else if(ACEMV_CanMove(self, MOVE_RIGHT))
		{
			ucmd->sidemove = mSpeed;
			self->sidemove = 1;
		}
		else if(ACEMV_CanMove(self, MOVE_LEFT))
		{
			ucmd->sidemove = -mSpeed;
			self->sidemove = -1;
		}
		return;
	}

	// Otherwise move as fast as we can
	if(ACEMV_CanMove(self, MOVE_FORWARD))
		ucmd->forwardmove = mSpeed;

	if(self->skill == 3)
	{	//ultra skill level(will be 3)
		c = random();

		if(grapple->value && c <= .7)
		{	//use the grapple once in awhile to pull itself around

			if(self->client->ctf_grapplestate == CTF_GRAPPLE_STATE_HANG)
			{
				CTFPlayerResetGrapple(self);
				ACEMV_ChangeBotAngle(self);
				return;
			}

			ACEMV_ChangeBotAngle(self);
			ACEIT_ChangeWeapon(self,FindItem("grapple"));
			ucmd->buttons = BUTTON_ATTACK;
			ACEMV_ChangeBotAngle(self);
			return;
		}
		else if(self->client->ctf_grapplestate != CTF_GRAPPLE_STATE_PULL &&
			self->client->ctf_grapplestate != CTF_GRAPPLE_STATE_HANG)
		{	//don't interrupt a pull
			float weight;
			int strafeJump = false;

			//Strafejumping should only occur now if a bot is far enough from a node
			//and not wandering.  We really don't want them to jump around when wandering
			//as it seems to hinder locating a goal.

			VectorSubtract(self->s.origin, nodes[self->current_node].origin, dist);
			weight = VectorLength( dist );
			if(weight > 300)
				strafeJump = true;

			//note - do not use sidemove animations when strafejumping
			if(strafeJump)
			{
				if(c > .7)
					ucmd->upmove = 400; //jump around the level

				if(c > 0.9 && ACEMV_CanMove(self, MOVE_LEFT))
					ucmd->sidemove = -200; //strafejump left(was -400)

				else if(c > 0.8 && ACEMV_CanMove(self, MOVE_RIGHT))
					ucmd->sidemove = 200; //strafejump right(was 400)
			}
		}

		//Now if we have the Alien Smartgun, drop some prox mines :)
		if (self->client->pers.weapon == FindItem("alien smartgun") && c < 0.2)
			ucmd->buttons = BUTTON_ATTACK2;
	}

	ACEMV_ChangeBotAngle(self);
}


///////////////////////////////////////////////////////////////////////
// Wandering code (based on old ACE movement code)
///////////////////////////////////////////////////////////////////////
void ACEMV_Wander(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  temp;
	float c;
	short mSpeed = ACEMV_GetWalkSpeed (self);

	//not going to use sidestepping animations(for now)
	self->sidemove = 0;
	self->backpedal = false;

	// Do not move
	if(self->next_move_time > level.time)
		return;

	// Special check for elevators, stand still until the ride comes to a complete stop.
	if(self->groundentity != NULL && self->groundentity->use == Use_Plat)
		if(self->groundentity->moveinfo.state == STATE_UP ||
		   self->groundentity->moveinfo.state == STATE_DOWN) // only move when platform not
		{
			self->velocity[0] = 0;
			self->velocity[1] = 0;
			self->velocity[2] = 0;
			self->next_move_time = level.time + 0.5;
			return;
		}

	// Is there a target to move to
	if (self->movetarget)
		ACEMV_MoveToGoal(self,ucmd);

	////////////////////////////////
	// Swimming?
	////////////////////////////////
	VectorCopy(self->s.origin,temp);
	temp[2]+=24;

	if(gi.pointcontents (temp) & MASK_WATER)
	{
		// If drowning and no node, move up
		if(self->client->next_drown_time > 0)
		{
			ucmd->upmove = 1;
			self->s.angles[PITCH] = -45;
		}
		else
			ucmd->upmove = 15;

		ucmd->forwardmove = 300;
	}
	else
		self->client->next_drown_time = 0; // probably shound not be messing with this, but

	////////////////////////////////
	// Lava?
	////////////////////////////////
	temp[2]-=48;
	if(gi.pointcontents(temp) & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		//	safe_bprintf(PRINT_MEDIUM,"lava jump\n");
		self->s.angles[YAW] += random() * 360 - 180;
		ucmd->forwardmove = mSpeed;
		ucmd->upmove = 400;
		return;
	}

	if(ACEMV_CheckEyes(self,ucmd))
		return;

	// Check for special movement if we have a normal move (have to test)
 	if(VectorLength(self->velocity) < 37)
	{
		/*if(random() > 0.1 && ACEMV_SpecialMove(self,ucmd))
			return;*/ //removed this because when wandering, the last thing you want is bots jumping
		//over things and going off ledges.  It's better for them to just bounce around the map.

		self->s.angles[YAW] += random() * 180 - 90;

		// Try to move forward, if blocked, try to strafe around obstacle
		if(ACEMV_CanMove(self, MOVE_FORWARD))
		{
			ucmd->forwardmove = mSpeed;
		}
		else if(ACEMV_CanMove(self, MOVE_BACK))
		{
			ucmd->forwardmove = -mSpeed;
			self->backpedal = true;
		}
		else if(ACEMV_CanMove(self, MOVE_RIGHT))
		{
			ucmd->sidemove = mSpeed;
			self->sidemove = 1;
		}
		else if(ACEMV_CanMove(self, MOVE_LEFT))
		{
			ucmd->sidemove = -mSpeed;
			self->sidemove = -1;
		}
		if(!M_CheckBottom(self) || !self->groundentity) // if there is ground continue, otherwise wait for next move.
		{
			if(ACEMV_CanMove(self, MOVE_FORWARD))
				ucmd->forwardmove = mSpeed;
		}

		return;
	}

	if(ACEMV_CanMove(self, MOVE_FORWARD))
		ucmd->forwardmove = mSpeed;

	if(self->skill == 3)
	{ //ultra skill level(will be 3)
		c = random();

		//If we have the Alien Smartgun, drop some prox mines :)
		if (self->client->pers.weapon == FindItem("alien smartgun") && c < 0.2)
			ucmd->buttons = BUTTON_ATTACK2;
	}
}

/**
 * @brief  Apply some imprecision to an attacking bot's aim
 *
 * @detail Uses a randomly oriented X,Y vector with a random length
 *         plus an offset, which leaves a hole in the center of the
 *         target area.
 *
 * @param self entity for the bot
 * @param pdx  x for targeted enemy's origin
 * @param pdy  y for targeted enemy's origin
 *
 * @return  *pdx and *pdy altered
 */
/*
 * constants for target fuzzification
 */
static const float ktgt_scale = 250.0f; //scaling factor
static const float ktgt_ofs   = 20.0f;  //radius of hole
/* scaling for weap skill for bot cfg skill 0, 1, 2 & 3 */
static const float ktgt_skill[4] =
	{ 0.7f, 0.8f, 0.9f, 1.0f }; //accuracy scaling by skill
//accuracy factor = ktgt_skill[0] * self->accuracy minimum clamp
static const float ktgt_acc = 0.35;

/*
1 : 1.0 //blaster accuracy
2 : 1.0 //alien disruptor accuracy
3 : 1.0 //chaingun accuracy
4 : 1.0 //flame thrower accuracy
5 : 1.0 //not used
6 : 1.0 //rocket launcher accuracy
7 : 1.0 //alien smartgun accuracy
8 : 1.0 //alien beamgun accuracy
9 : 1.0 //alien vaporizer accuracy
*/

struct
{
	classnum_t classnum; // classnum enum from gitem_t
	int accuracy_idx; // index into weapacc array
} weapon_accuracy_idx_lookup[] = 
{
	{weapon_blaster,			1},
	{weapon_alienblaster,		1}, // same accuracy as blaster
	{weapon_disruptor,			2}, // disruptor
	{weapon_chaingun,			3}, // chaingun
	{weapon_flamethrower,		4}, // flamethrower
	{weapon_rocketlauncher,		6},
	{weapon_minderaser,			6}, // minderaser has same acc as RL
	{weapon_smartgun,			7}, // smartgun
	{weapon_beamgun,			8}, // beamgun
	{weapon_vaporizer,			9}, // vaporizer
};

static void fuzzy_target( edict_t *self, float *pdx, float *pdy )
{
	unsigned int i;
	float accuracy;
	//float random_r;
	//float radius;
	//float angle;
	//float ca,sa;
	float dx,dy;

	/*
	 * self->accuracy is weapon specific accuracy from bot .cfg file
	 */
	accuracy = 0.75f; // default accuracy
	if (self->client->pers.weapon != NULL)
	{
		if (self->client->pers.weapon->classnum == weapon_violator ||
		    self->client->pers.weapon->classnum == weapon_warrior_punch ||
		    self->client->pers.weapon->classnum == weapon_wizard_punch)
		{
			accuracy = 1.0f; // meelee is always 100% accurate
		}
		else for (i = 0; i < sizeof (weapon_accuracy_idx_lookup) / sizeof (weapon_accuracy_idx_lookup[0]); i++)
		{
			if (self->client->pers.weapon->classnum == weapon_accuracy_idx_lookup[i].classnum)
			{
				accuracy = self->weapacc[weapon_accuracy_idx_lookup[i].accuracy_idx];
				break;
			}
		}
	}
	if ( accuracy < 0.5f )
		accuracy = 0.5f;
	else if ( accuracy > 1.0f )
		accuracy = 1.0f;

	//calc weap accuracy factor with scaling on bot skill
	switch ( self->skill )
	{
	case -1:
	case 0:
		accuracy *= ktgt_skill[0];
		break;
	case 1:
	default:
		accuracy *= ktgt_skill[1];
		break;
	case 2:
		accuracy *= ktgt_skill[2];
		break;
	case 3:
		//override 3->2 for CTF
		accuracy *= (ctf->integer) ? ktgt_skill[2] : ktgt_skill[3];
		break;
	}

	////radius calc
	//random_r = ktgt_scale * crandom();
	//radius =  (random_r * ( ktgt_acc / accuracy)) + ktgt_ofs;
	////angle calc
	//angle = random() * 2.0f * M_PI;
	//sa = sinf( angle );
	//ca = cosf( angle );
	////apply delta to target
	//*pdx += dx = ca * radius;
	//*pdy += dy = sa * radius;

	//The above is far too twitchy, let's do something that can be made smooth.
	dx = random() * 24.0f/(TENFPS/FRAMETIME);
	dy = random() * 24.0f/(TENFPS/FRAMETIME);

	if(self->targDeltaX > 64.0/accuracy)
	{
		self->targDeltaX = 63.9/accuracy;
		self->targDirSwitchX = true;
	}
	else if(self->targDeltaX < -64.0/accuracy)
	{
		self->targDeltaX = -63.9/accuracy;
		self->targDirSwitchX = false;
	}

	if(self->targDirSwitchX)
		self->targDeltaX -= dx;
	else
		self->targDeltaX += dx;

	if(self->targDeltaY > 64.0/accuracy)
	{
		self->targDeltaY = 63.9/accuracy;
		self->targDirSwitchY = true;
	}
	else if(self->targDeltaY < -64.0/accuracy)
	{
		self->targDeltaY = -63.9/accuracy;
		self->targDirSwitchY = false;
	}

	if(self->targDirSwitchY)
		self->targDeltaY -= dy;
	else
		self->targDeltaY += dy;

	*pdx += self->targDeltaX;
	*pdy += self->targDeltaY;

//	if ( debug_mode )
//	{
//		gi.dprintf("{\t%0.2f\t%0.2f\tacc%0.1f\t%i}\n",
//				dx, dy, accuracy, self->skill );
//	}

}

///////////////////////////////////////////////////////////////////////
// Attack movement routine
///////////////////////////////////////////////////////////////////////
void ACEMV_Attack (edict_t *self, usercmd_t *ucmd)
{
	float c, d;
	vec3_t  target;
	vec3_t  angles;
	vec3_t down;
	float jump_thresh;
	float crouch_thresh;
	gitem_t *vehicle;
	float range = 0.0f;
	vec3_t v;
	qboolean use_fuzzy_aim;
	short mSpeed = ACEMV_GetWalkSpeed (self);

	self->last_sidemove = self->sidemove;

	if(level.time - self->last_sidemove_time > 0.125)
		self->sidemove = 0;
	self->backpedal = false;

	ucmd->buttons = 0;
	use_fuzzy_aim = true; // unless overridden by special cases
	if ( dmflags->integer & DF_BOT_FUZZYAIM )
	{ // when bit is set it means fuzzy aim is disabled. for testing mostly.
		use_fuzzy_aim = false;
	}

	vehicle = FindItemByClassname("item_jetpack");

	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) 
	{
		//if we are too low, don't shoot, and move up.  Should be fairly simple, right?
		if(self->enemy->s.origin[2] >= self->s.origin[2] - 128) { //we want to be well above our target
			ucmd->upmove += 400;
			// face the enemy while moving up
			VectorCopy(self->enemy->s.origin,target);
			VectorSubtract (target, self->s.origin, self->move_vector);
			vectoangles (self->move_vector, angles);
			VectorCopy(angles,self->s.angles);
			return;
		}
	}

	switch ( self->skill )
	{ // set up the skill levels
	case -1:
	case 0:
		jump_thresh   = 1.0f;  // never jump
		crouch_thresh = 0.0f;  // never crouch
		break;

	case 1:
	default:
		jump_thresh   = 0.95f;
		crouch_thresh = 0.6f; // crouch much
		break;

	case 2:
	case 3:
		if ( !joustmode->integer )
		{
			jump_thresh   = 0.8f;
			crouch_thresh = 0.5f;
		}
		else
		{
			jump_thresh   = 0.5f; // want to jump a whole lot more in joust mode
			crouch_thresh = 0.4f; // and crouch less
		}
		break;
	}

	d = random(); // for skill 0 movement
	c = random(); // for strafing, jumping, crouching

	// violator attack
	if ( self->client->pers.weapon == FindItem( "Violator" ) )
	{
		use_fuzzy_aim = false; // avoid potential odd melee attack behaviour
		if ( ACEMV_CanMove( self, MOVE_FORWARD ) )
		{
			ucmd->forwardmove += 300; //lunge at enemy
			self->backpedal = false;
		}
		goto attack;
	}

	//machinegun/blaster/beamgun strafing for level 2/3 bots
	if ( !joustmode->value
			&& self->skill >= 2
			&& (self->client->pers.weapon == FindItem( "Blaster" )
					|| self->client->pers.weapon == FindItem( "Alien Blaster" )
					|| self->client->pers.weapon == FindItem( "Chaingun" )
					|| self->client->pers.weapon == FindItem( "Disruptor" )))
	{
		//strafe no matter what
		if(level.time - self->last_sidemove_time > 0.125)
		{
			if ( c < 0.5f && ACEMV_CanMove( self, MOVE_LEFT ) )
			{
				ucmd->sidemove -= mSpeed;
				self->sidemove = -1;
				self->last_sidemove_time = level.time;
			}
			else if ( c < 1.0f && ACEMV_CanMove( self, MOVE_RIGHT ) )
			{
				ucmd->sidemove += mSpeed;
				self->sidemove = 1;
				self->last_sidemove_time = level.time;
			}
			else
			{ //don't want high skill level bots just standing around
				goto standardmove;
			}
							
			self->sidemove_speed = ucmd->sidemove;
		}
		else
			ucmd->sidemove = self->sidemove_speed;

		//allow for some circle strafing
		if ( self->health < 50 && ACEMV_CanMove( self, MOVE_BACK ) )
		{
			ucmd->forwardmove -= mSpeed;
			self->backpedal = true;
		}
		else if ( c < 0.6f && ACEMV_CanMove( self, MOVE_FORWARD ) )
		{ //keep this at default, not make them TOO hard
			ucmd->forwardmove += mSpeed;
			self->backpedal = false;
		}
		else if ( c < 0.8f && ACEMV_CanMove( self, MOVE_BACK ) )
		{
			ucmd->forwardmove -= mSpeed;
			self->backpedal = true;
		}
		goto attack;
		//skip any jumping or crouching
	}

	if (self->skill <= 0 && d < 0.9f)
		goto attack; //skill 0 bots will barely move while firing

standardmove:

	if(level.time - self->last_sidemove_time > 0.125)
	{
		if ( c < 0.2f && ACEMV_CanMove( self, MOVE_LEFT ) )
		{
			ucmd->sidemove -= mSpeed;
			self->sidemove = -1;
			self->last_sidemove_time = level.time;
		}
		else if ( c < 0.4f && ACEMV_CanMove( self, MOVE_RIGHT ) )
		{
			ucmd->sidemove += mSpeed;
			self->sidemove = 1;
			self->last_sidemove_time = level.time;
		}
		else
			self->sidemove = 0;
		
		self->sidemove_speed = ucmd->sidemove;
	}
	else
		ucmd->sidemove = self->sidemove_speed;

	if ( self->health < 50 && ACEMV_CanMove( self, MOVE_BACK ) )
	{ //run away if wounded
		ucmd->forwardmove -= mSpeed;
		self->backpedal = true;
	}
	else if ( c < 0.6f && ACEMV_CanMove( self, MOVE_FORWARD ) )
	{ //keep this at default, not make them TOO hard
		ucmd->forwardmove += mSpeed;
		self->backpedal = false;
	}
	else if ( c < 0.8f && ACEMV_CanMove( self, MOVE_BACK ) )
	{
		ucmd->forwardmove -= mSpeed;
		self->backpedal = true;
	}

	c = random(); //really mix this up some
	if ( self->health >= 50 && c < crouch_thresh )
	{
		//TO DO - no crouching for now - maybe ever.  It's too un-human like, and the animation is non-existent for going in and out of crouches as of yet
		ucmd->upmove -= 200;
	}
	else if ( c > jump_thresh )
	{
#ifndef ALTERIA
		c = random();
		if ( self->health >= 70
				&& self->skill >= 2
		        && ACEIT_ChangeWeapon( self, FindItem( "Rocket Launcher" ) )
		        && c < 0.6f )
		{ // Rocket Jump
			self->s.angles[PITCH] = 90.0f;
			AngleVectors( self->s.angles, down, NULL, NULL );
			fire_rocket( self, self->s.origin, down, 200, 650, 120, 120 );
			ucmd->upmove += 200;
			self->s.angles[PITCH] = 0.0f;
			if ( (!(dmflags->integer & DF_INFINITE_AMMO))
			        && !rocket_arena->integer && !insta_rockets->integer )
			{
				self->client->pers.inventory[self->client->ammo_index]--;
			}
			return;
		}
		else
#endif
		{ // Normal Jump
			ucmd->upmove += 200;
		}
	}

attack:
	// Set the attack
	if(ACEAI_CheckShot(self)) 
	{
		//bot is taking a shot, lose spawn protection
		// and calculate distance to enemy
		range = 0.0f;
		if(self->enemy)
		{
			self->client->spawnprotected = false;
			VectorSubtract (self->s.origin, self->enemy->s.origin, v);
			range = VectorLength(v);
			if ( range < 32.0f )
			{ // point blank range, avoid potentially odd behaviour
				use_fuzzy_aim = false;
			}
		}		

		if(self->skill >= 2) 
		{	//skill 2/3 bots can use alt-fires!

			// Base selection on distance.
			ucmd->buttons = BUTTON_ATTACK;

			if (self->client->pers.weapon == FindItem("Blaster") || self->client->pers.weapon == FindItem("Alien Blaster")) 
			{
				if( range > 500)
					ucmd->buttons = BUTTON_ATTACK2;
				else
					ucmd->buttons = BUTTON_ATTACK;
			}

			if (self->client->pers.weapon == FindItem("Alien Disruptor")) 
			{
				if(range > 200) 
				{
					c = random();
					if( c < 0.5)
						ucmd->buttons = BUTTON_ATTACK2;
					else
						ucmd->buttons = BUTTON_ATTACK;
				}
				else
					ucmd->buttons = BUTTON_ATTACK;
			}

			if (self->client->pers.weapon == FindItem("Flame Thrower")) 
			{
				if(range < 500)
					ucmd->buttons = BUTTON_ATTACK;
				else
					ucmd->buttons = BUTTON_ATTACK2;
			}

			if (self->client->pers.weapon == FindItem("Chaingun")) 
			{
				if(range < 200)
					ucmd->buttons = BUTTON_ATTACK2;
				else
					ucmd->buttons = BUTTON_ATTACK;
			}

			if (self->client->pers.weapon == FindItem("Disruptor")) 
			{
				if(range < 300)
					ucmd->buttons = BUTTON_ATTACK2;
				else
					ucmd->buttons = BUTTON_ATTACK;
			}

			if (self->client->pers.weapon == FindItem("Alien Vaporizer")) 
			{
				if(range < 300)
					ucmd->buttons = BUTTON_ATTACK2;
				else
					ucmd->buttons = BUTTON_ATTACK;
			}

			if (self->client->pers.weapon == FindItem("Minderaser")) 
			{
				if(range < 400)
					ucmd->buttons = BUTTON_ATTACK;
				else
					ucmd->buttons = BUTTON_ATTACK2;
			}
		}
		else
		{
			ucmd->buttons = BUTTON_ATTACK;
		}
	}
	else
	{ // not taking a shot
		ucmd->buttons = 0;
		use_fuzzy_aim = false;
	}

	// Aim
	VectorCopy(self->enemy->s.origin,target);
	if ( use_fuzzy_aim  )
	{
		fuzzy_target( self, &target[0], &target[1] );
	}
	// Set movement direction toward targeted enemy
	VectorSubtract (target, self->s.origin, self->move_vector);
	vectoangles (self->move_vector, angles);
	VectorCopy(angles,self->s.angles);
}

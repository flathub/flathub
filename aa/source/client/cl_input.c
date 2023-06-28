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
// cl.input.c  -- builds an intended movement command to send to the server

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"

cvar_t	*cl_nodelta;

extern	unsigned	sys_frame_time;
unsigned	frame_msec;
unsigned	old_sys_frame_time;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition


Key_Event (int key, qboolean down, unsigned time);

  +mlook src time

===============================================================================
*/


kbutton_t	in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_use, in_attack, in_attack2, in_leanright, in_leanleft, in_sneak, in_zoom;
kbutton_t	in_up, in_down;

int			in_impulse;


void KeyDown (kbutton_t *b)
{
	int		k;
	char	*c;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Com_Printf ("Three keys down for a button!\n");
		return;
	}

	if (b->state & 1)
		return;		// still down

	// save timestamp
	c = Cmd_Argv(2);
	b->downtime = atoi(c);
	if (!b->downtime)
		b->downtime = sys_frame_time - 100;

	b->state |= 1 + 2;	// down + impulse down
}

void KeyUp (kbutton_t *b)
{
	int		k;
	char	*c;
	unsigned	uptime;

	c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	// save timestamp
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if (uptime)
		b->msec += uptime - b->downtime;
	else
		b->msec += 10;

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_KLookDown (void) {KeyDown(&in_klook);}
void IN_KLookUp (void) {KeyUp(&in_klook);}
void IN_UpDown(void) {KeyDown(&in_up);}
void IN_UpUp(void) {KeyUp(&in_up);}
void IN_DownDown(void) {KeyDown(&in_down);}
void IN_DownUp(void) {KeyUp(&in_down);}
void IN_LeftDown(void) {KeyDown(&in_left);}
void IN_LeftUp(void) {KeyUp(&in_left);}
void IN_RightDown(void) {KeyDown(&in_right);}
void IN_RightUp(void) {KeyUp(&in_right);}
void IN_ForwardDown(void) {KeyDown(&in_forward);}
void IN_ForwardUp(void) {KeyUp(&in_forward);}
void IN_BackDown(void) {KeyDown(&in_back);}
void IN_BackUp(void) {KeyUp(&in_back);}
void IN_LookupDown(void) {KeyDown(&in_lookup);}
void IN_LookupUp(void) {KeyUp(&in_lookup);}
void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {KeyDown(&in_moveright);}
void IN_MoverightUp(void) {KeyUp(&in_moveright);}

void IN_SpeedDown(void) {KeyDown(&in_speed);}
void IN_SpeedUp(void) {KeyUp(&in_speed);}
void IN_StrafeDown(void) {KeyDown(&in_strafe);}
void IN_StrafeUp(void) {KeyUp(&in_strafe);}

void IN_AttackDown(void) {KeyDown(&in_attack);}
void IN_AttackUp(void) {KeyUp(&in_attack);}

//alt fire
void IN_Attack2Down(void) {KeyDown(&in_attack2);}
void IN_Attack2Up(void) {KeyUp(&in_attack2);}

//leaning
void IN_LeanRightDown(void) {KeyDown(&in_leanright);}
void IN_LeanRightUp(void) {KeyUp(&in_leanright);}
void IN_LeanLeftDown(void) {KeyDown(&in_leanleft);}
void IN_LeanLeftUp(void) {KeyUp(&in_leanleft);}

//sneaking
void IN_SneakDown(void) {KeyDown(&in_sneak);}
void IN_SneakUp(void) {KeyUp(&in_sneak);}

//zooming
void IN_ZoomDown(void) {KeyDown(&in_zoom);}
void IN_ZoomUp(void) {KeyUp(&in_zoom);}

void IN_UseDown (void) {KeyDown(&in_use);}
void IN_UseUp (void) {KeyUp(&in_use);}

void IN_Impulse (void) {in_impulse=atoi(Cmd_Argv(1));}

/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
static float CL_KeyState (kbutton_t *key, qboolean reset_accum)
{
	float		val;
	int			msec;

	if (reset_accum)
		key->state &= 1; // clear impulses

	msec = key->msec;
	if (reset_accum)
		key->msec = 0;

	if ((key->state & 1))
	{	// non-impulse keypress
		msec += sys_frame_time - key->downtime;
		if (reset_accum)
			key->downtime = sys_frame_time;
	}

#if 0
	if (msec)
	{
		Com_Printf ("%i ", msec);
	}
#endif

	val = (float)msec / frame_msec;
	if (val < 0)
		val = 0;
	if (val > 1)
		val = 1;

	return val;
}




//==========================================================================

cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_sidespeed;

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

cvar_t	*cl_anglespeedkey;


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
static void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;

	if (in_speed.state & 1)
		speed = cls.frametime * cl_anglespeedkey->value;
	else
		speed = cls.frametime;

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right, true);
		cl.viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left, true);
	}
	if (in_klook.state & 1)
	{
		cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_forward, true);
		cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_back, true);
	}

	up = CL_KeyState (&in_lookup, true);
	down = CL_KeyState (&in_lookdown, true);

	cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed->value * down;
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove (usercmd_t *cmd, qboolean reset_accum)
{
	frame_msec = sys_frame_time - old_sys_frame_time;
	if (frame_msec < 1)
		frame_msec = 1;
	if (frame_msec > 200)
		frame_msec = 200;

	if (reset_accum)
		CL_AdjustAngles ();

	memset (cmd, 0, sizeof(*cmd));

	VectorCopy (cl.viewangles, cmd->angles);
	if (in_strafe.state & 1)
	{
		cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_right, reset_accum);
		cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_left, reset_accum);
	}

	cmd->sidemove += cl_sidespeed->value * CL_KeyState (&in_moveright, reset_accum);
	cmd->sidemove -= cl_sidespeed->value * CL_KeyState (&in_moveleft, reset_accum);

	cmd->upmove += cl_upspeed->value * CL_KeyState (&in_up, reset_accum);
	cmd->upmove -= cl_upspeed->value * CL_KeyState (&in_down, reset_accum);

	if (! (in_klook.state & 1) )
	{
		cmd->forwardmove += cl_forwardspeed->value * CL_KeyState (&in_forward, reset_accum);
		cmd->forwardmove -= cl_forwardspeed->value * CL_KeyState (&in_back, reset_accum);
	}

//
// adjust for speed key / running
//
	if (!cl.tactical && ( (in_speed.state & 1) ^ cl_run->integer ))
	{
		cmd->forwardmove *= 2;
		cmd->sidemove *= 2;
		cmd->upmove *= 2;
	}
}

void CL_ClampPitch (void)
{
	float	pitch;

	pitch = SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
	if (pitch > 180)
		pitch -= 360;

	if (cl.viewangles[PITCH] + pitch < -360)
		cl.viewangles[PITCH] += 360; // wrapped
	if (cl.viewangles[PITCH] + pitch > 360)
		cl.viewangles[PITCH] -= 360; // wrapped

	if (cl.viewangles[PITCH] + pitch > 89)
		cl.viewangles[PITCH] = 89 - pitch;
	if (cl.viewangles[PITCH] + pitch < -89)
		cl.viewangles[PITCH] = -89 - pitch;
}

/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove (usercmd_t *cmd)
{
	int		ms;
	int		i;

//
// figure button bits
//
	if ( in_attack.state & 3 )
		cmd->buttons |= BUTTON_ATTACK;
	in_attack.state &= ~2;

	//alt fire
	if ( in_attack2.state & 3 )
		cmd->buttons |= BUTTON_ATTACK2;
	in_attack2.state &= ~2;

	//leaning
	if ( in_leanright.state & 3 )
		cmd->buttons |= BUTTON_LEANRIGHT;
	in_leanright.state &= ~2;
	if ( in_leanleft.state & 3 )
		cmd->buttons |= BUTTON_LEANLEFT;
	in_leanleft.state &= ~2;

	//sneaking
	if ( in_sneak.state & 3 )
		cmd->buttons |= BUTTON_SNEAK;
	in_sneak.state &= ~2;

	//zooming
	if ( in_zoom.state & 3 )
		cmd->buttons |= BUTTON_ZOOM;
	in_zoom.state &= ~2;

	if (in_use.state & 3)
		cmd->buttons |= BUTTON_USE;
	in_use.state &= ~2;

	if (anykeydown && cls.key_dest == key_game)
		cmd->buttons |= BUTTON_ANY;

	// send milliseconds of time to apply the move
	ms = cls.frametime * 1000;
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = ms;

	CL_ClampPitch ();
	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);

	cmd->impulse = in_impulse;
	in_impulse = 0;
}

/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd (void)
{
	usercmd_t	cmd;

	// get basic movement from keyboard
	CL_BaseMove (&cmd, true);

	// allow mice or other external controllers to add to the move
	IN_Move (&cmd);
	IN_JoyMove (&cmd);

	CL_FinishMove (&cmd);

	old_sys_frame_time = sys_frame_time;

//cmd.impulse = cls.framecount;

	return cmd;
}


void IN_CenterView (void)
{
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	Cmd_AddCommand ("centerview",IN_CenterView);

	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand ("+attack", IN_AttackDown);
	Cmd_AddCommand ("-attack", IN_AttackUp);
	Cmd_AddCommand ("+use", IN_UseDown);
	Cmd_AddCommand ("-use", IN_UseUp);
	Cmd_AddCommand ("impulse", IN_Impulse);
	Cmd_AddCommand ("+klook", IN_KLookDown);
	Cmd_AddCommand ("-klook", IN_KLookUp);
	//alt fire
	Cmd_AddCommand ("+attack2", IN_Attack2Down);
	Cmd_AddCommand ("-attack2", IN_Attack2Up);
	//leaning
	Cmd_AddCommand ("+leanright", IN_LeanRightDown);
	Cmd_AddCommand ("-leanright", IN_LeanRightUp);
	Cmd_AddCommand ("+leanleft", IN_LeanLeftDown);
	Cmd_AddCommand ("-leanleft", IN_LeanLeftUp);
	//sneaking
	Cmd_AddCommand ("+sneak", IN_SneakDown);
	Cmd_AddCommand ("-sneak", IN_SneakUp);
	//zooming
	Cmd_AddCommand ("+zoom", IN_ZoomDown);
	Cmd_AddCommand ("-zoom", IN_ZoomUp);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
}



/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	sizebuf_t	buf;
	byte		data[128];
	int			i;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			checksumIndex;

	// build a command even if not connected

	// save this command off for prediction
	i = cls.netchan.outgoing_sequence & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	*cmd = CL_CreateCmd ();

	cl.cmd = *cmd;

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
		return;

	if ( cls.state == ca_connected)
	{
		if (cls.netchan.message.cursize	|| curtime - cls.netchan.last_sent > 1000 )
			Netchan_Transmit (&cls.netchan, 0, data);
		return;
	}

	// send a userinfo update if needed
	if (userinfo_modified)
	{
		CL_FixUpGender();
		userinfo_modified = false;
		MSG_WriteByte (&cls.netchan.message, clc_userinfo);
		MSG_WriteString (&cls.netchan.message, Cvar_Userinfo() );
	}

	SZ_Init (&buf, data, sizeof(data));
	SZ_SetName ( &buf, "CL_SendCmd", false );

	// begin a client move command
	MSG_WriteByte (&buf, clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	MSG_WriteByte (&buf, 0);

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if (cl_nodelta->value || !cl.frame.valid || cls.demowaiting)
		MSG_WriteLong (&buf, -1);	// no compression
	else
		MSG_WriteLong (&buf, cl.frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (cls.netchan.outgoing_sequence-2) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	memset (&nullcmd, 0, sizeof(nullcmd));
	MSG_WriteDeltaUsercmd (&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence-1) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		cls.netchan.outgoing_sequence);

	//
	// deliver the message
	//
	Netchan_Transmit (&cls.netchan, buf.cursize, buf.data);
}


#if defined WIN32_VARIANT
# define OS_MENU_MOUSESCALE 0.35
#else
# if defined UNIX_VARIANT
#  define OS_MENU_MOUSESCALE 0.1
# else
#  define OS_MENU_MOUSESCALE 1
# endif
#endif


qboolean mouse_available = false;
qboolean mouse_is_position = false;
qboolean mlooking = false;
int mouse_diff_x = 0;
int mouse_diff_y = 0;
cursor_t cursor;


void IN_MLookDown (void)
{
	mlooking = true;
}

void IN_MLookUp (void)
{
	mlooking = false;
	if (!freelook->integer && lookspring->integer) {
		IN_CenterView ();
	}
}

static void IN_MoveMenuMouse( int x , int y )
{
	cursor.oldx = cursor.x;
	cursor.oldy = cursor.y;

	// Clip cursor location to window
	if ( x < 0 ) {
		x = 0;
	} else if ( x > viddef.width ) {
		x = viddef.width;
	}
	if ( y < 0 ) {
		y = 0;
	} else if ( y > viddef.height ) {
		y = viddef.height;
	}

	cursor.x = x;
	cursor.y = y;
	cursor.mouseaction = cursor.mouseaction || cursor.x != cursor.oldx
		|| cursor.y != cursor.oldy;

	M_Think_MouseCursor();
}

extern cvar_t *fov;
void IN_Move (usercmd_t *cmd)
{
	float fmx;
	float fmy;
	float adjust;

	if ( ! mouse_available ) {
		return;
	}

	// If we have a position instead of a diff, don't go through
	// the normal process. Instead, update the menu's cursor
	// as necessary, and bail out.
	if ( mouse_is_position ) {
		if ( cls.key_dest == key_menu ) {
			IN_MoveMenuMouse( mouse_diff_x , mouse_diff_y );
		}
		return;
	}

	// Apply interpolation with previous value if necessary
	fmx = (float) mouse_diff_x;
	fmy = (float) mouse_diff_y;
	if (cmd)
		mouse_diff_x = mouse_diff_y = 0;

	// No mouse in console
	if ( cls.key_dest == key_console ) {
		return;
	}

	// Compute sensitivity adjustments
	adjust = 1.0;

	// Update menu cursor location
	if ( cls.key_dest == key_menu ) {
		adjust *= menu_sensitivity->value * OS_MENU_MOUSESCALE;
		IN_MoveMenuMouse( cursor.x + fmx * adjust ,
				cursor.y + fmy * adjust );
		return;
	}

	// Game mouse
	adjust *= sensitivity->value * cl.refdef.fov_x/fov->value;
	fmx *= adjust;
	fmy *= adjust;

	// Add mouse X/Y movement to cmd
	if ( ( lookstrafe->integer && mlooking ) || ( in_strafe.state & 1 ) ) {
		if (!cmd)
			return;
		cmd->sidemove += (short)( ( m_side->value * fmx ) + 0.5f );
	} else {
		if (cmd)
			cl.viewangles[ YAW ] -= m_yaw->value * fmx;
		else
			cl.predicted_angles[ YAW ] = cl.last_predicted_angles[ YAW ] - m_yaw->value * fmx;
	}

	if ( ( mlooking || freelook->integer ) && !( in_strafe.state & 1 ) ) {
		if (cmd)
			cl.viewangles[ PITCH ] += m_pitch->value * fmy;
		else
			cl.predicted_angles[ PITCH ] = cl.last_predicted_angles[ PITCH ] + m_pitch->value * fmy;
	} else {
		if (!cmd)
			return;
		cmd->forwardmove -= (short)( ( m_forward->value * fmy )
				+ 0.5f );
	}
}

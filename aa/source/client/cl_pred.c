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

#include "client.h"

/*
===================
CL_CheckPredictionError
===================
*/
void CL_CheckPredictionError (void)
{
	int		frame;
	int		delta[3];
	int		i;
	int		len;

	if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (CMD_BACKUP-1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear (cl.prediction_error);
	}
	else
	{
		if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]) )
		{ // use sum of absolute differences from above, scaled to quake units
			Com_Printf("prediction miss: frame %i: %0.3f\n",
					cl.frame.serverframe, ((float)len)/8.0f );
		}

		VectorCopy (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame]);

		// save for error itnerpolation
		for (i=0 ; i<3 ; i++)
			cl.prediction_error[i] = delta[i]*0.125;
	}
}


/*
====================
CL_ClipMoveToEntities

====================
*/
void CL_ClipMoveToEntities ( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr )
{
	int			i, x, zd, zu;
	trace_t		trace;
	int			headnode;
	float		*angles;
	entity_state_t	*ent;
	int			num;
	cmodel_t		*cmodel;
	vec3_t		bmins, bmaxs;

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cl.playernum+1)
			continue;

		if (ent->solid == 31)
		{	// special value for bmodel
			cmodel = cl.model_clip[ent->modelindex];
			if (!cmodel)
				continue;
			headnode = cmodel->headnode;
			angles = ent->angles;
		}
		else
		{	// encoded bbox
			x = 8*(ent->solid & 31);
			zd = 8*((ent->solid>>5) & 31);
			zu = 8*((ent->solid>>10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;
			
			headnode = CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3_origin;	// boxes don't rotate
		}

		if (tr->allsolid)
			return;

		trace = CM_TransformedBoxTrace (start, end,
			mins, maxs, headnode,  MASK_PLAYERSOLID,
			ent->origin, angles);
		
		if (trace.allsolid || trace.startsolid ||
		trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_s *)ent;
		 	if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
				*tr = trace;
		}
		else if (trace.startsolid)
			tr->startsolid = true;
	}
}


/*
================
CL_PMTrace
================
*/
trace_t		CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t	t;

	// check against world
	t = CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *)1;

	// check all other solid models
	CL_ClipMoveToEntities (start, mins, maxs, end, &t);

	return t;
}

int		CL_PMpointcontents (vec3_t point)
{
	int			i;
	entity_state_t	*ent;
	int			num;
	cmodel_t		*cmodel;
	int			contents;

	contents = CM_PointContents (point, 0);

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (ent->solid != 31) // special value for bmodel
			continue;

		cmodel = cl.model_clip[ent->modelindex];
		if (!cmodel)
			continue;

		contents |= CM_TransformedPointContents (point, cmodel->headnode, ent->origin, ent->angles);
	}

	return contents;
}

//Knightmare added- this can check using masks, good for checking surface flags
//   also checks for bmodels
/*
================
CL_PMSurfaceTrace
================
*/
trace_t CL_PMSurfaceTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentmask)
{
   trace_t   t;

   if (!mins)
      mins = vec3_origin;
   if (!maxs)
      maxs = vec3_origin;

   // check against world
   t = CM_BoxTrace (start, end, mins, maxs, 0, contentmask);
   if (t.fraction < 1.0)
      t.ent = (struct edict_s *)1;

   // check all other solid models
   CL_ClipMoveToEntities (start, mins, maxs, end, &t);

   return t;
}


/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CL_PredictMovement (int msec_since_packet)
{
	int			ack, current;
	int			frame;
	int			oldframe;
	usercmd_t	*cmd;
	pmove_t		pm;
	int			i;
	int			step;
	int			oldz;

	if (cls.state != ca_active)
		return;

	if (cl_paused->value)
		return;

	if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// just set angles
		for (i=0 ; i<3 ; i++)
		{
			cl.last_predicted_angles[i] = cl.predicted_angles[i] = cl.viewangles[i] + SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[i]);
		}
		return;
	}

	ack = cls.netchan.incoming_acknowledged;
	current = cls.netchan.outgoing_sequence;

	// if we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP)
	{
		if (cl_showmiss->value)
			Com_Printf ("exceeded CMD_BACKUP\n");
		return;
	}

	// copy current state to pmove
	memset (&pm, 0, sizeof(pm));
	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMpointcontents;

	pm_airaccelerate = atof(cl.configstrings[CS_AIRACCEL]);

	pm.s = cl.frame.playerstate.pmove;

//	SCR_DebugGraph (current - ack - 1, 0);

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (CMD_BACKUP-1);
		cmd = &cl.cmds[frame];

		pm.cmd = *cmd;
		Pmove (&pm);

		// save for debug checking
		VectorCopy (pm.s.origin, cl.predicted_origins[frame]);
	}

	VectorCopy (pm.viewangles, cl.predicted_angles);
	VectorCopy (pm.viewangles, cl.last_predicted_angles);

	if (cl_test->integer)
	{
		/*
		 * update view angles and create a movement command, based on
		 * accumulated keyboard and mouse events, which are *not* reset
		 */
		IN_Move (NULL);
		CL_BaseMove (&pm.cmd, false);
		for (i = 0; i < 3; i++)
		{
			pm.cmd.angles[i] = ANGLE2SHORT (cl.predicted_angles[i]);
			pm.s.delta_angles[i] = 0;
		}
		pm.cmd.msec = msec_since_packet;
		// Don't even try to jump except on packet frames. It can't work because
		// of how jump simulation is done.
		pm.cmd.upmove = 0;
		Pmove (&pm);
	}

	oldframe = (ack-2) & (CMD_BACKUP-1);
	oldz = cl.predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & PMF_ON_GROUND) )
	{
		cl.predicted_step = step * 0.125;
		cl.predicted_step_time = cls.realtime - cls.frametime * 500;
	}

	// copy results out for rendering
	cl.predicted_origin[0] = pm.s.origin[0]*0.125;
	cl.predicted_origin[1] = pm.s.origin[1]*0.125;
	cl.predicted_origin[2] = pm.s.origin[2]*0.125;
	
	cl.predicted_velocity[0] = pm.s.velocity[0]*0.125;
	cl.predicted_velocity[1] = pm.s.velocity[1]*0.125;
	cl.predicted_velocity[2] = pm.s.velocity[2]*0.125;
}

/*
 =================
 CL_Trace
 =================
*/
trace_t CL_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int skipNumber, int brushMask, qboolean brushOnly, int *entNumber){

	trace_t			trace, tmp;
	entity_state_t	*ent;
	cmodel_t		*cmodel;
	vec3_t			bmins, bmaxs;
	int				i, xy, zd, zu, headNode, num;

	// Check against world
	trace = CM_BoxTrace(start, end, mins, maxs, 0, brushMask);
	if (trace.fraction < 1.0){
		if (entNumber)
			*entNumber = 0;

		trace.ent = (struct edict_s *)1;
	}

	if (trace.allsolid || trace.fraction == 0.0)
		return trace;

	// Check all other solid models
	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (ent->number == skipNumber)
			continue;

		if (ent->solid == 31){
			// Special value for brush model
			cmodel = cl.model_clip[ent->modelindex];
			if (!cmodel)
				continue;

			tmp = CM_TransformedBoxTrace(start, end, mins, maxs, cmodel->headnode, brushMask, ent->origin, ent->angles);
		}
		else {
			if (brushOnly)
				continue;

			// Encoded bounding box
			xy = 8 * (ent->solid & 31);
			zd = 8 * ((ent->solid >> 5) & 31);
			zu = 8 * ((ent->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -xy;
			bmaxs[0] = bmaxs[1] = xy;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headNode = CM_HeadnodeForBox (bmins, bmaxs);
			tmp = CM_TransformedBoxTrace(start, end, mins, maxs, headNode, brushMask, ent->origin, vec3_origin);
		}

		if (tmp.allsolid || tmp.startsolid || tmp.fraction < trace.fraction){
			if (entNumber)
				*entNumber = ent->number;

			tmp.ent = (struct edict_s *)ent;
			if (trace.startsolid){
				trace = tmp;
				trace.startsolid = true;
			}
			else
				trace = tmp;
		}
		else if (tmp.startsolid)
			trace.startsolid = true;

		if (trace.allsolid)
			break;
	}

	return trace;
}



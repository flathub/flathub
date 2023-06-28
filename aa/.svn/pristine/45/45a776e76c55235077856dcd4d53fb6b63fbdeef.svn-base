/*
Copyright (C) 20?? COR Entertainment, LLC.

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

/*#define DEBUG_DEATHCAM*/ /* Uncomment this or set as a CFLAG at build time */

void DeathcamTrack (edict_t *ent);

/*  The ent is the owner of the chasecam  */
void DeathcamStart (edict_t *ent)
{

	/* This creates a tempory entity we can manipulate within this
	 * function */
	edict_t      *chasecam;

	/* Tell everything that looks at the toggle that our chasecam is on
	 * and working */
	ent->client->chasetoggle = 1;

    /* Make out gun model "non-existent" so it's more realistic to the
     * player using the chasecam */
    ent->client->ps.gunindex = 0;

    chasecam = G_Spawn ();
    chasecam->owner = ent;
    chasecam->solid = SOLID_NOT;
    chasecam->movetype = MOVETYPE_FLYMISSILE;

	/* Stop movement prediction */
    ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;

	/* Don't send camera info to other players */
    ent->svflags |= SVF_NOCLIENT;

	/* Now, make the angles of the player model, (!NOT THE HUMAN VIEW!) be
     * copied to the same angle of the chasecam entity */
    VectorCopy (ent->s.angles, chasecam->s.angles);

	/*alt fire of zoom weapons-- reset some things.*/
    ent->client->ps.fov = atoi(Info_ValueForKey(ent->client->pers.userinfo, "fov")); //alt fire - reset the fov;
    ent->client->ps.stats[STAT_ZOOMED] = 0;

    /* Clear the size of the entity, so it DOES technically have a size,
     * but that of '0 0 0'-'0 0 0'. (xyz, xyz). mins = Minimum size,
     * maxs = Maximum size */
    VectorClear (chasecam->mins);
    VectorClear (chasecam->maxs);
	VectorClear (chasecam->velocity);

    /* Make the chasecam's origin (position) be the same as the player
     * entity's because as the camera starts, it will force itself out
     * slowly backwards from the player model */
     VectorCopy (ent->s.origin, chasecam->s.origin);
	 VectorCopy (ent->s.origin, chasecam->death_origin);

     chasecam->classname = "chasecam";
     chasecam->nextthink = level.time + 0.100;
     chasecam->think = DeathcamTrack;
	 ent->client->chasecam = chasecam;
     ent->client->oldplayer = G_Spawn();

}

void DeathcamRemove (edict_t *ent, char *opt)
{
	if(ent->client->chasetoggle == 1) /* Safety check */
	{
		ent->client->chasetoggle = 0;

		/* Stop the chasecam from moving */
		VectorClear (ent->client->chasecam->velocity);

		/* Re-enable sending entity info to other clients */
		ent->svflags &= ~SVF_NOCLIENT;

		if(ent->client->oldplayer->client != NULL)
		{
			#ifdef DEBUG_DEATHCAM
			printf("--- Deathcam = %p\n", ent->client->oldplayer->client);
			#endif
			free(ent->client->oldplayer->client);
		}

		G_FreeEdict (ent->client->oldplayer);
		G_FreeEdict (ent->client->chasecam);
	}
}

/* The "ent" is the chasecam */
void DeathcamTrack (edict_t *ent)
{


	trace_t      tr;
    vec3_t       spot1, spot2;
    vec3_t       forward, right, up;

    ent->nextthink = level.time + 0.100;

	AngleVectors (ent->s.angles, forward, right, up);

	//JKD - 7/16/06 - tweaked this slightly so that it's a little closer to the body
    /* find position for camera to end up */
    VectorMA (ent->death_origin, -150, forward, spot1);

    /* Move camera destination up slightly too */
	spot1[2] += 30;

	/* Make sure we don't go outside the level */
	tr = gi.trace (ent->s.origin, NULL, NULL, spot1, ent, false);

    /* subtract the endpoint from the start point for length and
     * direction manipulation */
    VectorSubtract (tr.endpos, ent->s.origin, spot2);

	/* Use this to modify camera velocity */
	VectorCopy(spot2, ent->velocity);

}


void CheckDeathcam_Viewent (edict_t *ent)
{
	gclient_t       *cl;
	int	tmp;

	if (!ent->client->oldplayer->client)
    {
        cl = (gclient_t *) malloc(sizeof(gclient_t));
        ent->client->oldplayer->client = cl;
		#ifdef DEBUG_DEATHCAM
		printf("+++ Deathcam = %p\n", cl);
		#endif
    }

    if (ent->client->oldplayer)
    {
		ent->client->oldplayer->s.frame = ent->s.frame;
		/* Copy the origin, the speed, and the model angle, NOT
         * literal angle to the display entity */
        VectorCopy (ent->s.origin, ent->client->oldplayer->s.origin);
        VectorCopy (ent->velocity, ent->client->oldplayer->velocity);
        VectorCopy (ent->s.angles, ent->client->oldplayer->s.angles);
	}
	tmp = ent->client->oldplayer->s.number;
	ent->client->oldplayer->s = ent->s;
	ent->client->oldplayer->s.number = tmp; // keep same s.number, not client's

	 gi.linkentity (ent->client->oldplayer);

}

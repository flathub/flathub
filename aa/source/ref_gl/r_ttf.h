/*
Copyright (C) 2010 COR Entertainment, LLC.

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
#ifndef __R_TTF_H
#define __R_TTF_H

#include <qcommon/qcommon.h>
#include "r_text.h"


/* Initialise TTF engine */
qboolean TTF_Initialise( void );

/* Shutdown TTF engine */
void TTF_Shutdown( void );

/*
 * Initialise a TTF face.
 *
 * This function will try loading the face's file and, on success, it will
 * load and verify the TTF information.
 */
qboolean TTF_InitFace( FNT_face_t face );


#endif //__R_TTF_H

/*
=============
LOD calculation

The width and height occupied by a model on screen after it's been rendered
will scale as the square of the FOV setting, and proportionally to the
width/height of the screen itself. Based on the assumption that 500 units is
an adequate LOD cutoff distance at 1920*1080 with an FOV of 90, we can scale
the LOD cutoff distance to the lowest point where there will be no noticable
ugliness.

NOTE: Turns out the player's FOV setting goes to fov_x and not fov_y. Go
figure.
=============
*/
#define LOD_BASE_H      1920.0
#define LOD_BASE_W      1080.0
#define LOD_BASE_DIST   500.0
#define LOD_BASE_FOV    90.0
#define LOD_DIST        (LOD_BASE_DIST*\
                        (r_newrefdef.width/LOD_BASE_W)*(r_newrefdef.height/LOD_BASE_H)*\
                        (LOD_BASE_FOV/r_newrefdef.fov_x)*\
                        (LOD_BASE_FOV/r_newrefdef.fov_x))

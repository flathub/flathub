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
// r_math.h
typedef vec_t mat4x4_t[16];

extern const mat4x4_t mat4x4_identity;

void Matrix4_Identity( mat4x4_t m );
void Matrix4_Copy( const mat4x4_t m1, mat4x4_t m2 );
qboolean Matrix4_Compare( const mat4x4_t m1, const mat4x4_t m2 );
void Matrix4_Multiply( const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out );
void Matrix4_MultiplyFast( const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out );
void Matrix4_Rotate( mat4x4_t m, vec_t angle, vec_t x, vec_t y, vec_t z );
void Matrix4_Translate( mat4x4_t m, vec_t x, vec_t y, vec_t z );
void Matrix4_Scale( mat4x4_t m, vec_t x, vec_t y, vec_t z );
void Matrix4_Transpose( const mat4x4_t m, mat4x4_t out );
void Matrix4_Matrix( const mat4x4_t in, vec3_t out[3] );
void Matrix4_Multiply_Vector( const mat4x4_t m, const vec4_t v, vec4_t out );

void Matrix4_Copy2D( const mat4x4_t m1, mat4x4_t m2 );
void Matrix4_Multiply2D( const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out );
void Matrix4_Scale2D( mat4x4_t m, vec_t x, vec_t y );
void Matrix4_Translate2D( mat4x4_t m, vec_t x, vec_t y );
void Matrix4_Stretch2D( mat4x4_t m, vec_t s, vec_t t );

void Matrix3x4_Invert (matrix3x4_t *out, matrix3x4_t in);
void Matrix3x4_FromQuatAndVectors (matrix3x4_t *out, vec4_t rot, const float trans[3], const float scale[3]);
void Matrix3x4_Multiply (matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2);
void Matrix3x4_Scale (matrix3x4_t *out, matrix3x4_t in, float scale);
void Matrix3x4_Add (matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2);
void Matrix3x4_Copy (matrix3x4_t *out, matrix3x4_t in);
void Matrix3x4GenRotate (matrix3x4_t *out, float angle, const vec3_t axis);

void R_CalcTangent
( 	const vec3_t v0, const vec3_t v1, const vec3_t v2,
	const vec2_t st0, const vec2_t st1, const vec2_t st2,
	const vec3_t normal, vec4_t tangent );

// HACK: defined in cmodel.c
extern void AnglesToMatrix3x3 (const vec3_t angles, float rotation_matrix[3][3]);

#define nanmask (255<<23)
#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

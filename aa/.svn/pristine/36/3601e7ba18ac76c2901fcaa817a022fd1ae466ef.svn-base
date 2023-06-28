/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2014 COR Entertainment, LLC.

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
// r_math.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game/q_shared.h"
#include "r_math.h"

const mat4x4_t mat4x4_identity =
{
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

void Matrix4_Identity (mat4x4_t m)
{
	int i;

	for (i = 0; i < 16; i++)
	{
		if (i == 0 || i == 5 || i == 10 || i == 15)
			m[i] = 1.0;
		else
			m[i] = 0.0;
	}
}

void Matrix4_Copy (const mat4x4_t m1, mat4x4_t m2)
{
	int i;

	for (i = 0; i < 16; i++)
		m2[i] = m1[i];
}

qboolean Matrix4_Compare (const mat4x4_t m1, const mat4x4_t m2)
{
	int i;

	for (i = 0; i < 16; i++)
		if (m1[i] != m2[i])
			return false;

	return true;
}

void Matrix4_Multiply (const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out)
{
	out[0]  = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3];
	out[1]  = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3];
	out[2]  = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3];
	out[3]  = m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3];
	out[4]  = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7];
	out[5]  = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7];
	out[6]  = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7];
	out[7]  = m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7];
	out[8]  = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11];
	out[9]  = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11];
	out[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11];
	out[11] = m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11];
	out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15];
	out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15];
	out[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
	out[15] = m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];
}

void Matrix4_MultiplyFast (const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out)
{
	out[0]  = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2];
	out[1]  = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2];
	out[2]  = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2];
	out[3]  = 0.0f;
	out[4]  = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6];
	out[5]  = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6];
	out[6]  = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6];
	out[7]  = 0.0f;
	out[8]  = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10];
	out[9]  = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10];
	out[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10];
	out[11] = 0.0f;
	out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12];
	out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13];
	out[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14];
	out[15] = 1.0f;
}

void Matrix4_Rotate (mat4x4_t m, vec_t angle, vec_t x, vec_t y, vec_t z)
{
	mat4x4_t t, b;
	vec_t c = cos (DEG2RAD(angle));
	vec_t s = sin (DEG2RAD(angle));
	vec_t mc = 1 - c, t1, t2;

	t[0]  = (x * x * mc) + c;
	t[5]  = (y * y * mc) + c;
	t[10] = (z * z * mc) + c;

	t1 = y * x * mc;
	t2 = z * s;
	t[1] = t1 + t2;
	t[4] = t1 - t2;

	t1 = x * z * mc;
	t2 = y * s;
	t[2] = t1 - t2;
	t[8] = t1 + t2;

	t1 = y * z * mc;
	t2 = x * s;
	t[6] = t1 + t2;
	t[9] = t1 - t2;

	t[3] = t[7] = t[11] = t[12] = t[13] = t[14] = 0;
	t[15] = 1;

	Matrix4_Copy (m, b);
	Matrix4_MultiplyFast (b, t, m);
}

void Matrix4_Translate (mat4x4_t m, vec_t x, vec_t y, vec_t z)
{
	m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
	m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
	m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
	m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

void Matrix4_Scale (mat4x4_t m, vec_t x, vec_t y, vec_t z)
{
	m[0] *= x; m[4] *= y; m[8]  *= z;
	m[1] *= x; m[5] *= y; m[9]  *= z;
	m[2] *= x; m[6] *= y; m[10] *= z;
	m[3] *= x; m[7] *= y; m[11] *= z;
}

void Matrix4_Transpose (const mat4x4_t m, mat4x4_t out)
{
	out[0] = m[0]; out[1] = m[4]; out[2] = m[8]; out[3] = m[12];
	out[4] = m[1]; out[5] = m[5]; out[6] = m[9]; out[7] = m[13];
	out[8] = m[2]; out[9] = m[6]; out[10] = m[10]; out[11] = m[14];
	out[12] = m[3]; out[13] = m[7]; out[14] = m[11]; out[15] = m[15];
}

void Matrix4_Matrix (const mat4x4_t in, vec3_t out[3])
{
	out[0][0] = in[0];
	out[0][1] = in[4];
	out[0][2] = in[8];

	out[1][0] = in[1];
	out[1][1] = in[5];
	out[1][2] = in[9];

	out[2][0] = in[2];
	out[2][1] = in[6];
	out[2][2] = in[10];
}

void Matrix4_Multiply_Vector (const mat4x4_t m, const vec4_t v, vec4_t out)
{
	out[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
	out[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
}

//============================================================================

void Matrix4_Copy2D (const mat4x4_t m1, mat4x4_t m2)
{
	m2[0] = m1[0];
	m2[1] = m1[1];
	m2[4] = m1[4];
	m2[5] = m1[5];
	m2[12] = m1[12];
	m2[13] = m1[13];
}

void Matrix4_Multiply2D (const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out)
{
	out[0]  = m1[0] * m2[0] + m1[4] * m2[1];
	out[1]  = m1[1] * m2[0] + m1[5] * m2[1];
	out[4]  = m1[0] * m2[4] + m1[4] * m2[5];
	out[5]  = m1[1] * m2[4] + m1[5] * m2[5];
	out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[12];
	out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[13];
}

void Matrix4_Scale2D (mat4x4_t m, vec_t x, vec_t y)
{
	m[0] *= x;
	m[1] *= x;
	m[4] *= y;
	m[5] *= y;
}

void Matrix4_Translate2D (mat4x4_t m, vec_t x, vec_t y)
{
	m[12] += x;
	m[13] += y;
}

void Matrix4_Stretch2D (mat4x4_t m, vec_t s, vec_t t)
{
	m[0] *= s;
	m[1] *= s;
	m[4] *= s;
	m[5] *= s;
	m[12] = s * m[12] + t;
	m[13] = s * m[13] + t;
}

void Matrix3x4_Invert (matrix3x4_t *out, matrix3x4_t in)
{
	vec3_t a, b, c, trans;

	VectorSet(a, in.a[0], in.b[0], in.c[0]);
	VectorSet(b, in.a[1], in.b[1], in.c[1]);
	VectorSet(c, in.a[2], in.b[2], in.c[2]);

	VectorScale(a, 1/DotProduct(a, a), a);
	VectorScale(b, 1/DotProduct(b, b), b);
	VectorScale(c, 1/DotProduct(c, c), c);

	VectorSet(trans, in.a[3], in.b[3], in.c[3]);

	Vector4Set(out->a, a[0], a[1], a[2], -DotProduct(a, trans));
	Vector4Set(out->b, b[0], b[1], b[2], -DotProduct(b, trans));
	Vector4Set(out->c, c[0], c[1], c[2], -DotProduct(c, trans));
}

void Matrix3x4_FromQuatAndVectors (matrix3x4_t *out, vec4_t rot, const float trans[3], const float scale[3])
{
	vec3_t a, b, c;

	//Convert the quat
	{
		float	x = rot[0], y = rot[1], z = rot[2], w = rot[3],
				tx = 2*x, ty = 2*y, tz = 2*z,
				txx = tx*x, tyy = ty*y, tzz = tz*z,
				txy = tx*y, txz = tx*z, tyz = ty*z,
				twx = w*tx, twy = w*ty, twz = w*tz;
		VectorSet(a, 1 - (tyy + tzz), txy - twz, txz + twy);
		VectorSet(b, txy + twz, 1 - (txx + tzz), tyz - twx);
		VectorSet(c, txz - twy, tyz + twx, 1 - (txx + tyy));
	}

	Vector4Set(out->a, a[0]*scale[0], a[1]*scale[1], a[2]*scale[2], trans[0]);
	Vector4Set(out->b, b[0]*scale[0], b[1]*scale[1], b[2]*scale[2], trans[1]);
	Vector4Set(out->c, c[0]*scale[0], c[1]*scale[1], c[2]*scale[2], trans[2]);
}

void Matrix3x4_Multiply (matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2)
{
	vec4_t a, b, c, d;

	Vector4Scale(mat2.a, mat1.a[0], a);
	Vector4Scale(mat2.b, mat1.a[1], b);
	Vector4Scale(mat2.c, mat1.a[2], c);
	Vector4Add(a, b, d);
	Vector4Add(d, c, d);
	Vector4Set(out->a, d[0], d[1], d[2], d[3] + mat1.a[3]);

	Vector4Scale(mat2.a, mat1.b[0], a);
	Vector4Scale(mat2.b, mat1.b[1], b);
	Vector4Scale(mat2.c, mat1.b[2], c);
	Vector4Add(a, b, d);
	Vector4Add(d, c, d);
	Vector4Set(out->b, d[0], d[1], d[2], d[3] + mat1.b[3]);

	Vector4Scale(mat2.a, mat1.c[0], a);
	Vector4Scale(mat2.b, mat1.c[1], b);
	Vector4Scale(mat2.c, mat1.c[2], c);
	Vector4Add(a, b, d);
	Vector4Add(d, c, d);
	Vector4Set(out->c, d[0], d[1], d[2], d[3] + mat1.c[3]);
}

void Matrix3x4_Scale (matrix3x4_t *out, matrix3x4_t in, float scale)
{
	Vector4Scale(in.a, scale, out->a);
	Vector4Scale(in.b, scale, out->b);
	Vector4Scale(in.c, scale, out->c);
}

void Matrix3x4_Add (matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2)
{
	Vector4Add(mat1.a, mat2.a, out->a);
	Vector4Add(mat1.b, mat2.b, out->b);
	Vector4Add(mat1.c, mat2.c, out->c);
}

void Matrix3x4_Copy (matrix3x4_t *out, matrix3x4_t in)
{
	Vector4Copy(in.a, out->a);
	Vector4Copy(in.b, out->b);
	Vector4Copy(in.c, out->c);
}

void Matrix3x4GenRotate (matrix3x4_t *out, float angle, const vec3_t axis)
{
	float ck = cos(angle), sk = sin(angle);

	Vector4Set(out->a, axis[0]*axis[0]*(1-ck)+ck, axis[0]*axis[1]*(1-ck)-axis[2]*sk, axis[0]*axis[2]*(1-ck)+axis[1]*sk, 0);
	Vector4Set(out->b, axis[1]*axis[0]*(1-ck)+axis[2]*sk, axis[1]*axis[1]*(1-ck)+ck, axis[1]*axis[2]*(1-ck)-axis[0]*sk, 0);
	Vector4Set(out->c, axis[0]*axis[2]*(1-ck)-axis[1]*sk, axis[1]*axis[2]*(1-ck)+axis[0]*sk, axis[2]*axis[2]*(1-ck)+ck, 0);
}

void R_CalcTangent
( 	const vec3_t v0, const vec3_t v1, const vec3_t v2,
	const vec2_t st0, const vec2_t st1, const vec2_t st2,
	const vec3_t normal, vec4_t tangent )
{
	float	s;
	vec3_t	v01, v02, temp1, temp2, temp3;
	vec3_t	bitangent;
	
	VectorSubtract (v1, v0, v01);
	VectorSubtract (v2, v0, v02);
	
	// get the tangent
	s =	(st1[0] - st0[0]) * (st2[1] - st0[1]) -
		(st2[0] - st0[0]) * (st1[1] - st0[1]);
	s = 1.0f / s;

	VectorScale (v01, st2[1] - st0[1], temp1);
	VectorScale (v02, st1[1] - st0[1], temp2);
	VectorSubtract (temp1, temp2, temp3);
	VectorScale (temp3, s, tangent);
	VectorNormalize (tangent);
	
	// now get the bitangent (used to check handedness)
	// bitangent will be recomputed in vertex shaders.
	VectorScale (v02, st1[0] - st0[0], temp1);
	VectorScale (v01, st2[0] - st0[0], temp2);
	VectorSubtract (temp1, temp2, temp3);
	VectorScale (temp3, s, bitangent);
	VectorNormalize (bitangent);

	// handedness
	CrossProduct (normal, tangent, temp1);
	if (DotProduct (temp1, bitangent) < 0.0f)
		tangent[3] = -1.0f;
	else
		tangent[3] = 1.0f;
}

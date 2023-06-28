// mathlib.c -- math primitives

#include "cmdlib.h"
#include "mathlib.h"

vec3_t vec3_origin = {0,0,0};

#ifndef MATH_INLINE

qboolean VectorCompare (vec3_t v1, vec3_t v2)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
		if (fabs(v1[i]-v2[i]) > EQUAL_EPSILON)
			return false;
			
	return true;
}

vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0]-vb[0];
	out[1] = va[1]-vb[1];
	out[2] = va[2]-vb[2];
}

void _VectorAdd (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0]+vb[0];
	out[1] = va[1]+vb[1];
	out[2] = va[2]+vb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale (vec3_t v, vec_t scale, vec3_t out)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}

vec_t VectorNormalize (vec3_t in, vec3_t out)
{
	vec_t	length, ilength;

	length = sqrt (in[0]*in[0] + in[1]*in[1] + in[2]*in[2]);
	if (length == 0)
	{
		VectorClear (out);
		return 0;
	}

	ilength = 1.0/length;
	out[0] = in[0]*ilength;
	out[1] = in[1]*ilength;
	out[2] = in[2]*ilength;

	return length;
}

vec_t ColorNormalize (vec3_t in, vec3_t out)
{
	float	max, scale;

	max = in[0];
	if (in[1] > max)
		max = in[1];
	if (in[2] > max)
		max = in[2];

	if (max == 0)
		return 0;

	scale = 1.0 / max;

	VectorScale (in, scale, out);

	return max;
}


void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = BOGUS_RANGE;
	maxs[0] = maxs[1] = maxs[2] = -BOGUS_RANGE;
}

void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

qboolean RayPlaneIntersect(vec3_t p_n, vec_t p_d, vec3_t l_o, vec3_t l_n,
        vec3_t res)
    {
    float dot, t;

    dot = DotProduct(p_n, l_n);

    if(dot > -0.001)
        return false;

    t = (p_d - (l_o[0] * p_n[0]) - (l_o[1] * p_n[1]) - (l_o[2] * p_n[2])) / dot;

    res[0] = l_o[0] + (t * l_n[0]);
    res[1] = l_o[1] + (t * l_n[1]);
    res[2] = l_o[2] + (t * l_n[2]);
                
    return true;
    }
#endif

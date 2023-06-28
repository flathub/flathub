#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qrad.h"

// Contains the routines for loading images into buffers
// TODO: maybe move the rest of the image formats here?

static void raw_sample (const byte *texture, int tex_w, int tex_h, int s, int t, vec4_t out)
{
	int i;
	
	if (s >= tex_w)
		s = tex_w-1;
	if (s < 0)
		s = 0;
	if (t >= tex_h)
		t = tex_h-1;
	if (t < 0)
		t = 0;
	
	for (i = 0; i < 4; i++)
		out[i] = (float)texture[(t*tex_w+s)*4+i]/255.0f;
}

// Used for sampling textures on the CPU.
void bilinear_sample (const byte *texture, int tex_w, int tex_h, float u, float v, vec4_t out)
{
	int i;
 	int x, y;
 	float u_ratio, v_ratio, u_opposite, v_opposite;
 	vec4_t sample1, sample2, sample3, sample4;
 	
	u = u * tex_w - 0.5;
	v = v * tex_h - 0.5;
	x = floor(u);
	y = floor(v);
	u_ratio = u - x;
	v_ratio = v - y;
	u_opposite = 1 - u_ratio;
	v_opposite = 1 - v_ratio;
	
#define SAMPLE(x,y,out) raw_sample (texture, tex_w, tex_h, x, y, out)
	SAMPLE (x, y, sample1);
	SAMPLE (x+1, y, sample2);
	SAMPLE (x, y+1, sample3);
	SAMPLE (x+1, y+1, sample4);
#undef SAMPLE
	
	for (i = 0; i < 4; i++)
		out[i] =	(sample1[i] * u_opposite + sample2[i] * u_ratio) * v_opposite +
					(sample3[i] * u_opposite + sample4[i] * u_ratio) * v_ratio;
}

void SaveTGA (const byte *texture, int tex_w, int tex_h, const char *name)
{
    byte		*buffer;
    FILE		*f;
    int		i, c, temp;
    
    buffer = malloc(tex_w*tex_h*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = tex_w&255;
	buffer[13] = tex_w>>8;
	buffer[14] = tex_h&255;
	buffer[15] = tex_h>>8;
	buffer[16] = 24;	// pixel size
	
    memcpy (buffer+18, texture, tex_w*tex_h*3);

	// swap rgb to bgr
	c = 18+tex_w*tex_h*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (name, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);

	free (buffer);
}

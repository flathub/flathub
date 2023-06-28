/*
Copyright (C) 2009-2014 COR Entertainment, LLC

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
// r_program.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

//GLSL
PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObjectARB	= NULL;
PFNGLDELETEOBJECTARBPROC			glDeleteObjectARB			= NULL;
PFNGLUSEPROGRAMOBJECTARBPROC		glUseProgramObjectARB		= NULL;
PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObjectARB		= NULL;
PFNGLSHADERSOURCEARBPROC			glShaderSourceARB			= NULL;
PFNGLCOMPILESHADERARBPROC			glCompileShaderARB			= NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameterivARB	= NULL;
PFNGLATTACHOBJECTARBPROC			glAttachObjectARB			= NULL;
PFNGLGETINFOLOGARBPROC				glGetInfoLogARB				= NULL;
PFNGLLINKPROGRAMARBPROC				glLinkProgramARB			= NULL;
PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocationARB		= NULL;
PFNGLUNIFORM4IARBPROC				glUniform4iARB				= NULL;
PFNGLUNIFORM4FARBPROC				glUniform4fARB				= NULL;
PFNGLUNIFORM3FARBPROC				glUniform3fARB				= NULL;
PFNGLUNIFORM2FARBPROC				glUniform2fARB				= NULL;
PFNGLUNIFORM1IARBPROC				glUniform1iARB				= NULL;
PFNGLUNIFORM1FARBPROC				glUniform1fARB				= NULL;
PFNGLUNIFORM4IVARBPROC				glUniform4ivARB				= NULL;
PFNGLUNIFORM4FVARBPROC				glUniform4fvARB				= NULL;
PFNGLUNIFORM3FVARBPROC				glUniform3fvARB				= NULL;
PFNGLUNIFORM2FVARBPROC				glUniform2fvARB				= NULL;
PFNGLUNIFORM1IVARBPROC				glUniform1ivARB				= NULL;
PFNGLUNIFORM1FVARBPROC				glUniform1fvARB				= NULL;
PFNGLUNIFORMMATRIX3FVARBPROC		glUniformMatrix3fvARB		= NULL;
PFNGLUNIFORMMATRIX3X4FVARBPROC		glUniformMatrix3x4fvARB		= NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC	 glVertexAttribPointerARB	= NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC		glBindAttribLocationARB		= NULL;
PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog			= NULL;

// Used for dumping internal assembly of GLSL programs. Purely for developer
// use, end-users need not have this OpenGL extension supported.
/*#define DUMP_GLSL_ASM*/
#ifdef DUMP_GLSL_ASM
static void (*glGetProgramBinary) (GLuint program, GLsizei bufsize, GLsizei *length, GLenum *binaryFormat, void *binary) = NULL;
#endif

#define STRINGIFY(...) #__VA_ARGS__

//GLSL Programs

// Functions and other declarations for the GLSL standard vertex library.
// Prepended to every GLSL vertex shader. We rely on the GLSL compiler to throw
// away anything that a given shader doesn't use.
static char vertex_library[] = STRINGIFY (

	// common declarations, must be used the same way in every script if they're
	// used at all
	attribute vec4 tangent;
)

#define IFDYNAMIC(n,...) \
"\n#if DYNAMIC >= " #n "\n" \
#__VA_ARGS__ \
"\n#endif\n"

	// Dynamic Lighting
	// Relies on DYNAMIC being defined as a macro within the GLSL, but don't 
	// worry. R_LoadGLSLProgram handles that for you.
IFDYNAMIC (1, 
	uniform vec3 lightPosition[DYNAMIC];
	varying vec3 LightVec[DYNAMIC];
)
STRINGIFY (
	varying vec3 EyeDir;
	
	vec4 viewVertex;
	mat3 tangentSpaceTransform;
	
	// arguments given in model space
	void computeDynamicLightingVert (vec4 vertex, vec3 normal, vec4 tangent)
	{
		vec3 bitangent = tangent.w * cross (normal, tangent.xyz);
		tangentSpaceTransform = transpose (mat3 (gl_NormalMatrix * tangent.xyz, gl_NormalMatrix * bitangent, gl_NormalMatrix * normal));
		
		viewVertex = gl_ModelViewMatrix * vertex;
		
		EyeDir = tangentSpaceTransform * (-viewVertex.xyz);
)
IFDYNAMIC (1, LightVec[0] = tangentSpaceTransform * (lightPosition[0] - viewVertex.xyz);)
IFDYNAMIC (2, LightVec[1] = tangentSpaceTransform * (lightPosition[1] - viewVertex.xyz);)
IFDYNAMIC (3, LightVec[2] = tangentSpaceTransform * (lightPosition[2] - viewVertex.xyz);)
IFDYNAMIC (4, LightVec[3] = tangentSpaceTransform * (lightPosition[3] - viewVertex.xyz);)
IFDYNAMIC (5, LightVec[4] = tangentSpaceTransform * (lightPosition[4] - viewVertex.xyz);)
IFDYNAMIC (6, LightVec[5] = tangentSpaceTransform * (lightPosition[5] - viewVertex.xyz);)
IFDYNAMIC (7, LightVec[6] = tangentSpaceTransform * (lightPosition[6] - viewVertex.xyz);)
IFDYNAMIC (8, LightVec[7] = tangentSpaceTransform * (lightPosition[7] - viewVertex.xyz);)
STRINGIFY (
	}

	// Mesh Animation
	uniform mat3x4 bonemats[70]; // Keep this equal to SKELETAL_MAX_BONEMATS
	uniform int GPUANIM; // 0 for none, 1 for IQM skeletal, 2 for MD2 lerp
	
	// MD2 only
	uniform float lerp; // 1.0 = all new vertex, 0.0 = all old vertex

	// IQM only
	attribute vec4 weights;
	attribute vec4 bones;
	
	// MD2 only
	attribute vec3 oldvertex;
	attribute vec3 oldnormal;
	attribute vec4 oldtangent;
	
	// anim_compute () output
	vec4 anim_vertex;
	vec3 anim_normal;
	vec4 anim_tangent;
	
	// if dotangent is true, compute anim_tangent and anim_tangent_w
	// if donormal is true, compute anim_normal
	// hopefully the if statements for these booleans will get optimized out
	void anim_compute (bool dotangent, bool donormal)
	{
		if (GPUANIM == 1)
		{
			mat3x4 m = bonemats[int(bones.x)] * weights.x;
			m += bonemats[int(bones.y)] * weights.y;
			m += bonemats[int(bones.z)] * weights.z;
			m += bonemats[int(bones.w)] * weights.w;
			
			anim_vertex = vec4 (gl_Vertex * m, gl_Vertex.w);
			if (donormal)
				anim_normal = vec4 (gl_Normal, 0.0) * m;
			if (dotangent)
				anim_tangent = vec4 (vec4 (tangent.xyz, 0.0) * m, tangent.w);
		}
		else if (GPUANIM == 2)
		{
			anim_vertex = mix (vec4 (oldvertex, 1), gl_Vertex, lerp);
			if (donormal)
				anim_normal = normalize (mix (oldnormal, gl_Normal, lerp));
			if (dotangent)
				anim_tangent = mix (oldtangent, tangent, lerp);
		}
		else
		{
			anim_vertex = gl_Vertex;
			if (donormal)
				anim_normal = gl_Normal;
			if (dotangent)
				anim_tangent = tangent;
		}
	}
);

// Functions and other declarations for the GLSL standard fragment library.
// Prepended to every GLSL fragment shader. We rely on the GLSL compiler to
// throw away anything that a given shader doesn't use.
static char fragment_library[] =

	// Dynamic Lighting
	// Relies on DYNAMIC being defined as a macro within the GLSL, but don't 
	// worry. R_LoadGLSLProgram handles that for you.
IFDYNAMIC (1,
	uniform vec3 lightAmount[DYNAMIC];
	uniform float lightCutoffSquared[DYNAMIC];
	varying vec3 LightVec[DYNAMIC];
)
STRINGIFY (
	varying vec3 EyeDir;
	
	// increase this number to tone down dynamic lighting more
	const float attenuation_boost = 1.5;
	
	// normal argument given in tangent space
	void ComputeForLight (	const vec3 normal, const float specular,
							const vec3 nEyeDir, const vec3 textureColor3,
							const vec3 LightVec, const vec3 lightAmount, const float lightCutoffSquared,
							const float shadowCoef, inout vec3 ret )
	{
		float distanceSquared = dot (LightVec, LightVec);
		if (distanceSquared < lightCutoffSquared)
		{
			// If we get this far, the fragment is within range of the 
			// dynamic light
			vec3 attenuation = clamp (vec3 (1.0) - attenuation_boost * vec3 (distanceSquared) / lightAmount, vec3 (0.0), vec3 (1.0));
			vec3 relativeLightDirection = LightVec / sqrt (distanceSquared);
			float diffuseTerm = max (0.0, dot (relativeLightDirection, normal));
			vec3 specularAdd = vec3 (0.0, 0.0, 0.0);
	
			if (diffuseTerm > 0.0)
			{
				vec3 halfAngleVector = normalize (relativeLightDirection + nEyeDir);
			
				float specularTerm = clamp (dot (normal, halfAngleVector), 0.0, 1.0);
				specularTerm = pow (specularTerm, 32.0);

				specularAdd = vec3 (specular * specularTerm / 2.0);
			}
			vec3 swamp = attenuation * attenuation;
			swamp *= swamp;
			swamp *= swamp;
			swamp *= swamp;
	
			ret += shadowCoef * (((vec3 (0.5) - swamp) * diffuseTerm + swamp) * textureColor3 + specularAdd) * attenuation;
		}
	}
	vec3 computeDynamicLightingFrag (const vec3 textureColor, const vec3 normal, const float specular, const float shadowCoef)
	{
		vec3 ret = vec3 (0.0);
		vec3 nEyeDir = normalize (EyeDir);
		vec3 textureColor3 = textureColor * 3.0;
)
		// dynamic shadows currently only get cast by the first
		// (most influential) dynamic light. That could change
		// eventually.
IFDYNAMIC (1, ComputeForLight (normal, specular, nEyeDir, textureColor3, LightVec[0], lightAmount[0], lightCutoffSquared[0], shadowCoef, ret);)
IFDYNAMIC (2, ComputeForLight (normal, specular, nEyeDir, textureColor3, LightVec[1], lightAmount[1], lightCutoffSquared[1], 1.0, ret);)
IFDYNAMIC (3, ComputeForLight (normal, specular, nEyeDir, textureColor3, LightVec[2], lightAmount[2], lightCutoffSquared[2], 1.0, ret);)
IFDYNAMIC (4, ComputeForLight (normal, specular, nEyeDir, textureColor3, LightVec[3], lightAmount[3], lightCutoffSquared[3], 1.0, ret);)
IFDYNAMIC (5, ComputeForLight (normal, specular, nEyeDir, textureColor3, LightVec[4], lightAmount[4], lightCutoffSquared[4], 1.0, ret);)
IFDYNAMIC (6, ComputeForLight (normal, specular, nEyeDir, textureColor3, LightVec[5], lightAmount[5], lightCutoffSquared[5], 1.0, ret);)
IFDYNAMIC (7, ComputeForLight (normal, specular, nEyeDir, textureColor3, LightVec[6], lightAmount[6], lightCutoffSquared[6], 1.0, ret);)
IFDYNAMIC (8, ComputeForLight (normal, specular, nEyeDir, textureColor3, LightVec[7], lightAmount[7], lightCutoffSquared[7], 1.0, ret);)
STRINGIFY (
		return ret;
	}
	
	// Shadow Mapping
	uniform int sunstatic_enabled, otherstatic_enabled;
	uniform shadowsampler_t sunstatic_texture, otherstatic_texture, dynamic_texture; 
	uniform float sunstatic_pixelOffset, otherstatic_pixelOffset, dynamic_pixelOffset;
)
"\n#ifndef AMD_GPU\n#define fudge _fudge\n"
STRINGIFY (
	float lookup (vec2 offSet, sampler2DShadow Map, vec4 ShadowCoord)
	{	
		return shadow2DProj(Map, ShadowCoord + vec4(offSet * ShadowCoord.w, 0.05, 0.0) ).w;
	}
)
"\n#else\n#define fudge 0.0\n"
STRINGIFY (
	float lookup (vec2 offSet, sampler2D Map, vec4 ShadowCoord)
	{
		vec4 shadowCoordinateWdivide = (ShadowCoord + vec4(offSet * ShadowCoord.w, 0.0, 0.0)) / ShadowCoord.w ;
		// Used to lower moir pattern and self-shadowing
		shadowCoordinateWdivide.z += 0.0005;

		float distanceFromLight = texture2D(Map, shadowCoordinateWdivide.xy).z;

		if (ShadowCoord.w > 0.0)
			return distanceFromLight < shadowCoordinateWdivide.z ? 0.25 : 1.0 ;
		return 1.0;
	}
)
"\n#endif\n"
STRINGIFY (
	float lookupShadow (shadowsampler_t Map, vec4 ShadowCoord, float offset, float _fudge)
	{
		float shadow = 1.0;

		if (ShadowCoord.w > 1.0)
		{
			vec2 o = mod(floor(gl_FragCoord.xy), 2.0);
			
			shadow += lookup ((vec2(-1.5, 1.5) + o) * offset, Map, ShadowCoord);
			shadow += lookup ((vec2( 0.5, 1.5) + o) * offset, Map, ShadowCoord);
			shadow += lookup ((vec2(-1.5, -0.5) + o) * offset, Map, ShadowCoord);
			shadow += lookup ((vec2( 0.5, -0.5) + o) * offset, Map, ShadowCoord);
			shadow *= 0.25 ;
		}
		shadow += fudge; 
		if(shadow > 1.0)
			shadow = 1.0;
		
		return shadow;
	}
	float lookup_sunstatic (vec4 sPos, float _fudge)
	{
		if (sunstatic_enabled == 0)
			return 1.0;
		vec4 coord = gl_TextureMatrix[5] * sPos;
		return lookupShadow (sunstatic_texture, coord, sunstatic_pixelOffset, fudge);
	}
	float lookup_otherstatic_always (vec4 sPos, float _fudge)
	{
		vec4 coord = gl_TextureMatrix[6] * sPos;
		if (coord.w <= 1.0)
			return 1.0;
		return lookupShadow (otherstatic_texture, coord, otherstatic_pixelOffset, fudge);
	}
	float lookup_otherstatic (vec4 sPos, float _fudge)
	{
		if (otherstatic_enabled == 0)
			return 1.0;
		return lookup_otherstatic_always (sPos, fudge);
	}
	float lookup_dynamic (vec4 sPos, float _fudge)
	{
		vec4 coord = gl_TextureMatrix[7] * sPos;
		if (coord.w <= 1.0)
			return 1.0;
		return lookupShadow (dynamic_texture, coord, dynamic_pixelOffset, fudge);
	}
);




//world Surfaces
static char world_vertex_program[] = STRINGIFY (
	uniform vec3 staticLightPosition;
	uniform float rsTime;
	uniform int LIQUID;
	uniform int FOG;
	uniform int PARALLAX;
	uniform int SHINY;
	
	const float Eta = 0.66;
	const float FresnelPower = 5.0;
	const float F = ((1.0-Eta) * (1.0-Eta))/((1.0+Eta) * (1.0+Eta));

	varying float FresRatio;
	varying vec3 LightDir;
	varying vec3 StaticLightDir;
	varying vec4 sPos;
	varying float fog;

	void main( void )
	{
		sPos = gl_Vertex;

		gl_Position = ftransform();
		
		computeDynamicLightingVert (gl_Vertex, gl_Normal, tangent);
		
		if(SHINY > 0)
		{
			vec3 norm = vec3(0, 0, 1); 

			vec3 refeyeDir = viewVertex.xyz / viewVertex.w;
			refeyeDir = normalize(refeyeDir);

			FresRatio = F + (1.0-F) * pow((1.0-dot(refeyeDir,norm)),FresnelPower);
		}

		gl_FrontColor = gl_Color;
		
		if (PARALLAX > 0)
		{
			StaticLightDir = tangentSpaceTransform * ((gl_ModelViewMatrix * vec4 (staticLightPosition, 1.0)).xyz - viewVertex.xyz);
		}

		// pass any active texunits through
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_TexCoord[1] = gl_MultiTexCoord1;

		//fog

		if(FOG > 0){
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 1.0);
		}
	}
);

static char world_fragment_program[] = STRINGIFY (
	uniform sampler2D surfTexture;
	uniform sampler2D HeightTexture;
	uniform sampler2D NormalTexture;
	uniform sampler2D lmTexture;
	uniform sampler2D chromeTex;
	uniform int FOG;
	uniform int PARALLAX;
	uniform int SHINY;
	
	uniform sampler2D liquidTexture;
	uniform sampler2D liquidNormTex;
	uniform int LIQUID;
	uniform float rsTime;

	varying float FresRatio;
	varying vec4 sPos;
	varying vec3 StaticLightDir;
	varying float fog;

	// results of liquid_effects function
	vec4 bloodColor;
	vec2 liquidDisplacement;
	
	// this function will be inlined by the GLSL compiler.
	void liquid_effects (vec3 relativeEyeDirection, vec4 Offset, vec2 BaseTexCoords)
	{
		bloodColor = vec4 (0.0);
		liquidDisplacement = vec2 (0.0);
		if (LIQUID > 0)
		{
			//for liquid fx scrolling
			vec2 texco = BaseTexCoords;
			texco.t -= rsTime*1.0/LIQUID;

			vec2 texco2 = BaseTexCoords;
			texco2.t -= rsTime*0.9/LIQUID;
			//shift the horizontal here a bit
			texco2.s /= 1.5;

			vec2 TexCoords = Offset.xy * relativeEyeDirection.xy + BaseTexCoords;

			vec2 noiseVec = normalize (texture2D (liquidNormTex, texco)).xy;
			noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

			vec2 noiseVec2 = normalize (texture2D (liquidNormTex, texco2)).xy;
			noiseVec2 = (noiseVec2 * 2.0 - 0.635) * 0.035;

			if (LIQUID > 2)
			{
				vec2 texco3 = BaseTexCoords;
				texco3.t -= rsTime*0.6/LIQUID;

				vec2 noiseVec3 = normalize (texture2D (liquidNormTex, texco3)).xy;
				noiseVec3 = (noiseVec3 * 2.0 - 0.635) * 0.035;

				vec4 diffuse1 = texture2D (liquidTexture, 2.0 * texco + noiseVec + TexCoords);
				vec4 diffuse2 = texture2D (liquidTexture, 2.0 * texco2 + noiseVec2 + TexCoords);
				vec4 diffuse3 = texture2D (liquidTexture, 2.0 * texco3 + noiseVec3 + TexCoords);
				bloodColor = max (diffuse1, diffuse2);
				bloodColor = max (bloodColor, diffuse3);
			}
			else
			{
				// used for water effect only
				liquidDisplacement = noiseVec + noiseVec2;
			}
		}
	}

	void main( void )
	{
		vec4 diffuse;
		vec4 lightmap;
		vec4 alphamask;
		float distanceSquared;
		vec3 halfAngleVector;
		float specularTerm;
		float swamp;
		float attenuation;
		vec4 litColour;
		float statshadowval;

		vec3 relativeEyeDirection = normalize( EyeDir );
		vec3 relativeLightDirection = normalize (StaticLightDir);

		vec4 normal = texture2D (NormalTexture, gl_TexCoord[0].xy);
		normal.xyz = 2.0 * (normal.xyz - vec3 (0.5));
		vec3 textureColour = texture2D (surfTexture, gl_TexCoord[0].xy).rgb;

		lightmap = texture2D( lmTexture, gl_TexCoord[1].st );
		alphamask = texture2D( surfTexture, gl_TexCoord[0].xy );

		//shadows
		statshadowval = lookup_sunstatic (sPos, 0.2);

		if (PARALLAX > 0) 
		{
			vec4 Offset = texture2D (HeightTexture, gl_TexCoord[0].xy);
			Offset = Offset * 0.04 - 0.02;
			
			// Liquid effects only get applied if parallax mapping is on for the
			// surface.
			liquid_effects (relativeEyeDirection, Offset, gl_TexCoord[0].st);
			
			//do the parallax mapping
			vec2 TexCoords = Offset.xy * relativeEyeDirection.xy + gl_TexCoord[0].xy + liquidDisplacement.xy;

			diffuse = texture2D( surfTexture, TexCoords );
			
			float diffuseTerm = dot (normal.xyz, relativeLightDirection);

			if (diffuseTerm > 0.0)
			{
				halfAngleVector = normalize (relativeLightDirection + relativeEyeDirection);

				specularTerm = clamp (dot (normal.xyz, halfAngleVector), 0.0, 1.0 );
				specularTerm = pow (specularTerm, 32.0);

				litColour = vec4 (specularTerm * normal.a + (3.0 * diffuseTerm) * textureColour, 6.0);
			}
			else
			{
				litColour = vec4( 0.0, 0.0, 0.0, 6.0 );
			}

			gl_FragColor = max(litColour, diffuse * 2.0);
			gl_FragColor = (gl_FragColor * lightmap) + bloodColor;
			gl_FragColor = (gl_FragColor * statshadowval);
			
			// Normalmapping for static lighting. We want any light
			// attenuation to come from the normalmap, not from the normal of
			// the polygon (since the lightmap compiler already accounts for
			// that.) So we calculate how much light we'd lose from the normal
			// of the polygon and give that much back.
			float face_normal_coef = relativeLightDirection[2];
			float normal_coef = diffuseTerm + (1.0 - face_normal_coef);
			gl_FragColor.rgb *= normal_coef;
		}
		else
		{
			diffuse = texture2D(surfTexture, gl_TexCoord[0].xy);
			gl_FragColor = (diffuse * lightmap * 2.0);
			gl_FragColor = (gl_FragColor * statshadowval);
		}

		if(DYNAMIC > 0) 
		{
			lightmap = texture2D(lmTexture, gl_TexCoord[1].st);
		
			float dynshadowval = lookup_dynamic (sPos, 0.2);
			vec3 dynamicColor = computeDynamicLightingFrag (textureColour, normal.xyz, normal.a, dynshadowval);
			gl_FragColor.rgb += dynamicColor;
		}

		gl_FragColor = mix(vec4(0.0, 0.0, 0.0, alphamask.a), gl_FragColor, alphamask.a);

		if(SHINY > 0)
		{
			vec3 reflection = reflect(relativeEyeDirection, normal.xyz);
			vec3 refraction = refract(relativeEyeDirection, normal.xyz, 0.66);

			vec4 Tl = texture2DProj(chromeTex, vec4(reflection.xy, 1.0, 1.0) );
			vec4 Tr = texture2DProj(chromeTex, vec4(refraction.xy, 1.0, 1.0) );

			vec4 cubemap = mix(Tl,Tr,FresRatio);  

			gl_FragColor = max(gl_FragColor, (cubemap * 0.05 * alphamask.a));
		}

		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);


//SHADOWS
static char shadow_vertex_program[] = STRINGIFY (		
	varying vec4 sPos;

	void main( void )
	{
		sPos = gl_Vertex;

		gl_Position = ftransform();

		gl_Position.z -= 0.05; //eliminate z-fighting on some drivers
	}
);

static char shadow_fragment_program[] = STRINGIFY (
	uniform float fadeShadow;
	
	varying vec4 sPos;

	void main( void )
	{
		gl_FragColor = vec4 (1.0/fadeShadow * lookup_otherstatic_always (sPos, 0.3));
	}
);

// Minimap
static char minimap_vertex_program[] = STRINGIFY (
	attribute vec2 colordata;
	
	void main (void)
	{
		vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
		
		gl_Position.xywz = vec4 (pos.xyw, 0.0);
		
		gl_FrontColor.a = pos.z / -2.0;
		
		if (gl_FrontColor.a > 0.0)
		{
			gl_FrontColor.rgb = vec3 (0.5, 0.5 + colordata[0], 0.5);
			gl_FrontColor.a = 1.0 - gl_FrontColor.a;
		}
		else
		{
			gl_FrontColor.rgb = vec3 (0.5, colordata[0], 0);
			gl_FrontColor.a += 1.0;
		}
		
		gl_FrontColor.a *= colordata[1];
	}
);


//RSCRIPTS
static char rscript_vertex_program[] = STRINGIFY (
	uniform vec3 staticLightPosition;
	uniform int envmap;
	uniform int	numblendtextures;
	uniform int FOG;
	// 0 means no lightmap, 1 means lightmap using the main texcoords, and 2
	// means lightmap using its own set of texcoords.
	uniform int lightmap; 
	uniform vec3 meshPosition;
	uniform mat3 meshRotation;
	
	varying float fog;
	varying vec3 orig_normal;
	varying vec3 orig_coord;
	varying vec3 StaticLightDir;
	varying vec4 sPos;
	
	void main ()
	{
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_FrontColor = gl_BackColor = gl_Color;

		sPos = vec4 ((meshRotation * gl_Vertex.xyz) + meshPosition, 1.0);
		
		vec4 maincoord;
		
		if (envmap == 1)
		{
			maincoord.st = normalize (gl_Position.xyz).xy;
			maincoord.pq = vec2 (0.0, 1.0);
		}
		else
		{
			maincoord = gl_MultiTexCoord0;
		}
		
		// XXX: tri-planar projection requires the vertex normal, so don't use
		// the blendmap RScript command on BSP surfaces yet!
		if (numblendtextures != 0)
		{
			orig_normal = gl_Normal.xyz;
			orig_coord = (gl_TextureMatrix[0] * gl_Vertex).xyz;
			gl_TexCoord[0] = maincoord;
		}
		else
		{
			gl_TexCoord[0] = gl_TextureMatrix[0] * maincoord;
		}
		
		if (lightmap == 1)
			gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord0;
		else if (lightmap == 2)
			gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
		
		//fog
		if(FOG > 0)
		{
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 1.0);
		}
		
		computeDynamicLightingVert (gl_Vertex, gl_Normal, tangent);
		
		StaticLightDir = tangentSpaceTransform * ((gl_ModelViewMatrix * vec4 (staticLightPosition, 1.0)).xyz - viewVertex.xyz);
	}
);

static char rscript_fragment_program[] = STRINGIFY (
	uniform sampler2D mainTexture;
	uniform sampler2D mainTexture2;
	uniform sampler2D lightmapTexture;
	uniform sampler2D blendTexture0;
	uniform sampler2D blendTexture1;
	uniform sampler2D blendTexture2;
	uniform sampler2D blendTexture3;
	uniform sampler2D blendTexture4;
	uniform sampler2D blendTexture5;
	uniform sampler2D blendNormalmap0;
	uniform sampler2D blendNormalmap1;
	uniform sampler2D blendNormalmap2;
	uniform int	numblendtextures, numblendnormalmaps;
	uniform int	static_normalmaps;
	uniform int FOG;
	uniform vec2 blendscales[6];
	uniform int normalblendindices[3];
	// 0 means no lightmap, 1 means lightmap using the main texcoords, and 2
	// means lightmap using its own set of texcoords.
	uniform int lightmap;

	varying float fog;
	varying vec3 orig_normal;
	varying vec3 orig_coord;
	varying vec3 StaticLightDir;
	varying vec4 sPos;
	
	// This is tri-planar projection, based on code from NVIDIA's GPU Gems
	// website. Potentially could be replaced with bi-planar projection, for
	// roughly 1/3 less sampling overhead but also less accuracy, or 
	// alternately, for even less overhead and *greater* accuracy, this fancy
	// thing: http://graphics.cs.williams.edu/papers/IndirectionI3D08/
	
	vec4 triplanar_sample (sampler2D tex, vec3 blend_weights, vec2 scale)
	{
		return
			blend_weights[0] * texture2D (tex, orig_coord.yz * scale) +
			blend_weights[1] * texture2D (tex, orig_coord.zx * scale) +
			blend_weights[2] * texture2D (tex, orig_coord.xy * scale);
	}
	
	void main ()
	{
		
		vec4 mainColor = texture2D (mainTexture, gl_TexCoord[0].st);
		vec4 normal = vec4 (0.0, 0.0, 1.0, 0.0);

		if (numblendtextures == 0)
		{
			gl_FragColor = mainColor;
		}
		else
		{
			vec4 mainColor2 = vec4 (0.0);
			if (numblendtextures > 3)
				mainColor2 = texture2D (mainTexture2, gl_TexCoord[0].st);
			
			float totalbrightness =		dot (mainColor.rgb, vec3 (1.0)) +
										dot (mainColor2.rgb, vec3 (1.0));
			mainColor.rgb /= totalbrightness;
			mainColor2.rgb /= totalbrightness;
			
			vec3 blend_weights = abs (normalize (orig_normal));
			blend_weights = (blend_weights - vec3 (0.2)) * 7;
			blend_weights = max (blend_weights, 0);
			blend_weights /= (blend_weights.x + blend_weights.y + blend_weights.z);
			
			// Sigh, GLSL doesn't allow you to index arrays of samplers with
			// variables.
			gl_FragColor = vec4 (0.0);
			
			// TODO: go back to switch-case as soon as we start using GLSL
			// 1.30. 
			if (mainColor.r > 0.0)
				gl_FragColor += triplanar_sample (blendTexture0, blend_weights, blendscales[0]) * mainColor.r;
			if (numblendtextures > 1)
			{
				if (mainColor.g > 0.0)
					gl_FragColor += triplanar_sample (blendTexture1, blend_weights, blendscales[1]) * mainColor.g;
				if (numblendtextures > 2)
				{
					if (mainColor.b > 0.0)
						gl_FragColor += triplanar_sample (blendTexture2, blend_weights, blendscales[2]) * mainColor.b;
					if (numblendtextures > 3)
					{
						if (mainColor2.r > 0.0)
							gl_FragColor += triplanar_sample (blendTexture3, blend_weights, blendscales[3]) * mainColor2.r;
						if (numblendtextures > 4)
						{
							if (mainColor2.g > 0.0)
								gl_FragColor += triplanar_sample (blendTexture4, blend_weights, blendscales[4]) * mainColor2.g;
							if (numblendtextures > 5)
							{
								if (mainColor2.b > 0.0)
									gl_FragColor += triplanar_sample (blendTexture5, blend_weights, blendscales[5]) * mainColor2.b;
							}
						}
					}
				}
			}
			
			if ((DYNAMIC > 0 || static_normalmaps != 0) && numblendnormalmaps > 0)
			{
				float totalnormal = 0.0;
				
				normal = vec4 (0.0);
				
				float normalcoef = normalblendindices[0] >= 3 ? mainColor2[normalblendindices[0]-3] : mainColor[normalblendindices[0]];
				if (normalcoef > 0.0)
				{
					totalnormal += normalcoef;
					normal += triplanar_sample (blendNormalmap0, blend_weights, blendscales[normalblendindices[0]]) * normalcoef;
				}
				if (numblendnormalmaps > 1)
				{
					normalcoef = normalblendindices[1] >= 3 ? mainColor2[normalblendindices[1]-3] : mainColor[normalblendindices[1]];
					if (normalcoef > 0.0)
					{
						totalnormal += normalcoef;
						normal += triplanar_sample (blendNormalmap1, blend_weights, blendscales[normalblendindices[1]]) * normalcoef;
					}
					if (numblendnormalmaps > 2)
					{
						normalcoef = normalblendindices[2] >= 3 ? mainColor2[normalblendindices[2]-3] : mainColor[normalblendindices[2]];
						if (normalcoef > 0.0)
						{
							totalnormal += normalcoef;
							normal += triplanar_sample (blendNormalmap2, blend_weights, blendscales[normalblendindices[2]]) * normalcoef;
						}
					}
				}
				
				// We substitute "straight up" as the normal for channels that
				// don't have corresponding normalmaps.
				normal.xyz += vec3 (0.5, 0.5, 1.0) * (1.0 - totalnormal);
				
				normal.xyz = 2.0 * (normal.xyz - vec3 (0.5));
			}
		}
		
		gl_FragColor *= gl_Color;
		vec3 textureColor = gl_FragColor.rgb;
		
		if (lightmap != 0)
			gl_FragColor *= 2.0 * texture2D (lightmapTexture, gl_TexCoord[1].st);
		
		if (static_normalmaps != 0 && numblendnormalmaps > 0)
		{
			// We want any light attenuation to come from the normalmap, not
			// from the normal of the polygon (since the lightmap compiler
			// already accounts for that.) So we calculate how much light we'd
			// lose from the normal of the polygon and give that much back.

			vec3 RelativeLightDirection = normalize (StaticLightDir);
			// note that relativeLightDirection[2] == dot (RelativeLightDirection, up)
			float face_normal_coef = RelativeLightDirection[2];
			float normalmap_normal_coef = dot (normal.xyz, RelativeLightDirection);
			float normal_coef = normalmap_normal_coef + (1.0 - face_normal_coef);
			gl_FragColor.rgb *= normal_coef;

			//add specularity for appropriate areas
			if (normalmap_normal_coef > 0.0)
			{
				vec3 relativeEyeDirection = normalize( EyeDir );
				vec3 halfAngleVector = normalize (RelativeLightDirection + relativeEyeDirection);

				float specularTerm = clamp (dot (normal.xyz, halfAngleVector), 0.0, 1.0 );
				specularTerm = pow (specularTerm, 32.0) * normal.a;

				//Take away some of the effect of areas that are in shadow
				float shadowTerm = clamp(length(texture2D (lightmapTexture, gl_TexCoord[1].st).rgb)/0.3, 0.15, 0.85);

				//if a pretty dark area, let's try and subtly remove some specularity, without making too sharp of a line artifact
				if(shadowTerm < 0.25)
				{
					specularTerm *= shadowTerm*2.0;
				}

				gl_FragColor.rgb = max(gl_FragColor.rgb, vec3 (specularTerm + normalmap_normal_coef * textureColor * shadowTerm));
			}
		}

		gl_FragColor.rgb *= lookup_sunstatic (sPos, 0.2);

		if (DYNAMIC > 0)
		{
			vec3 dynamicColor = computeDynamicLightingFrag (textureColor, normal.xyz, normal.a, 1.0);
			gl_FragColor.rgb += dynamicColor;
		}

		if (FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);

// Old-style per-vertex water effects
// These days commonly combined with transparency to form a mist volume effect
// Be sure to check the warptest map when you change this
// TODO: both passes into a single run of this shader, using a fragment
// shader. If each pass has an alpha of n, then the the output (incl. alpha)
// should be n * pass2 + (n - n*n) * pass1.
static char warp_vertex_program[] = STRINGIFY (
	uniform float time;
	uniform int warpvert;
	uniform int envmap; // select which pass
	
	// = 1/2 wave amplitude on each axis
	// = 1/4 wave amplitude on both axes combined
	const float wavescale = 2.0;
	
	void main ()
	{
		gl_FrontColor = gl_Color;
		
		// warping effect
		vec4 vert = gl_Vertex;
		
		if (warpvert != 0)
		{
			vert[2] += wavescale *
			(
				sin (vert[0] * 0.025 + time) * sin (vert[2] * 0.05 + time) + 
				sin (vert[1] * 0.025 + time * 2) * sin (vert[2] * 0.05 + time) - 
				2 // top of brush = top of waves
			);
		}
		
		gl_Position = gl_ModelViewProjectionMatrix * vert;
		
		if (envmap == 0) // main texture
		{
			gl_TexCoord[0] = gl_TextureMatrix[0] * vec4
			(
				gl_MultiTexCoord0.s + 4.0 * sin (gl_MultiTexCoord0.t / 8.0 + time),
				gl_MultiTexCoord0.t + 4.0 * sin (gl_MultiTexCoord0.s / 8.0 + time),
				0, 1
			);
		}
		else // env map texture (if enabled)
		{
			gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
		}
	}
);

//MESHES
static char mesh_vertex_program[] = STRINGIFY (
	uniform vec3 staticLightColor;
	uniform vec3 staticLightPosition;
	uniform vec3 totalLightPosition;
	uniform vec3 meshPosition;
	uniform mat3 meshRotation;
	uniform float time;
	uniform int FOG;
	uniform int TEAM;
	uniform float useShell; // doubles as shell scale
	uniform int fromView;
	uniform int useCube;
	// For now, only applies to vertexOnly. If 0, don't do the per-vertex shading.
	uniform int doShading; 
	// 0 means no lightmap, 1 means lightmap using the main texcoords, and 2
	// means lightmap using its own set of texcoords.
	uniform int lightmap;
	
	const float Eta = 0.66;
	const float FresnelPower = 5.0;
	const float F = ((1.0-Eta) * (1.0-Eta))/((1.0+Eta) * (1.0+Eta));

	varying vec4 sPos;
	varying vec3 StaticLightDir;
	varying float fog;
	varying float FresRatio;
	varying vec3 SSlightVec, SSEyeDir, worldNormal;

	void subScatterVS(in vec4 ecVert)
	{
		if(useShell == 0 && useCube == 0)
		{
			SSEyeDir = vec3(gl_ModelViewMatrix * anim_vertex);
			SSlightVec = totalLightPosition - ecVert.xyz;
		}
	}

	void main()
	{
		fog = 0.0;

		anim_compute (true, true);
		
		if (useShell > 0)
			anim_vertex += normalize (vec4 (anim_normal, 0)) * useShell;		
		
		gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;
		subScatterVS (gl_Position);

		sPos = vec4 ((meshRotation * anim_vertex.xyz) + meshPosition, 1.0);
		
		worldNormal = normalize (gl_NormalMatrix * anim_normal);
		
		computeDynamicLightingVert (anim_vertex, anim_normal, anim_tangent);
		StaticLightDir = tangentSpaceTransform * staticLightPosition;
		
		vec4 neyeDir = gl_ModelViewMatrix * anim_vertex;
		
		if(useShell > 0)
		{
			gl_TexCoord[0] = vec4 ((anim_vertex[1]+anim_vertex[0])/40.0, anim_vertex[2]/40.0 - time, 0.0, 1.0);
		}
		else
		{
			gl_TexCoord[0] = gl_MultiTexCoord0;
			//for scrolling fx
			vec4 texco = gl_MultiTexCoord0;
			texco.s = texco.s + time*1.0;
			texco.t = texco.t + time*2.0;
			gl_TexCoord[2] = texco;
		}
		
		if (lightmap == 1)
			gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord0;
		else if (lightmap == 2)
			gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;

		// vertexOnly is defined as const, so this branch should get optimized
		// out.
		if (vertexOnly == 1)
		{
			// We try to bias toward light instead of shadow, but then make
			// the contrast between light and shadow more pronounced.
			float lightness;
			if (doShading == 1)
			{
				lightness = max (normalize (StaticLightDir).z, 0.0) * 3.0 + 0.25;
				lightness += lightness * lightness * lightness;
			}
			else
			{
				lightness = 1.0;
			}
			gl_FrontColor = gl_BackColor = vec4 (staticLightColor * lightness, 1.0);
			if (FOG == 1) // TODO: team colors with normalmaps disabled!
				gl_FogFragCoord = length (gl_Position);
		}
		else
		{
			if(useCube == 1)
			{
				vec3 refeyeDir = neyeDir.xyz / neyeDir.w;
				refeyeDir = normalize(refeyeDir);

				FresRatio = F + (1.0-F) * pow((1.0-dot(refeyeDir, worldNormal)), FresnelPower);
			}
		
			if(TEAM > 0)
			{
				fog = (gl_Position.z - 100.0)/1000.0;
				if(TEAM == 3)
					fog = clamp(fog, 0.0, 0.5);
				else
					fog = clamp(fog, 0.0, 0.75);
			}
			else if(FOG == 1) 
			{
				fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
				fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear
			}			
		}
	}
);

static char mesh_fragment_program[] = STRINGIFY (
	uniform vec3 staticLightColor;
	uniform vec3 totalLightPosition, totalLightColor;
	uniform sampler2D baseTex;
	uniform sampler2D normalTex;
	uniform sampler2D fxTex;
	uniform sampler2D fx2Tex;
	uniform sampler2D lightmapTexture;
	// 0 means no lightmap, 1 means lightmap using the main texcoords, and 2
	// means lightmap using its own set of texcoords.
	uniform int lightmap;
	uniform int FOG;
	uniform int TEAM;
	uniform int useFX;
	uniform int useCube;
	uniform int useGlow;
	uniform float useShell;
	uniform float shellAlpha;
	uniform int fromView;

	const float SpecularFactor = 0.50;
	//next group could be made uniforms if we want to control this 
	const float MaterialThickness = 2.0; //this val seems good for now
	const vec3 ExtinctionCoefficient = vec3(0.80, 0.12, 0.20); //controls subsurface value
	const float RimScalar = 10.0; //intensity of the rim effect

	varying vec4 sPos;
	varying vec3 StaticLightDir;
	varying float fog;
	varying float FresRatio;
	varying vec3 SSlightVec, SSEyeDir, worldNormal; 

	float halfLambert(in vec3 vect1, in vec3 vect2)
	{
		float product = dot(vect1,vect2);
		return product * 0.5 + 0.5;
	}

	float blinnPhongSpecular(in vec3 normalVec, in vec3 lightVec, in float specPower)
	{
		vec3 halfAngle = normalize(normalVec + lightVec);
		return pow(clamp(0.0,1.0,dot(normalVec,halfAngle)),specPower);
	}

	void main()
	{
		vec3 litColor;
		vec4 fx;
		vec4 glow;
		vec4 scatterCol = vec4(0.0);
		float shadowval = 1.0;

		vec3 textureColour = texture2D( baseTex, gl_TexCoord[0].xy ).rgb * 1.1;
		vec3 normal = 2.0 * ( texture2D( normalTex, gl_TexCoord[0].xy).xyz - vec3( 0.5 ) );

		vec4 alphamask = texture2D( baseTex, gl_TexCoord[0].xy);
		vec4 specmask = texture2D( normalTex, gl_TexCoord[0].xy);

		if(useShell == 0)
			shadowval = lookup_otherstatic (sPos, 0.2) * lookup_sunstatic (sPos, 0.2);
		
		if(useShell == 0 && useCube == 0 && specmask.a < 1.0 && lightmap == 0)
		{
			vec4 SpecColor = vec4 (totalLightColor, 1.0)/2.0;

			//overall brightness should be more of a factor than distance(for example, hold your hand up to block the sun)
			float attenuation = length(clamp(totalLightColor, 0, .05)) * 0.1;
			vec3 wNorm = worldNormal;
			vec3 eVec = normalize(SSEyeDir);
			vec3 lVec = normalize(SSlightVec);

			vec4 dotLN = vec4(halfLambert(lVec, wNorm) * attenuation);

			vec3 indirectLightComponent = vec3(MaterialThickness * max(0.0,dot(-wNorm, lVec)));
			indirectLightComponent += MaterialThickness * halfLambert(-eVec, lVec);
			indirectLightComponent *= attenuation;
			indirectLightComponent.rgb *= ExtinctionCoefficient.rgb;

			vec3 rim = vec3(1.0 - max(0.0,dot(wNorm, eVec)));
			rim *= rim;
			rim *= max(0.0,dot(wNorm, lVec)) * SpecColor.rgb;

			scatterCol = dotLN + vec4(indirectLightComponent, 1.0);
			scatterCol.rgb += (rim * RimScalar * attenuation * scatterCol.a);
			scatterCol.rgb += vec3(blinnPhongSpecular(wNorm, lVec, SpecularFactor*2.0) * attenuation * SpecColor * scatterCol.a * 0.05);
			scatterCol.rgb *= totalLightColor;
			scatterCol.rgb /= (specmask.a * specmask.a);//we use the spec mask for scatter mask, presuming non-spec areas are always soft/skin
		}
		
		vec3 relativeLightDirection = normalize (StaticLightDir);

		float diffuseTerm = dot (normal, relativeLightDirection);
		if (diffuseTerm > 0.0)
		{
			vec3 relativeEyeDirection = normalize (EyeDir);
			vec3 halfAngleVector = normalize (relativeLightDirection + relativeEyeDirection);

			float specularTerm = clamp (dot (normal, halfAngleVector), 0.0, 1.0);
			specularTerm = pow (specularTerm, 32.0);
			
			litColor = vec3 (specularTerm);
			if (lightmap == 0)
				 litColor += (3.0 * diffuseTerm) * textureColour;
		}
		else
		{
			litColor = vec3 (0.0);
		}

		if (lightmap != 0)
		{
			gl_FragColor.rgb = litColor + 2.0 * texture2D (lightmapTexture, gl_TexCoord[1].st).rgb * textureColour;
			gl_FragColor.a = 1.0;
		}
		else if (useShell == 0)
		{
			litColor = litColor * shadowval * staticLightColor;
			gl_FragColor.rgb = max(litColor, textureColour * 0.15);
			gl_FragColor.a = 1.0;
		}
		else
		{
			gl_FragColor.rgb = max (litColor, textureColour * 0.25) * staticLightColor;
			gl_FragColor.a = shellAlpha;
		}

		vec3 dynamicColor = computeDynamicLightingFrag (textureColour, normal, specmask.a, 1.0);
		gl_FragColor.rgb += dynamicColor;
		
		//moving fx texture
		if(useFX > 0)
			fx = texture2D( fxTex, gl_TexCoord[2].xy );
		else
			fx = vec4(0.0, 0.0, 0.0, 0.0);

		gl_FragColor = mix(fx, gl_FragColor + scatterCol, alphamask.a);

		if(useCube > 0 && specmask.a < 1.0)
		{			
			vec3 relEyeDir;
			
			if(fromView > 0)
				relEyeDir = normalize(StaticLightDir);
			else
				relEyeDir = normalize(EyeDir);
			
			vec3 reflection = reflect(relEyeDir, normal);
			vec3 refraction = refract(relEyeDir, normal, 0.66);

			vec4 Tl = texture2D(fx2Tex, reflection.xy );
			vec4 Tr = texture2D(fx2Tex, refraction.xy );
			
			vec4 cubemap = mix(Tl,Tr,FresRatio);
			
			cubemap.rgb = max(gl_FragColor.rgb, cubemap.rgb * litColor);

			gl_FragColor = mix(cubemap, gl_FragColor, specmask.a);
		}
		
		if(useGlow > 0)
		{
			glow = texture2D(fxTex, gl_TexCoord[0].xy );
			gl_FragColor = mix(gl_FragColor, glow, glow.a);
		}		

		if(TEAM == 1)
		{
			gl_FragColor = mix(gl_FragColor, vec4(0.3, 0.0, 0.0, 1.0), fog);
			if(dot(worldNormal, EyeDir) <= 0.01)
			{
				gl_FragColor = max(vec4(1.0, 0.2, 0.0, 1.0) * (fog * 2.0), gl_FragColor);
			}
		}
		else if(TEAM == 2)
		{
			gl_FragColor = mix(gl_FragColor, vec4(0.0, 0.1, 0.4, 1.0), fog);
			if(dot(worldNormal, EyeDir) <= 0.01)
			{
				gl_FragColor = max(vec4(0.0, 0.2, 1.0, 1.0) * (fog * 2.0), gl_FragColor);
			}
		}
		else if(TEAM == 3)
		{
			gl_FragColor = mix(gl_FragColor, vec4(0.0, 0.4, 0.3, 1.0), fog);
			if(dot(worldNormal, EyeDir) <= 0.01)
			{
				gl_FragColor = max(vec4(0.2, 1.0, 0.0, 1.0) * (fog * 2.0), gl_FragColor);
			}	
		}
		else if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);		
	}
);

//GLASS 
static char glass_vertex_program[] = STRINGIFY (

	uniform int FOG;

	varying vec3 r;
	varying float fog;
	varying vec3 orig_normal, normal, vert;

	void main(void)
	{
		anim_compute (false, true);

		gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;

		vec3 u = normalize( vec3(gl_ModelViewMatrix * anim_vertex) ); 	
		vec3 n = normalize(gl_NormalMatrix * anim_normal); 

		r = reflect( u, n );

		normal = n;
		vert = vec3( gl_ModelViewMatrix * anim_vertex );

		orig_normal = anim_normal;

		//fog
	   if(FOG > 0) 
	   {
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear
	   }
	   
	   // for mirroring
	   gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char glass_fragment_program[] = STRINGIFY (

	uniform vec3 LightPos;
	uniform vec3 left;
	uniform vec3 up;
	uniform sampler2D refTexture;
	uniform sampler2D mirTexture;
	uniform int FOG;
	uniform int type; // 1 means mirror only, 2 means glass only, 3 means both

	varying vec3 r;
	varying float fog;
	varying vec3 orig_normal, normal, vert;
	
	void main (void)
	{
		vec3 light_dir = normalize( LightPos - vert );  	
		vec3 eye_dir = normalize( -vert.xyz );  	
		vec3 ref = normalize( -reflect( light_dir, normal ) );  
	
		float ld = max( dot(normal, light_dir), 0.0 ); 	
		float ls = 0.75 * pow( max( dot(ref, eye_dir), 0.0 ), 0.70 ); //0.75 specular, .7 shininess

		float m = -1.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
		
		vec3 n_orig_normal = normalize (orig_normal);
		vec2 coord_offset = vec2 (dot (n_orig_normal, left), dot (n_orig_normal, up));
		vec2 glass_coord = -vec2 (r.x/m + 0.5, r.y/m + 0.5);
		vec2 mirror_coord = vec2 (-gl_TexCoord[0].s, gl_TexCoord[0].t);

		vec4 mirror_color, glass_color;
		if (type == 1)
			mirror_color = texture2D(mirTexture, mirror_coord.st);
		else if (type == 3)
			mirror_color = texture2D(mirTexture, mirror_coord.st + coord_offset.st);
		if (type != 1)
			glass_color = texture2D(refTexture, glass_coord.st + coord_offset.st/2.0);
		
		if (type == 3)
			gl_FragColor = 0.3 * glass_color + 0.3 * mirror_color * vec4 (ld + ls + 0.35);
		else if (type == 2)
			gl_FragColor = glass_color/2.0;
		else if (type == 1)
			gl_FragColor = mirror_color;

		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
	}
);

//NO TEXTURE 
static char blankmesh_vertex_program[] = STRINGIFY (
	void main(void)
	{
		anim_compute (false, false);
		gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char blankmesh_fragment_program[] = STRINGIFY (
	uniform sampler2D baseTex;

	void main (void)
	{		
		vec4 alphamask = texture2D( baseTex, gl_TexCoord[0].xy);

		gl_FragColor = vec4(1.0, 1.0, 1.0, alphamask.a);
	}
);

// For dumping the static lighting into a lightmap texture
static char mesh_extract_lightmap_vertex_program[] = STRINGIFY (
	uniform vec3 staticLightPosition;
	
	varying vec3 StaticLightDir;

	void main()
	{
		anim_compute (true, true);
		StaticLightDir = tangentSpaceTransform * staticLightPosition;
		gl_Position = gl_MultiTexCoord0;
		gl_Position.xy *= 2.0;
		gl_Position.xy -= vec2 (1.0);
	}
);

static char mesh_extract_lightmap_fragment_program[] = STRINGIFY (
	uniform vec3 staticLightColor;

	varying vec3 StaticLightDir;

	void main()
	{
		gl_FragColor.rgb = vec3 (max (1.5 * staticLightColor * normalize (StaticLightDir).z, 0.075));
		gl_FragColor.a = 1.0;
	}
);


static char water_vertex_program[] = STRINGIFY (
	uniform vec3 LightPos;
	uniform float time;
	uniform int	REFLECT;
	uniform int FOG;

	const float Eta = 0.66;
	const float FresnelPower = 2.5;
	const float F = ((1.0-Eta) * (1.0-Eta))/((1.0+Eta) * (1.0+Eta));

	varying float FresRatio;
	varying vec3 lightDir;
	varying vec3 eyeDir;
	varying float fog;

	void main(void)
	{
		gl_Position = ftransform();

		vec4 viewVertex = gl_ModelViewMatrix * gl_Vertex;
		vec3 binormal = tangent.w * cross (gl_Normal, tangent.xyz);
		mat3 tangentSpaceTransform = transpose (mat3 (tangent.xyz, binormal, gl_Normal));

		lightDir = tangentSpaceTransform * ((gl_ModelViewMatrix * vec4 (LightPos, 1.0)).xyz - viewVertex.xyz);

		vec4 neyeDir = gl_ModelViewMatrix * gl_Vertex;
		vec3 refeyeDir = neyeDir.xyz / neyeDir.w;
		refeyeDir = normalize(refeyeDir);

		// The normal is always 0, 0, 1 because water is always up. Thus, 
		// dot (refeyeDir,norm) is always refeyeDir.z
		FresRatio = F + (1.0-F) * pow((1.0-refeyeDir.z),FresnelPower);

		eyeDir = tangentSpaceTransform * (-viewVertex.xyz);

		vec4 texco = gl_MultiTexCoord0;
		if(REFLECT > 0) 
		{
			texco.s = texco.s - LightPos.x/256.0;
			texco.t = texco.t + LightPos.y/256.0;
		}
		gl_TexCoord[0] = texco;

		texco = gl_MultiTexCoord0;
		texco.s = texco.s + time*0.05;
		texco.t = texco.t + time*0.05;
		gl_TexCoord[1] = texco;

		texco = gl_MultiTexCoord0;
		texco.s = texco.s - time*0.05;
		texco.t = texco.t - time*0.05;
		gl_TexCoord[2] = texco;

		//fog
	   if(FOG > 0)
	   {
			fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
			fog = clamp(fog, 0.0, 1.0);
	  	}
	}
);

static char water_fragment_program[] = STRINGIFY (
	varying vec3 lightDir;
	varying vec3 eyeDir;
	varying float FresRatio;

	varying float fog;

	uniform sampler2D refTexture;
	uniform sampler2D normalMap;
	uniform sampler2D baseTexture;

	uniform float TRANSPARENT;
	uniform int FOG;
	
	void main (void)
	{
		vec4 refColor;

		vec3 vVec = normalize(eyeDir);

		refColor = texture2D(refTexture, gl_TexCoord[0].xy);

		vec3 bump = normalize( texture2D(normalMap, gl_TexCoord[1].xy).xyz - 0.5);
		vec3 secbump = normalize( texture2D(normalMap, gl_TexCoord[2].xy).xyz - 0.5);
		vec3 modbump = mix(secbump,bump,0.5);

		vec3 reflection = reflect(vVec,modbump);
		vec3 refraction = refract(vVec,modbump,0.66);

		vec4 Tl = texture2D(baseTexture, reflection.xy);

		vec4 Tr = texture2D(baseTexture, refraction.xy);

		vec4 cubemap = mix(Tl,Tr,FresRatio);

		gl_FragColor = mix(cubemap,refColor,0.5);

		gl_FragColor.a = TRANSPARENT;

		if(FOG > 0)
			gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);

	}
);

//FRAMEBUFFER DISTORTION EFFECTS
static char fb_vertex_program[] = STRINGIFY (
	void main( void )
	{
		gl_Position = ftransform();

		gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char fb_fragment_program[] = STRINGIFY (
	uniform sampler2D fbtexture;
	uniform sampler2D distortiontexture;
	uniform float intensity;
	
	void main(void)
	{
		vec2 noiseVec;
		vec4 displacement;
		
		noiseVec = normalize(texture2D(distortiontexture, gl_TexCoord[0].st)).xy;
		
		// It's a bit of a rigomorole to do partial screen updates. This can
		// go away as soon as the distort texture is a screen sized buffer.
		displacement = gl_TexCoord[0] + vec4 ((noiseVec * 2.0 - vec2 (0.6389, 0.6339)) * intensity, 0, 0);
		gl_FragColor = texture2D (fbtexture, (gl_TextureMatrix[0] * displacement).st);
	}
);

//COLOR SCALING SHADER - because glColor can't go outside the range 0..1
static char colorscale_fragment_program[] = STRINGIFY (
	uniform vec3		scale;
	uniform sampler2D	textureSource;
	
	void main (void)
	{
		gl_FragColor = texture2D (textureSource, gl_TexCoord[0].st) * vec4 (scale, 1.0);
	}
);

//COLOR EXPONENTIATION SHADER - for contrast boosting
static char color_exponent_fragment_program[] = STRINGIFY (
	uniform vec4		exponent;
	uniform sampler2D	textureSource;
	
	void main (void)
	{
		gl_FragColor = pow (texture2D (textureSource, gl_TexCoord[0].st), exponent);
	}
);

//GAUSSIAN BLUR EFFECTS
static char blur_vertex_program[] = STRINGIFY (
	varying vec2	texcoord1, texcoord2, texcoord3,
					texcoord4, texcoord5, texcoord6,
					texcoord7, texcoord8, texcoord9;
	uniform vec2	ScaleU;
	
	void main()
	{
		gl_Position = ftransform();
		
		// If we do all this math here, and let GLSL do its built-in
		// interpolation of varying variables, the math still comes out right,
		// but it's faster.
		texcoord1 = gl_MultiTexCoord0.xy-4.0*ScaleU;
		texcoord2 = gl_MultiTexCoord0.xy-3.0*ScaleU;
		texcoord3 = gl_MultiTexCoord0.xy-2.0*ScaleU;
		texcoord4 = gl_MultiTexCoord0.xy-ScaleU;
		texcoord5 = gl_MultiTexCoord0.xy;
		texcoord6 = gl_MultiTexCoord0.xy+ScaleU;
		texcoord7 = gl_MultiTexCoord0.xy+2.0*ScaleU;
		texcoord8 = gl_MultiTexCoord0.xy+3.0*ScaleU;
		texcoord9 = gl_MultiTexCoord0.xy+4.0*ScaleU;
	}
);

static char blur_fragment_program[] = STRINGIFY (
	varying vec2	texcoord1, texcoord2, texcoord3,
					texcoord4, texcoord5, texcoord6,
					texcoord7, texcoord8, texcoord9;
	uniform sampler2D textureSource;
	
	void main()
	{
		vec4 sum = vec4(0.0);

		// take nine samples
		sum += texture2D(textureSource, texcoord1) * 0.05;
		sum += texture2D(textureSource, texcoord2) * 0.09;
		sum += texture2D(textureSource, texcoord3) * 0.12;
		sum += texture2D(textureSource, texcoord4) * 0.15;
		sum += texture2D(textureSource, texcoord5) * 0.16;
		sum += texture2D(textureSource, texcoord6) * 0.15;
		sum += texture2D(textureSource, texcoord7) * 0.12;
		sum += texture2D(textureSource, texcoord8) * 0.09;
		sum += texture2D(textureSource, texcoord9) * 0.05;

		gl_FragColor = sum;
	}
);

//TRANSPARENT-ONLY BLUR EFFECTS-- image preprocessor for fixing fringing artifacts.
static char defringe_vertex_program[] = STRINGIFY (
	varying vec2	texcoordul, texcoorduc, texcoordur,
					texcoordcl, texcoordcc, texcoordcr,
					texcoordll, texcoordlc, texcoordlr;
	uniform vec2	ScaleU; // Should be set to the pixel size of the image
	
	void main()
	{
		gl_Position = ftransform ();
		
		texcoordul = texcoorduc = texcoordur =
		texcoordcl = texcoordcc = texcoordcr =
		texcoordlr = texcoordlc = texcoordlr = gl_MultiTexCoord0.xy;
		texcoordul.y += 1.0/ScaleU.y;
		texcoorduc.y += 1.0/ScaleU.y;
		texcoordur.y += 1.0/ScaleU.y;
		texcoordll.y -= 1.0/ScaleU.y;
		texcoordlc.y -= 1.0/ScaleU.y;
		texcoordlr.y -= 1.0/ScaleU.y;
		texcoordur.x += 1.0/ScaleU.x;
		texcoordcr.x += 1.0/ScaleU.x;
		texcoordlr.x += 1.0/ScaleU.x;
		texcoordul.x -= 1.0/ScaleU.x;
		texcoordcl.x -= 1.0/ScaleU.x;
		texcoordll.x -= 1.0/ScaleU.x;
	}
);

static char defringe_fragment_program[] = STRINGIFY (
	varying vec2	texcoordul, texcoorduc, texcoordur,
					texcoordcl, texcoordcc, texcoordcr,
					texcoordll, texcoordlc, texcoordlr;
	uniform sampler2D textureSource;
	
	void main()
	{
		vec4 center = texture2D (textureSource, texcoordcc);

		// fast path for non-transparent samples
		if (center.a > 0.0)
		{
			gl_FragColor = center;
			return;
		}

		// for opaque samples, we borrow color from neighbor pixels
		vec4 sum = vec4 (0.0);
		vec4 sample;

		sample = texture2D (textureSource, texcoordul);
		sum += vec4 (sample.xyz * sample.a, sample.a);
		sample = texture2D (textureSource, texcoorduc);
		sum += vec4 (sample.xyz * sample.a, sample.a);
		sample = texture2D (textureSource, texcoordur);
		sum += vec4 (sample.xyz * sample.a, sample.a);
		sample = texture2D (textureSource, texcoordcl);
		sum += vec4 (sample.xyz * sample.a, sample.a);
		sample = texture2D (textureSource, texcoordcr);
		sum += vec4 (sample.xyz * sample.a, sample.a);
		sample = texture2D (textureSource, texcoordll);
		sum += vec4 (sample.xyz * sample.a, sample.a);
		sample = texture2D (textureSource, texcoordlc);
		sum += vec4 (sample.xyz * sample.a, sample.a);
		sample = texture2D (textureSource, texcoordlr);
		sum += vec4 (sample.xyz * sample.a, sample.a);

		// if there are no neighboring non-transparent pixels at all, nothing
		// we can do
		if (sum.a == 0.0)
		{
			gl_FragColor = center;
			return;
		}

		gl_FragColor = vec4 (sum.xyz / sum.a, 0.0);
	}
);

//KAWASE BLUR FILTER
// for an explanation of how this works, see these references:
// https://software.intel.com/en-us/blogs/2014/07/15/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms
// http://www.daionet.gr.jp/~masa/archives/GDC2003_DSTEAL.ppt
static char kawase_vertex_program[] = STRINGIFY (
	varying vec2	texcoord1, texcoord2, texcoord3, texcoord4;

	// scale should be desired blur size (i.e. 9 for 9x9 blur) / 2 - 1. Since
	// the desired blur size is always an odd number, scale always has a
	// fractional part of 0.5. Blurs are created by running successive Kawase
	// filters at increasing scales until the desired size is reached. Divide
	// scale value by resolution of the input texture to get this scale vector.
	uniform vec2	ScaleU;
	
	void main()
	{
		gl_Position = ftransform();
		
		// If we do all this math here, and let GLSL do its built-in
		// interpolation of varying variables, the math still comes out right,
		// but it's faster.
		texcoord1 = gl_MultiTexCoord0.xy + ScaleU * vec2 (-1, -1);
		texcoord2 = gl_MultiTexCoord0.xy + ScaleU * vec2 (-1, 1);
		texcoord3 = gl_MultiTexCoord0.xy + ScaleU * vec2 (1, -1);
		texcoord4 = gl_MultiTexCoord0.xy + ScaleU * vec2 (1, 1);
	}
);

static char kawase_fragment_program[] = STRINGIFY (
	varying vec2	texcoord1, texcoord2, texcoord3, texcoord4;
	uniform sampler2D textureSource;
	
	void main()
	{
		gl_FragColor = (texture2D (textureSource, texcoord1) +
						texture2D (textureSource, texcoord2) +
						texture2D (textureSource, texcoord3) +
						texture2D (textureSource, texcoord4)) / 4.0;
	}
);

//RADIAL BLUR EFFECTS // xy = radial center screen space position, z = radius attenuation, w = blur strength
static char rblur_vertex_program[] = STRINGIFY (
	void main()
	{
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char rblur_fragment_program[] = STRINGIFY (
	uniform sampler2D rtextureSource;
	uniform vec3 radialBlurParams;
	
	const float sampleDist = 2.0;
	const float sampleStrength = 2.0;

	void main()
	{
		float samples[10];
		samples[0] = -0.08;
		samples[1] = -0.05;
		samples[2] = -0.03;
		samples[3] = -0.02;
		samples[4] = -0.01;
		samples[5] = 0.01;
		samples[6] = 0.02;
		samples[7] = 0.03;
		samples[8] = 0.05;
		samples[9] = 0.08;

		vec2 dir = vec2(0.5) - gl_TexCoord[0].st;
		float dist = sqrt(dir.x*dir.x + dir.y*dir.y);
		dir = dir / dist;

		vec4 color = texture2D(rtextureSource, gl_TexCoord[0].st);
		vec4 sum = color;

		for (int i = 0; i < 10; i++)
			sum += texture2D(rtextureSource, gl_TexCoord[0].st + dir * samples[i] * sampleDist);

		sum *= 1.0 / 11.0;
		float t = dist * sampleStrength;
		t = clamp(t, 0.0, 1.0);

		gl_FragColor = mix(color, sum, t);

		// Shade depending on effect.
		gl_FragColor *= vec4(radialBlurParams, 1.0);		
	}
);

//WATER DROPLETS
static char droplets_vertex_program[] = STRINGIFY (
	uniform float drTime;

	void main( void )
	{
		gl_Position = ftransform();

		 //for vertical scrolling
		 vec4 texco = gl_MultiTexCoord0;
		 texco.t = texco.t + drTime*1.0;
		 gl_TexCoord[1] = texco;

		 texco = gl_MultiTexCoord0;
		 texco.t = texco.t + drTime*0.8;
		 gl_TexCoord[2] = texco;

		gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char droplets_fragment_program[] = STRINGIFY (
	uniform sampler2D drSource;
	uniform sampler2D drTex;
	
	void main(void)
	{
		vec3 noiseVec;
		vec3 noiseVec2;
		vec2 displacement;

		displacement = gl_TexCoord[1].st;

		noiseVec = normalize(texture2D(drTex, displacement.xy)).xyz;
		noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

		displacement = gl_TexCoord[2].st;

		noiseVec2 = normalize(texture2D(drTex, displacement.xy)).xyz;
		noiseVec2 = (noiseVec2 * 2.0 - 0.635) * 0.035;

		//clamp edges to prevent artifacts
		if(gl_TexCoord[0].s > 0.1 && gl_TexCoord[0].s < 0.992)
			displacement.x = gl_TexCoord[0].s + noiseVec.x + noiseVec2.x;
		else
			displacement.x = gl_TexCoord[0].s;

		if(gl_TexCoord[0].t > 0.1 && gl_TexCoord[0].t < 0.972) 
			displacement.y = gl_TexCoord[0].t + noiseVec.y + noiseVec2.y;
		else
			displacement.y = gl_TexCoord[0].t;

		gl_FragColor = texture2D (drSource, displacement.xy);
	}
);

static char rgodrays_vertex_program[] = STRINGIFY (
	void main()
	{
		gl_TexCoord[0] =  gl_MultiTexCoord0;
		gl_Position = ftransform();
	}
);

static char rgodrays_fragment_program[] = STRINGIFY (
	uniform vec2 lightPositionOnScreen;
	uniform sampler2D sunTexture;
	uniform float aspectRatio; //width/height
	uniform float sunRadius;

	//note - these could be made uniforms to control externally
	const float exposure = 0.0034;
	const float decay = 1.0;
	const float density = 0.84;
	const float weight = 5.65;
	const int NUM_SAMPLES = 75;
	
	void main()
	{
		vec2 deltaTextCoord = vec2( gl_TexCoord[0].st - lightPositionOnScreen.xy );
		vec2 textCoo = gl_TexCoord[0].st;
		float adjustedLength = length (vec2 (deltaTextCoord.x*aspectRatio, deltaTextCoord.y));
		deltaTextCoord *= 1.0 /  float(NUM_SAMPLES) * density;
		float illuminationDecay = 1.0;

		int lim = NUM_SAMPLES;

		if (adjustedLength > sunRadius)
		{		
			//first simulate the part of the loop for which we won't get any
			//samples anyway
			float ratio = (adjustedLength-sunRadius)/adjustedLength;
			lim = int (float(lim)*ratio);

			textCoo -= deltaTextCoord*lim;
			illuminationDecay *= pow (decay, lim);

			//next set up the following loop so it gets the correct number of
			//samples.
			lim = NUM_SAMPLES-lim;
		}

		gl_FragColor = vec4(0.0);
		for(int i = 0; i < lim; i++)
		{
			textCoo -= deltaTextCoord;
			
			vec4 sample = texture2D(sunTexture, textCoo );

			sample *= illuminationDecay * weight;

			gl_FragColor += sample;

			illuminationDecay *= decay;
		}
		gl_FragColor *= exposure;
	}
);

static char DOF_vertex_program[] = STRINGIFY (
	void main()
	{
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
	}
);

static char DOF_fragment_program[] = STRINGIFY (

	uniform sampler2D renderedTexture;
	uniform sampler2D depthTexture;

	const float pi = 3.14159265f;
	const float sigma = 3.0f;
	const float blurSize = 1.0f / 512.0f; //Note - should be screen width

	const vec2  blurMultiplyVecH = vec2(1.0f, 0.0f);
	const vec2  blurMultiplyVecV = vec2(0.0f, 1.0f);

	void main(void)
	{
		// Possible better way to use the depth
		float n = 1.0;
		float f = 3000.0;
		float z = texture2D(depthTexture, gl_TexCoord[0].xy).x;
		float blur = (2.0 * n) / (f + n - z*(f - n));

		float numBlurPixelsPerSide = 3.0f * blur;
		vec3 incrementalGaussian;
		incrementalGaussian.x = 1.0f / (sqrt(2.0f * pi) * sigma);
		incrementalGaussian.y = exp(-0.5f / (sigma * sigma));
		incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;

		vec4 avgValue = vec4(0.0f, 0.0f, 0.0f, 0.0f);
		float coefficientSum = 0.0f;

		// Take the central sample first...
		avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy) * incrementalGaussian.x;
		coefficientSum += incrementalGaussian.x;
		incrementalGaussian.xy *= incrementalGaussian.yz;

		for (float i = 1.0f; i <= numBlurPixelsPerSide; i++) {
			avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy - i * blurSize *
				blurMultiplyVecV) * incrementalGaussian.x;
			avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy + i * blurSize *
				blurMultiplyVecV) * incrementalGaussian.x;
			coefficientSum += 2 * incrementalGaussian.x;
			incrementalGaussian.xy *= incrementalGaussian.yz;
		}

		// Blur Horizontally
		for (float i = 1.0f; i <= numBlurPixelsPerSide; i++) {
			avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy - i * blurSize *
				blurMultiplyVecH) * incrementalGaussian.x;
			avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy + i * blurSize *
				blurMultiplyVecH) * incrementalGaussian.x;
			coefficientSum += 2 * incrementalGaussian.x;
			incrementalGaussian.xy *= incrementalGaussian.yz;
		}

		gl_FragColor = mix(gl_FragColor, avgValue / coefficientSum, 1.0);
		//gl_FragColor = vec4(blur, blur, blur, 1.0); // Uncomment to see depth buffer.
	}
);

static char vegetation_vertex_program[] = STRINGIFY (
	uniform float rsTime;
	uniform vec3 up, right;
	attribute float swayCoef, addup, addright;
	
	void main ()
	{
		// use cosine so that negative swayCoef is different from positive
		float sway = swayCoef * cos (swayCoef * rsTime);
		vec4 swayvec = vec4 (sway, sway, 0, 0);
		vec4 vertex =	gl_Vertex + swayvec +
						addup * vec4 (up, 0) + addright * vec4 (right, 0);
		gl_Position = gl_ModelViewProjectionMatrix * vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_FrontColor = gl_BackColor = gl_Color;
		gl_FogFragCoord = length (gl_Position);
	}
);

static char lensflare_vertex_program[] = STRINGIFY (
	uniform vec3 up, right;
	attribute float size, addup, addright;
	
	void main ()
	{
		float dist = length (gl_ModelViewMatrix * gl_Vertex) * 0.01;

		// Flares which are very close are too small to see; fade them out as
		// we get closer.
		float alpha = gl_Color.a;
		if (dist < 2.0)
			alpha *= (dist - 1.0) / 2.0;
		gl_FrontColor = gl_BackColor = vec4 (gl_Color.rgb * alpha, 1.0);
		
		dist = min (dist, 10.0) * size + 1;
		
		vec4 vertex =	gl_Vertex +
						addup * dist * vec4 (up, 0) +
						addright * dist * vec4 (right, 0);
		gl_Position = gl_ModelViewProjectionMatrix * vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_FogFragCoord = length (gl_Position);
	}
);


typedef struct {
	const char	*name;
	int			index;
} vertex_attribute_t;

// add new vertex attributes here
#define NO_ATTRIBUTES			0
const vertex_attribute_t standard_attributes[] = 
{
	#define	ATTRIBUTE_TANGENT	(1<<0)
	{"tangent",		ATTR_TANGENT_IDX},
	#define	ATTRIBUTE_WEIGHTS	(1<<1)
	{"weights",		ATTR_WEIGHTS_IDX},
	#define ATTRIBUTE_BONES		(1<<2)
	{"bones",		ATTR_BONES_IDX},
	#define ATTRIBUTE_OLDVTX	(1<<3)
	{"oldvertex",	ATTR_OLDVTX_IDX},
	#define ATTRIBUTE_OLDNORM	(1<<4)
	{"oldnormal",	ATTR_OLDNORM_IDX},
	#define ATTRIBUTE_OLDTAN	(1<<5)
	{"oldtangent",	ATTR_OLDTAN_IDX},
	#define ATTRIBUTE_MINIMAP	(1<<6)
	{"colordata",	ATTR_MINIMAP_DATA_IDX},
	#define ATTRIBUTE_SWAYCOEF	(1<<7)
	{"swaycoef",	ATTR_SWAYCOEF_DATA_IDX},
	#define ATTRIBUTE_ADDUP		(1<<8)
	{"addup",		ATTR_ADDUP_DATA_IDX},
	#define ATTRIBUTE_ADDRIGHT	(1<<9)
	{"addright",	ATTR_ADDRIGHT_DATA_IDX},
	#define ATTRIBUTE_SIZE		(1<<10)
	{"size",		ATTR_SIZE_DATA_IDX},
};
const int num_standard_attributes = sizeof(standard_attributes)/sizeof(vertex_attribute_t);
	
void R_LoadGLSLProgram (const char *name, char *vertex, char *fragment, int attributes, int ndynamic, GLhandleARB *program)
{
	char		str[4096], macros[64];
	const char	*shaderStrings[5];
	int			nResult;
	int			i;
	
	Com_sprintf (macros, sizeof (macros), "#version 120\n#define DYNAMIC %d\n", ndynamic);
	shaderStrings[0] = macros;
	
	*program = glCreateProgramObjectARB();
	
	if (vertex != NULL)
	{
		g_vertexShader = glCreateShaderObjectARB (GL_VERTEX_SHADER_ARB);
		
		shaderStrings[1] = vertex_library;
		if (fragment == NULL)
			shaderStrings[2] = "const int vertexOnly = 1;";
		else
			shaderStrings[2] = "const int vertexOnly = 0;";
		shaderStrings[3] = vertex;
		
		glShaderSourceARB (g_vertexShader, 4, shaderStrings, NULL);
		glCompileShaderARB (g_vertexShader);
		glGetObjectParameterivARB (g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult);

		if (nResult)
		{
			glAttachObjectARB (*program, g_vertexShader);
		}
		else
		{
			Com_Printf ("...%s_%ddynamic Vertex Shader Compile Error\n", name, ndynamic);
			if (glGetShaderInfoLog != NULL)
			{
				glGetShaderInfoLog (g_vertexShader, sizeof(str), NULL, str);
				Com_Printf ("%s\n", str);
			}
		}
	}
	
	if (fragment != NULL)
	{
		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		
		if (gl_state.ati)
			shaderStrings[1] = "#define AMD_GPU\n#define shadowsampler_t sampler2D\n";
		else
			shaderStrings[1] = "#define shadowsampler_t sampler2DShadow\n";
		shaderStrings[2] = fragment_library;
		shaderStrings[3] = fragment;
	
		glShaderSourceARB (g_fragmentShader, 4, shaderStrings, NULL);
		glCompileShaderARB (g_fragmentShader);
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if (nResult)
		{
			glAttachObjectARB (*program, g_fragmentShader);
		}
		else
		{
			Com_Printf ("...%s_%ddynamic Fragment Shader Compile Error\n", name, ndynamic);
			if (glGetShaderInfoLog != NULL)
			{
				glGetShaderInfoLog (g_fragmentShader, sizeof(str), NULL, str);
				Com_Printf ("%s\n", str);
			}
		}
	}
	
	for (i = 0; i < num_standard_attributes; i++)
	{
		if (attributes & (1<<i))
			glBindAttribLocationARB (*program, standard_attributes[i].index, standard_attributes[i].name);
	}

	glLinkProgramARB (*program);
	glGetObjectParameterivARB (*program, GL_OBJECT_LINK_STATUS_ARB, &nResult);

	glGetInfoLogARB (*program, sizeof(str), NULL, str);
	if (!nResult)
	{
		Com_Printf("...%s_%d Shader Linking Error\n%s\n", name, ndynamic, str);
	}
#ifdef DUMP_GLSL_ASM
	else
	{
		char	binarydump[1<<16];
		GLsizei	binarydump_length;
		GLenum	format;
		FILE	*out;
		
		// use this to get the actual assembly
		// for i in *dump_*.dat ; do strings "$i" > "$i".txt ; rm "$i" ; done
		Com_sprintf (str, sizeof (str), "dump_%s_%d.dat", name, ndynamic);
		out = fopen (str, "wb");
		
		glGetProgramBinary (*program, sizeof(binarydump), &binarydump_length, &format, binarydump);
		fwrite (binarydump, 1, sizeof(binarydump), out);
		
		fclose (out); 
	}
#endif
}

static void get_dlight_uniform_locations (GLhandleARB programObj, dlight_uniform_location_t *out)
{
	out->lightAmountSquared = glGetUniformLocationARB (programObj, "lightAmount");
	out->lightPosition = glGetUniformLocationARB (programObj, "lightPosition");
	out->lightCutoffSquared = glGetUniformLocationARB (programObj, "lightCutoffSquared");
}

static void get_mesh_anim_uniform_locations (GLhandleARB programObj, mesh_anim_uniform_location_t *out)
{
	out->useGPUanim = glGetUniformLocationARB (programObj, "GPUANIM");
	out->outframe = glGetUniformLocationARB (programObj, "bonemats");
	out->lerp = glGetUniformLocationARB (programObj, "lerp");
}

static void get_shadowmap_channel_uniform_locations (GLhandleARB programObj, shadowmap_channel_uniform_location_t *out, const char *prefix)
{
	char name[64];
	
	Com_sprintf (name, sizeof (name), "%s_enabled", prefix);
	out->enabled = glGetUniformLocationARB (programObj, name);
	
	Com_sprintf (name, sizeof (name), "%s_pixelOffset", prefix);
	out->pixelOffset = glGetUniformLocationARB (programObj, name);
	
	Com_sprintf (name, sizeof (name), "%s_texture", prefix);
	out->texture = glGetUniformLocationARB (programObj, name);
}

static void get_shadowmap_uniform_locations (GLhandleARB programObj, shadowmap_uniform_location_t *out)
{
	get_shadowmap_channel_uniform_locations (programObj, &out->sunStatic, "sunstatic");
	get_shadowmap_channel_uniform_locations (programObj, &out->otherStatic, "otherstatic");
	get_shadowmap_channel_uniform_locations (programObj, &out->dynamic, "dynamic");
}

static void get_mesh_uniform_locations (GLhandleARB programObj, mesh_uniform_location_t *out)
{
	get_mesh_anim_uniform_locations (programObj, &out->anim_uniforms);
	get_dlight_uniform_locations (programObj, &out->dlight_uniforms);
	get_shadowmap_uniform_locations (programObj, &out->shadowmap_uniforms);
	out->staticLightPosition = glGetUniformLocationARB (programObj, "staticLightPosition");
	out->staticLightColor = glGetUniformLocationARB (programObj, "staticLightColor");
	out->totalLightPosition = glGetUniformLocationARB (programObj, "totalLightPosition");
	out->totalLightColor = glGetUniformLocationARB (programObj, "totalLightColor");
	out->meshPosition = glGetUniformLocationARB (programObj, "meshPosition");
	out->meshRotation = glGetUniformLocationARB (programObj, "meshRotation");
	out->baseTex = glGetUniformLocationARB (programObj, "baseTex");
	out->normTex = glGetUniformLocationARB (programObj, "normalTex");
	out->fxTex = glGetUniformLocationARB (programObj, "fxTex");
	out->fx2Tex = glGetUniformLocationARB (programObj, "fx2Tex");
	out->lightmapTexture = glGetUniformLocationARB (programObj, "lightmapTexture");
	out->time = glGetUniformLocationARB (programObj, "time");
	out->lightmap = glGetUniformLocationARB (programObj, "lightmap");
	out->fog = glGetUniformLocationARB (programObj, "FOG");
	out->useFX = glGetUniformLocationARB (programObj, "useFX");
	out->useGlow = glGetUniformLocationARB (programObj, "useGlow");
	out->useShell = glGetUniformLocationARB (programObj, "useShell");
	out->shellAlpha = glGetUniformLocationARB (programObj, "shellAlpha");
	out->useCube = glGetUniformLocationARB (programObj, "useCube");
	out->fromView = glGetUniformLocationARB (programObj, "fromView");
	out->doShading = glGetUniformLocationARB (programObj, "doShading");
	out->team = glGetUniformLocationARB (programObj, "TEAM");
}

void R_LoadGLSLPrograms(void)
{
	int i, j;
	
	//load glsl (to do - move to own file)
	if ( GL_QueryExtension("GL_ARB_shader_objects") )
	{
		glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glCreateProgramObjectARB");
		glDeleteObjectARB		 = (PFNGLDELETEOBJECTARBPROC)qwglGetProcAddress("glDeleteObjectARB");
		glUseProgramObjectARB	 = (PFNGLUSEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glUseProgramObjectARB");
		glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)qwglGetProcAddress("glCreateShaderObjectARB");
		glShaderSourceARB		 = (PFNGLSHADERSOURCEARBPROC)qwglGetProcAddress("glShaderSourceARB");
		glCompileShaderARB		= (PFNGLCOMPILESHADERARBPROC)qwglGetProcAddress("glCompileShaderARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)qwglGetProcAddress("glGetObjectParameterivARB");
		glAttachObjectARB		 = (PFNGLATTACHOBJECTARBPROC)qwglGetProcAddress("glAttachObjectARB");
		glGetInfoLogARB		   = (PFNGLGETINFOLOGARBPROC)qwglGetProcAddress("glGetInfoLogARB");
		glLinkProgramARB		  = (PFNGLLINKPROGRAMARBPROC)qwglGetProcAddress("glLinkProgramARB");
		glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)qwglGetProcAddress("glGetUniformLocationARB");
		glUniform4iARB			= (PFNGLUNIFORM4IARBPROC)qwglGetProcAddress("glUniform4iARB");
		glUniform4fARB			= (PFNGLUNIFORM4FARBPROC)qwglGetProcAddress("glUniform4fARB");
		glUniform3fARB			= (PFNGLUNIFORM3FARBPROC)qwglGetProcAddress("glUniform3fARB");
		glUniform2fARB			= (PFNGLUNIFORM2FARBPROC)qwglGetProcAddress("glUniform2fARB");
		glUniform1iARB			= (PFNGLUNIFORM1IARBPROC)qwglGetProcAddress("glUniform1iARB");
		glUniform1fARB		  = (PFNGLUNIFORM1FARBPROC)qwglGetProcAddress("glUniform1fARB");
		glUniform4ivARB			= (PFNGLUNIFORM4IVARBPROC)qwglGetProcAddress("glUniform4ivARB");
		glUniform4fvARB			= (PFNGLUNIFORM4FVARBPROC)qwglGetProcAddress("glUniform4fvARB");
		glUniform3fvARB			= (PFNGLUNIFORM3FVARBPROC)qwglGetProcAddress("glUniform3fvARB");
		glUniform2fvARB			= (PFNGLUNIFORM2FVARBPROC)qwglGetProcAddress("glUniform2fvARB");
		glUniform1ivARB			= (PFNGLUNIFORM1IVARBPROC)qwglGetProcAddress("glUniform1ivARB");
		glUniform1fvARB		  = (PFNGLUNIFORM1FVARBPROC)qwglGetProcAddress("glUniform1fvARB");
		glUniformMatrix3fvARB	  = (PFNGLUNIFORMMATRIX3FVARBPROC)qwglGetProcAddress("glUniformMatrix3fvARB");
		glUniformMatrix3x4fvARB	  = (PFNGLUNIFORMMATRIX3X4FVARBPROC)qwglGetProcAddress("glUniformMatrix3x4fv");
		glVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)qwglGetProcAddress("glVertexAttribPointerARB");
		glEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glEnableVertexAttribArrayARB");
		glDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glDisableVertexAttribArrayARB");
		glBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)qwglGetProcAddress("glBindAttribLocationARB");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)qwglGetProcAddress("glGetShaderInfoLog");

#ifdef DUMP_GLSL_ASM
		glGetProgramBinary = qwglGetProcAddress("glGetProgramBinary");
#endif

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
			!glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB ||
			!glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB ||
			!glLinkProgramARB || !glGetUniformLocationARB || !glUniform3fARB ||
				!glUniform4fARB || !glUniform4fvARB || !glUniform4ivARB ||
				!glUniform4iARB || !glUniform1iARB || !glUniform1fARB ||
				!glUniform3fvARB || !glUniform2fvARB || !glUniform1fvARB ||
				!glUniformMatrix3fvARB || !glUniformMatrix3x4fvARB ||
				!glVertexAttribPointerARB || !glEnableVertexAttribArrayARB ||
				!glBindAttribLocationARB)
		{
			Com_Error (ERR_FATAL, "...One or more GL_ARB_shader_objects functions were not found\n");
		}
	}
	else
	{
		Com_Error (ERR_FATAL, "...One or more GL_ARB_shader_objects functions were not found\n");
	}

	gl_dynamic = Cvar_Get ("gl_dynamic", "1", CVAR_ARCHIVE);

	//standard bsp surfaces
	for (i = 0; i <= GLSL_MAX_DLIGHTS; i++)
	{
		R_LoadGLSLProgram ("World", (char*)world_vertex_program, (char*)world_fragment_program, ATTRIBUTE_TANGENT, i, &g_worldprogramObj[i]);

		get_dlight_uniform_locations (g_worldprogramObj[i], &worldsurf_uniforms[i].dlight_uniforms);
		get_shadowmap_uniform_locations (g_worldprogramObj[i], &worldsurf_uniforms[i].shadowmap_uniforms);
		worldsurf_uniforms[i].surfTexture = glGetUniformLocationARB (g_worldprogramObj[i], "surfTexture");
		worldsurf_uniforms[i].heightTexture = glGetUniformLocationARB (g_worldprogramObj[i], "HeightTexture");
		worldsurf_uniforms[i].lmTexture = glGetUniformLocationARB (g_worldprogramObj[i], "lmTexture");
		worldsurf_uniforms[i].normalTexture = glGetUniformLocationARB (g_worldprogramObj[i], "NormalTexture");
		worldsurf_uniforms[i].fog = glGetUniformLocationARB (g_worldprogramObj[i], "FOG");
		worldsurf_uniforms[i].parallax = glGetUniformLocationARB (g_worldprogramObj[i], "PARALLAX");
		worldsurf_uniforms[i].staticLightPosition = glGetUniformLocationARB (g_worldprogramObj[i], "staticLightPosition");
		worldsurf_uniforms[i].liquid = glGetUniformLocationARB (g_worldprogramObj[i], "LIQUID");
		worldsurf_uniforms[i].shiny = glGetUniformLocationARB (g_worldprogramObj[i], "SHINY");
		worldsurf_uniforms[i].rsTime = glGetUniformLocationARB (g_worldprogramObj[i], "rsTime");
		worldsurf_uniforms[i].liquidTexture = glGetUniformLocationARB (g_worldprogramObj[i], "liquidTexture");
		worldsurf_uniforms[i].liquidNormTex = glGetUniformLocationARB (g_worldprogramObj[i], "liquidNormTex");
		worldsurf_uniforms[i].chromeTex = glGetUniformLocationARB (g_worldprogramObj[i], "chromeTex");
	}

	//shadowed white bsp surfaces
	R_LoadGLSLProgram ("Shadow", (char*)shadow_vertex_program, (char*)shadow_fragment_program, NO_ATTRIBUTES, 0, &g_shadowprogramObj);

	// Locate some parameters by name so we can set them later...
	secondpass_bsp_shadow_uniforms.fade = glGetUniformLocationARB( g_shadowprogramObj, "fadeShadow" );
	get_shadowmap_uniform_locations (g_shadowprogramObj, &secondpass_bsp_shadow_uniforms.shadowmap_uniforms);
	
	// Old-style per-vertex water effects
	R_LoadGLSLProgram ("Warp", (char*)warp_vertex_program, NULL, NO_ATTRIBUTES, 0, &g_warpprogramObj);
	
	warp_uniforms.time = glGetUniformLocationARB (g_warpprogramObj, "time");
	warp_uniforms.warpvert = glGetUniformLocationARB (g_warpprogramObj, "warpvert");
	warp_uniforms.envmap = glGetUniformLocationARB (g_warpprogramObj, "envmap");
	
	// Minimaps
	R_LoadGLSLProgram ("Minimap", (char*)minimap_vertex_program, NULL, ATTRIBUTE_MINIMAP, 0, &g_minimapprogramObj);
	
	//rscript surfaces
	for (i = 0; i <= GLSL_MAX_DLIGHTS; i++)
	{
		R_LoadGLSLProgram ("RScript", (char*)rscript_vertex_program, (char*)rscript_fragment_program, ATTRIBUTE_TANGENT, i, &g_rscriptprogramObj[i]);
	
		get_dlight_uniform_locations (g_rscriptprogramObj[i], &rscript_uniforms[i].dlight_uniforms);
		get_shadowmap_uniform_locations (g_rscriptprogramObj[i], &rscript_uniforms[i].shadowmap_uniforms);
		rscript_uniforms[i].staticLightPosition = glGetUniformLocationARB (g_rscriptprogramObj[i], "staticLightPosition");
		rscript_uniforms[i].envmap = glGetUniformLocationARB (g_rscriptprogramObj[i], "envmap");
		rscript_uniforms[i].numblendtextures = glGetUniformLocationARB (g_rscriptprogramObj[i], "numblendtextures");
		rscript_uniforms[i].numblendnormalmaps = glGetUniformLocationARB (g_rscriptprogramObj[i], "numblendnormalmaps");
		rscript_uniforms[i].static_normalmaps = glGetUniformLocationARB (g_rscriptprogramObj[i], "static_normalmaps");
		rscript_uniforms[i].lightmap = glGetUniformLocationARB (g_rscriptprogramObj[i], "lightmap");
		rscript_uniforms[i].fog = glGetUniformLocationARB (g_rscriptprogramObj[i], "FOG");
		rscript_uniforms[i].mainTexture = glGetUniformLocationARB (g_rscriptprogramObj[i], "mainTexture");
		rscript_uniforms[i].mainTexture2 = glGetUniformLocationARB (g_rscriptprogramObj[i], "mainTexture2");
		rscript_uniforms[i].lightmapTexture = glGetUniformLocationARB (g_rscriptprogramObj[i], "lightmapTexture");
		rscript_uniforms[i].blendscales = glGetUniformLocationARB (g_rscriptprogramObj[i], "blendscales");
		rscript_uniforms[i].normalblendindices = glGetUniformLocationARB (g_rscriptprogramObj[i], "normalblendindices");
		rscript_uniforms[i].meshPosition = glGetUniformLocationARB (g_rscriptprogramObj[i], "meshPosition");
		rscript_uniforms[i].meshRotation = glGetUniformLocationARB (g_rscriptprogramObj[i], "meshRotation");

		for (j = 0; j < 6; j++)
		{
			char uniformname[] = "blendTexture.";
		
			assert (j < 10); // We only have space for one digit.
			uniformname[12] = '0'+j;
			rscript_uniforms[i].blendTexture[j] = glGetUniformLocationARB (g_rscriptprogramObj[i], uniformname);
		}
		
		for (j = 0; j < 3; j++)
		{
			char uniformname[] = "blendNormalmap.";
		
			assert (j < 10); // We only have space for one digit.
			uniformname[14] = '0'+j;
			rscript_uniforms[i].blendNormalmap[j] = glGetUniformLocationARB (g_rscriptprogramObj[i], uniformname);
		}
	}

	// per-pixel warp(water) bsp surfaces
	R_LoadGLSLProgram ("Water", (char*)water_vertex_program, (char*)water_fragment_program, ATTRIBUTE_TANGENT, 0, &g_waterprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_baseTexture = glGetUniformLocationARB( g_waterprogramObj, "baseTexture" );
	g_location_normTexture = glGetUniformLocationARB( g_waterprogramObj, "normalMap" );
	g_location_refTexture = glGetUniformLocationARB( g_waterprogramObj, "refTexture" );
	g_location_time = glGetUniformLocationARB( g_waterprogramObj, "time" );
	g_location_lightPos = glGetUniformLocationARB( g_waterprogramObj, "LightPos" );
	g_location_reflect = glGetUniformLocationARB( g_waterprogramObj, "REFLECT" );
	g_location_trans = glGetUniformLocationARB( g_waterprogramObj, "TRANSPARENT" );
	g_location_fogamount = glGetUniformLocationARB( g_waterprogramObj, "FOG" );

	for (i = 0; i <= GLSL_MAX_DLIGHTS; i++)
	{
		//meshes
		R_LoadGLSLProgram (
			"Mesh", (char*)mesh_vertex_program, (char*)mesh_fragment_program,
			ATTRIBUTE_TANGENT|ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM|ATTRIBUTE_OLDTAN,
			i, &g_meshprogramObj[i]
		);
	
		get_mesh_uniform_locations (g_meshprogramObj[i], &mesh_uniforms[i]);

		//vertex-only meshes
		R_LoadGLSLProgram (
			"VertexOnly_Mesh", (char*)mesh_vertex_program, NULL,
			ATTRIBUTE_TANGENT|ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM|ATTRIBUTE_OLDTAN,
			i, &g_vertexonlymeshprogramObj[i]
		);
	
		get_mesh_uniform_locations (g_vertexonlymeshprogramObj[i], &mesh_vertexonly_uniforms[i]);
	}
	
	//Glass
	R_LoadGLSLProgram ("Glass", (char*)glass_vertex_program, (char*)glass_fragment_program, ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM, 0, &g_glassprogramObj);

	// Locate some parameters by name so we can set them later...
	get_mesh_anim_uniform_locations (g_glassprogramObj, &glass_uniforms.anim_uniforms);
	glass_uniforms.fog = glGetUniformLocationARB (g_glassprogramObj, "FOG");
	glass_uniforms.type = glGetUniformLocationARB (g_glassprogramObj, "type");
	glass_uniforms.left = glGetUniformLocationARB (g_glassprogramObj, "left");
	glass_uniforms.up = glGetUniformLocationARB (g_glassprogramObj, "up");
	glass_uniforms.lightPos = glGetUniformLocationARB (g_glassprogramObj, "LightPos");
	glass_uniforms.mirTexture = glGetUniformLocationARB (g_glassprogramObj, "mirTexture");
	glass_uniforms.refTexture = glGetUniformLocationARB (g_glassprogramObj, "refTexture");

	//Blank mesh (for shadowmapping efficiently)
	R_LoadGLSLProgram ("Blankmesh", (char*)blankmesh_vertex_program, (char*)blankmesh_fragment_program, ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX, 0, &g_blankmeshprogramObj);

	// Locate some parameters by name so we can set them later...
	get_mesh_anim_uniform_locations (g_blankmeshprogramObj, &blankmesh_uniforms.anim_uniforms);
	blankmesh_uniforms.baseTex = glGetUniformLocationARB (g_blankmeshprogramObj, "baseTex");

	//Per-pixel static lightmapped mesh rendered into texture space
	R_LoadGLSLProgram ("Extract Lightmap", (char*)mesh_extract_lightmap_vertex_program, (char*)mesh_extract_lightmap_fragment_program, ATTRIBUTE_TANGENT|ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM|ATTRIBUTE_OLDTAN, 0, &g_extractlightmapmeshprogramObj);
	
	get_mesh_anim_uniform_locations (g_extractlightmapmeshprogramObj, &mesh_extract_lightmap_uniforms.anim_uniforms);
	mesh_extract_lightmap_uniforms.staticLightPosition = glGetUniformLocationARB (g_extractlightmapmeshprogramObj, "staticLightPosition");
	mesh_extract_lightmap_uniforms.staticLightColor = glGetUniformLocationARB (g_extractlightmapmeshprogramObj, "staticLightColor");
	
	
	//fullscreen distortion effects
	R_LoadGLSLProgram ("Framebuffer Distort", (char*)fb_vertex_program, (char*)fb_fragment_program, NO_ATTRIBUTES, 0, &g_fbprogramObj);

	// Locate some parameters by name so we can set them later...
	distort_uniforms.framebuffTex = glGetUniformLocationARB (g_fbprogramObj, "fbtexture");
	distort_uniforms.distortTex = glGetUniformLocationARB (g_fbprogramObj, "distortiontexture");
	distort_uniforms.intensity = glGetUniformLocationARB (g_fbprogramObj, "intensity");

	//gaussian blur
	R_LoadGLSLProgram ("Framebuffer Gaussian Blur", (char*)blur_vertex_program, (char*)blur_fragment_program, NO_ATTRIBUTES, 0, &g_blurprogramObj);

	// Locate some parameters by name so we can set them later...
	gaussian_uniforms.scale = glGetUniformLocationARB( g_blurprogramObj, "ScaleU" );
	gaussian_uniforms.source = glGetUniformLocationARB( g_blurprogramObj, "textureSource");

	//defringe filter (transparent-only blur)
	R_LoadGLSLProgram ("Framebuffer Defringe Filter", (char*)defringe_vertex_program, (char*)defringe_fragment_program, NO_ATTRIBUTES, 0, &g_defringeprogramObj);

	// Locate some parameters by name so we can set them later...
	defringe_uniforms.scale = glGetUniformLocationARB (g_defringeprogramObj, "ScaleU");
	defringe_uniforms.source = glGetUniformLocationARB (g_defringeprogramObj, "textureSource");

	//kawase filter blur
	R_LoadGLSLProgram ("Framebuffer Kawase Blur", (char*)kawase_vertex_program, (char*)kawase_fragment_program, NO_ATTRIBUTES, 0, &g_kawaseprogramObj);

	// Locate some parameters by name so we can set them later...
	kawase_uniforms.scale = glGetUniformLocationARB( g_blurprogramObj, "ScaleU" );
	kawase_uniforms.source = glGetUniformLocationARB( g_blurprogramObj, "textureSource");
	
	// Color scaling
	R_LoadGLSLProgram ("Color Scaling", NULL, (char*)colorscale_fragment_program, NO_ATTRIBUTES, 0, &g_colorscaleprogramObj);
	colorscale_uniforms.scale = glGetUniformLocationARB (g_colorscaleprogramObj, "scale");
	colorscale_uniforms.source = glGetUniformLocationARB (g_colorscaleprogramObj, "textureSource");
	
	// Color exponentiation
	R_LoadGLSLProgram ("Color Exponentiation", NULL, (char*)color_exponent_fragment_program, NO_ATTRIBUTES, 0, &g_colorexpprogramObj);
	colorexp_uniforms.exponent = glGetUniformLocationARB (g_colorexpprogramObj, "exponent");
	colorexp_uniforms.source = glGetUniformLocationARB (g_colorexpprogramObj, "textureSource");
	
	// Radial blur
	R_LoadGLSLProgram ("Framebuffer Radial Blur", (char*)rblur_vertex_program, (char*)rblur_fragment_program, NO_ATTRIBUTES, 0, &g_rblurprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_rsource = glGetUniformLocationARB( g_rblurprogramObj, "rtextureSource");
	g_location_rparams = glGetUniformLocationARB( g_rblurprogramObj, "radialBlurParams");

	// Water droplets
	R_LoadGLSLProgram ("Framebuffer Droplets", (char*)droplets_vertex_program, (char*)droplets_fragment_program, NO_ATTRIBUTES, 0, &g_dropletsprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_drSource = glGetUniformLocationARB( g_dropletsprogramObj, "drSource" );
	g_location_drTex = glGetUniformLocationARB( g_dropletsprogramObj, "drTex");
	g_location_drTime = glGetUniformLocationARB( g_dropletsprogramObj, "drTime" );

	// Depth of field
	R_LoadGLSLProgram("Framebuffer DOF", (char*)DOF_vertex_program, (char*)DOF_fragment_program, NO_ATTRIBUTES, 0, &g_DOFprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_dofSource = glGetUniformLocationARB(g_DOFprogramObj, "renderedTexture");
	g_location_dofDepth = glGetUniformLocationARB(g_DOFprogramObj, "depthTexture");

	// God rays
	R_LoadGLSLProgram ("God Rays", (char*)rgodrays_vertex_program, (char*)rgodrays_fragment_program, NO_ATTRIBUTES, 0, &g_godraysprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_lightPositionOnScreen = glGetUniformLocationARB( g_godraysprogramObj, "lightPositionOnScreen" );
	g_location_sunTex = glGetUniformLocationARB( g_godraysprogramObj, "sunTexture");
	g_location_godrayScreenAspect = glGetUniformLocationARB( g_godraysprogramObj, "aspectRatio");
	g_location_sunRadius = glGetUniformLocationARB( g_godraysprogramObj, "sunRadius");
	
	// Vegetation
	R_LoadGLSLProgram ("Vegetation", (char*)vegetation_vertex_program, NULL, ATTRIBUTE_SWAYCOEF|ATTRIBUTE_ADDUP|ATTRIBUTE_ADDRIGHT, 0, &g_vegetationprogramObj);
	
	vegetation_uniforms.rsTime = glGetUniformLocationARB (g_vegetationprogramObj, "rsTime");
	vegetation_uniforms.up = glGetUniformLocationARB (g_vegetationprogramObj, "up");
	vegetation_uniforms.right = glGetUniformLocationARB (g_vegetationprogramObj, "right");
	
	// Lens flare
	R_LoadGLSLProgram ("Lens Flare", (char*)lensflare_vertex_program, NULL, ATTRIBUTE_SIZE|ATTRIBUTE_ADDUP|ATTRIBUTE_ADDRIGHT, 0, &g_lensflareprogramObj);
	
	lensflare_uniforms.up = glGetUniformLocationARB (g_lensflareprogramObj, "up");
	lensflare_uniforms.right = glGetUniformLocationARB (g_lensflareprogramObj, "right");

}

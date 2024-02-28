/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================================
QUAKE3 MD3 MODELS
==============================================================================
*/

#include "r_local.h"

extern model_t* Mod_ForNum(int index);

extern float	sinTable[FUNCTABLE_SIZE];
extern vec3_t	model_shadevector;
extern float	model_shadelight[3];

/*
=================
MD3_FastDecodeNormals

Decode the lat/lng normal to a 3 float normal

lat *= M_PI / 128;
lng *= M_PI / 128;
decode X as cos( lat ) * sin( long )
decode Y as sin( lat ) * sin( long )
decode Z as cos( long )

based on:
https://github.com/id-Software/Quake-III-Arena/blob/dbe4ddb10315479fc00086f08e25d968b4b43c49/q3map/misc_model.c#L210
=================
*/
static void MD3_FastDecodeNormals(model_t* mod, md3Header_t* model)
{
	md3Surface_t* surf;
	md3XyzNormal_t* xyz;
	float			lat, lng;
	//	vec3_t			normal;

	surf = (md3Surface_t*)((byte*)model + model->ofsSurfaces);
	for (int i = 0; i < model->numSurfaces; i++)
		mod->nv[i] = surf->numVerts;

	for (int i = 0; i < model->numSurfaces; i++)
	{
		mod->nv[i] = surf->numVerts;
		mod->normals[i] = (vec3_t*)Hunk_Alloc(sizeof(vec3_t) * mod->nv[i]);

		xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);
		for (int j = 0; j < surf->numVerts; j++, xyz++)
		{
			lat = (xyz->normal >> 8) & 0xff;
			lng = (xyz->normal & 0xff);
			lat *= M_PI / 128;
			lng *= M_PI / 128;

			mod->normals[i][j][0] = cos(lat) * sin(lng);
			mod->normals[i][j][1] = sin(lat) * sin(lng);
			mod->normals[i][j][2] = cos(lng);

//			printf("surf %i vert %i normal [%f %f %f]\n", i, j, mod->normals[i][j][0], mod->normals[i][j][1], mod->normals[i][j][2]);
		}
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}
}


/*
=================
Mod_LoadMD3

Loads Q3 .md3 model
=================
*/
extern int modfilelen;
#define	LL(x) x=LittleLong(x)
void Mod_LoadMD3(model_t* mod, void* buffer, lod_t lod)
{
	int				i, j;
	md3Header_t		* pinmodel;
	md3Frame_t		* frame;
	md3Surface_t	* surf;
	md3Shader_t		* shader;
	md3Triangle_t	* tri;
	md3St_t			* st;
	md3XyzNormal_t	* xyz;
	md3Tag_t		* tag;
	int				version;
	int				size;

	mod->type = MOD_BAD;

	pinmodel = (md3Header_t*)buffer;
	version = LittleLong(pinmodel->version);
	if (version != MD3_VERSION)
	{
		ri.Error(ERR_DROP, "MOD_LoadMD3: '%s' has wrong version (%i should be %i)\n", mod->name, version, MD3_VERSION);
		return;
	}

	size = LittleLong(pinmodel->ofsEnd);
	if(modfilelen != size)
	{
		ri.Error(ERR_DROP, "MOD_LoadMD3: '%s' has weird size (probably broken)\n", mod->name);
		return;
	}

	mod->extradatasize = size;
	mod->md3[lod] = Hunk_Alloc(size);

	memcpy(mod->md3[lod], buffer, LittleLong(pinmodel->ofsEnd));

	LL(mod->md3[lod]->ident);
	LL(mod->md3[lod]->version);
	LL(mod->md3[lod]->numFrames);
	LL(mod->md3[lod]->numTags);
	LL(mod->md3[lod]->numSurfaces);
	LL(mod->md3[lod]->ofsFrames);
	LL(mod->md3[lod]->ofsTags);
	LL(mod->md3[lod]->ofsSurfaces);
	LL(mod->md3[lod]->ofsEnd);

	if (mod->md3[lod]->numFrames < 1)
	{
		ri.Error(ERR_DROP, "Mod_LoadMD3: %s has no frames\n", mod->name);
		return;
	}

	mod->type = MOD_MD3;

	// swap all the frames
	frame = (md3Frame_t*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsFrames);
	for (i = 0; i < mod->md3[lod]->numFrames; i++, frame++)
	{
		frame->radius = LittleFloat(frame->radius);
		for (j = 0; j < 3; j++) {
			frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
			frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
		}
	}

	// swap all the tags
	tag = (md3Tag_t*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsTags);
	for (i = 0; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames; i++, tag++)
	{
		for (j = 0; j < 3; j++) 
		{
			tag->origin[j] = LittleFloat(tag->origin[j]);
			tag->axis[0][j] = LittleFloat(tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(tag->axis[2][j]);
		}	
		_strlwr(tag->name); // lowercase the tag name so search compares are faster
	}

	// swap all the surfaces
	surf = (md3Surface_t*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsSurfaces);
	for (i = 0; i < mod->md3[lod]->numSurfaces; i++)
	{
		LL(surf->ident);
		LL(surf->flags);
		LL(surf->numFrames);
		LL(surf->numShaders);
		LL(surf->numTriangles);
		LL(surf->ofsTriangles);
		LL(surf->numVerts);
		LL(surf->ofsShaders);
		LL(surf->ofsSt);
		LL(surf->ofsXyzNormals);
		LL(surf->ofsEnd);

		if (surf->numVerts > MD3_MAX_VERTS)
		{
			ri.Error(ERR_DROP, "Mod_LoadMD3: %s has more than %i verts on a surface (%i)", mod->name, MD3_MAX_VERTS, surf->numVerts);
		}
		if (surf->numTriangles > MD3_MAX_TRIANGLES)
		{
			ri.Error(ERR_DROP, "Mod_LoadMD3: %s has more than %i triangles on a surface (%i)", mod->name, MD3_MAX_TRIANGLES, surf->numTriangles);
		}


		// lowercase the surface name so skin compares are faster
		_strlwr(surf->name);

		// strip off a trailing _1 or _2
		j = strlen(surf->name);
		if (j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		// register the shaders
		shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);
		for (j = 0; j < surf->numShaders; j++, shader++)
		{
			mod->skins[j] = GL_FindImage(shader->name, it_model);
			shader->shaderIndex = mod->skins[j]->texnum;
			if (mod->skins[j] == r_notexture)
			{
				ri.Printf(PRINT_ALL, "Mod_LoadMD3: cannot load '%s' for model '%s'\n", shader->name, mod->name);
			}

		}

		// swap all the triangles
		tri = (md3Triangle_t*)((byte*)surf + surf->ofsTriangles);
		for (j = 0; j < surf->numTriangles; j++, tri++)
		{
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
		st = (md3St_t*)((byte*)surf + surf->ofsSt);
		for (j = 0; j < surf->numVerts; j++, st++)
		{
			st->st[0] = LittleFloat(st->st[0]);
			st->st[1] = LittleFloat(st->st[1]);
		}

		// swap all the XyzNormals
		xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);
		for (j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++)
		{
			xyz->xyz[0] = LittleShort(xyz->xyz[0]);
			xyz->xyz[1] = LittleShort(xyz->xyz[1]);
			xyz->xyz[2] = LittleShort(xyz->xyz[2]);

			xyz->normal = LittleShort(xyz->normal);

			// scale verticles to qu
			//xyz->xyz[0] = (xyz->xyz[0] / 64);
			//xyz->xyz[1] = (xyz->xyz[1] / 64);
			//xyz->xyz[2] = (xyz->xyz[2] / 64);
		}

		// find the next surface
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);

	}

//	MD3_DecodeNormals(mod, mod->md3[lod]);

	VectorSet(mod->mins, -32, -32, -32);
	VectorSet(mod->maxs, 32, 32, 32);
}


/*
================
MD3_GetTag
================
*/
//static md3Tag_t* MD3_GetTag(md3Header_t* mod, int frame, const char* tagName)
static md3Tag_t* MD3_GetTag(md3Header_t* mod, int frame, int tagIndex)
{
	md3Tag_t* tag;
	int			i;

	if (frame >= mod->numFrames)
	{
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}
	else if (frame < 0)
	{
		frame = 0;
	}

	tag = (md3Tag_t*)((byte*)mod + mod->ofsTags) + frame * mod->numTags;
	for (i = 0; i < mod->numTags; i++, tag++)
	{
		if (i == tagIndex)
		{
			return tag;	// found it
		}
	}
	return NULL;
}

/*
================
R_TagIndexForName
================
*/
int R_TagIndexForName(struct model_s *model, const char* tagName)
{
	md3Tag_t* tag;
	int			i;

	if (!model->md3[0])
		return -1;

	md3Header_t *mod = model->md3[LOD_HIGH];
	tag = (md3Tag_t*)((byte*)mod + mod->ofsTags);
	for (i = 0; i < mod->numTags; i++, tag++)
	{
		if (!strcmp(tag->name, tagName))
		{
			return i;	// found it
		}
	}
	return -1;
}

/*
================
MD3_LerpTag
================
*/
//static qboolean MD3_LerpTag(orientation_t* tag, model_t* model, int startFrame, int endFrame, float frac, const char* tagName)
static qboolean MD3_LerpTag(orientation_t* tag, model_t* model, int startFrame, int endFrame, float frac, int tagIndex)
{
	md3Tag_t* start, * end;
	float		frontLerp, backLerp;
	int			i;

	if (!model->md3[LOD_HIGH])
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	start = MD3_GetTag(model->md3[LOD_HIGH], startFrame, tagIndex);
	end = MD3_GetTag(model->md3[LOD_HIGH], endFrame, tagIndex);

	if (!start || !end)
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	frontLerp = frac;
	backLerp = 1.0f - frac;

	for (i = 0; i < 3; i++)
	{
		tag->origin[i] = start->origin[i] * backLerp + end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp + end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp + end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp + end->axis[2][i] * frontLerp;
	}
	VectorNormalize(tag->axis[0]);
	VectorNormalize(tag->axis[1]);
	VectorNormalize(tag->axis[2]);
	return true;
}


/*
====================
MD3_ModelBounds
====================
*/
static void MD3_ModelBounds(int handle, vec3_t mins, vec3_t maxs)
{
	model_t* model;
	md3Header_t* header;
	md3Frame_t* frame;

	model = Mod_ForNum(handle);
	if (!model->md3[LOD_HIGH])
	{
		VectorClear(mins);
		VectorClear(maxs);
		return;
	}

	header = model->md3[LOD_HIGH];

	frame = (md3Frame_t*)((byte*)header + header->ofsFrames);

	VectorCopy(frame->bounds[0], mins);
	VectorCopy(frame->bounds[1], maxs);
}


/*
=================
R_LerpMD3Frame

Smoothly transitions vertices between two animation frames and also calculates normal
=================
*/
static void R_LerpMD3Frame(float lerp, int index, md3XyzNormal_t* oldVert, md3XyzNormal_t* vert, vec3_t outVert, vec3_t outNormal)
{
	int lat, lng;
	vec3_t p1, p2, n1, n2;

	// linear interpolation between the current and next vertex positions
	VectorCopy(oldVert->xyz, p1);
	VectorCopy(vert->xyz, p2);

	outVert[0] = (p1[0] + lerp * (p2[0] - p1[0]));
	outVert[1] = (p1[1] + lerp * (p2[1] - p1[1]));
	outVert[2] = (p1[2] + lerp * (p2[2] - p1[2]));

	// scale verticles to qu
	outVert[0] = (outVert[0] / 64);
	outVert[1] = (outVert[1] / 64);
	outVert[2] = (outVert[2] / 64);

	// retrieve and linear interpolate normal
	lat = (oldVert->normal >> 8) & 0xff;
	lng = (oldVert->normal & 0xff);
	lat *= (FUNCTABLE_SIZE / 256);
	lng *= (FUNCTABLE_SIZE / 256);

	n1[0] = sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * sinTable[lng];
	n1[1] = sinTable[lat] * sinTable[lng];
	n1[2] = sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

	lat = (vert->normal >> 8) & 0xff;
	lng = (vert->normal & 0xff);
	lat *= (FUNCTABLE_SIZE / 256);
	lng *= (FUNCTABLE_SIZE / 256);

	n2[0] = sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * sinTable[lng];
	n2[1] = sinTable[lat] * sinTable[lng];
	n2[2] = sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

	outNormal[0] = (n1[0] + lerp * (n2[0] - n1[0]));
	outNormal[1] = (n1[1] + lerp * (n2[1] - n1[1]));
	outNormal[2] = (n1[2] + lerp * (n2[2] - n1[2]));
}


/*
=================
R_DrawMD3Model

Draws md3 model
=================
*/
void R_DrawMD3Model(rentity_t* ent, lod_t lod, float animlerp)
{
	md3Header_t		*model;
	md3Surface_t	*surface = 0;
	md3Shader_t		*md3Shader = 0;
	md3Frame_t		*md3Frame = 0;
	image_t			*shader = NULL;
	md3St_t			*texcoord;
	md3XyzNormal_t	*vert, *oldVert; //interp
	md3Triangle_t	*triangle;
	int				i, j, k;
	float			lambert;

	vec3_t v, n; //vert and normal after lerp

	if (lod < 0 || lod >= NUM_LODS)
		ri.Error(ERR_DROP, "R_DrawMD3Model: '%s' wrong LOD num %i\n", ent->model->name, lod);

	model = ent->model->md3[lod];
	if (model == NULL)
	{
		Com_Printf("R_DrawMD3Model: '%s' has no LOD %i model\n", ent->model->name, lod);
		return;
	}


	// for each surface
	surface = (md3Surface_t*)((byte*)model + model->ofsSurfaces);
	for (i = 0; i < model->numSurfaces; i++)
	{	
		if( (ent->hiddenPartsBits & (1 << i)))
		{
			surface = (md3Surface_t*)((byte*)surface + surface->ofsEnd);
			continue; // surface is hidden
		}

		if (surface->numShaders > 0) 
		{
			//bind texture
			md3Shader = (md3Shader_t*)((byte*)surface + surface->ofsShaders);
			GL_Bind(md3Shader->shaderIndex);
		}

		// grab tris, texcoords and verts
		texcoord = (md3St_t*)((byte*)surface + surface->ofsSt);
		triangle = (md3Triangle_t*)((byte*)surface + surface->ofsTriangles);

		oldVert = (short*)((byte*)surface + surface->ofsXyzNormals) + (ent->oldframe * surface->numVerts * 4); // current keyframe verts
		vert = (short*)((byte*)surface + surface->ofsXyzNormals) + (ent->frame * surface->numVerts * 4); // next keyframe verts

		if(r_speeds->value)
			rperf.alias_tris += surface->numTriangles;

		// for each triangle in surface
		glBegin(GL_TRIANGLES);
		for (j = 0; j < surface->numTriangles; j++, triangle++)
		{
			//for each vert in triangle
			for (k = 0; k < 3; k++)
			{
				int index = triangle->indexes[k];

				R_LerpMD3Frame(animlerp, index, &oldVert[index], &vert[index], v, n);

				if (r_fullbright->value || pCurrentRefEnt->renderfx & RF_FULLBRIGHT)
				{
					lambert = 1.0f;
				}
				else
				{
					// poor mans half lambert to prevent models from being too dark
					lambert = 1 + DotProduct(n, model_shadevector);
					if (lambert > 1)
						lambert = 1;
					else if (lambert < 0)
						lambert = 0;
				}

				glColor4f(lambert * model_shadelight[0], lambert * model_shadelight[1], lambert * model_shadelight[2], ent->alpha);	
				glTexCoord2f(texcoord[index].st[0], texcoord[index].st[1]);
				glVertex3fv(v);
				glNormal3fv(n);
			}
		}
		glEnd();

		surface = (md3Surface_t*)((byte*)surface + surface->ofsEnd);
	}
}


qboolean R_LerpTag(orientation_t* tag, struct model_t* model, int startFrame, int endFrame, float frac, int tagIndex)
{
	return MD3_LerpTag(tag, model, startFrame, endFrame, frac, tagIndex);
}


#if 0
static void MD3_Normals(md3Header_t* model, float yangle)
{
	md3Surface_t* surf;
	md3XyzNormal_t* xyz;

	float			angle;
	float			lat, lng;
	float			angleCos, angleSin;
	vec3_t			temp;
	vec3_t			normal;

	angle = yangle / 180 * M_PI;
	angleCos = cos(angle);
	angleSin = sin(angle);

	surf = (md3Surface_t*)((byte*)model + model->ofsSurfaces);
	for (int i = 0; i < model->numSurfaces; i++)
	{
		xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);

		for (int j = 0; j < surf->numVerts; j++, xyz++)
		{
			// decode the lat/lng normal to a 3 float normal
			lat = (xyz->normal >> 8) & 0xff;
			lng = (xyz->normal & 0xff);
			lat *= M_PI / 128;
			lng *= M_PI / 128;

			temp[0] = cos(lat) * sin(lng);
			temp[1] = sin(lat) * sin(lng);
			temp[2] = cos(lng);

			// rotate the normal
			normal[0] = temp[0] * angleCos - temp[1] * angleSin;
			normal[1] = temp[0] * angleSin + temp[1] * angleCos;
			normal[2] = temp[2];

//			printf("yawangle %f = surf %i tri %i normal [%f %f %f]\n", yangle, i, j, normal[0], normal[1], normal[2]);
		}
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}
}
#endif
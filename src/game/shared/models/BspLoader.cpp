/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 * Use, distribution, and modification of this source code and/or resulting
 * object code is restricted to non-commercial enhancements to products from
 * Valve LLC.  All other use, distribution, or modification is prohibited
 * without written permission from Valve LLC.
 *
 ****/
#include "cbase.h"
#include "BspLoader.h"

#if defined(TRINITY)
#include <cstring>
#else
#define vec3_t Vector
#include "../../utils/common/bspfile.h"
#endif

std::optional<BspData> BspLoader::Load(const char* fileName)
{
	const auto contents = FileSystem_LoadFileIntoBuffer(fileName, FileContentFormat::Binary);

	if (contents.size() < sizeof(dheader_t))
		return {};

	dheader_t header;
	std::memcpy(&header, contents.data(), sizeof(dheader_t));

	if (header.version != BSPVERSION)
		return {};

	BspData data;

#if defined(TRINITY)
	auto ReadLump = [&contents, &header](int lumpIndex, auto& outVector)
	{
		using T = typename std::decay_t<decltype(outVector)>::value_type;
		const auto& lump = header.lumps[lumpIndex];

		if (lump.filelen > 0 && static_cast<std::size_t>(lump.fileofs + lump.filelen) <= contents.size())
		{
			std::size_t count = static_cast<std::size_t>(lump.filelen) / sizeof(T);
			outVector.resize(count);
			std::memcpy(outVector.data(), contents.data() + lump.fileofs, lump.filelen);
		}
	};

	ReadLump(LUMP_MODELS, data.Models);
	data.SubModelCount = data.Models.size();

	ReadLump(LUMP_FACES, data.Faces);
	ReadLump(LUMP_VERTEXES, data.Vertices);
	ReadLump(LUMP_EDGES, data.Edges);
	ReadLump(LUMP_SURFEDGES, data.SurfEdges);
	ReadLump(LUMP_TEXINFO, data.TexInfos);
	ReadLump(LUMP_LIGHTING, data.Lighting);

	// NUEVO: Leemos los planos
	ReadLump(LUMP_PLANES, data.Planes);

	// NUEVO: Leemos los nombres de las texturas (LUMP_TEXTURES)
	const auto& texLump = header.lumps[LUMP_TEXTURES];
	if (texLump.filelen > 0 && static_cast<std::size_t>(texLump.fileofs + texLump.filelen) <= contents.size())
	{
		const uint8_t* texData = reinterpret_cast<const uint8_t*>(contents.data()) + texLump.fileofs;
		int numMipTex = *reinterpret_cast<const int*>(texData);
		const int* offsets = reinterpret_cast<const int*>(texData + sizeof(int));

		data.TextureNames.resize(numMipTex);
		for (int i = 0; i < numMipTex; i++)
		{
			if (offsets[i] != -1) // -1 indica que la textura no está cargada
			{
				const char* namePtr = reinterpret_cast<const char*>(texData + offsets[i]);
				std::memcpy(data.TextureNames[i].data(), namePtr, 16);
			}
			else
			{
				std::memset(data.TextureNames[i].data(), 0, 16);
			}
		}
	}
#else
	const auto& modelLump = header.lumps[LUMP_MODELS];
	data.SubModelCount = static_cast<std::size_t>(modelLump.filelen) / sizeof(dmodel_t);
#endif

	return data;
}
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
#pragma once

#include <cstddef>
#include <optional>

#if defined(TRINITY)
#include <vector>
#ifndef vec3_t
#define vec3_t Vector
#endif
#include "../../utils/common/bspfile.h"
#endif

struct BspData
{
	std::size_t SubModelCount{};

#if defined(TRINITY)
	std::vector<dmodel_t> Models;
	std::vector<dface_t> Faces;
	std::vector<dvertex_t> Vertices;
	std::vector<dedge_t> Edges;
	std::vector<int> SurfEdges;
	std::vector<texinfo_t> TexInfos;
	std::vector<unsigned char> Lighting;
	std::vector<dplane_t> Planes;
	std::vector<std::array<char, 16>> TextureNames;
#endif
};

/**
 *	@brief Loads BSP data into memory for use.
 *	Extend as needed if more data is required.
 */
class BspLoader final
{
public:
	BspLoader() = delete;

	static std::optional<BspData> Load(const char* fileName);
};
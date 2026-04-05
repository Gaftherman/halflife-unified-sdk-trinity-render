#if defined(TRINITY)
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "../shared/models/BspLoader.h"
#include <vector>
#include <string>
#include <fmt/format.h>

struct ExportVertex
{
	float pos[3];
	float s, t;
	float u, v;
};

void ExportDetails()
{
	char szMapPath[256];
	snprintf(szMapPath, sizeof(szMapPath), "maps/%s.bsp", STRING(gpGlobals->mapname));

	auto bspDataOpt = BspLoader::Load(szMapPath);
	if (!bspDataOpt)
	{
		return;
	}

	const BspData& bsp = bspDataOpt.value();

	std::vector<edict_t*> detailEnts;
	edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(0);

	for (int i = 0; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (!pEdict->free && FStrEq(STRING(pEdict->v.classname), "func_detail_ext"))
		{
			detailEnts.push_back(pEdict);
		}
	}

	if (detailEnts.empty())
		return;

	std::vector<uint8_t> outBuffer;

	auto WriteData = [&outBuffer](const void* data, size_t size)
	{
		const uint8_t* pBytes = static_cast<const uint8_t*>(data);
		outBuffer.insert(outBuffer.end(), pBytes, pBytes + size);
	};

	int iNumDetailEnts = static_cast<int>(detailEnts.size());
	WriteData(&iNumDetailEnts, sizeof(int));

	for (edict_t* ent : detailEnts)
	{
		int modelIndex = ent->v.modelindex;
		if (modelIndex <= 0 || static_cast<size_t>(modelIndex) >= bsp.Models.size())
			continue;

		const dmodel_t& mod = bsp.Models[modelIndex];

		WriteData(&mod.mins, sizeof(Vector));
		WriteData(&mod.maxs, sizeof(Vector));
		WriteData(&mod.numfaces, sizeof(int));

		for (int j = 0; j < mod.numfaces; j++)
		{
			const dface_t& face = bsp.Faces[mod.firstface + j];
			const texinfo_t& tex = bsp.TexInfos[face.texinfo];

			char texName[16] = {0};
			if (tex.miptex >= 0 && static_cast<size_t>(tex.miptex) < bsp.TextureNames.size())
			{
				std::memcpy(texName, bsp.TextureNames[tex.miptex].data(), 16);
			}
			else
			{
				std::strncpy(texName, "null", 15);
			}
			WriteData(texName, 16);

			int lightmapIdx = (int)face.styles[0];
			int numVerts = (int)face.numedges;
			WriteData(&lightmapIdx, sizeof(int));
			WriteData(&numVerts, sizeof(int));

			const dplane_t& plane = bsp.Planes[face.planenum];
			Vector realNorm(plane.normal[0], plane.normal[1], plane.normal[2]);
			float realDist = plane.dist;

			if (face.side)
			{
				realNorm = realNorm * -1.0f;
				realDist = -realDist;
			}
			WriteData(&realNorm, sizeof(Vector));
			WriteData(&realDist, sizeof(float));

			WriteData(&tex.flags, sizeof(int));
			float mipadj = 1.0f;
			WriteData(&mipadj, sizeof(float));
			WriteData(&tex.vecs, sizeof(float) * 8);

			short sShort = 0;
			for (int k = 0; k < 4; k++)
				WriteData(&sShort, sizeof(short));

			int dummyInt = 0;
			WriteData(&face.lightofs, sizeof(int));
			WriteData(&dummyInt, sizeof(int));
			WriteData(&dummyInt, sizeof(int));

			for (int k = 0; k < 3; k++)
			{
				uint8_t colorDummy = 255;
				WriteData(&colorDummy, sizeof(uint8_t));
			}

			// 5. Reconstrucción de la geometría y cálculo de UV
			for (int e = 0; e < face.numedges; e++)
			{
				int surfEdge = bsp.SurfEdges[face.firstedge + e];
				int edgeIdx = abs(surfEdge);
				int vIdx = (surfEdge >= 0) ? bsp.Edges[edgeIdx].v[0] : bsp.Edges[edgeIdx].v[1];

				const float* pos = bsp.Vertices[vIdx].point;

				float s = pos[0] * tex.vecs[0][0] + pos[1] * tex.vecs[0][1] + pos[2] * tex.vecs[0][2] + tex.vecs[0][3];
				float t = pos[0] * tex.vecs[1][0] + pos[1] * tex.vecs[1][1] + pos[2] * tex.vecs[1][2] + tex.vecs[1][3];

				ExportVertex vertex = {
					{pos[0], pos[1], pos[2]},
					s, t,
					0.0f, 0.0f
				};

				WriteData(&vertex, sizeof(ExportVertex));
			}
		}
	}

	char szOutPath[256];
	g_engfuncs.pfnGetGameDir(szOutPath);
	strcat(szOutPath, "/maps/");
	strcat(szOutPath, (char*)STRING(gpGlobals->mapname));
	strcat(szOutPath, ".edd");

	FILE* pFile = fopen(szOutPath, "wb");
	if (pFile)
	{
		fwrite(outBuffer.data(), outBuffer.size(), 1, pFile);
		fclose(pFile);
	}
}
#endif
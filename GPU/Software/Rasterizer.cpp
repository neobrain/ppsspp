// Copyright (c) 2013- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#include "../../Core/MemMap.h"
#include "../GPUState.h"

#include "Rasterizer.h"

extern u8* fb;
extern u8* depthbuf;

extern u32 clut[4096];

namespace Rasterizer {

static int orient2d(const DrawingCoords& v0, const DrawingCoords& v1, const DrawingCoords& v2)
{
	return ((int)v1.x-(int)v0.x)*((int)v2.y-(int)v0.y) - ((int)v1.y-(int)v0.y)*((int)v2.x-(int)v0.x);
}

int GetPixelDataOffset(int texel_size_bits, int u, int v, int width)
{
	if (!(gstate.texmode & 1))
		return v * width * texel_size_bits / 8 + u * texel_size_bits / 8;

	int tile_size_bits = 32;
	int texels_per_tile = tile_size_bits / texel_size_bits; // 32/8
	int block_width_in_tiles = 4; // 4 tiles (generally != 4 texels)
	int block_height_in_tiles = 8; // 8 tiles = 8 texels
	int tiles_per_block = block_width_in_tiles * block_height_in_tiles;
	int block_stride_bits = tiles_per_block * tile_size_bits;
	return u / (texels_per_tile * block_width_in_tiles) * (block_stride_bits/8) + 
			(u % (texels_per_tile * block_width_in_tiles)) * (texel_size_bits / 8) +
			(v % block_height_in_tiles) * (block_width_in_tiles * tile_size_bits / 8) +
			(v / block_height_in_tiles) * (width * texel_size_bits * block_height_in_tiles / 8);
}

u32 SampleNearest(int level, float s, float t)
{
	int texfmt = gstate.texformat & 0xF;
	u32 texaddr = (gstate.texaddr[level] & 0xFFFFF0) | ((gstate.texbufwidth[level] << 8) & 0x0F000000);
	u8* srcptr = (u8*)Memory::GetPointer(texaddr); // TODO: not sure if this is the right place to load from...?

	int width = 1 << (gstate.texsize[level] & 0xf);
	int height = 1 << ((gstate.texsize[level]>>8) & 0xf);

	// TODO: Should probably check if textures are aligned properly...

	// TODO: Not sure if that through mode treatment is correct..
	int u = (gstate.isModeThrough()) ? s : s * width; // TODO: -1?
	int v = (gstate.isModeThrough()) ? t : t * height; // TODO: -1?

	// TODO: texcoord wrapping!!

	// TODO: Assert tmap.tmn == 0 (uv texture mapping mode)

	if (texfmt == GE_TFMT_4444) {
		srcptr += GetPixelDataOffset(16, u, v, width);
		u8 r = (*srcptr) >> 4;
		u8 g = (*srcptr) & 0xF;
		u8 b = (*(srcptr+1)) >> 4;
		u8 a = (*(srcptr+1)) & 0xF;
		r = (r << 4) | r;
		g = (g << 4) | g;
		b = (b << 4) | b;
		a = (a << 4) | a;
		return (r << 24) | (g << 16) | (b << 8) | a;
	} else if (texfmt == GE_TFMT_5551) {
		srcptr += GetPixelDataOffset(16, u, v, width);
		u8 r = (*srcptr) & 0x1F;
		u8 g = (((*srcptr) & 0xE0) >> 5) | (((*(srcptr+1))&0x3) << 3);
		u8 b = ((*srcptr+1) & 0x7C) >> 2;
		u8 a = (*(srcptr+1)) >> 7;
		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);
		a = (a) ? 0xff : 0;
		return (r << 24) | (g << 16) | (b << 8) | a;
	} else if (texfmt == GE_TFMT_5650) {
		srcptr += GetPixelDataOffset(16, u, v, width);
		u8 r = (*srcptr) & 0x1F;
		u8 g = (((*srcptr) & 0xE0) >> 5) | (((*(srcptr+1))&0x7) << 3);
		u8 b = ((*srcptr+1) & 0xF8) >> 3;
		u8 a = 0xff;
		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);
		return (r << 24) | (g << 16) | (b << 8) | a;
	} else if (texfmt == GE_TFMT_8888) {
		srcptr += GetPixelDataOffset(32, u, v, width);
		u8 r = *srcptr++;
		u8 g = *srcptr++;
		u8 b = *srcptr++;
		u8 a = *srcptr++;
		return (r << 24) | (g << 16) | (b << 8) | a;
	} else if (texfmt == GE_TFMT_CLUT8) {
		srcptr += GetPixelDataOffset(8, u, v, width);

		u16 index = (((u32)*srcptr) >> gstate.getClutIndexShift()) & 0xFF;
		index &= gstate.getClutIndexMask();
		index = (index & 0xFF) | gstate.getClutIndexStartPos(); // Topmost bit is copied from start pos

		// TODO: Assert that we're using GE_CMODE_32BIT_ABGR8888;
		return clut[index];
	} else if (texfmt == GE_TFMT_CLUT4) {
		srcptr += GetPixelDataOffset(4, u, v, width);

		u8 val = (u%2) ? (*srcptr & 0xF) : (*srcptr >> 4);
		u16 index = (((u32)val) >> gstate.getClutIndexShift()) & 0xFF;
		index &= gstate.getClutIndexMask();
		index = (index & 0xFF) | gstate.getClutIndexStartPos(); // Topmost bit is copied from start pos

		// TODO: Assert that we're using GE_CMODE_32BIT_ABGR8888;
		return clut[index];
	} else {
		ERROR_LOG(G3D, "Unsupported texture format: %x", texfmt);
		return 0;
	}
}

static inline u32 GetPixelColor(int x, int y)
{
	return *(u32*)&fb[4*x + 4*y*gstate.FrameBufStride()];
}

static inline void SetPixelColor(int x, int y, u32 value)
{
	*(u32*)&fb[4*x + 4*y*gstate.FrameBufStride()] = value;
}

static inline u16 GetPixelDepth(int x, int y)
{
	return *(u16*)&depthbuf[2*x + 2*y*gstate.DepthBufStride()];
}

static inline void SetPixelDepth(int x, int y, u16 value)
{
	*(u16*)&depthbuf[2*x + 2*y*gstate.DepthBufStride()] = value;
}

static inline bool DepthTestPassed(int x, int y, u16 z)
{
	u16 reference_z = GetPixelDepth(x, y);

	if (gstate.isModeClear())
		return true;

	switch (gstate.getDepthTestFunc()) {
	case GE_COMP_NEVER:
		return false;

	case GE_COMP_ALWAYS:
		return true;

	case GE_COMP_EQUAL:
		return (z == reference_z);

	case GE_COMP_NOTEQUAL:
		return (z != reference_z);

	case GE_COMP_LESS:
		return (z < reference_z);

	case GE_COMP_LEQUAL:
		return (z <= reference_z);

	case GE_COMP_GREATER:
		return (z > reference_z);

	case GE_COMP_GEQUAL:
		return (z >= reference_z);

	default:
		return 0;
	}
}

bool IsRightSideOrFlatBottomLine(const Vec2<u10,u10>& vertex, const Vec2<u10,u10>& line1, const Vec2<u10,u10>& line2)
{
	if (line1.y == line2.y) {
		// just check if vertex is above us => bottom line parallel to x-axis
		return vertex.y < line1.y;
	} else {
		// check if vertex is on our left => right side
		return vertex.x < line1.x + (line2.x - line1.x) * (vertex.y - line1.y) / (line2.y - line1.y);
	}
}

// Draws triangle, vertices specified in counter-clockwise direction (TODO: Make sure this is actually enforced)
void DrawTriangle(const VertexData& v0, const VertexData& v1, const VertexData& v2)
{
	int minX = std::min(std::min(v0.drawpos.x, v1.drawpos.x), v2.drawpos.x);
	int minY = std::min(std::min(v0.drawpos.y, v1.drawpos.y), v2.drawpos.y);
	int maxX = std::max(std::max(v0.drawpos.x, v1.drawpos.x), v2.drawpos.x);
	int maxY = std::max(std::max(v0.drawpos.y, v1.drawpos.y), v2.drawpos.y);

	minX = std::max(minX, gstate.getScissorX1());
	maxX = std::min(maxX, gstate.getScissorX2());
	minY = std::max(minY, gstate.getScissorY1());
	maxY = std::min(maxY, gstate.getScissorY2());

	int bias0 = IsRightSideOrFlatBottomLine(v0.drawpos.xy(), v1.drawpos.xy(), v2.drawpos.xy()) ? -1 : 0;
	int bias1 = IsRightSideOrFlatBottomLine(v1.drawpos.xy(), v2.drawpos.xy(), v0.drawpos.xy()) ? -1 : 0;
	int bias2 = IsRightSideOrFlatBottomLine(v2.drawpos.xy(), v0.drawpos.xy(), v1.drawpos.xy()) ? -1 : 0;

	DrawingCoords p(minX, minY, 0);
	for (p.y = minY; p.y <= maxY; ++p.y) {
		for (p.x = minX; p.x <= maxX; ++p.x) {
			int w0 = orient2d(v1.drawpos, v2.drawpos, p) + bias0;
			int w1 = orient2d(v2.drawpos, v0.drawpos, p) + bias1;
			int w2 = orient2d(v0.drawpos, v1.drawpos, p) + bias2;

			// If p is on or inside all edges, render pixel
			// TODO: Should only render when it's on the left of the right edge
			if (w0 >=0 && w1 >= 0 && w2 >= 0) {
				if (w0 == w1 && w1 == w2 && w2 == 0)
					continue;

				// TODO: Make sure this is not ridiculously small?
				float den = 1.0f/v0.clippos.w * w0 + 1.0f/v1.clippos.w * w1 + 1.0f/v2.clippos.w * w2;

				// TODO: Depth range test

				// TODO: Is it safe to ignore gstate.isDepthTestEnabled() when clear mode is enabled?
				if ((gstate.isDepthTestEnabled() && !gstate.isModeThrough()) || gstate.isModeClear()) {
					// TODO: Is that the correct way to interpolate?
					u16 z = (u16)((v0.drawpos.z * w0 + v1.drawpos.z * w1 + v2.drawpos.z * w2) / (w0+w1+w2));

					if (!DepthTestPassed(p.x, p.y, z))
						continue;

					// TODO: Is this condition correct?
					if (gstate.isDepthWriteEnabled() || ((gstate.clearmode&0x40) && gstate.isModeClear()))
						SetPixelDepth(p.x, p.y, z);
				}

				float s = (v0.texturecoords.s * w0 / v0.clippos.w + v1.texturecoords.s * w1 / v1.clippos.w + v2.texturecoords.s * w2 / v2.clippos.w) / den;
				float t = (v0.texturecoords.t * w0 / v0.clippos.w + v1.texturecoords.t * w1 / v1.clippos.w + v2.texturecoords.t * w2 / v2.clippos.w) / den;

				Vec4<int> prim_color(0, 0, 0, 0);
				Vec3<int> sec_color(0, 0, 0);
				if ((gstate.shademodel&1) == GE_SHADE_GOURAUD) {
					// NOTE: When not casting color0 and color1 to float vectors, this code suffers from severe overflow issues.
					// Not sure if that should be regarded as a bug or if casting to float is a valid fix.
					// TODO: Is that the correct way to interpolate?
					prim_color = ((v0.color0.Cast<float,float,float,float>() * w0 +
									v1.color0.Cast<float,float,float,float>() * w1 +
									v2.color0.Cast<float,float,float,float>() * w2) / (w0+w1+w2)).Cast<int,int,int,int>();
					sec_color = ((v0.color1.Cast<float,float,float>() * w0 +
									v1.color1.Cast<float,float,float>() * w1 +
									v2.color1.Cast<float,float,float>() * w2) / (w0+w1+w2)).Cast<int,int,int>();
				} else {
					prim_color = v2.color0;
					sec_color = v2.color1;
				}

				// TODO: Also disable if vertex has no texture coordinates?
				if (gstate.isTextureMapEnabled() && !gstate.isModeClear()) {
					Vec4<int> texcolor = Vec4<int>::FromRGBA(/*TextureDecoder::*/SampleNearest(0, s, t));

					bool rgba = (gstate.texfunc & 0x10) != 0;

					// texture function
					switch (gstate.getTextureFunction()) {
					case GE_TEXFUNC_MODULATE:
						prim_color.rgb() = prim_color.rgb() * texcolor.rgb() / 255;
						prim_color.a = (rgba) ? (prim_color.a * texcolor.a / 255) : prim_color.a;
						break;

					case GE_TEXFUNC_DECAL:
					{
						int t = (rgba) ? texcolor.a : 255;
						int invt = (rgba) ? 255 - t : 0;
						prim_color.rgb() = (invt * prim_color.rgb() + t * texcolor.rgb()) / 255;
						// prim_color.a = prim_color.a;
						break;
					}

					case GE_TEXFUNC_BLEND:
					{
						const Vec3<int> const255(255, 255, 255);
						const Vec3<int> texenv(gstate.getTextureEnvColR(), gstate.getTextureEnvColG(), gstate.getTextureEnvColB());
						prim_color.rgb() = ((const255 - texcolor.rgb()) * prim_color.rgb() + texcolor.rgb() * texenv) / 255;
						prim_color.a = prim_color.a * ((rgba) ? texcolor.a : 255) / 255;
						break;
					}

					case GE_TEXFUNC_REPLACE:
						prim_color.rgb() = texcolor.rgb();
						prim_color.a = (rgba) ? texcolor.a : prim_color.a;
						break;

					case GE_TEXFUNC_ADD:
						prim_color.rgb() += texcolor.rgb();
						if (prim_color.r > 255) prim_color.r = 255;
						if (prim_color.g > 255) prim_color.g = 255;
						if (prim_color.b > 255) prim_color.b = 255;
						prim_color.a = prim_color.a * ((rgba) ? texcolor.a : 255) / 255;
						break;

					default:
						ERROR_LOG(G3D, "Unknown texture function %x", gstate.getTextureFunction());
					}
				}

				if (gstate.isColorDoublingEnabled()) {
					// TODO: Do we need to clamp here?
					prim_color.rgb() *= 2;
					sec_color *= 2;
				}

				prim_color.rgb() += sec_color;
				if (prim_color.r > 255) prim_color.r = 255;
				if (prim_color.g > 255) prim_color.g = 255;
				if (prim_color.b > 255) prim_color.b = 255;
				if (prim_color.r < 0) prim_color.r = 0;
				if (prim_color.g < 0) prim_color.g = 0;
				if (prim_color.b < 0) prim_color.b = 0;

				// TODO: Fogging

				if (gstate.isAlphaBlendEnabled()) {
					Vec4<int> dst = Vec4<int>::FromRGBA(GetPixelColor(p.x, p.y));

					Vec3<int> srccol(0, 0, 0);
					Vec3<int> dstcol(0, 0, 0);

					switch (gstate.getBlendFuncA()) {
					case GE_SRCBLEND_DSTCOLOR:
						srccol = dst.rgb();
						break;
					case GE_SRCBLEND_INVDSTCOLOR:
						srccol = Vec3<int>(255, 255, 255) - dst.rgb();
						break;
					case GE_SRCBLEND_SRCALPHA:
						srccol = Vec3<int>(prim_color.a, prim_color.a, prim_color.a);
						break;
					case GE_SRCBLEND_INVSRCALPHA:
						srccol = Vec3<int>(255 - prim_color.a, 255 - prim_color.a, 255 - prim_color.a);
						break;
					case GE_SRCBLEND_DSTALPHA:
						srccol = Vec3<int>(dst.a, dst.a, dst.a);
						break;
					case GE_SRCBLEND_INVDSTALPHA:
						srccol = Vec3<int>(255 - dst.a, 255 - dst.a, 255 - dst.a);
						break;
					case GE_SRCBLEND_DOUBLESRCALPHA:
						srccol = 2 * Vec3<int>(prim_color.a, prim_color.a, prim_color.a);
						break;
					case GE_SRCBLEND_DOUBLEINVSRCALPHA:
						srccol = 2 * Vec3<int>(255 - prim_color.a, 255 - prim_color.a, 255 - prim_color.a);
						break;
					case GE_SRCBLEND_DOUBLEDSTALPHA:
						srccol = 2 * Vec3<int>(dst.a, dst.a, dst.a);
						break;
					case GE_SRCBLEND_DOUBLEINVDSTALPHA:
						srccol = 2 * Vec3<int>(255 - dst.a, 255 - dst.a, 255 - dst.a);
						break;
					case GE_SRCBLEND_FIXA:
						srccol = Vec4<int>::FromRGBA(gstate.getFixA()).rgb();
						break;
					}

					switch (gstate.getBlendFuncB()) {
					case GE_DSTBLEND_SRCCOLOR:
						dstcol = prim_color.rgb();
						break;
					case GE_DSTBLEND_INVSRCCOLOR:
						dstcol = Vec3<int>(255, 255, 255) - prim_color.rgb();
						break;
					case GE_DSTBLEND_SRCALPHA:
						dstcol = Vec3<int>(prim_color.a, prim_color.a, prim_color.a);
						break;
					case GE_DSTBLEND_INVSRCALPHA:
						dstcol = Vec3<int>(255 - prim_color.a, 255 - prim_color.a, 255 - prim_color.a);
						break;
					case GE_DSTBLEND_DSTALPHA:
						dstcol = Vec3<int>(dst.a, dst.a, dst.a);
						break;
					case GE_DSTBLEND_INVDSTALPHA:
						dstcol = Vec3<int>(255 - dst.a, 255 - dst.a, 255 - dst.a);
						break;
					case GE_DSTBLEND_DOUBLESRCALPHA:
						dstcol = 2 * Vec3<int>(prim_color.a, prim_color.a, prim_color.a);
						break;
					case GE_DSTBLEND_DOUBLEINVSRCALPHA:
						dstcol = 2 * Vec3<int>(255 - prim_color.a, 255 - prim_color.a, 255 - prim_color.a);
						break;
					case GE_DSTBLEND_DOUBLEDSTALPHA:
						dstcol = 2 * Vec3<int>(dst.a, dst.a, dst.a);
						break;
					case GE_DSTBLEND_DOUBLEINVDSTALPHA:
						dstcol = 2 * Vec3<int>(255 - dst.a, 255 - dst.a, 255 - dst.a);
						break;
					case GE_DSTBLEND_FIXB:
						dstcol = Vec4<int>::FromRGBA(gstate.getFixB()).rgb();
						break;
					}

					switch (gstate.getBlendEq()) {
					case GE_BLENDMODE_MUL_AND_ADD:
						prim_color.rgb() = (prim_color.rgb() * srccol + dst.rgb() * dstcol) / 255;
						break;
					case GE_BLENDMODE_MUL_AND_SUBTRACT:
						prim_color.rgb() = (prim_color.rgb() * srccol - dst.rgb() * dstcol) / 255;
						break;
					case GE_BLENDMODE_MUL_AND_SUBTRACT_REVERSE:
						prim_color.rgb() = (dst.rgb() * dstcol - prim_color.rgb() * srccol) / 255;
						break;
					case GE_BLENDMODE_MIN:
						prim_color.r = std::min(prim_color.r, dst.r);
						prim_color.g = std::min(prim_color.g, dst.g);
						prim_color.b = std::min(prim_color.b, dst.b);
						break;
					case GE_BLENDMODE_MAX:
						prim_color.r = std::max(prim_color.r, dst.r);
						prim_color.g = std::max(prim_color.g, dst.g);
						prim_color.b = std::max(prim_color.b, dst.b);
						break;
					case GE_BLENDMODE_ABSDIFF:
						prim_color.r = std::abs(prim_color.r - dst.r);
						prim_color.g = std::abs(prim_color.g - dst.g);
						prim_color.b = std::abs(prim_color.b - dst.b);
						break;
					}
				}
				SetPixelColor(p.x, p.y, prim_color.ToRGBA());
			}
		}
	}
}

} // namespace

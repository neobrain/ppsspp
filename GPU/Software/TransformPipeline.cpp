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

#include "../GPUState.h"
#include "../GLES/VertexDecoder.h"

#include "TransformPipeline.h"
#include "Rasterizer.h"

template<typename BaseType>
class Mat3x3
{
public:
	// Convention: first three values in arrow = first column
	Mat3x3(const BaseType values[])
	{
		for (unsigned int i = 0; i < 3*3; ++i)
		{
			this->values[i] = values[i];
		}
	}

	Mat3x3(BaseType _00, BaseType _01, BaseType _02, BaseType _10, BaseType _11, BaseType _12, BaseType _20, BaseType _21, BaseType _22)
	{
		values[0] = _00;
		values[1] = _01;
		values[2] = _02;
		values[3] = _10;
		values[4] = _11;
		values[5] = _12;
		values[6] = _20;
		values[7] = _21;
		values[8] = _22;
	}

	template<typename X, typename Y, typename Z>
	Vec3<X,Y,Z> operator * (const Vec3<X,Y,Z>& vec)
	{
		Vec3<X,Y,Z> ret;
		ret.x = values[0]*vec.x + values[3]*vec.y + values[6]*vec.z;
		ret.y = values[1]*vec.x + values[4]*vec.y + values[7]*vec.z;
		ret.z = values[2]*vec.x + values[5]*vec.y + values[8]*vec.z;
		return ret;
	}

private:
	BaseType values[3*3];
};

template<typename BaseType>
class Mat4x4
{
public:
	// Convention: first four values in arrow = first column
	Mat4x4(const BaseType values[])
	{
		for (unsigned int i = 0; i < 4*4; ++i)
		{
			this->values[i] = values[i];
		}
	}

	template<typename X, typename Y, typename Z, typename W>
	Vec4<X,Y,Z,W> operator * (const Vec4<X,Y,Z,W>& vec)
	{
		Vec4<X,Y,Z,W> ret;
		ret.x = values[0]*vec.x + values[4]*vec.y + values[8]*vec.z + values[12]*vec.w;
		ret.y = values[1]*vec.x + values[5]*vec.y + values[9]*vec.z + values[13]*vec.w;
		ret.z = values[2]*vec.x + values[6]*vec.y + values[10]*vec.z + values[14]*vec.w;
		ret.w = values[3]*vec.x + values[7]*vec.y + values[11]*vec.z + values[15]*vec.w;
		return ret;
	}

private:
	BaseType values[4*4];
};

WorldCoords TransformUnit::ModelToWorld(const ModelCoords& coords)
{
	Mat3x3<float> world_matrix(gstate.worldMatrix);
	return WorldCoords(world_matrix * coords) + Vec3<float>(gstate.worldMatrix[9], gstate.worldMatrix[10], gstate.worldMatrix[11]);
}

ViewCoords TransformUnit::WorldToView(const WorldCoords& coords)
{
	Mat3x3<float> view_matrix(gstate.viewMatrix);
	return ViewCoords(view_matrix * coords) + Vec3<float>(gstate.viewMatrix[9], gstate.viewMatrix[10], gstate.viewMatrix[11]);
}

ClipCoords TransformUnit::ViewToClip(const ViewCoords& coords)
{
	Vec4<float> coords4(coords.x, coords.y, coords.z, 1.0f);
	Mat4x4<float> projection_matrix(gstate.projMatrix);
	return ClipCoords(projection_matrix * coords4);
}

ScreenCoords TransformUnit::ClipToScreen(const ClipCoords& coords)
{
	ScreenCoords ret;
	float vpx1 = getFloat24(gstate.viewportx1);
	float vpx2 = getFloat24(gstate.viewportx2);
	float vpy1 = getFloat24(gstate.viewporty1);
	float vpy2 = getFloat24(gstate.viewporty2);
	float vpz1 = getFloat24(gstate.viewportz1);
	float vpz2 = getFloat24(gstate.viewportz2);
	// TODO: Check for invalid parameters (x2 < x1, etc)
	ret.x = (coords.x * vpx1 / coords.w + vpx2) / 4095.9375 * 0xFFFF;
	ret.y = (coords.y * vpy1 / coords.w + vpy2) / 4096.9375 * 0xFFFF;
	ret.z = (coords.z * vpz1 / coords.w + vpz2) / 4096.9375 * 0xFFFF;
	return ret;
}

DrawingCoords TransformUnit::ScreenToDrawing(const ScreenCoords& coords)
{
	DrawingCoords ret;
	// TODO: What to do when offset > coord?
	// TODO: Mask can be re-enabled now, I guess.
	ret.x = (((u32)coords.x - (gstate.offsetx&0xffff))/16) & 0x3ff;
	ret.y = (((u32)coords.y - (gstate.offsety&0xffff))/16) & 0x3ff;
	return ret;
}

void TransformUnit::SubmitPrimitive(void* vertices, u32 prim_type, int vertex_count, u32 vertex_type)
{
	// TODO: Cache VertexDecoder objects
	VertexDecoder vdecoder;
	vdecoder.SetVertexType(vertex_type);
	const DecVtxFormat& vtxfmt = vdecoder.GetDecVtxFmt();

	u8 buf[102400]; // yolo
	vdecoder.DecodeVerts(buf, vertices, 0, vertex_count - 1);

	VertexReader vreader(buf, vtxfmt, vertex_type);

	// We only support triangle lists, for now.
	for (int vtx = 0; vtx < vertex_count; ++vtx)
	{
		DrawingCoords dcoords[3];
		for (unsigned int i = 0; i < 3; ++i)
		{
			float pos[3];
			vreader.Goto(vtx+i);
			vreader.ReadPos(pos);

			ModelCoords mcoords(pos[0], pos[1], pos[2]);
			ClipCoords ccoords(ClipCoords(TransformUnit::ViewToClip(TransformUnit::WorldToView(TransformUnit::ModelToWorld(mcoords)))));

			// TODO: Split primitives in these cases!
			// TODO: Check if the equal case needs to be included, too
			if (ccoords.x < -ccoords.w || ccoords.x > ccoords.w) {
				ERROR_LOG(G3D, "X outside view volume!");
				goto skip;
			}
			if (ccoords.y < -ccoords.w || ccoords.y > ccoords.w) {
				ERROR_LOG(G3D, "Y outside view volume!");
				goto skip;
			}
			if (ccoords.z < -ccoords.w || ccoords.z > ccoords.w) {
				ERROR_LOG(G3D, "Z outside view volume!");
				goto skip;
			}
			dcoords[i] = DrawingCoords(TransformUnit::ScreenToDrawing(TransformUnit::ClipToScreen(ccoords)));
		}
		Rasterizer::DrawTriangle(dcoords);
skip:;
	}
}

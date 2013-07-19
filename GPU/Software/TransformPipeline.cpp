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
#include "Clipper.h"
#include "Lighting.h"

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
	ret.x = (coords.x * vpx1 / coords.w + vpx2) * 16; // 16 = 0xFFFF / 4095.9375;
	ret.y = (coords.y * vpy1 / coords.w + vpy2) * 16; // 16 = 0xFFFF / 4095.9375;
	ret.z = (coords.z * vpz1 / coords.w + vpz2) * 16; // 16 = 0xFFFF / 4095.9375;
	return ret;
}

DrawingCoords TransformUnit::ScreenToDrawing(const ScreenCoords& coords)
{
	DrawingCoords ret;
	// TODO: What to do when offset > coord?
	ret.x = (((u32)coords.x - (gstate.offsetx&0xffff))/16) & 0x3ff;
	ret.y = (((u32)coords.y - (gstate.offsety&0xffff))/16) & 0x3ff;
	ret.z = coords.z;
	return ret;
}

static VertexData ReadVertex(VertexReader& vreader)
{
	VertexData vertex;

	float pos[3];
	vreader.ReadPos(pos);

	if (!gstate.isModeClear() && gstate.textureMapEnable && vreader.hasUV()) {
		float uv[2];
		vreader.ReadUV(uv);
		vertex.texturecoords = Vec2<float>(uv[0], uv[1]);
	}

	if (vreader.hasNormal()) {
		float normal[3];
		vreader.ReadNrm(normal);
		vertex.normal = Vec3<float>(normal[0], normal[1], normal[2]);
	}

	if (vreader.hasColor0()) {
		float col[4];
		vreader.ReadColor0(col);
		vertex.color0 = Vec4<int>(col[0]*255, col[1]*255, col[2]*255, col[3]*255);
	} else {
		vertex.color0 = Vec4<int>(gstate.materialdiffuse&0xFF, (gstate.materialdiffuse>>8)&0xFF, (gstate.materialdiffuse>>16)&0xFF, gstate.materialalpha&0xFF);
	}

	if (vreader.hasColor1()) {
		float col[3];
		vreader.ReadColor0(col);
		vertex.color1 = Vec3<int>(col[0]*255, col[1]*255, col[2]*255);
	} else {
		vertex.color1 = Vec3<int>(0, 0, 0);
	}

	if (!gstate.isModeThrough()) {
		ModelCoords mcoords(pos[0], pos[1], pos[2]);
		vertex.worldpos = WorldCoords(TransformUnit::ModelToWorld(mcoords));
		vertex.clippos = ClipCoords(TransformUnit::ViewToClip(TransformUnit::WorldToView(vertex.worldpos)));
		vertex.drawpos = DrawingCoords(TransformUnit::ScreenToDrawing(TransformUnit::ClipToScreen(vertex.clippos)));

		if (vreader.hasNormal()) {
			vertex.worldnormal = TransformUnit::ModelToWorld(vertex.normal) - Vec3<float>(gstate.worldMatrix[9], gstate.worldMatrix[10], gstate.worldMatrix[11]);
			vertex.worldnormal /= vertex.worldnormal.Length(); // TODO: Shouldn't be necessary..
		}

		Lighting::Process(vertex);
	} else {
		vertex.drawpos.x = pos[0];
		vertex.drawpos.y = pos[1];
		vertex.drawpos.z = pos[2];
		vertex.clippos.w = 1.f;
	}

	return vertex;
}

void TransformUnit::SubmitPrimitive(void* vertices, void* indices, u32 prim_type, int vertex_count, u32 vertex_type)
{
	// TODO: Cache VertexDecoder objects
	VertexDecoder vdecoder;
	vdecoder.SetVertexType(vertex_type);
	const DecVtxFormat& vtxfmt = vdecoder.GetDecVtxFmt();

	u8 buf[65536 * 48]; // yolo
	u16 index_lower_bound = 0;
	u16 index_upper_bound = vertex_count - 1;
	bool indices_8bit = (vertex_type & GE_VTYPE_IDX_MASK) == GE_VTYPE_IDX_8BIT;
	bool indices_16bit = (vertex_type & GE_VTYPE_IDX_MASK) == GE_VTYPE_IDX_16BIT;
	u8* indices8 = (u8*)indices;
	u16* indices16 = (u16*)indices;
	if (indices)
		GetIndexBounds(indices, vertex_count, vertex_type, &index_lower_bound, &index_upper_bound);
	vdecoder.DecodeVerts(buf, vertices, index_lower_bound, index_upper_bound);

	VertexReader vreader(buf, vtxfmt, vertex_type);

	int vtcs_per_prim = 0;
	if (prim_type == GE_PRIM_POINTS) vtcs_per_prim = 1;
	else if (prim_type == GE_PRIM_LINES) vtcs_per_prim = 2;
	else if (prim_type == GE_PRIM_TRIANGLES) vtcs_per_prim = 3;
	else if (prim_type == GE_PRIM_RECTANGLES) vtcs_per_prim = 2;
	else {
		// TODO: Unsupported
	}

	if (prim_type == GE_PRIM_POINTS || prim_type == GE_PRIM_LINES || prim_type == GE_PRIM_TRIANGLES || prim_type == GE_PRIM_RECTANGLES) {
		for (int vtx = 0; vtx < vertex_count; vtx += vtcs_per_prim) {
			VertexData data[vtcs_per_prim];

			for (unsigned int i = 0; i < vtcs_per_prim; ++i) {
				if (indices)
					vreader.Goto(indices_16bit ? indices16[vtx+i] : indices8[vtx+i]);
				else
					vreader.Goto(vtx+i);

				data[i] = ReadVertex(vreader);
			}


			switch (prim_type) {
			case GE_PRIM_TRIANGLES:
				Clipper::ProcessTriangle(data);
				break;

			case GE_PRIM_RECTANGLES:
				Clipper::ProcessQuad(data);
				break;
			}
		}
	} else if (prim_type == GE_PRIM_TRIANGLE_STRIP) {
		VertexData data[3];

		for (int vtx = 0; vtx < vertex_count; ++vtx) {
			if (indices)
				vreader.Goto(indices_16bit ? indices16[vtx] : indices8[vtx]);
			else
				vreader.Goto(vtx);

			data[vtx % 3] = ReadVertex(vreader);

			if (vtx < 2)
				continue;

			// TODO: Should make sure to draw the vertices in the correct order!
			Clipper::ProcessTriangle(data);
		}
	}
}

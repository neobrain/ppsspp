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

#include "TransformPipeline.h"
#include "../GPUState.h"

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

	template<typename T>
	Vec3<T> operator * (const Vec3<T>& vec)
	{
		Vec3<T> ret;
		ret.x = values[0]*vec.x + values[1]*vec.y + values[2]*vec.z;
		ret.y = values[3]*vec.x + values[4]*vec.y + values[5]*vec.z;
		ret.z = values[6]*vec.x + values[7]*vec.y + values[8]*vec.z;
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

	template<typename T>
	Vec4<T> operator * (const Vec4<T>& vec)
	{
		Vec4<T> ret;
		ret.x = values[0]*vec.x + values[1]*vec.y + values[2]*vec.z + values[3]*vec.w;
		ret.y = values[4]*vec.x + values[5]*vec.y + values[6]*vec.z + values[7]*vec.w;
		ret.z = values[8]*vec.x + values[9]*vec.y + values[10]*vec.z + values[11]*vec.w;
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
	float viewport_dx = gstate.viewportx2 - gstate.viewportx1; // TODO: -1?
	float viewport_dy = gstate.viewporty2 - gstate.viewporty1; // TODO: -1?
	float viewport_dz = gstate.viewportz2 - gstate.viewportz1; // TODO: -1?
	// TODO: Check for invalid parameters (x2 < x1, etc)

	ret.x = (coords.x * viewport_dx / coords.w + gstate.viewportx1) * 0xFFFF;
	ret.y = (coords.y * viewport_dy / coords.w + gstate.viewporty1) * 0xFFFF;
	ret.z = (coords.z * viewport_dz / coords.w + gstate.viewportz1) * 0xFFFF;
	return ret;
}

DrawingCoords TransformUnit::ScreenToDrawing(const ScreenCoords& coords)
{
	DrawingCoords ret;
	ret.x = (coords.x - gstate.offsetx) & 0x3ff;
	ret.y = (coords.y - gstate.offsety) & 0x3ff;
	return ret;
}

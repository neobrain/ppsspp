// Copyright (c) 2012- PPSSPP Project.

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

#pragma once

#include <cmath>
#include <type_traits>

/**
 * Vec2 - two dimensional vector with arbitrary base type
 */
template<typename X, typename Y=X>
class Vec2
{
public:
	struct
	{
		X x;
		Y y;
	};

	// aliases
	struct
	{
		X& u;
		Y& v;
	};
	struct
	{
		X& s;
		Y& t;
	};

	X* AsArray() { return &x; }

	Vec2() : u(x), v(y), s(x), t(y) {}
	Vec2(const X& _x, const Y& _y) : x(_x), y(_y), u(x), v(y), s(x), t(y)  {}
	Vec2(const X a[2]) : x(a[0]), y(a[1]), u(x), v(y), s(x), t(y) {}
	Vec2(const Vec2& other) : x(other.x), y(other.y), u(x), v(y), s(x), t(y) {}

	static Vec2 AssignToAll(X f)
	{
		return Vec2(f, f, f);
	}

	template<typename X2, typename Y2>
	Vec2<X2,Y2> Cast() const
	{
		return Vec2<X2,Y2>((X2)x, (Y2)y);
	}

	void Write(X a[2])
	{
		a[0] = x; a[1] = y;
	}

	// operators acting on "this"
	Vec2& operator = (const Vec2& other)
	{
		if (this == &other)
			return *this;

		x = other.x;
		y = other.y;

		return *this;
	}
	void operator += (const Vec2& other)
	{
		x+=other.x; y+=other.y;
	}
	void operator -= (const Vec2& other)
	{
		x-=other.x; y-=other.y;
	}

	void operator *= (const X& f)
	{
		x*=f; y*=f;
	}
	void operator /= (const X& f)
	{
		x/=f; y/=f;
	}

	// operators which require creating a new Vec2 object
	Vec2 operator +(const Vec2& other) const;
	Vec2 operator -(const Vec2& other) const;
	Vec2 operator -() const;
	Vec2 operator *(const Vec2& other) const;
	Vec2 operator * (const X& f) const;
	Vec2 operator / (const X& f) const;

	// methods which don't create new Vec2 objects
	// Length, SetLength and Normalize are only implemented for X=Y=Z=float
	X Length2() const;
	float Length() const;
	void SetLength(const float l);
	float Normalize(); // returns the previous length, is often useful
	X &operator [] (int i); // allow vector[2] = 3   (vector.z=3)
	X operator [] (const int i) const;
	void SetZero()
	{
		x=(X)0; y=(Y)0;
	}

	// methods that create a new Vec2 object
	Vec2 WithLength(const X& l) const;
	Vec2 Normalized() const;
	float Distance2To(const Vec2 &other) const;

	// swizzlers - create a subvector of specific components
	const Vec2<Y,X> yx() const { return Vec2<Y,X>(y, x); }
	const Vec2<Y,X> vu() const { return Vec2<Y,X>(v, u); }
	const Vec2<Y,X> ts() const { return Vec2<Y,X>(t, s); }
};

typedef Vec2<float> Vec2f;

template<typename X, typename Y>
Vec2<X,Y> Vec2<X,Y>::operator +(const Vec2<X,Y>& other) const
{
	return Vec2<X,Y>(x+other.x, y+other.y);
}

template<typename X, typename Y>
Vec2<X,Y> Vec2<X,Y>::operator -(const Vec2<X,Y>& other) const
{
	return Vec2<X,Y>(x-other.x, y-other.y);
}

template<typename X, typename Y>
Vec2<X,Y> Vec2<X,Y>::operator -() const
{
	return Vec2<X,Y>(-x,-y);
}

template<typename X, typename Y>
Vec2<X,Y> Vec2<X,Y>::operator *(const Vec2<X,Y>& other) const
{
	return Vec2<X,Y>(x*other.x, y*other.y);
}

template<typename X, typename Y>
Vec2<X,Y> Vec2<X,Y>::operator * (const X& f) const
{
	return Vec2<X,Y>(x*f,y*f);
}

template<typename X, typename Y>
Vec2<X,Y> operator * (const X& f, const Vec2<X,Y>& vec)
{
	return Vec2<X,Y>(f*vec.x, f*vec.y);
}

template<typename X, typename Y>
Vec2<X,Y> Vec2<X,Y>::operator / (const X& f) const
{
	float invf = (1.0f/f);
	return Vec2<X,Y>(x*invf,y*invf);
}

template<typename X, typename Y>
Vec2<X,Y> Vec2<X,Y>::WithLength(const X& l) const
{
	return (*this) * l / Length();
}

template<typename X, typename Y>
Vec2<X,Y> Vec2<X,Y>::Normalized() const
{
	return (*this) / Length();
}

// Template specializations below
// Many functions only really make sense for floating point vectors and/or when the base types for each component are equal.
template<typename X, typename Y>
X Vec2<X,Y>::Length2() const
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	return x*x + y*y;
}

template<typename X, typename Y>
X& Vec2<X,Y>::operator [] (int i) //allow vector[1] = 3   (vector.y=3)
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	return (i==0) ? x : y;
}

template<typename X, typename Y>
X Vec2<X,Y>::operator [] (const int i) const
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	return (i==0) ? x : y;
}

template<typename X, typename Y>
float Vec2<X,Y>::Distance2To(const Vec2<X,Y>& other) const
{
	return (other-(*this)).Length2();
}

/**
 * Vec3 - three dimensional vector with arbitrary base type
 */
template<typename X, typename Y=X, typename Z=X>
class Vec3
{
public:
	struct
	{
		X x;
		Y y;
		Z z;
	};

	// aliases
	struct
	{
		X& r;
		Y& g;
		Z& b;
	};
	struct
	{
		X& u;
		Y& v;
		Z& w;
	};

	X* AsArray() { return &x; }

	Vec3() : r(x), g(y), b(z), u(x), v(y), w(z) {}
	Vec3(const X& _x, const Y& _y, const Z& _z) : x(_x), y(_y), z(_z), r(x), g(y), b(z), u(x), v(y), w(z) {}
	Vec3(const X a[3]) : x(a[0]), y(a[1]), z(a[2]), r(x), g(y), b(z), u(x), v(y), w(z) {}
	Vec3(const Vec3& other) : x(other.x), y(other.y), z(other.z), r(x), g(y), b(z), u(x), v(y), w(z) {}

	// Only defined for X=float and X=int
	static Vec3 FromRGB(unsigned int rgb);

	static Vec3 AssignToAll(X f)
	{
		return Vec3(f, f, f);
	}

	template<typename X2, typename Y2, typename Z2>
	Vec3<X2,Y2,Z2> Cast() const
	{
		return Vec3<X2,Y2,Z2>((X2)x, (Y2)y, (Z2)z);
	}

	void Write(X a[3])
	{
		a[0] = x; a[1] = y; a[2] = z;
	}

	// Only defined for X=float and X=int; alpha bits set to zero
	unsigned int ToRGB() const;

	// operators acting on "this"
	Vec3& operator = (const Vec3& other)
	{
		if (this == &other)
			return *this;

		x = other.x;
		y = other.y;
		z = other.z;

		return *this;
	}
	void operator += (const Vec3& other)
	{
		x+=other.x; y+=other.y; z+=other.z;
	}
	void operator -= (const Vec3& other)
	{
		x-=other.x; y-=other.y; z-=other.z;
	}

	void operator *= (const X& f)
	{
		x*=f; y*=f; z*=f;
	}
	void operator /= (const X& f)
	{
		x/=f; y/=f; z/=f;
	}

	// operators which require creating a new Vec3 object
	Vec3 operator +(const Vec3 &other) const;
	Vec3 operator -(const Vec3 &other) const;
	Vec3 operator -() const;
	Vec3 operator *(const Vec3 &other) const;
	Vec3 operator * (const X& f) const;
	Vec3 operator / (const X& f) const;

	// methods which don't create new Vec3 objects
	// Length, SetLength and Normalize are only implemented for X=Y=Z=float
	X Length2() const;
	float Length() const;
	void SetLength(const float l);
	float Normalize(); // returns the previous length, is often useful
	X &operator [] (int i); // allow vector[2] = 3   (vector.z=3)
	X operator [] (const int i) const;
	void SetZero()
	{
		x=(X)0; y=(Y)0; z=(Z)0;
	}

	// methods that create a new Vec3 object
	Vec3 WithLength(const X& l) const;
	Vec3 Normalized() const;
	float Distance2To(const Vec3 &other) const;

	// swizzlers - create a subvector of specific components
	// e.g. Ref<X,Y> xy() { return Vec2<X,Y>(x,y); }
	// _DEFINE_SWIZZLER2 defines a single such function
	// DEFINE_SWIZZLER2 defines all of them for all component names (x<->r) and permutations (xy<->yx)

	#define _DEFINE_SWIZZLER2(a, b, A, B) const Vec2<A,B> a##b() const { return Vec2<A,B>(a,b); }
	#define DEFINE_SWIZZLER2(a, b, a2, b2, a3, b3, A, B) _DEFINE_SWIZZLER2(a, b, A, B); _DEFINE_SWIZZLER2(a2, b2, A, B); _DEFINE_SWIZZLER2(a3, b3, A, B); _DEFINE_SWIZZLER2(b, a, B, A); _DEFINE_SWIZZLER2(b2, a2, B, A); _DEFINE_SWIZZLER2(b3, a3, B, A);
	DEFINE_SWIZZLER2(x, y, r, g, u, v, X, Y);
	DEFINE_SWIZZLER2(x, z, r, b, u, w, X, Z);
	DEFINE_SWIZZLER2(y, z, g, b, v, w, Y, Z);
	#undef DEFINE_SWIZZLER2
	#undef _DEFINE_SWIZZLER2
};

typedef Vec3<float> Vec3f;

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> Vec3<X,Y,Z>::operator +(const Vec3<X,Y,Z> &other) const
{
	return Vec3<X,Y,Z>(x+other.x, y+other.y, z+other.z);
}

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> Vec3<X,Y,Z>::operator -(const Vec3<X,Y,Z> &other) const
{
	return Vec3<X,Y,Z>(x-other.x, y-other.y, z-other.z);
}

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> Vec3<X,Y,Z>::operator -() const
{
	return Vec3<X,Y,Z>(-x,-y,-z);
}

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> Vec3<X,Y,Z>::operator *(const Vec3<X,Y,Z> &other) const
{
	return Vec3<X,Y,Z>(x*other.x, y*other.y, z*other.z);
}

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> Vec3<X,Y,Z>::operator * (const X& f) const
{
	return Vec3<X,Y,Z>(x*f,y*f,z*f);
}

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> operator * (const X& f, const Vec3<X,Y,Z>& vec)
{
	return Vec3<X,Y,Z>(f*vec.x, f*vec.y, f*vec.z);
}

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> Vec3<X,Y,Z>::operator / (const X& f) const
{
	float invf = (1.0f/f);
	return Vec3<X,Y,Z>(x*invf,y*invf,z*invf);
}

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> Vec3<X,Y,Z>::WithLength(const X& l) const
{
	return (*this) * l / Length();
}

template<typename X, typename Y, typename Z>
Vec3<X,Y,Z> Vec3<X,Y,Z>::Normalized() const
{
	return (*this) / Length();
}

// Template specializations below
// Many functions only really make sense for floating point vectors and/or when the base types for each component are equal.
template<typename X, typename Y, typename Z>
X Vec3<X,Y,Z>::Length2() const
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	static_assert(std::is_same<X,Z>::value, "base types need to be equal");
	return x*x + y*y + z*z;
}

template<typename X, typename Y, typename Z>
X& Vec3<X,Y,Z>::operator [] (int i) //allow vector[1] = 3   (vector.y=3)
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	static_assert(std::is_same<X,Z>::value, "base types need to be equal");
	return (i==0) ? x : (i==1) ? y : z;
}

template<typename X, typename Y, typename Z>
X Vec3<X,Y,Z>::operator [] (const int i) const
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	static_assert(std::is_same<X,Z>::value, "base types need to be equal");
	return (i==0) ? x : (i==1) ? y : z;
}

template<typename X, typename Y, typename Z>
float Vec3<X,Y,Z>::Distance2To(const Vec3<X,Y,Z>& other) const
{
	return (other-(*this)).Length2();
}

/**
 * Vec4 - four dimensional vector with arbitrary base type
 */
template<typename X, typename Y=X, typename Z=X, typename W=X>
class Vec4
{
public:
	struct
	{
		X x;
		Y y;
		Z z;
		W w;
	};

	// aliases
	struct
	{
		X& r;
		Y& g;
		Z& b;
		W& a;
	};

	X* AsArray() { return &x; }

	Vec4() : r(x), g(y), b(z), a(w) {}
	Vec4(const X& _x, const Y& _y, const Z& _z, const W& _w) : x(_x), y(_y), z(_z), w(_w), r(x), g(y), b(z), a(w) {}
	Vec4(const X a[4]) : x(a[0]), y(a[1]), z(a[2]), w(a[3]), r(x), g(y), b(z), a(w) {}
	Vec4(const Vec4& other) : x(other.x), y(other.y), z(other.z), w(other.w), r(x), g(y), b(z), a(w) {}

	// Only defined for X=float and X=int
	static Vec4 FromRGBA(unsigned int rgba);

	static Vec4 AssignToAll(X f)
	{
		return Vec4(f, f, f, f);
	}

	template<typename X2, typename Y2, typename Z2, typename W2>
	Vec4<X2,Y2,Z2,W2> Cast() const
	{
		return Vec4<X2,Y2,Z2,W2>((X2)x, (Y2)y, (Z2)z, (W2)w);
	}

	void Write(X a[4])
	{
		a[0] = x; a[1] = y; a[2] = z; a[3] = w;
	}

	// Only defined for X=float and X=int
	unsigned int ToRGBA() const;

	// operators acting on "this"
	Vec4& operator = (const Vec4& other)
	{
		if (this == &other)
			return *this;

		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;

		return *this;
	}
	void operator += (const Vec4 &other)
	{
		x+=other.x; y+=other.y; z+=other.z; w+=other.w;
	}
	void operator -= (const Vec4 &other)
	{
		x-=other.x; y-=other.y; z-=other.z; w-=other.w;
	}

	void operator *= (const X& f)
	{
		x*=f; y*=f; z*=f; w*=f;
	}
	void operator /= (const X& f)
	{
		x/=f; y/=f; z/=f; w/=f;
	}

	// operators which require creating a new Vec4 object
	Vec4 operator +(const Vec4 &other) const;
	Vec4 operator -(const Vec4 &other) const;
	Vec4 operator -() const;
	Vec4 operator *(const Vec4 &other) const;
	Vec4 operator * (const X& f) const;
	Vec4 operator / (const X& f) const;

	// methods which don't create new Vec4 objects
	// Length, SetLength and Normalize are only implemented for X=Y=Z=float
	X Length2() const;
	float Length() const;
	void SetLength(const float l);
	float Normalize(); //returns the previous length, is often useful
	X &operator [] (int i); // allow vector[2] = 3   (vector.z=3)
	X operator [] (const int i) const;
	void SetZero()
	{
		x=(X)0; y=(Y)0; z=(Z)0; w=(W)0;
	}

	// methods that create a new Vec4 object
	Vec4 WithLength(const X& l) const;
	Vec4 Normalized() const;
	float Distance2To(const Vec4 &other) const;

	// swizzlers - create a subvector of specific components
	// e.g. Vec2<X,Y> xy() { return Vec2<X,Y>(x,y); }
	// _DEFINE_SWIZZLER2 defines a single such function
	// DEFINE_SWIZZLER2 defines all of them for all component names (x<->r) and permutations (xy<->yx)

	#define _DEFINE_SWIZZLER2(a, b, A, B) const Vec2<A,B> a##b() const { return Vec2<A,B>(a,b); }
	#define DEFINE_SWIZZLER2(a, b, a2, b2, A, B) _DEFINE_SWIZZLER2(a, b, A, B); _DEFINE_SWIZZLER2(a2, b2, A, B); _DEFINE_SWIZZLER2(b, a, B, A); _DEFINE_SWIZZLER2(b2, a2, B, A);
	DEFINE_SWIZZLER2(x, y, r, g, X, Y);
	DEFINE_SWIZZLER2(x, z, r, b, X, Z);
	DEFINE_SWIZZLER2(x, w, r, a, X, W);
	DEFINE_SWIZZLER2(y, z, g, b, Y, Z);
	DEFINE_SWIZZLER2(y, w, g, a, Y, W);
	DEFINE_SWIZZLER2(z, w, b, a, Z, W);
	#undef DEFINE_SWIZZLER2
	#undef _DEFINE_SWIZZLER2

	#define _DEFINE_SWIZZLER3(a, b, c, A, B, C) const Vec3<A,B,C> a##b##c() const { return Vec3<A,B,C>(a,b,c); }
	#define DEFINE_SWIZZLER3(a, b, c, a2, b2, c2, A, B, C) \
	_DEFINE_SWIZZLER3(a, b, c, A, B, C); \
	_DEFINE_SWIZZLER3(a, c, b, A, C, B); \
	_DEFINE_SWIZZLER3(b, a, c, B, A, C); \
	_DEFINE_SWIZZLER3(b, c, a, B, C, A); \
	_DEFINE_SWIZZLER3(c, a, b, C, A, B); \
	_DEFINE_SWIZZLER3(c, b, a, C, B, A); \
	_DEFINE_SWIZZLER3(a2, b2, c2, A, B, C); \
	_DEFINE_SWIZZLER3(a2, c2, b2, A, C, B); \
	_DEFINE_SWIZZLER3(b2, a2, c2, B, A, C); \
	_DEFINE_SWIZZLER3(b2, c2, a2, B, C, A); \
	_DEFINE_SWIZZLER3(c2, a2, b2, C, A, B); \
	_DEFINE_SWIZZLER3(c2, b2, a2, C, B, A);

	DEFINE_SWIZZLER3(x, y, z, r, g, b, X, Y, Z);
	DEFINE_SWIZZLER3(x, y, w, r, g, a, X, Y, W);
	DEFINE_SWIZZLER3(x, z, w, r, b, a, X, Z, W);
	DEFINE_SWIZZLER3(y, z, w, g, b, a, Y, Z, W);
	#undef DEFINE_SWIZZLER3
	#undef _DEFINE_SWIZZLER3
};

typedef Vec4<float> Vec4f;

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> Vec4<X,Y,Z,W>::operator +(const Vec4<X,Y,Z,W> &other) const
{
	return Vec4<X,Y,Z,W>(x+other.x, y+other.y, z+other.z, w+other.w);
}

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> Vec4<X,Y,Z,W>::operator -(const Vec4<X,Y,Z,W> &other) const
{
	return Vec4<X,Y,Z,W>(x-other.x, y-other.y, z-other.z, w-other.w);
}

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> Vec4<X,Y,Z,W>::operator -() const
{
	return Vec4<X,Y,Z,W>(-x,-y,-z,-w);
}

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> Vec4<X,Y,Z,W>::operator *(const Vec4<X,Y,Z,W> &other) const
{
	return Vec4<X,Y,Z,W>(x*other.x, y*other.y, z*other.z, w*other.w);
}

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> Vec4<X,Y,Z,W>::operator * (const X& f) const
{
	return Vec4<X,Y,Z,W>(x*f,y*f,z*f,w*f);
}

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> operator * (const X& f, const Vec4<X,Y,Z,W>& vec)
{
	return Vec4<X,Y,Z,W>(f*vec.x, f*vec.y, f*vec.z, f*vec.w);
}

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> Vec4<X,Y,Z,W>::operator / (const X& f) const
{
	float invf = (1.0f/f);
	return Vec4<X,Y,Z,W>(x*invf,y*invf,z*invf,w*invf);
}

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> Vec4<X,Y,Z,W>::WithLength(const X& l) const
{
	return (*this) * l / Length();
}

template<typename X, typename Y, typename Z, typename W>
Vec4<X,Y,Z,W> Vec4<X,Y,Z,W>::Normalized() const
{
	return (*this) / Length();
}

// Template specializations below
// Many functions only really make sense for floating point vectors and/or when the base types for each component are equal.
template<typename X, typename Y, typename Z, typename W>
X Vec4<X,Y,Z,W>::Length2() const
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	static_assert(std::is_same<X,Z>::value, "base types need to be equal");
	static_assert(std::is_same<X,W>::value, "base types need to be equal");
	return x*x + y*y + z*z + w*w;
}

template<typename X, typename Y, typename Z, typename W>
X& Vec4<X,Y,Z,W>::operator [] (int i) //allow vector[1] = 3   (vector.y=3)
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	static_assert(std::is_same<X,Z>::value, "base types need to be equal");
	static_assert(std::is_same<X,W>::value, "base types need to be equal");
	return (i==0) ? x : (i==1) ? y : (i==2) ? z : w;
}

template<typename X, typename Y, typename Z, typename W>
X Vec4<X,Y,Z,W>::operator [] (const int i) const
{
	static_assert(std::is_same<X,Y>::value, "base types need to be equal");
	static_assert(std::is_same<X,Z>::value, "base types need to be equal");
	static_assert(std::is_same<X,W>::value, "base types need to be equal");
	return (i==0) ? x : (i==1) ? y : (i==2) ? z : w;
}

template<typename X, typename Y, typename Z, typename W>
float Vec4<X,Y,Z,W>::Distance2To(const Vec4<X,Y,Z,W>& other) const
{
	return (other-(*this)).Length2();
}


template<typename BaseType>
class Mat3x3
{
public:
	// Convention: first three values = first column
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

	Mat3x3 Inverse()
	{
		float a = values[0];
		float b = values[1];
		float c = values[2];
		float d = values[3];
		float e = values[4];
		float f = values[5];
		float g = values[6];
		float h = values[7];
		float i = values[8];
		return Mat3x3(e*i-f*h, f*g-d*i, d*h-e*g,
						c*h-b*i, a*i-c*g, b*g-a*h,
						b*f-c*e, c*d-a*f, a*e-b*d) / Det();
	}

	BaseType Det()
	{
		return values[0]*values[4]*values[8] + values[3]*values[7]*values[2] +
				values[6]*values[1]*values[5] - values[2]*values[4]*values[6] -
				values[5]*values[7]*values[0] - values[8]*values[1]*values[3];
	}

	Mat3x3 operator / (const BaseType& val) const
	{
		return Mat3x3(values[0]/val, values[1]/val, values[2]/val,
						values[3]/val, values[4]/val, values[5]/val,
						values[6]/val, values[7]/val, values[8]/val);
	}

private:
	BaseType values[3*3];
};


inline void Vec3ByMatrix43(float vecOut[3], const float v[3], const float m[12])
{
	vecOut[0] = v[0] * m[0] + v[1] * m[3] + v[2] * m[6] + m[9];
	vecOut[1] = v[0] * m[1] + v[1] * m[4] + v[2] * m[7] + m[10];
	vecOut[2] = v[0] * m[2] + v[1] * m[5] + v[2] * m[8] + m[11];
}

inline void Norm3ByMatrix43(float vecOut[3], const float v[3], const float m[12])
{
	vecOut[0] = v[0] * m[0] + v[1] * m[3] + v[2] * m[6];
	vecOut[1] = v[0] * m[1] + v[1] * m[4] + v[2] * m[7];
	vecOut[2] = v[0] * m[2] + v[1] * m[5] + v[2] * m[8];
}

inline float Vec3Dot(const float v1[3], const float v2[3])
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

// Dot product only really makes sense for same types
template<typename X>
inline X Dot(const Vec2<X> &a, const Vec2<X>& b)
{
	return a.x*b.x + a.y*b.y;
}

template<typename X>
inline X Dot(const Vec3<X> &a, const Vec3<X>& b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

template<typename X>
inline X Dot(const Vec4<X> &a, const Vec4<X>& b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

template<typename X, typename Y, typename Z>
inline Vec3<X,Y,Z> Cross(const Vec3<X,Y,Z> &a, const Vec3<X,Y,Z>& b)
{
	return Vec3<X,Y,Z>(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x);
}


// linear interpolation via float: 0.0=begin, 1.0=end
template<typename X>
inline X Lerp(const X& begin, const X& end, const float t)
{
	return begin*(1.f-t) + end*t;
}

// linear interpolation via int: 0=begin, base=end
template<typename X, int base>
inline X LerpInt(const X& begin, const X& end, const int t)
{
	return (begin*(base-t) + end*t) / base;
}

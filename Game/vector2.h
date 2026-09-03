/*============================================================================
Contents   :  [vector2.h]

Author     : Chin Qing You
LastUpdate : 2026/07/27
-----------------------------------------------------------------------------

============================================================================*/
#ifndef VECTOR2_H
#define VECTOR2_H

#include <cmath>
#include <DirectXMath.h>  

struct Vector2
{
	float x = 0.0f;
	float y = 0.0f;

	// --- operators ---
	Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }
	Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }
	Vector2 operator*(float s)          const { return { x * s,   y * s }; }
	Vector2 operator/(float s)          const { return { x / s,   y / s }; }
	Vector2 operator-()                 const { return { -x, -y }; }


	Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
	Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
	Vector2& operator*=(float s) { x *= s;   y *= s;   return *this; }

	float LengthSq() const { return x * x + y * y; }
	float Length()   const { return sqrtf(LengthSq()); }
	bool  IsZero()   const { return x == 0.0f && y == 0.0f; }

	DirectX::XMFLOAT2 ToXMFLOAT2() const { return { x, y }; }
};

// scalar * vector 
inline Vector2 operator*(float s, const Vector2& v) { return v * s; }
// ============================================================================

inline Vector2 Vector2_Normalize(const Vector2& v)
{
	const float len_sq = v.LengthSq();
	if (len_sq <= 0.000001f)         
	{
		return { 0.0f, 0.0f };   
	}
	const float inv_len = 1.0f / sqrtf(len_sq);
	return { v.x * inv_len, v.y * inv_len };
}

inline float Vector2_Dot(const Vector2& a, const Vector2& b)
{
	return a.x * b.x + a.y * b.y;
}

inline float Vector2_DistanceSq(const Vector2& a, const Vector2& b)
{
	return (b - a).LengthSq();
}

inline float Vector2_Distance(const Vector2& a, const Vector2& b)
{
	return (b - a).Length();
}

// Angle (radians) -> unit direction
inline Vector2 Vector2_FromAngle(float radians)
{
	return { cosf(radians), sinf(radians) };
}

// Direction -> angle (radians)
inline float Vector2_ToAngle(const Vector2& v)
{
	return atan2f(v.y, v.x);
}

// Linear interpolation: t=0 -> a, t=1 -> b.
inline Vector2 Vector2_Lerp(const Vector2& a, const Vector2& b, float t)
{
	return a + (b - a) * t;
}

#endif
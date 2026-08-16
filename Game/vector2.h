/*============================================================================
Contents   :  [vector2.h]

Author     : Chin Qing You
LastUpdate : 2026/07/27
-----------------------------------------------------------------------------
Small 2D vector math for GAMEPLAY code (player, enemies, bullets, skills).
Plain scalar floats on purpose — no SIMD, no load/store ceremony.

The engine layer (sprite.cpp matrices) keeps using DirectXMath; that is
what it is good at. This header is for the "one enemy chases one player"
kind of math, where clarity beats micro-optimization.

Header-only: every function is small enough that the compiler will
inline it, so there is no .cpp file.
============================================================================*/
#ifndef VECTOR2_H
#define VECTOR2_H

#include <cmath>
#include <DirectXMath.h>  

struct Vector2
{
	float x = 0.0f;
	float y = 0.0f;

	// --- operators: make vectors feel like numbers ---
	Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }
	Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }
	Vector2 operator*(float s)          const { return { x * s,   y * s }; }
	Vector2 operator/(float s)          const { return { x / s,   y / s }; }
	Vector2 operator-()                 const { return { -x, -y }; }


	Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
	Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
	Vector2& operator*=(float s) { x *= s;   y *= s;   return *this; }

	// --- queries (const: they don't change the vector) ---
	float LengthSq() const { return x * x + y * y; }
	float Length()   const { return sqrtf(LengthSq()); }
	bool  IsZero()   const { return x == 0.0f && y == 0.0f; }

	// --- conversion at the borders (draw calls, collision struct) ---
	DirectX::XMFLOAT2 ToXMFLOAT2() const { return { x, y }; }
};

// scalar * vector (so both 2.0f * v and v * 2.0f compile)
inline Vector2 operator*(float s, const Vector2& v) { return v * s; }

// ============================================================================
// Free functions
//
// Normalize returns a NEW vector instead of modifying in place, so the
// original (often "target minus me") stays available for range checks.
// ============================================================================

// Direction with length 1. THE zero-vector guard lives here, in exactly
// one place, instead of being re-remembered at every call site.
inline Vector2 Vector2_Normalize(const Vector2& v)
{
	const float len_sq = v.LengthSq();
	if (len_sq <= 0.000001f)          // zero (or effectively zero) vector
	{
		return { 0.0f, 0.0f };        // "no direction" — caller decides meaning
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

// Angle (radians) -> unit direction. Replaces the cos/sin pair in the
// spawner's ring spawn, and later: radial skills, spread shots.
inline Vector2 Vector2_FromAngle(float radians)
{
	return { cosf(radians), sinf(radians) };
}

// Direction -> angle (radians). For sprite rotation (bullet facing).
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
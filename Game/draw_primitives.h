/*============================================================================
Contents   :  [draw_primitives.h]

Author     : Chin Qing You
LastUpdate : 2026/08/15
-----------------------------------------------------------------------------

============================================================================*/
#ifndef DRAW_PRIMITIVES_H
#define DRAW_PRIMITIVES_H

#include <DirectXMath.h>

#include "vector2.h"

void DrawPrim_Initialize();
void DrawPrim_Finalize();

void DrawPrim_Rect(const Vector2& center, float width, float height, float angle,
    const DirectX::XMFLOAT3& color, float alpha = 1.0f);

void DrawPrim_Line(const Vector2& from, const Vector2& to, float thickness,
    const DirectX::XMFLOAT3& color, float alpha, float overlap = 1.0f);

void DrawPrim_Beam(const Vector2& from, const Vector2& to, float thickness,
    const DirectX::XMFLOAT3& color, float alpha = 1.0f);

void DrawPrim_BeamGrow(const Vector2& from, const Vector2& to, float thickness,
    const DirectX::XMFLOAT3& color, float grow, float alpha = 1.0f);

void DrawPrim_Circle(const Vector2& center, float radius,
    const DirectX::XMFLOAT3& color, float alpha = 1.0f);

void DrawPrim_Trail(const Vector2* points, int count, float thickness,
    const DirectX::XMFLOAT3& color,
    float alpha_head = 1.0f, float alpha_tail = 0.0f);

void DrawPrim_Ring(const Vector2& center, float radius, float thickness,
    const DirectX::XMFLOAT3& color, float alpha = 1.0f);

#endif
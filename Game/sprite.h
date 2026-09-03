/*============================================================================
Contents   :  [sprite.h]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#ifndef SPRITE_H
#define SPRITE_H
#include <DirectXMath.h>

struct SpriteDrawParams
{
    DirectX::XMFLOAT3 color{ 1.0f, 1.0f, 1.0f }; // RGB tint  (1,1,1 = no tint)
    float             alpha{ 1.0f };             // Opacity   (1 = fully opaque, 0 = invisible)
    DirectX::XMFLOAT2 scale{ 1.0f, 1.0f };       // Size multiplier (1,1 = original size)
    float             angle{ 0.0f };             // Rotation in radians (0 = no rotation)
    bool              flip_x{ false };           // Mirror horizontally
    bool              flip_y{ false };           // Mirror vertically
};

enum SpriteFilter
{
    kSpriteFilter_Point, 
    kSpriteFilter_Linear,  
};

void Sprite_SetFilter(SpriteFilter filter);

void Sprite_Flush();

bool Sprite_Initialize();
void Sprite_Finalize();

// ----------------------------------------------------------------------------
// Sprite_Draw overloads
//
// All overloads accept an optional SpriteDrawParams as the last argument.
//
//  (1)  Sprite_Draw(id, x, y)
//         Draw at (x,y) at the texture's native size.
//
//  (2)  Sprite_Draw(id, x, y, w, h)
//         Draw at (x,y) stretched to (w, h).
//
//  (3)  Sprite_Draw(id, x, y, w, h, tx, ty, tw, th)
//         Draw a sub-region of the texture — use this for sprite sheets.
//
// Append a SpriteDrawParams to any of the above for tint / alpha / scale / rotation.
// ----------------------------------------------------------------------------

// (1) full texture
void Sprite_Draw(int texture_id, float x, float y);
void Sprite_Draw(int texture_id, float x, float y,
    const SpriteDrawParams& params);

// (2) sized
void Sprite_Draw(int texture_id, float x, float y,
    float width, float height);
void Sprite_Draw(int texture_id, float x, float y,
    float width, float height,
    const SpriteDrawParams& params);

// (3) sprite sheet
void Sprite_Draw(int texture_id, float x, float y,
    float width, float height,
    float texture_x, float texture_y,
    int texture_width, int texture_height);
void Sprite_Draw(int texture_id, float x, float y,
    float width, float height,
    float texture_x, float texture_y,
    int texture_width, int texture_height,
    const SpriteDrawParams& params);

#endif
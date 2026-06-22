/*============================================================================
Contents   :  [sprite.h]
              
Author     : Chin Qing You
LastUpdate : 2026/06/15
-----------------------------------------------------------------------------

============================================================================*/
#ifndef SPRITE_H
#define SPRITE_H

#include <d3d11.h>
#include <DirectXMath.h>


bool Sprite_Initialize();
void Sprite_Finalize();

enum SpriteFilter
{
    kSpriteFilter_Point,
    kSpriteFilter_Linear
};

void Sprite_SetFilter(SpriteFilter filter);

//好きなテクスチャを好きな座標に描画する
void Sprite_Draw(int texture_ID, float x, float y, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

//好きなテクスチャを好きな座標とサイズに描画する
void Sprite_Draw(int texture_ID, float x, float y, float width, float height, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

//好きなテクスチャを好きなサイズで切り取って好きな座標に描画する
void Sprite_Draw(int texture_ID, float x, float y, float texture_x, float texture_y, int texture_width, int texture_height, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

//好きなテクスチャを好きなサイズで切り取って好きな座標に好きなサイズに描画する
void Sprite_Draw(int texture_ID, float x, float y, float width, float height, float texture_x, float texture_y, int texture_width, int texture_height, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

void Sprite_Draw(
    int texture_ID,
    float x, float y,
    float width, float height,
    float texture_x, float texture_y,
    int texture_width, int texture_height,
    float angle,
    const DirectX::XMFLOAT2& scale = { 1.0f, 1.0f},
    const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f }
);



#endif

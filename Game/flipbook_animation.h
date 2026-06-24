/*============================================================================
Contents   :  [flipbook_animation.h]
              
Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#ifndef FLIPBOOK_ANIMATION_H
#define FLIPBOOK_ANIMATION_H

#include <DirectXMath.h>

void FlipBookAnimation_Initialize();
void FlipBookAnimation_Finalize();

int FlipBookAnimation_Create(int texture_id, int pattern_width, int pattern_height, int pattern_count_max, int pattern_column_count_max, float pattern_update_time);
void FlipBookAnimation_Update(float delta_time);
void FlipBookAnimation_Destroy(int animation_id);

void FlipBookAnimation_Draw(int animation_id, float x, float y);
void FlipBookAnimation_Draw(int animation_id, float x, float y, const DirectX::XMFLOAT4& color);

#endif

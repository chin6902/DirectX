/*============================================================================
Contents   :  [game_score.h]
              
Author     : Chin Qing You
LastUpdate : 2026/07/08
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_SCORE_H
#define GAME_SCORE_H

#include <DirectXMath.h>

void GameScore_Initialize(int digit); // 0埋めするかどうか・左詰めなど
void GameScore_Finalize();
void GameScore_SetScore(int score);
void GameScore_Update(float delta_time);
void GameScore_Draw(float x, float y, float scale, DirectX::XMFLOAT3 color = {0.0f, 0.0f, 0.0f});

#endif

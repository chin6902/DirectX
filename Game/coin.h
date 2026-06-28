/*============================================================================
Contents   :  [coin.h]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#ifndef COIN_H
#define COIN_H

void  Coin_Initialize();
void  Coin_Finalize();

bool  Coin_AllCollected();

float Coin_GetJumpVelocityForNearest(float runner_world_x, float lead_distance);

void  Coin_Update(float runner_world_cx, float runner_world_cy);

void  Coin_Draw(float camera_x);

#endif
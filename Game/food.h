/*============================================================================
Contents   :  [food.h]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#ifndef FOOD_H
#define FOOD_H

void Food_Initialize();
void Food_Finalize();

void Food_Spawn(float screen_y);

void Food_Update(float delta_time);

void Food_Draw();

bool Food_IsSpawned();
bool Food_HasHit();   

bool Food_CheckAndApplyHit(float runner_screen_cx, float runner_screen_cy);

#endif 
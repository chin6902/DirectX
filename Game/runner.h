/*============================================================================
Contents   :  [runner.h]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#ifndef RUNNER_H
#define RUNNER_H

void  Runner_Initialize();
void  Runner_Finalize();

void  Runner_TriggerJump(float velocity);

void  Runner_TriggerHit();

void  Runner_Update(float delta_time);
void  Runner_Draw(float camera_x);

bool  Runner_IsOnGround();  
bool  Runner_IsHit();       
bool  Runner_IsGone();      

float Runner_GetWorldX();
float Runner_GetY();
int   Runner_GetWidth();
int   Runner_GetHeight();

#endif
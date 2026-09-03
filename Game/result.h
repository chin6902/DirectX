/*============================================================================
Contents   :  [result.h]

Author     : Chin Qing You
LastUpdate : 2026/08/26
-----------------------------------------------------------------------------
============================================================================*/
#ifndef RESULT_H
#define RESULT_H

void Result_Initialize();
void Result_Finalize();
void Result_Update(float delta_time);
void Result_Draw();

void Result_SetOutcome(bool cleared, int level_reached, float run_time);

#endif
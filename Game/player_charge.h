/*============================================================================
Contents   :  [player_charge.h]
              
Author     : Chin Qing You
LastUpdate : 2026/08/05
-----------------------------------------------------------------------------

============================================================================*/
#ifndef PLAYER_CHARGE_H
#define PLAYER_CHARGE_H

#include "element.h"

static constexpr int CHARGE_SLOT_MAX = 3;

void PlayerCharge_Initialize();
void PlayerCharge_Finalize();
void PlayerCharge_Update(float delta_time);  
void PlayerCharge_Draw();                      // slot UI

bool PlayerCharge_IsChargeKeyHeld();      
bool PlayerCharge_TryRelease();               
int  PlayerCharge_GetSlotCount();

void PlayerCharge_UpdateAlways(float delta_time);

float PlayerCharge_GetUltimateFraction(); 
ElementType PlayerCharge_GetUltimateElement();

#endif

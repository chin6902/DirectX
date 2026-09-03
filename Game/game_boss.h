/*============================================================================
Contents   :  [game_boss.h]

Author     : Chin Qing You
LastUpdate : 2026/08/18
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_BOSS_H
#define GAME_BOSS_H

#include "vector2.h"
#include "game_enemy.h"   

inline constexpr int BOSS_MAX = 2;

void GameBoss_Initialize();
void GameBoss_Finalize();
void GameBoss_Update(float delta_time);
void GameBoss_Draw();     
void GameBoss_DrawUI();   

void GameBoss_Spawn(const Vector2& pos, int variant, bool summons);

// --- per instance ---
bool        GameBoss_IsActive(int index);
Vector2     GameBoss_GetPos(int index);
float       GameBoss_GetRadius(int index);
float       GameBoss_GetHPFraction(int index);
bool        GameBoss_HasShield(int index);
ElementType GameBoss_GetShieldElement(int index);

bool GameBoss_ApplyHit(int index, const HitInfo& hit);
void GameBoss_ApplyStatus(int index, StatusType type, float duration, float magnitude);

int  GameBoss_GetActiveCount();
bool GameBoss_AnyActive();

#endif
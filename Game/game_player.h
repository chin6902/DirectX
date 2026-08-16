/*============================================================================
Contents   :  [game_player.h]
              
Author     : Chin Qing You
LastUpdate : 2026/08/15
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

#include "vector2.h"
#include "collision.h"

void GamePlayer_Initialize(float startX, float startY);
void GamePlayer_Finalize();
void GamePlayer_Update(float delta_time);
void GamePlayer_Draw();

float   GamePlayer_GetPosX();
float   GamePlayer_GetPosY();
Vector2 GamePlayer_GetPos();
Vector2 GamePlayer_GetAimDir();
float   GamePlayer_GetSpeed();

CollisionCircle GamePlayer_GetCollisionCircle();

// --- health ---
struct PlayerHit
{
	int     damage = 1;
	float   knockback_speed = 0.0f;
	float   knockback_time = 0.0f;
	Vector2 direction = { 1.0f, 0.0f }; 
};

void GamePlayer_TakeHit(const PlayerHit& hit);
void GamePlayer_Heal(float amount);

int   GamePlayer_GetHP();
int   GamePlayer_GetMaxHP();
float GamePlayer_GetHPFraction();    
bool  GamePlayer_IsDead();
bool  GamePlayer_IsInvincible();

// --- shield ---
void GamePlayer_GrantShield(float duration, int charges);
int  GamePlayer_GetShieldCharges();
bool GamePlayer_IsShielded();

#endif
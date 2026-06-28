/*============================================================================
Contents   :  [game_player.h]
              
Author     : Chin Qing You
LastUpdate : 2026/06/24
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

void GamePlayer_Initialize(float startX, float startY);
void GamePlayer_Finalize();
void GamePlayer_Update(float delta_time);
//void GamePlayer_FixedUpdate();
void GamePlayer_Draw();

#endif

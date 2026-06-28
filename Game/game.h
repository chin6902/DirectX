/*============================================================================
Contents   :  [game.h]
              
Author     : Chin Qing You
LastUpdate : 2026/06/24
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_H
#define GAME_H

#include <Windows.h>

void Game_Initialize();
void Game_Finalize();
void Game_Update(float delta_time);
//void Game_FixedUpdate();
void Game_Draw();

#endif

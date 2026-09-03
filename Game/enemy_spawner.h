/*============================================================================
Contents   :  [enemy_spawner.h]

Author     : Chin Qing You
LastUpdate : 2026/08/17
-----------------------------------------------------------------------------

============================================================================*/
#ifndef ENEMY_SPAWNER_H
#define ENEMY_SPAWNER_H

void EnemySpawner_Initialize();
void EnemySpawner_Finalize();
void EnemySpawner_Update(float delta_time);

float EnemySpawner_GetElapsed();    
int   EnemySpawner_GetWaveNumber();   
int   EnemySpawner_GetWaveCount();
float EnemySpawner_GetWaveTimeout();

bool  EnemySpawner_IsBossWave();
bool  EnemySpawner_IsFinished();

#endif
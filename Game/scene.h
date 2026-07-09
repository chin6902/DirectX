/*============================================================================
Contents   :  [scene.h]
              
Author     : Chin Qing You
LastUpdate : 2026/07/09
-----------------------------------------------------------------------------

============================================================================*/
#ifndef SCENE_H
#define SCENE_H

void Scene_Initialize();
void Scene_Finalize();
void Scene_Update(float delta_time);
void Scene_Draw();

enum Scene
{
	SCENE_TITLE,
	SCENE_GAME,
	SCENE_RESULT
};

void Scene_SetNextScene(Scene next_scene);
void Scene_Change();

#endif

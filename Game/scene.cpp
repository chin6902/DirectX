/*============================================================================
Contents   :  [scene.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/26
-----------------------------------------------------------------------------

============================================================================*/
#include "scene.h"
#include "title.h"
#include "game.h"
#include "result.h"

static Scene g_currentScene = SCENE_GAME;
static Scene g_nextScene = g_currentScene;

void Scene_Initialize()
{
	switch (g_currentScene)
	{
	case SCENE_TITLE:  Title_Initialize();  break;
	case SCENE_GAME:   Game_Initialize();   break;
	case SCENE_RESULT: Result_Initialize(); break;
	}
}

void Scene_Finalize()
{
	switch (g_currentScene)
	{
	case SCENE_TITLE:  Title_Finalize();  break;
	case SCENE_GAME:   Game_Finalize();   break;
	case SCENE_RESULT: Result_Finalize(); break;
	}
}

void Scene_Update(float delta_time)
{
	switch (g_currentScene)
	{
	case SCENE_TITLE:  Title_Update(delta_time);  break;
	case SCENE_GAME:   Game_Update(delta_time);   break;
	case SCENE_RESULT: Result_Update(delta_time); break;
	}
}

void Scene_Draw()
{
	switch (g_currentScene)
	{
	case SCENE_TITLE:  Title_Draw();  break;
	case SCENE_GAME:   Game_Draw();   break;
	case SCENE_RESULT: Result_Draw(); break;
	}
}

void Scene_SetNextScene(Scene scene)
{
	g_nextScene = scene;
}

void Scene_Change()
{
	if (g_nextScene == g_currentScene) { return; }

	Scene_Finalize();
	g_currentScene = g_nextScene;
	Scene_Initialize();
}
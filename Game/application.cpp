/*============================================================================
Contents   :  [application.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/15
-----------------------------------------------------------------------------

============================================================================*/
#include "application.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"
#include "config.h"
#include "sprite.h"

// テクスチャ管理番号
static int g_TextureId_Bg = TEXTURE_INVALID_ID;
static int g_TextureId_Sky = TEXTURE_INVALID_ID;
static int g_TextureId_Moutain01 = TEXTURE_INVALID_ID;
static int g_TextureId_Moutain02 = TEXTURE_INVALID_ID;
static int g_TextureId_Coco = TEXTURE_INVALID_ID;
static int g_TextureId_Food = TEXTURE_INVALID_ID;

static float g_TextureScroll = 0.0f;
static float g_ParallexSkyScroll = 0.0f;
static float g_ParallexMoutain01Scroll = 0.0f;
static float g_ParallexMoutain02Scroll = 0.0f;

void DrawBackground();

bool Application_Initialize(HWND hWnd)
{
    if (!Direct3DInitialize(hWnd))
    {
        return false;
    }   

    // 各システムの初期化
    Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
    Sprite_Initialize();
    
    // テクスチャのロード
    g_TextureId_Bg = Texture_Load(L"Background.png", false);
	g_TextureId_Food = Texture_Load(L"food.png", false);
    g_TextureId_Coco = Texture_Load(L"coco.png", false);
    g_TextureId_Sky = Texture_Load(L"Sky.png", false);
    g_TextureId_Moutain01 = Texture_Load(L"Hills_1.png", false);
    g_TextureId_Moutain02 = Texture_Load(L"Hills_2.png", false);
	return true;
}

void Application_Finalize()
{
	// 各システムの終了処理
	Sprite_Finalize();
	Texture_Finalize();
	Shader_Finalize();

	Direct3DFinalize();
}

void Application_Update()
{
	// ゲームの更新処理 (例: 入力処理、ゲームロジックの更新など)
	g_TextureScroll += 0.2f;
	g_ParallexSkyScroll += 0.0f;
	g_ParallexMoutain01Scroll += 0.15f;
	g_ParallexMoutain02Scroll += 0.2f;
}

void Application_Draw()
{
    DrawBackground();

    //Sprite_Draw(g_TextureId_Food, 0.0f, 0.0f);

    Sprite_Draw(g_TextureId_Coco, 200.0f, 600.0f, 128.0f, 128.0f, 32 * 2, 32 * 0, 32, 32, 0, { 2.0f, 2.0f }, {1.0f, 0.0f, 0.0f, 1.0f});
}

void DrawBackground()
{
    Sprite_Draw(g_TextureId_Bg, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, g_TextureScroll, 0.0f, Texture_GetWidth(g_TextureId_Bg), Texture_GetHeight(g_TextureId_Bg));
/*
    Sprite_Draw(g_TextureId_Sky, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, g_ParallexSkyScroll, 0.0f, Texture_GetWidth(g_TextureId_Sky), Texture_GetHeight(g_TextureId_Sky));

    Sprite_Draw(g_TextureId_Moutain01, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, g_ParallexMoutain01Scroll, 0.0f, Texture_GetWidth(g_TextureId_Moutain01), Texture_GetHeight(g_TextureId_Moutain01));

    Sprite_Draw(g_TextureId_Moutain02, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, g_ParallexMoutain02Scroll, 0.0f, Texture_GetWidth(g_TextureId_Moutain02), Texture_GetHeight(g_TextureId_Moutain02));
*/
}

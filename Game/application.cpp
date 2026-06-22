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
static int g_TextureId_Coin = TEXTURE_INVALID_ID;

static float g_TextureScroll = 0.0f;
static float g_ParallexSkyScroll = 0.0f;
static float g_ParallexMoutain01Scroll = 0.0f;
static float g_ParallexMoutain02Scroll = 0.0f;

static int g_patternCount = 0;

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
    g_TextureId_Bg = Texture_Load(L"assets/textures/Background.png", false);
	g_TextureId_Food = Texture_Load(L"assets/textures/food.png", false);
    g_TextureId_Coco = Texture_Load(L"assets/textures/coco.png", false);
    g_TextureId_Sky = Texture_Load(L"assets/textures/Sky.png", false);
    g_TextureId_Moutain01 = Texture_Load(L"assets/textures/Hills_1.png", false);
    g_TextureId_Moutain02 = Texture_Load(L"assets/textures/Hills_2.png", false);
    g_TextureId_Coin = Texture_Load(L"assets/textures/coin_anim.png", false);
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

void Application_Update(float delta_time)
{
	// ゲームの更新処理 (例: 入力処理、ゲームロジックの更新など)
	g_TextureScroll += 50.0f * delta_time;
	g_ParallexSkyScroll += 0.0f;
	g_ParallexMoutain01Scroll += 0.15f;
	g_ParallexMoutain02Scroll += 0.2f;
}

void Application_FixedUpdate()
{
    g_patternCount += 1;
}

void Application_Draw()
{
    Sprite_SetFilter(kSpriteFilter_Linear);

    DrawBackground();

    //Sprite_Draw(g_TextureId_Food, 0.0f, 0.0f);

    Sprite_SetFilter(kSpriteFilter_Point);

    int current_pattern = g_patternCount / 8 % 13;
    Sprite_Draw(g_TextureId_Coco, 200.0f, 600.0f, 128.0f, 128.0f, 32 * current_pattern, 32 * 0, 32, 32, 0, { 2.0f, 2.0f }, {1.0f, 0.0f, 0.0f, 1.0f});

    int coin_current_pattern = g_patternCount / 6 % 20; //全体のパターン数
    int current_pattern_x = coin_current_pattern % 5; //横方向のパターン数
    int current_pattern_y = coin_current_pattern / 5;
    Sprite_Draw(g_TextureId_Coin, 400.0f, 380.0f, 400 * current_pattern_x, 400 * current_pattern_y, 400.0f, 400.0f);
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

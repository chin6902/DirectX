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
#include "flipbook_animation.h"
#include "input_keyboard.h"
#include "input_mouse.h"
#include "input_xinput.h"

// テクスチャ管理番号
static int g_TextureId_Bg = TEXTURE_INVALID_ID;
static int g_TextureId_Sky = TEXTURE_INVALID_ID;
static int g_TextureId_Moutain01 = TEXTURE_INVALID_ID;
static int g_TextureId_Moutain02 = TEXTURE_INVALID_ID;
static int g_TextureId_Coco = TEXTURE_INVALID_ID;
static int g_TextureId_Food = TEXTURE_INVALID_ID;
static int g_TextureId_Coin = TEXTURE_INVALID_ID;
static int g_TextureId_RunningMan = TEXTURE_INVALID_ID;

static int g_AnimationId_RunningMan = -1;
static int g_AnimationId_Coin = -1;

static float g_TextureScroll = 0.0f;
static float g_ParallexSkyScroll = 0.0f;
static float g_ParallexMoutain01Scroll = 0.0f;
static float g_ParallexMoutain02Scroll = 0.0f;

static constexpr int RUNMAN_PATTERN_W = 140;  // width  of one frame in pixels
static constexpr int RUNMAN_PATTERN_H = 200;  // height of one frame in pixels
static constexpr int RUNMAN_PATTERN_COUNT = 10;   // total number of frames
static constexpr int RUNMAN_COLUMN_COUNT = 5;    // 5 columns in the sheet
static constexpr float RUNMAN_FPS = 12.0f; // animation speed (frames per second)

static constexpr float RUNMAN_X = 200.0f;
static constexpr float RUNMAN_Y = 550.0f;

static constexpr int COIN_PATTERN_W = 400;
static constexpr int COIN_PATTERN_H = 400;
static constexpr int COIN_PATTERN_COUNT = 20;
static constexpr int COIN_COLUMN_COUNT = 5;
static constexpr float COIN_FPS = 12.0f;

static int g_patternCount = 0;

static int g_mouseX = 0;
static int g_mouseY = 0;
static bool g_flag{};

void DrawBackground();

bool Application_Initialize(HWND hWnd)
{
	InputKeyboard_Initialize();
	InputMouse_Initialize(hWnd);
	InputXInput_Initialize();

	//InputMouse_SetVisible(false);

    if (!Direct3DInitialize(hWnd))
    {
        return false;
    }   

    // 各システムの初期化
    Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
    Sprite_Initialize();
    FlipBookAnimation_Initialize();
    
    // テクスチャのロード
    g_TextureId_Bg = Texture_Load(L"assets/textures/Background.png", false);
	g_TextureId_Food = Texture_Load(L"assets/textures/food.png", false);
    g_TextureId_Coco = Texture_Load(L"assets/textures/coco.png", false);
    g_TextureId_Sky = Texture_Load(L"assets/textures/Sky.png", false);
    g_TextureId_Moutain01 = Texture_Load(L"assets/textures/Hills_1.png", false);
    g_TextureId_Moutain02 = Texture_Load(L"assets/textures/Hills_2.png", false);
    g_TextureId_Coin = Texture_Load(L"assets/textures/coin_anim.png", false);
    g_TextureId_RunningMan = Texture_Load(L"assets/textures/runningman003.png", false);

    g_AnimationId_RunningMan = FlipBookAnimation_Create(
        g_TextureId_RunningMan,
        RUNMAN_PATTERN_W,
        RUNMAN_PATTERN_H,
        RUNMAN_PATTERN_COUNT,
        RUNMAN_COLUMN_COUNT,
        1.0f / RUNMAN_FPS
    );

    g_AnimationId_Coin = FlipBookAnimation_Create(
        g_TextureId_Coin,
        COIN_PATTERN_W,
        COIN_PATTERN_H,
        COIN_PATTERN_COUNT,
        COIN_COLUMN_COUNT,
        1.0f / COIN_FPS
    );

	return true;
}

void Application_Finalize()
{
	// 各システムの終了処理
    FlipBookAnimation_Finalize();
	Sprite_Finalize();
	Texture_Finalize();
	Shader_Finalize();

	InputMouse_Finalize();

	Direct3DFinalize();
}

void Application_Update(float delta_time)
{
    // ゲームの更新処理
	InputKeyboard_Update(delta_time);
	InputMouse_Update();
	InputXInput_Update(delta_time);

	g_mouseX = InputMouse_GetX();
	g_mouseY = InputMouse_GetY();

    if (InputKeyboard_IsTrigger(KK_SPACE)) 
    {

    }

    g_TextureScroll += 50.0f * delta_time;
	
	g_ParallexSkyScroll += 0.0f;
	g_ParallexMoutain01Scroll += 0.15f;
	g_ParallexMoutain02Scroll += 0.2f;

    FlipBookAnimation_Update(delta_time);
}

void Application_FixedUpdate()
{
    g_patternCount += 1;
}

void Application_Draw()
{
    Sprite_SetFilter(kSpriteFilter_Linear);

    DrawBackground();

    Sprite_SetFilter(kSpriteFilter_Linear);
    FlipBookAnimation_Draw(g_AnimationId_RunningMan, RUNMAN_X, RUNMAN_Y);

/*
    int current_pattern = g_patternCount / 8 % 13;
    Sprite_Draw(g_TextureId_Coco, 200.0f, 600.0f, 128.0f, 128.0f, 32 * current_pattern, 32 * 0, 32, 32, 0, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
*/

    int coin_current_pattern = g_patternCount / 6 % 20; //全体のパターン数
    int current_pattern_x = coin_current_pattern % 5; //横方向のパターン数
    int current_pattern_y = coin_current_pattern / 5;
    Sprite_Draw(g_TextureId_Coin, 900.0f, 380.0f, 100.0f, 100.0f, 400 * current_pattern_x, 400 * current_pattern_y, 400, 400, 0, { 1.0f, 1.0f }, {1.0f, 1.0f, 1.0f, 1.0f});

    FlipBookAnimation_Draw(g_AnimationId_Coin, (float)g_mouseX, (float)g_mouseY);
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

/*============================================================================
Contents   :  [fade.h]
              
Author     : Chin Qing You
LastUpdate : 2026/07/10
-----------------------------------------------------------------------------

============================================================================*/
#ifndef FADE_H
#define FADE_H

void Fade_Initialize();
void Fade_Finalize();
void Fade_Update(float delta_time);
void Fade_Draw();

enum FadeType
{
	FADE_IN,
	FADE_OUT
};

void Fade_Start(FadeType type, float fade_time, const DirectX::XMFLOAT4& fade_color);
bool Fade_IsFinished();

#endif

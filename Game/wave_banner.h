/*============================================================================
Contents   :  [wave_banner.h]

Author     : Chin Qing You
LastUpdate : 2026/08/27
-----------------------------------------------------------------------------

============================================================================*/
#ifndef WAVE_BANNER_H
#define WAVE_BANNER_H

void WaveBanner_Initialize();
void WaveBanner_Finalize();
void WaveBanner_Update(float delta_time);

void WaveBanner_Draw();        
void WaveBanner_DrawCounter(); 

#endif
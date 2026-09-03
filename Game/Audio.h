/*============================================================================
Contents   :  [audio.h]

Author     : Chin Qing You
LastUpdate : 2026/08/27
-----------------------------------------------------------------------------

============================================================================*/
#ifndef AUDIO_H
#define AUDIO_H

struct AudioParams
{
	float volume = 1.0f;   
	float pitch = 1.0f;   
	float pitch_jitter = 0.0f;   
	bool  loop = false;
};

void Audio_Initialize();
void Audio_Finalize();
void Audio_Update(float delta_time);   

int  Audio_Load(const char* filename, int voice_count = 4, float min_retrigger = 0.0f);
void Audio_Unload(int sound_id);
void Audio_UnloadAll();

void Audio_Play(int sound_id);
void Audio_Play(int sound_id, const AudioParams& params);
void Audio_Stop(int sound_id);         
void Audio_StopAll();

void Audio_SetVolume(int sound_id, float volume);

void Audio_SetMasterVolume(float v);
void Audio_SetSfxVolume(float v);
void Audio_SetBgmVolume(float v);

void Audio_SetIsMusic(int sound_id, bool is_music);

#endif
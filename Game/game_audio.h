/*============================================================================
Contents   :  [game_audio.h]

Author     : Chin Qing You
LastUpdate : 2026/08/27
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

enum SoundId
{
	SND_NONE = -1,     

	// --- music ---
	SND_BGM_FIELD_1,
	SND_BGM_FIELD_2,
	SND_BGM_BOSS,

	// --- player ---
	SND_CHARGE,
	SND_CAST_FIRE,
	SND_CAST_ICE,
	SND_CAST_THUNDER,
	SND_SUMMON,
	SND_EXPLOSION,
	SND_SMALL_EXPLOSION,
	SND_WIRLPOOL,
	SND_BLIZZARD,
	SND_FREEZE,
	SND_LAZER,
	SND_ZAP,
	SND_PLAYER_HURT,
	SND_LEVEL_UP,
	SND_PICKUP,

	// --- enemies ---
	SND_ORC_SWING,
	SND_ELITE_SWING,
	SND_ENEMY_HURT,
	SND_ENEMY_DIE,

	// --- boss ---
	SND_BOSS_LAND,
	SND_BOSS_WARNING,

	// --- UI ---
	SND_BUTTON_SELECT,
	SND_BUTTON_CHOOSE,

	// --- result ---
	SND_GAME_CLEAR,
	SND_GAME_OVER,
	SND_WIN_VOICE,
	SND_LOSE_VOICE,

	// --- ult ---
	SND_ULT_FIRE,
	SND_ULT_ICE,
	SND_ULT_ELECTRIC,

	SOUND_ID_COUNT,
};

void GameAudio_Initialize();
void GameAudio_Finalize();

void GameAudio_Play(SoundId id);
void GameAudio_PlayPitched(SoundId id, float pitch_mul);   

void GameAudio_Stop(SoundId id);

enum MusicTrack
{
	MUSIC_NONE = -1,
	MUSIC_FIELD_1,
	MUSIC_FIELD_2,
	MUSIC_BOSS,
	MUSIC_TRACK_COUNT,
};

void       GameAudio_PlayMusic(MusicTrack track);
void       GameAudio_StopMusic();
void       GameAudio_UpdateMusic(float delta_time);
MusicTrack GameAudio_TrackForWave(int wave_number, bool is_boss_wave);

#endif
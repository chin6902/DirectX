/*============================================================================
Contents   :  [game_audio.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/27
-----------------------------------------------------------------------------

============================================================================*/
#include "game_audio.h"
#include "audio.h"

struct SoundDef
{
	const char* file;
	int         voices;
	float       volume;
	float       pitch;
	float       jitter;
	float       min_retrigger;
	bool        is_music;
	bool        loop;
};

static constexpr SoundDef g_SoundDef[] =
{
	//  file                                  voices  vol   pitch  jitter  retrig  music  loop
	{ "assets/sounds/bgm_field_1.wav",           1,  0.03f, 1.00f, 0.00f,  0.00f,  true,  true  },
	{ "assets/sounds/bgm_field_2.wav",           1,  0.10f, 1.00f, 0.00f,  0.00f,  true,  true  },
	{ "assets/sounds/bgm_boss.wav",              1,  0.10f, 1.00f, 0.00f,  0.00f,  true,  true  },

	{ "assets/sounds/charge.wav",				 1,  0.10f, 0.80f, 0.00f,  0.00f,  false, false },
	{ "assets/sounds/cast_fire.wav",             4,  0.30f, 1.00f, 0.06f,  0.05f,  false, false },
	{ "assets/sounds/cast_ice.wav",              4,  0.10f, 1.00f, 0.06f,  0.05f,  false, false },
	{ "assets/sounds/cast_thunder.wav",          3,  0.30f, 1.00f, 0.06f,  0.05f,  false, false },
	{ "assets/sounds/summon.wav",                5,  0.70f, 1.00f, 0.05f,  0.02f,  false, false },

	{ "assets/sounds/explosion.wav",             6,  0.60f, 1.00f, 0.08f,  0.05f,  false, false },
	{ "assets/sounds/explosion.wav",             5,  0.20f, 1.00f, 0.18f,  0.10f,  false, false },

	{ "assets/sounds/wirlpool.wav",              1,  0.30f, 1.00f, 0.08f,  0.05f,  false, false },
	{ "assets/sounds/blizzard.wav",              1,  0.05f, 1.00f, 0.18f,  0.25f,  false, false },
	{ "assets/sounds/freeze.wav",                3,  0.55f, 1.00f, 0.05f,  0.03f,  false, false },
	{ "assets/sounds/lazer.wav",                 2,  0.55f, 1.00f, 0.03f,  0.00f,  false, false },
	{ "assets/sounds/zap.wav",                   4,  0.15f, 0.90f, 0.05f,  0.00f,  false, false }, 
	{ "assets/sounds/player_hurt.wav",           2,  0.80f, 1.00f, 0.08f,  0.10f,  false, false },
	{ "assets/sounds/level_up.wav",              1,  0.70f, 1.00f, 0.00f,  0.00f,  false, false },
	{ "assets/sounds/pickup.wav",                6,  0.35f, 1.00f, 0.14f,  0.03f,  false, false },

	{ "assets/sounds/orc_swing.wav",             8,  0.45f, 1.00f, 0.18f,  0.04f,  false, false },
	{ "assets/sounds/elite_swing.wav",           1,  0.20f, 1.00f, 0.18f,  0.00f,  false, false },
	{ "assets/sounds/enemy_hurt.wav",            8,  0.30f, 1.00f, 0.15f,  0.03f,  false, false },
	{ "assets/sounds/enemy_die.wav",             6,  0.40f, 1.00f, 0.12f,  0.04f,  false, false },

	{ "assets/sounds/boss_land.wav",             2,  0.40f, 1.00f, 0.03f,  0.00f,  false, false },
	{ "assets/sounds/boss_warning.wav",          1,  0.95f, 1.00f, 0.00f,  0.00f,  false, false },

	{ "assets/sounds/button_select.wav",         1,  0.95f, 1.00f, 0.00f,  0.00f,  false, false },
	{ "assets/sounds/button_choose.wav",         1,  0.95f, 1.00f, 0.00f,  0.00f,  false, false },

	{ "assets/sounds/game_clear.wav",            1,  0.20f, 1.00f, 0.00f,  0.00f,  false, false },
	{ "assets/sounds/game_over.wav",             1,  0.20f, 1.00f, 0.00f,  0.00f,  false, false },
	{ "assets/sounds/win_voice.wav",             1,  0.85f, 1.00f, 0.00f,  0.00f,  false, false },
	{ "assets/sounds/lose_voice.wav",            1,  0.85f, 1.00f, 0.00f,  0.00f,  false, false },

	{ "assets/sounds/ult_fire.wav",              1,  0.95f, 1.00f, 0.00f,  0.00f,  false, false },
	{ "assets/sounds/ult_ice.wav",               1,  0.95f, 1.00f, 0.00f,  0.00f,  false, false },
	{ "assets/sounds/ult_electric.wav",          1,  0.95f, 1.00f, 0.00f,  0.00f,  false, false },
};

static_assert(sizeof(g_SoundDef) / sizeof(g_SoundDef[0]) == SOUND_ID_COUNT, "g_SoundDef is out of sync with SoundId");

static int g_Handle[SOUND_ID_COUNT];

static bool ValidId(SoundId id)
{
	return id >= 0 && id < SOUND_ID_COUNT && g_Handle[id] >= 0;
}

void GameAudio_Initialize()
{
	for (int i = 0; i < SOUND_ID_COUNT; i++)
	{
		const SoundDef& d = g_SoundDef[i];
		g_Handle[i] = Audio_Load(d.file, d.voices, d.min_retrigger);
		if (g_Handle[i] >= 0) { Audio_SetIsMusic(g_Handle[i], d.is_music); }
	}
}

void GameAudio_Finalize()
{
	GameAudio_StopMusic();

	for (int i = 0; i < SOUND_ID_COUNT; i++)
	{
		if (g_Handle[i] >= 0) { Audio_Unload(g_Handle[i]); }
		g_Handle[i] = -1;
	}
}

static AudioParams ParamsFor(SoundId id, float pitch_mul)
{
	const SoundDef& d = g_SoundDef[id];
	AudioParams p;
	p.volume = d.volume;
	p.pitch = d.pitch * pitch_mul;
	p.pitch_jitter = d.jitter;
	p.loop = d.loop;
	return p;
}

void GameAudio_Play(SoundId id)
{
	if (!ValidId(id)) { return; }
	Audio_Play(g_Handle[id], ParamsFor(id, 1.0f));
}

void GameAudio_PlayPitched(SoundId id, float pitch_mul)
{
	if (!ValidId(id)) { return; }
	Audio_Play(g_Handle[id], ParamsFor(id, pitch_mul));
}

void GameAudio_Stop(SoundId id)
{
	if (!ValidId(id)) { return; }
	Audio_Stop(g_Handle[id]);
}

// ============================================================================
// Music
// ============================================================================
static constexpr float MUSIC_FADE_TIME = 1.60f;

static constexpr SoundId g_TrackSound[MUSIC_TRACK_COUNT] =
{
	SND_BGM_FIELD_1,
	SND_BGM_FIELD_2,
	SND_BGM_BOSS,
};

static MusicTrack g_CurrentTrack = MUSIC_NONE;
static MusicTrack g_FadingTrack = MUSIC_NONE;   
static float      g_FadeLeft = 0.0f;

static SoundId TrackSound(MusicTrack t)
{
	if (t < 0 || t >= MUSIC_TRACK_COUNT) { return SND_NONE; }
	return g_TrackSound[t];
}

void GameAudio_PlayMusic(MusicTrack track)
{
	if (track == g_CurrentTrack) { return; }  


	if (g_FadingTrack != MUSIC_NONE && g_FadingTrack != g_CurrentTrack)
	{
		GameAudio_Stop(TrackSound(g_FadingTrack));
	}
	g_FadingTrack = g_CurrentTrack;
	g_CurrentTrack = track;

	if (g_CurrentTrack != MUSIC_NONE)
	{
		const SoundId id = TrackSound(g_CurrentTrack);
		GameAudio_Play(id);            
		Audio_SetVolume(g_Handle[id], 0.0f);   
	}

	g_FadeLeft = (g_FadingTrack != MUSIC_NONE) ? MUSIC_FADE_TIME : 0.0f;

	if (g_FadeLeft <= 0.0f && g_CurrentTrack != MUSIC_NONE)
	{
		const SoundDef& d = g_SoundDef[TrackSound(g_CurrentTrack)];
		Audio_SetVolume(g_Handle[TrackSound(g_CurrentTrack)], d.volume);
	}
}

void GameAudio_StopMusic()
{
	if (g_CurrentTrack != MUSIC_NONE) { GameAudio_Stop(TrackSound(g_CurrentTrack)); }
	if (g_FadingTrack != MUSIC_NONE) { GameAudio_Stop(TrackSound(g_FadingTrack)); }
	g_CurrentTrack = MUSIC_NONE;
	g_FadingTrack = MUSIC_NONE;
	g_FadeLeft = 0.0f;
}

void GameAudio_UpdateMusic(float delta_time)
{
	if (g_FadeLeft <= 0.0f) { return; }

	g_FadeLeft -= delta_time;
	const float t = 1.0f - (g_FadeLeft / MUSIC_FADE_TIME);   

	if (g_CurrentTrack != MUSIC_NONE)
	{
		const SoundId id = TrackSound(g_CurrentTrack);
		if (ValidId(id)) { Audio_SetVolume(g_Handle[id], g_SoundDef[id].volume * t); }
	}
	if (g_FadingTrack != MUSIC_NONE)
	{
		const SoundId id = TrackSound(g_FadingTrack);
		if (ValidId(id)) { Audio_SetVolume(g_Handle[id], g_SoundDef[id].volume * (1.0f - t)); }
	}

	if (g_FadeLeft <= 0.0f)
	{
		g_FadeLeft = 0.0f;
		if (g_FadingTrack != MUSIC_NONE)
		{
			GameAudio_Stop(TrackSound(g_FadingTrack));
			g_FadingTrack = MUSIC_NONE;
		}
		if (g_CurrentTrack != MUSIC_NONE)
		{
			const SoundId id = TrackSound(g_CurrentTrack);
			if (ValidId(id)) { Audio_SetVolume(g_Handle[id], g_SoundDef[id].volume); }
		}
	}
}

MusicTrack GameAudio_TrackForWave(int wave_number, bool is_boss_wave)
{
	if (is_boss_wave) { return MUSIC_BOSS; }
	return (wave_number <= 5) ? MUSIC_FIELD_1 : MUSIC_FIELD_2;
}
/*============================================================================
Contents   :  [audio.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/27
-----------------------------------------------------------------------------

============================================================================*/
#include <xaudio2.h>
#include <assert.h>
#include <cstdlib>
#include <algorithm>

#include "audio.h"

#pragma comment(lib, "winmm.lib")

static constexpr int AUDIO_MAX = 64;
static constexpr int VOICES_PER_SOUND_MAX = 8;

static constexpr float MAX_FREQ_RATIO = 4.0f;

static IXAudio2* g_Xaudio{};
static IXAudio2MasteringVoice* g_MasteringVoice{};

struct Sound
{
	BYTE* data{};
	int          length{};
	int          play_length{};
	WAVEFORMATEX wfx{};

	IXAudio2SourceVoice* voices[VOICES_PER_SOUND_MAX]{};
	int   voice_count{};
	int   next_voice{};        

	float min_retrigger{};     
	float retrigger_timer{};
	bool  is_music{};
	bool  in_use{};
};

static Sound g_Sounds[AUDIO_MAX]{};

static float g_MasterVolume = 1.0f;
static float g_SfxVolume = 1.0f;
static float g_BgmVolume = 1.0f;

void Audio_Initialize()
{
	XAudio2Create(&g_Xaudio, 0);
	g_Xaudio->CreateMasteringVoice(&g_MasteringVoice);

	for (Sound& s : g_Sounds) { s = {}; }

	g_MasterVolume = 1.0f;
	g_SfxVolume = 1.0f;
	g_BgmVolume = 1.0f;
}

void Audio_Finalize()
{
	Audio_UnloadAll();

	if (g_MasteringVoice) { g_MasteringVoice->DestroyVoice(); g_MasteringVoice = nullptr; }
	if (g_Xaudio) { g_Xaudio->Release();              g_Xaudio = nullptr; }
}

void Audio_Update(float delta_time)
{
	for (Sound& s : g_Sounds)
	{
		if (s.in_use && s.retrigger_timer > 0.0f) { s.retrigger_timer -= delta_time; }
	}
}

static bool LoadWav(const char* filename, Sound& out)
{
	HMMIO    hmmio = NULL;
	MMIOINFO mmioinfo = { 0 };
	MMCKINFO riffchunkinfo = { 0 };
	MMCKINFO datachunkinfo = { 0 };
	MMCKINFO mmckinfo = { 0 };

	hmmio = mmioOpen((LPSTR)filename, &mmioinfo, MMIO_READ);
	if (hmmio == NULL) { return false; }          

	riffchunkinfo.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	mmioDescend(hmmio, &riffchunkinfo, NULL, MMIO_FINDRIFF);

	mmckinfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmioDescend(hmmio, &mmckinfo, &riffchunkinfo, MMIO_FINDCHUNK);

	if (mmckinfo.cksize >= sizeof(WAVEFORMATEX))
	{
		mmioRead(hmmio, (HPSTR)&out.wfx, sizeof(out.wfx));
	}
	else
	{
		PCMWAVEFORMAT pcmwf = { 0 };
		mmioRead(hmmio, (HPSTR)&pcmwf, sizeof(pcmwf));
		memset(&out.wfx, 0x00, sizeof(out.wfx));
		memcpy(&out.wfx, &pcmwf, sizeof(pcmwf));
		out.wfx.cbSize = 0;
	}
	mmioAscend(hmmio, &mmckinfo, 0);

	datachunkinfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mmioDescend(hmmio, &datachunkinfo, &riffchunkinfo, MMIO_FINDCHUNK);

	const UINT32 buflen = datachunkinfo.cksize;
	out.data = new unsigned char[buflen];
	const LONG readlen = mmioRead(hmmio, (HPSTR)out.data, buflen);

	out.length = readlen;
	out.play_length = (out.wfx.nBlockAlign > 0) ? (readlen / out.wfx.nBlockAlign) : 0;

	mmioClose(hmmio, 0);
	return true;
}

int Audio_Load(const char* filename, int voice_count, float min_retrigger)
{
	if (filename == nullptr) { return -1; }   

	int index = -1;
	for (int i = 0; i < AUDIO_MAX; i++)
	{
		if (!g_Sounds[i].in_use) { index = i; break; }
	}
	if (index < 0) { return -1; }

	Sound& s = g_Sounds[index];
	s = {};

	if (!LoadWav(filename, s)) { return -1; }

	s.voice_count = std::clamp(voice_count, 1, VOICES_PER_SOUND_MAX);
	s.min_retrigger = min_retrigger;

	for (int v = 0; v < s.voice_count; v++)
	{
		g_Xaudio->CreateSourceVoice(&s.voices[v], &s.wfx, 0, MAX_FREQ_RATIO);
		if (s.voices[v] == nullptr) { s.voice_count = v; break; }
	}

	if (s.voice_count == 0)
	{
		delete[] s.data;
		s = {};
		return -1;
	}

	s.in_use = true;
	return index;
}

static bool ValidSound(int id)
{
	return id >= 0 && id < AUDIO_MAX && g_Sounds[id].in_use;
}

void Audio_Unload(int sound_id)
{
	if (!ValidSound(sound_id)) { return; }
	Sound& s = g_Sounds[sound_id];

	for (int v = 0; v < s.voice_count; v++)
	{
		if (s.voices[v] == nullptr) { continue; }
		s.voices[v]->Stop();
		s.voices[v]->FlushSourceBuffers();
		s.voices[v]->DestroyVoice();
		s.voices[v] = nullptr;
	}

	delete[] s.data;
	s = {};
}

void Audio_UnloadAll()
{
	for (int i = 0; i < AUDIO_MAX; i++) { Audio_Unload(i); }
}

void Audio_SetIsMusic(int sound_id, bool is_music)
{
	if (!ValidSound(sound_id)) { return; }
	g_Sounds[sound_id].is_music = is_music;
}

// ============================================================================
// Playback
// ============================================================================
static IXAudio2SourceVoice* PickVoice(Sound& s)
{
	for (int v = 0; v < s.voice_count; v++)
	{
		XAUDIO2_VOICE_STATE state{};
		s.voices[v]->GetState(&state);
		if (state.BuffersQueued == 0) { return s.voices[v]; }
	}

	IXAudio2SourceVoice* voice = s.voices[s.next_voice];
	s.next_voice = (s.next_voice + 1) % s.voice_count;
	return voice;
}

static float RandRange(float lo, float hi)
{
	const float t = (rand() % 1001) / 1000.0f;
	return lo + (hi - lo) * t;
}

void Audio_Play(int sound_id, const AudioParams& params)
{
	if (!ValidSound(sound_id)) { return; }
	Sound& s = g_Sounds[sound_id];

	if (s.retrigger_timer > 0.0f) { return; }
	s.retrigger_timer = s.min_retrigger;

	IXAudio2SourceVoice* voice = PickVoice(s);
	if (voice == nullptr) { return; }

	voice->Stop();
	voice->FlushSourceBuffers();

	XAUDIO2_BUFFER buf{};
	buf.AudioBytes = s.length;
	buf.pAudioData = s.data;
	buf.PlayBegin = 0;
	buf.PlayLength = s.play_length;

	if (params.loop)
	{
		buf.LoopBegin = 0;
		buf.LoopLength = s.play_length;
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	const float category = s.is_music ? g_BgmVolume : g_SfxVolume;
	voice->SetVolume(params.volume * category * g_MasterVolume);

	float ratio = params.pitch;
	if (params.pitch_jitter > 0.0f)
	{
		ratio += RandRange(-params.pitch_jitter, params.pitch_jitter);
	}
	voice->SetFrequencyRatio(std::clamp(ratio, 1.0f / MAX_FREQ_RATIO, MAX_FREQ_RATIO));

	voice->SubmitSourceBuffer(&buf, NULL);
	voice->Start();
}

void Audio_Play(int sound_id)
{
	static const AudioParams defaults{};
	Audio_Play(sound_id, defaults);
}

void Audio_Stop(int sound_id)
{
	if (!ValidSound(sound_id)) { return; }
	Sound& s = g_Sounds[sound_id];

	for (int v = 0; v < s.voice_count; v++)
	{
		if (s.voices[v] == nullptr) { continue; }
		s.voices[v]->Stop();
		s.voices[v]->FlushSourceBuffers();
	}
}

void Audio_SetVolume(int sound_id, float volume)
{
	if (!ValidSound(sound_id)) { return; }
	Sound& s = g_Sounds[sound_id];

	const float category = s.is_music ? g_BgmVolume : g_SfxVolume;
	const float final_volume = volume * category * g_MasterVolume;

	for (int v = 0; v < s.voice_count; v++)
	{
		if (s.voices[v] == nullptr) { continue; }
		s.voices[v]->SetVolume(final_volume);
	}
}

void Audio_StopAll()
{
	for (int i = 0; i < AUDIO_MAX; i++) { Audio_Stop(i); }
}

void Audio_SetMasterVolume(float v) { g_MasterVolume = std::clamp(v, 0.0f, 1.0f); }
void Audio_SetSfxVolume(float v) { g_SfxVolume = std::clamp(v, 0.0f, 1.0f); }
void Audio_SetBgmVolume(float v) { g_BgmVolume = std::clamp(v, 0.0f, 1.0f); }
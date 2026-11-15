#include "Stdafx.h"

#undef min
#undef max

#pragma warning(push)
#pragma warning(disable: 4100 4127 4189 4244 4245 4267 4456 4457 4505 4701 4706 4996)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#pragma warning(pop)

#include "MilesMini.h"

#include <stdlib.h>
#include <string.h>

static ma_engine g_engine;
static bool g_engineReady = false;

struct SMiniVoice
{
	ma_decoder decoder;
	ma_sound sound;
	void* owned;
	unsigned long ownedSize;
	bool ready;
	bool spatial;
};

bool MiniEnsureEngine()
{
	if (g_engineReady)
		return true;

	ma_engine_config config = ma_engine_config_init();
	config.channels = 2;
	config.sampleRate = 44100;

	if (ma_engine_init(&config, &g_engine) != MA_SUCCESS)
	{
		TraceError("miniaudio: the audio device did not open");
		return false;
	}

	g_engineReady = true;
	return true;
}

void MiniShutdownEngine()
{
	if (!g_engineReady)
		return;

	ma_engine_uninit(&g_engine);
	g_engineReady = false;
}

static void VoiceClear(SMiniVoice* voice)
{
	if (!voice)
		return;

	if (voice->ready)
	{
		ma_sound_uninit(&voice->sound);
		ma_decoder_uninit(&voice->decoder);
		voice->ready = false;
	}

	if (voice->owned)
	{
		free(voice->owned);
		voice->owned = nullptr;
		voice->ownedSize = 0;
	}
}

bool MiniVoiceCreate(SMiniVoice** voice)
{
	if (!voice)
		return false;

	SMiniVoice* created = new SMiniVoice;
	memset(created, 0, sizeof(*created));
	*voice = created;
	return true;
}

void MiniVoiceDestroy(SMiniVoice* voice)
{
	if (!voice)
		return;

	VoiceClear(voice);
	delete voice;
}

bool MiniVoiceLoad(SMiniVoice* voice, const void* data, unsigned long size, bool spatial)
{
	if (!voice || !data || !size)
		return false;

	if (!MiniEnsureEngine())
		return false;

	VoiceClear(voice);

	ma_decoder_config config = ma_decoder_config_init(
		ma_format_f32,
		ma_engine_get_channels(&g_engine),
		ma_engine_get_sample_rate(&g_engine));

	if (ma_decoder_init_memory(data, size, &config, &voice->decoder) != MA_SUCCESS)
		return false;

	const ma_uint32 flags = spatial ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;
	if (ma_sound_init_from_data_source(&g_engine, &voice->decoder, flags, nullptr, &voice->sound) != MA_SUCCESS)
	{
		ma_decoder_uninit(&voice->decoder);
		return false;
	}

	voice->ready = true;
	voice->spatial = spatial;

	if (spatial)
	{
		ma_sound_set_min_distance(&voice->sound, 1.0f);
		ma_sound_set_position(&voice->sound, 0.0f, 0.0f, 0.0f);
	}

	return true;
}

bool MiniVoiceLoadCopy(SMiniVoice* voice, const void* data, unsigned long size, bool spatial)
{
	if (!voice || !data || !size)
		return false;

	void* copy = malloc(size);
	if (!copy)
		return false;

	memcpy(copy, data, size);

	if (!MiniVoiceLoad(voice, copy, size, spatial))
	{
		free(copy);
		return false;
	}

	voice->owned = copy;
	voice->ownedSize = size;
	return true;
}

void MiniVoicePlay(SMiniVoice* voice, int loopCount)
{
	if (!voice || !voice->ready)
		return;

	ma_sound_set_looping(&voice->sound, loopCount != 1);
	ma_sound_seek_to_pcm_frame(&voice->sound, 0);
	ma_sound_start(&voice->sound);
}

void MiniVoicePause(SMiniVoice* voice)
{
	if (!voice || !voice->ready)
		return;

	ma_sound_stop(&voice->sound);
}

void MiniVoiceResume(SMiniVoice* voice)
{
	if (!voice || !voice->ready)
		return;

	ma_sound_start(&voice->sound);
}

void MiniVoiceStop(SMiniVoice* voice)
{
	if (!voice || !voice->ready)
		return;

	ma_sound_stop(&voice->sound);
	ma_sound_seek_to_pcm_frame(&voice->sound, 0);
}

void MiniVoiceClear(SMiniVoice* voice)
{
	VoiceClear(voice);
}

bool MiniVoiceHasData(SMiniVoice* voice)
{
	return voice != nullptr && voice->ready;
}

float MiniVoiceGetVolume(SMiniVoice* voice)
{
	if (!voice || !voice->ready)
		return 0.0f;

	return ma_sound_get_volume(&voice->sound);
}

void MiniVoiceSetVolume(SMiniVoice* voice, float volume)
{
	if (!voice || !voice->ready)
		return;

	if (volume < 0.0f)
		volume = 0.0f;
	if (volume > 1.0f)
		volume = 1.0f;

	ma_sound_set_volume(&voice->sound, volume);
}

bool MiniVoiceIsDone(SMiniVoice* voice)
{
	if (!voice || !voice->ready)
		return true;

	return ma_sound_is_playing(&voice->sound) == MA_FALSE;
}

void MiniVoiceSetPosition(SMiniVoice* voice, float x, float y, float z)
{
	if (!voice || !voice->ready)
		return;

	ma_sound_set_position(&voice->sound, x, y, z);
}

void MiniVoiceSetVelocity(SMiniVoice* voice, float x, float y, float z)
{
	if (!voice || !voice->ready)
		return;

	ma_sound_set_velocity(&voice->sound, x, y, z);
}

void MiniListenerPosition(float x, float y, float z)
{
	if (!g_engineReady)
		return;

	ma_engine_listener_set_position(&g_engine, 0, x, y, z);
}

void MiniListenerDirection(float fx, float fy, float fz, float ux, float uy, float uz)
{
	if (!g_engineReady)
		return;

	ma_engine_listener_set_direction(&g_engine, 0, fx, fy, fz);
	ma_engine_listener_set_world_up(&g_engine, 0, ux, uy, uz);
}

void MiniListenerVelocity(float x, float y, float z)
{
	if (!g_engineReady)
		return;

	ma_engine_listener_set_velocity(&g_engine, 0, x, y, z);
}

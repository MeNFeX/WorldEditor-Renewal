#include "stdafx.h"
#include "SoundManager2D.h"
#include "MilesMini.h"

CSoundInstance2D::CSoundInstance2D()
	: m_voice(nullptr), m_pSoundData(nullptr)
{
}

CSoundInstance2D::~CSoundInstance2D()
{
	Destroy();
}

void CSoundInstance2D::Destroy()
{
	SAFE_RELEASE(m_pSoundData);

	MiniVoiceDestroy(m_voice);
	m_voice = nullptr;
}

bool CSoundInstance2D::Initialize()
{
	if (m_voice)
		return true;

	return MiniVoiceCreate(&m_voice);
}

bool CSoundInstance2D::SetSound(CSoundData * pSoundData)
{
	assert(pSoundData != nullptr);

	const LPVOID lpData = pSoundData->Get();

	assert(m_voice != nullptr);

	if (!lpData || !MiniVoiceLoad(m_voice, lpData, pSoundData->GetSize(), false))
	{
		if (m_pSoundData != nullptr)
		{
			m_pSoundData->Release();
			m_pSoundData = nullptr;
		}

		pSoundData->Release();
		TraceError("miniaudio: %s", pSoundData->GetFileName());
		return false;
	}

	if (m_pSoundData != nullptr)
	{
		m_pSoundData->Release();
		m_pSoundData = nullptr;
	}

	m_pSoundData = pSoundData;
	return true;
}

bool CSoundInstance2D::IsDone() const
{
	return MiniVoiceIsDone(m_voice);
}

void CSoundInstance2D::Play(int iLoopCount, DWORD dwPlayCycleTimeLimit) const
{
	(void) dwPlayCycleTimeLimit;
	MiniVoicePlay(m_voice, iLoopCount);
}

void CSoundInstance2D::Pause() const
{
	MiniVoicePause(m_voice);
}

void CSoundInstance2D::Resume() const
{
	MiniVoiceResume(m_voice);
}

void CSoundInstance2D::Stop()
{
	MiniVoiceStop(m_voice);
}

void CSoundInstance2D::GetVolume(float& rfVolume) const
{
	rfVolume = MiniVoiceGetVolume(m_voice);
}

void CSoundInstance2D::SetVolume(float volume) const
{
	volume = max(0.0f, min(1.0f, volume));
	MiniVoiceSetVolume(m_voice, volume);
}

void CSoundInstance2D::SetPosition(float x, float y, float z) const
{
	assert(!"must not call this method");
}

void CSoundInstance2D::SetOrientation(float x_face, float y_face, float z_face,
									  float x_normal, float y_normal, float z_normal) const
{
	assert(!"must not call this method");
}

void CSoundInstance2D::SetVelocity(float fx, float fy, float fz, float fMagnitude) const
{
	assert(!"must not call this method");
}

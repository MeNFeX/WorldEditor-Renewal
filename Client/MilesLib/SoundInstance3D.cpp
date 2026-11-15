#include "stdafx.h"
#include "SoundManager3D.h"
#include "../eterBase/Timer.h"
#include "MilesMini.h"

CSoundInstance3D::CSoundInstance3D()
	: m_voice(nullptr), m_pSoundData(nullptr)
{
}

CSoundInstance3D::~CSoundInstance3D()
{
	Destroy();
}

void CSoundInstance3D::Destroy()
{
	SAFE_RELEASE(m_pSoundData);

	MiniVoiceDestroy(m_voice);
	m_voice = nullptr;
}

bool CSoundInstance3D::Initialize()
{
	if (m_voice)
		return true;

	return MiniVoiceCreate(&m_voice);
}

bool CSoundInstance3D::SetSound(CSoundData* pSoundData)
{
	assert(pSoundData != nullptr);

	const LPVOID lpData = pSoundData->Get();

	assert(m_voice != nullptr);

	if (!lpData || !MiniVoiceLoad(m_voice, lpData, pSoundData->GetSize(), true))
	{
		TraceError("miniaudio: %s", pSoundData->GetFileName());
		pSoundData->Release();
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

bool CSoundInstance3D::IsDone() const
{
	return MiniVoiceIsDone(m_voice);
}

void CSoundInstance3D::Play(int iLoopCount, DWORD dwPlayCycleTimeLimit) const
{
	if (!m_pSoundData)
		return;

	const DWORD dwCurTime = ELTimer_GetMSec();

	if (dwCurTime - m_pSoundData->GetPlayTime() < dwPlayCycleTimeLimit)
		return;

	m_pSoundData->SetPlayTime(dwCurTime);

	MiniVoicePlay(m_voice, iLoopCount);
}

void CSoundInstance3D::Pause() const
{
	MiniVoicePause(m_voice);
}

void CSoundInstance3D::Resume() const
{
	MiniVoiceResume(m_voice);
}

void CSoundInstance3D::Stop()
{
	MiniVoiceStop(m_voice);
}

void CSoundInstance3D::GetVolume(float& rfVolume) const
{
	rfVolume = MiniVoiceGetVolume(m_voice);
}

void CSoundInstance3D::SetVolume(float volume) const
{
	volume = max(0.0f, min(1.0f, volume));
	MiniVoiceSetVolume(m_voice, volume);
}

void CSoundInstance3D::SetPosition(float x, float y, float z) const
{
	MiniVoiceSetPosition(m_voice, x, y, -z);
}

void CSoundInstance3D::SetOrientation(float x_face, float y_face, float z_face,
									  float x_normal, float y_normal, float z_normal) const
{
	assert(!" CSoundInstance3D::SetOrientation - Don't use this function");
}

void CSoundInstance3D::SetVelocity(float fDistanceX, float fDistanceY, float fDistanceZ, float fNagnitude) const
{
	MiniVoiceSetVelocity(m_voice, fDistanceX, fDistanceY, fDistanceZ);
	(void) fNagnitude;
}

void CSoundInstance3D::UpdatePosition(float fElapsedTime) const
{
	(void) fElapsedTime;
}

#include "stdafx.h"
#include "SoundInstance.h"
#include "MilesMini.h"
#include "../EterPack/EterPackManager.h"

CSoundInstanceStream::CSoundInstanceStream()
	: m_voice(nullptr)
{
}

CSoundInstanceStream::~CSoundInstanceStream()
{
	Destroy();
}

void CSoundInstanceStream::Destroy()
{
	MiniVoiceDestroy(m_voice);
	m_voice = nullptr;
}

bool CSoundInstanceStream::Initialize()
{
	if (m_voice)
		return true;

	return MiniVoiceCreate(&m_voice);
}

bool CSoundInstanceStream::LoadFile(const char* filename)
{
	if (!m_voice)
		return false;

	CMappedFile file;
	LPCVOID view = nullptr;

	if (!CEterPackManager::Instance().Get(file, filename, &view) || !view)
		return false;

	return MiniVoiceLoadCopy(m_voice, view, file.Size(), false);
}

bool CSoundInstanceStream::IsDone() const
{
	return MiniVoiceIsDone(m_voice);
}

bool CSoundInstanceStream::IsData() const
{
	return MiniVoiceHasData(m_voice);
}

void CSoundInstanceStream::Play(int count, DWORD dwPlayCycleTimeLimit) const
{
	if (!IsData())
		return;

	(void) dwPlayCycleTimeLimit;
	MiniVoicePlay(m_voice, count);
}

void CSoundInstanceStream::Pause() const
{
	if (!IsData())
		return;

	MiniVoicePause(m_voice);
}

void CSoundInstanceStream::Resume() const
{
	if (!IsData())
		return;

	MiniVoiceResume(m_voice);
}

void CSoundInstanceStream::Stop()
{
	if (!IsData())
		return;

	MiniVoiceClear(m_voice);
}

void CSoundInstanceStream::GetVolume(float& rfVolume) const
{
	if (!IsData())
		return;

	rfVolume = MiniVoiceGetVolume(m_voice);
}

void CSoundInstanceStream::SetVolume(float volume) const
{
	if (!IsData())
		return;

	volume = max(0.0f, min(1.0f, volume));
	MiniVoiceSetVolume(m_voice, volume);
}

bool CSoundInstanceStream::SetSound(CSoundData* pSound)
{
	return true;
}

void CSoundInstanceStream::SetPosition(float x, float y, float z) const
{
}

void CSoundInstanceStream::SetOrientation(float x_face, float y_face, float z_face,
										  float x_normal, float y_normal, float z_normal) const
{
}

void CSoundInstanceStream::SetVelocity(float fx, float fy, float fz, float fMagnitude) const
{
}

#include "stdafx.h"
#include "SoundManagerStream.h"
#include "MilesMini.h"

CSoundManagerStream::CSoundManagerStream()
{
}

CSoundManagerStream::~CSoundManagerStream()
{
}

bool CSoundManagerStream::Initialize()
{
	CSoundBase::Initialize();

	if (!MiniEnsureEngine())
	{
		CSoundBase::Destroy();
		return false;
	}

	for (int i = 0; i < MUSIC_INSTANCE_MAX_NUM; ++i)
		m_Instances[i].Initialize();

	return true;
}

void CSoundManagerStream::Destroy()
{
	for (int i=0; i<MUSIC_INSTANCE_MAX_NUM; ++i)
		m_Instances[i].Destroy();

	CSoundBase::Destroy();
}

bool CSoundManagerStream::SetInstance(DWORD dwIndex, const char* filename)
{
	if (!CheckInstanceIndex(dwIndex))
		return false;

	return m_Instances[dwIndex].LoadFile(filename);
}

CSoundInstanceStream * CSoundManagerStream::GetInstance(DWORD dwIndex)
{
	if (!CheckInstanceIndex(dwIndex))
		return nullptr;

	return &m_Instances[dwIndex];
}

bool CSoundManagerStream::CheckInstanceIndex(DWORD dwIndex) const
{
	if (dwIndex >= DWORD(MUSIC_INSTANCE_MAX_NUM))
		return false;

	return true;
}


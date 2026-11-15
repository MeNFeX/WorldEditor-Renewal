#include "Stdafx.h"
#include "SoundBase.h"
#include "MilesMini.h"

TSoundDataMap			CSoundBase::ms_dataMap;
bool					CSoundBase::ms_bInitialized = false;
int						CSoundBase::ms_iRefCount = 0;

CSoundBase::CSoundBase()
{
}

CSoundBase::~CSoundBase()
{
}

void CSoundBase::Destroy() const
{
	if (ms_iRefCount > 1)
	{
		--ms_iRefCount;
		return;
	}

	ms_iRefCount = 0;

	if (!ms_dataMap.empty())
	{
		TSoundDataMap::iterator i;
		for (i = ms_dataMap.begin(); i != ms_dataMap.end(); ++i)
		{
			const CSoundData * pSoundData = i->second;
			delete pSoundData;
		}

		ms_dataMap.clear();
	}

	MiniShutdownEngine();
}

void CSoundBase::Initialize() const
{
	++ms_iRefCount;

	if (ms_iRefCount > 1)
		return;

	MiniEnsureEngine();
	ms_dataMap.clear();
}

DWORD CSoundBase::GetFileCRC(const char * filename) const
{
	return GetCRC32(filename, strlen(filename));
}

CSoundData * CSoundBase::AddFile(DWORD dwFileCRC, const char* filename) const
{
	auto pSoundData = new CSoundData;
	pSoundData->Assign(filename);
	ms_dataMap.insert(TSoundDataMap::value_type(dwFileCRC, pSoundData));
	return pSoundData;
}


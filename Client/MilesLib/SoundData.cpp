#include "StdAfx.h"
#include "SoundData.h"

#include "../EterPack/EterPackManager.h"
#include "../eterBase/Timer.h"

#include <stdlib.h>
#include <string.h>

const char * CSoundData::GetFileName() const
{
	return m_filename;
}

void CSoundData::Assign(const char* filename)
{
	assert(m_assigned == false);

	strncpy(m_filename, filename, sizeof(m_filename)-1);
	m_assigned = true;
}

LPVOID CSoundData::Get()
{
	++m_iRefCount;
	m_dwAccessTime = ELTimer_GetMSec();

	if (!m_data)
		ReadFromDisk();

	if (m_flag & FLAG_DATA_SIZE)
		return ((int *) m_data + 1);
	else
		return (m_data);
}

ULONG CSoundData::GetSize() const
{
	return m_size;
}

void CSoundData::Release()
{
	assert(m_iRefCount != 0);
	--m_iRefCount;
	m_dwAccessTime = ELTimer_GetMSec();
}

DWORD CSoundData::GetPlayTime() const
{
	return m_dwPlayTime;
}

void CSoundData::SetPlayTime(DWORD dwPlayTime)
{
	m_dwPlayTime = dwPlayTime;
}

DWORD CSoundData::GetAccessTime() const
{
	return m_dwAccessTime;
}

bool CSoundData::ReadFromDisk()
{
	assert(m_assigned == true);

	CMappedFile file;
	LPCVOID view = nullptr;

	if (!CEterPackManager::Instance().Get(file, m_filename, &view) || !view)
		return false;

	const DWORD size = file.Size();
	if (!size)
		return false;

	void* copy = malloc(size);
	if (!copy)
		return false;

	memcpy(copy, view, size);
	m_data = copy;
	m_size = size;
	m_flag &= ~FLAG_DATA_SIZE;
	return true;
}

void CSoundData::SetPackMode()
{
}

void CSoundData::Destroy()
{
	if (m_data)
	{
		free(m_data);
		m_data = nullptr;
	}
}

CSoundData::CSoundData() :
m_assigned(false),
m_iRefCount(0),
m_dwPlayTime(0),
m_dwAccessTime(ELTimer_GetMSec()),
m_size(0),
m_data(nullptr),
m_flag(0)
{
}

CSoundData::~CSoundData()
{
	Destroy();
}


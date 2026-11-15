#ifndef __MILESLIB_CSOUNDDATA_H__
#define __MILESLIB_CSOUNDDATA_H__

#include "../eterBase/MappedFile.h"

class CSoundData
{
	public:
		enum
		{
			FLAG_DATA_SIZE = 1,
			SOUND_FILE_MAX_NUM = 5,
		};

	public:
		static void		SetPackMode();

		CSoundData();
		virtual ~CSoundData();

		void			Assign(const char* filename);
		LPVOID			Get();
		ULONG			GetSize() const;
		void			Release();
		DWORD			GetAccessTime() const;
		const char *	GetFileName() const;

		void			SetPlayTime(DWORD dwPlayTime);
		DWORD			GetPlayTime() const;

	protected:
		bool			ReadFromDisk();
		void			Destroy();

	protected:
		char			m_filename[128];
		int				m_iRefCount;
		DWORD			m_dwAccessTime;
		DWORD			m_dwPlayTime;
		ULONG			m_size;
		LPVOID			m_data;
		long			m_flag;
		bool			m_assigned;
};

#endif


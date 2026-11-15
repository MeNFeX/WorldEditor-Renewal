#ifndef __MILESLIB_CSOUNDBASE_H__
#define __MILESLIB_CSOUNDBASE_H__

#include <map>
#include "SoundData.h"

typedef std::map<DWORD, CSoundData*> TSoundDataMap;

class CSoundBase
{
	public:
		CSoundBase();
		virtual ~CSoundBase();

		void					Initialize() const;
		void					Destroy() const;

		CSoundData *			AddFile(DWORD dwFileCRC, const char* filename) const;
		DWORD					GetFileCRC(const char* filename) const;

	protected:
		static int								ms_iRefCount;
		static TSoundDataMap					ms_dataMap;
		static bool								ms_bInitialized;
};

#endif


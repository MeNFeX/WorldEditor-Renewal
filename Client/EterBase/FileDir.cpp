#include "StdAfx.h"
#include "FileDir.h"
#include <string>

namespace
{
	std::wstring FilenameToWide(const char* filename)
	{
		if (!filename || !*filename)
			return std::wstring();

		int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, nullptr, 0);
		UINT cp = CP_UTF8;
		if (n <= 0)
		{
			cp = CP_ACP;
			n = MultiByteToWideChar(CP_ACP, 0, filename, -1, nullptr, 0);
		}
		if (n <= 1)
			return std::wstring();

		std::wstring out(static_cast<size_t>(n - 1), L'\0');
		MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, filename, -1, &out[0], n);
		return out;
	}

	std::string WideToUtf8(const wchar_t* text)
	{
		if (!text || !*text)
			return std::string();

		const int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		if (n <= 1)
			return std::string();

		std::string out(static_cast<size_t>(n - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, text, -1, &out[0], n, nullptr, nullptr);
		return out;
	}
}

CDir::CDir()
{
	Initialize();
}

CDir::~CDir()
{
	Destroy();
}

void CDir::Destroy()
{
	if (m_hFind)
		FindClose(m_hFind);

	Initialize();
}

bool CDir::Create(const char * c_szFilter, const char* c_szPath, BOOL bCheckedExtension)
{
	Destroy();

	std::wstring stPath = FilenameToWide(c_szPath);

	if (!stPath.empty())
	{
		const wchar_t end = stPath[stPath.length() - 1];

		if (end != L'\\' && end != L'/')
			stPath += L'\\';
	}

	const std::wstring stQuery = stPath + L"*.*";
	WIN32_FIND_DATAW wfd;
	memset(&wfd, 0, sizeof(wfd));
	m_hFind = FindFirstFileW(stQuery.c_str(), &wfd);

	if (m_hFind == INVALID_HANDLE_VALUE)
		return true;

	const std::string stPathUtf8 = WideToUtf8(stPath.c_str());

	do
	{
		if (wfd.cFileName[0] == L'.')
			continue;

		m_wfd.dwFileAttributes = wfd.dwFileAttributes;
		const std::string stName = WideToUtf8(wfd.cFileName);

		if (IsFolder())
		{
			if (!OnFolder(c_szFilter, stPathUtf8.c_str(), stName.c_str()))
				return false;
		}
		else
		{
			const char * c_szExtension = strchr(stName.c_str(), '.');
			if (!c_szExtension)
				continue;

			if (bCheckedExtension)
			{
				std::string strFilter = c_szFilter;
				const int iPos = strFilter.find_first_of(';', 0);
				if (iPos > 0)
				{
					std::string strFirstFilter = std::string(c_szFilter).substr(0, iPos);
					std::string strSecondFilter = std::string(c_szFilter).substr(iPos+1, strlen(c_szFilter));
					if (0 != strFirstFilter.compare(c_szExtension+1) && 0 != strSecondFilter.compare(c_szExtension+1))
						continue;
				}
				else
				{
					if (0 != stricmp(c_szExtension+1, c_szFilter))
						continue;
				}
			}

			if (!OnFile(stPathUtf8.c_str(), stName.c_str()))
				return false;
		}
	}
	while (FindNextFileW(m_hFind, &wfd));

	return true;
}

bool CDir::IsFolder() const
{
	if (m_wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	return false;
}

void CDir::Initialize()
{
	memset(&m_wfd, 0, sizeof(m_wfd));
	m_hFind = nullptr;
}


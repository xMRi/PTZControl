// CameraRename
// Copyright (C) 2026 Martin Richter (xMRi-Software) - webmaster@m-ri.de
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see
// <https://www.gnu.org/licenses/>.
// 
// SPDX-License-Identifier: GPL-3.0-or-later


#pragma once

#include <shlwapi.h>


class CFileVersionInfo : public VS_FIXEDFILEINFO 
{
public:
	CFileVersionInfo();
	virtual ~CFileVersionInfo();

	bool GetFileVersionInfo(HMODULE hModule=NULL);
	bool GetFileVersionInfo(LPCTSTR modulename);
	CString	GetValue(LPCTSTR lpKeyName) const;
	static bool DllGetVersion(LPCTSTR modulename, DLLVERSIONINFO& dvi);

	bool IsValid() const
	{
		return m_pVersionInfo!=NULL;
	}
	CString GetFileVersion() const
	{
		return GetValue(_T("FileVersion"));
	}
	CString GetProductName() const
	{
		return GetValue(_T("ProductName"));
	}
	CString GetProductVersion() const
	{
		return GetValue(_T("ProductVersion"));
	}
	CString GetLegalCopyright() const
	{
		return GetValue(_T("LegalCopyright"));
	}
protected:
	void Clear();

protected:
// Ausgelesene Daten aus der Datei
	BYTE* m_pVersionInfo;	// all version info

// Überstetzung ode Code Page
	struct TRANSLATION 
	{
		WORD langID;			// language ID
		WORD charset;			// character set (code page)
	} 
	m_translation;
};

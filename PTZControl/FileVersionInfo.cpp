// PTZControl
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

#include "pch.h"

// Code taken from an article of Paul DiLascia. MSDN April 1998
//   http://www.microsoft.com/msj/0498/c0498.aspx

#include "FileVersionInfo.h"

#pragma comment(lib, "version.lib")

CFileVersionInfo::CFileVersionInfo()
	: m_translation{}
{
	m_pVersionInfo = NULL;				
}

CFileVersionInfo::~CFileVersionInfo()
{
	Clear();
	
}

void CFileVersionInfo::Clear()
{
	delete [] m_pVersionInfo;
	m_pVersionInfo = NULL;

	// default = ANSI code page
	m_translation.langID = 0;
	m_translation.charset = 1252;
	// Die Basisklasse löschen
	memset(static_cast<VS_FIXEDFILEINFO*>(this), 0, sizeof(VS_FIXEDFILEINFO));
}

bool CFileVersionInfo::GetFileVersionInfo(HMODULE hModule)
{
	CString strFileName;
	if (::GetModuleFileName(hModule, CStrBuf(strFileName, _MAX_PATH), _MAX_PATH))
		return GetFileVersionInfo(strFileName);
	else
		return false;
}

bool CFileVersionInfo::GetFileVersionInfo(LPCTSTR szFilename)
{
	// Daten löschen
	Clear();

	// read file version info
	DWORD dwDummyHandle; 
	DWORD dwLen = ::GetFileVersionInfoSize(szFilename, &dwDummyHandle);
	if (dwLen==0)
		return false;

	// Buffer besorgen
	m_pVersionInfo = new BYTE[dwLen]; 
	if (!::GetFileVersionInfo(szFilename,0,dwLen,m_pVersionInfo))
	{
		// Buffer freigeben
		Clear();
		return false;
	}

	// Daten umladen
	LPVOID lpvi;
	UINT iLen;
	if (!::VerQueryValue(m_pVersionInfo, _T("\\"), &lpvi, &iLen))
	{
		Clear();
		return false;
	}

	// copy fixed info to myself, which am derived from VS_FIXEDFILEINFO
	*static_cast<VS_FIXEDFILEINFO*>(this) = *static_cast<VS_FIXEDFILEINFO*>(lpvi);

	// Get translation info
	if (::VerQueryValue(m_pVersionInfo,_T("\\VarFileInfo\\Translation"),&lpvi,&iLen) && iLen>=4) 
		m_translation = *static_cast<TRANSLATION*>(lpvi);
	return dwSignature==VS_FFI_SIGNATURE;
}

CString CFileVersionInfo::GetValue(LPCTSTR lpszKeyName) const
{
	CString sVal;
	if (m_pVersionInfo) 
	{
		// To get a string value must pass query in the form
		//    "\StringFileInfo\<langID><codepage>\keyname"
		// where <lang-codepage> is the languageID concatenated with the
		// code page, in hex. Wow.
		CString strQuery;
		strQuery.Format(_T("\\StringFileInfo\\%04x%04x\\%s"),
			m_translation.langID,
			m_translation.charset,
			lpszKeyName);

		// Wert abfragen
		LPVOID pVal;
		UINT iLenVal;
		if (::VerQueryValue(m_pVersionInfo,strQuery.GetBuffer(0),&pVal, &iLenVal)) 
			sVal = static_cast<LPCTSTR>(pVal);
	}
	return sVal;
}

bool CFileVersionInfo::DllGetVersion(LPCTSTR szModulename, DLLVERSIONINFO& dvi)
{
	HINSTANCE hinst = ::LoadLibrary(szModulename);
	if (!hinst)
		return false;

	// Must use GetProcAddress because the DLL might not implement 
	// DllGetVersion. Depending upon the DLL, the lack of implementation of the 
	// function may be a version marker in itself.
	DLLGETVERSIONPROC pDllGetVersion = (DLLGETVERSIONPROC)::GetProcAddress(hinst, "DllGetVersion");
	if (!pDllGetVersion)
	{
		// Entladen
		::FreeLibrary(hinst);
		return false;
	}

	// Datenbereich löschen
	memset(&dvi, 0, sizeof(dvi));			 // clear
	dvi.cbSize = sizeof(dvi);				 // set size for Windows
	bool bReturn = SUCCEEDED((*pDllGetVersion)(&dvi));

	// Free again
	::FreeLibrary(hinst);
	return bReturn;
}

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
#include "SingleInstance.h"

#include <ShLwApi.h>


////////////////////////////////////////////////////////////////////////////////
// Helper

CString GetFullPathName(const CString& str)
{
	CString strOut;
	::GetFullPathName(str, _MAX_PATH, CStrBuf(strOut, MAX_PATH), nullptr);
	return strOut;
}

CString GetModuleFileName(HMODULE hInstance)
{
	// Thee is a chance that we got started with a short filename by com automation.
	// we should always use the long filename here. Maybe the underlying code
	// needs the full filename
	CString strModuleFilenameTemp;
	if (::GetModuleFileName(hInstance,CStrBuf(strModuleFilenameTemp,_MAX_PATH),_MAX_PATH)==0)
	{
		// Should not happen hInstance handles should always be valid.
		ASSERT(FALSE);
		return {};
	}

	// Normalize the path
	strModuleFilenameTemp = GetFullPathName(strModuleFilenameTemp);

	// Maybe we get a short filename, so we convert it.
	CString strModuleFilename;
	DWORD dwLen = ::GetLongPathName(strModuleFilenameTemp,CStrBuf(strModuleFilename,_MAX_PATH),_MAX_PATH);

	// May be we get an overflow...
	if (dwLen>_MAX_PATH)
		dwLen = ::GetLongPathName(strModuleFilenameTemp,CStrBuf(strModuleFilename,dwLen),dwLen);

	// If the GetLongPathName fails we just return the unchanged module filename 
	if (dwLen==0)
		strModuleFilename = strModuleFilenameTemp;

	return strModuleFilename;
}

/////////////////////////////////////////////////////////////////////////////
// CSingleInstance

CSingleInstance::CSingleInstance()
	: m_hMutexApplication(NULL)
	, m_hSharedMemFile(NULL)
{
}

CSingleInstance::~CSingleInstance()
{
	Unregister();
}

CSingleInstance& CSingleInstance::Instance()
{
	static CSingleInstance singleton;	
	return singleton;
}

CString CSingleInstance::FormatInternalName(PCTSTR pszName)
{
	ASSERT_VALID(AfxGetApp());
	ASSERT(pszName!=NULL);

	// Because GetModulefilename may give the name of the file name in mixed case
	// and not as stored in the file system, we use MakeUpper for the name here.
	// Also to protect the programmer using the Find function we always use
	// MakeUpper. 
	return _T("SingleInstance::")+CString(pszName).MakeUpper();
}
	
void CSingleInstance::SetInstanceName(PCTSTR pszName)
{
	// There should be no handle hat all when the name is set
	ASSERT(m_hMutexApplication==NULL);
	m_strName = pszName;
}


bool CSingleInstance::Register()
{
	// Close Mutex if Open
	::CloseHandle(m_hMutexApplication);

	// called for the first time and a name is known?
	if (m_strName.IsEmpty())
	{
		// If there is no name, use the exe/dll's filename
		// There is a risk that the user renames the file and 
		// restarts the application!
		CString strProgName = GetModuleFileName(AfxGetInstanceHandle());
		m_strName = ::PathFindFileName(strProgName);
	}

	// Create the mutex
	bool bCreated;
	CreateMutex(m_strName,m_hMutexApplication,bCreated);

	// If the mutex already existed the bCreated flag is false and we are 
	// not the only instance
	return m_hMutexApplication!=NULL && bCreated;
}

void CSingleInstance::CreateMutex(PCTSTR pszName, HANDLE &hMutex, bool &bCreated)
{
	// There must be a name
	ASSERT(pszName!=NULL);
	// Note that the Name is case sensitiv.
	CString strMutexName = FormatInternalName(pszName)+_T("::Mutex");	
	// Create the Mutex
	hMutex = ::CreateMutex(NULL, FALSE, strMutexName); 

	// Set flag if it already exists
	bCreated = hMutex!=NULL && (::GetLastError()!=ERROR_ALREADY_EXISTS);
}

void CSingleInstance::Unregister()
{
	::CloseHandle(m_hSharedMemFile);
	m_hSharedMemFile = NULL;
	::CloseHandle(m_hMutexApplication);
	m_hMutexApplication = NULL;
}

HWND CSingleInstance::FindInstance()
{
	ASSERT(m_hMutexApplication!=NULL && !m_strName.IsEmpty());
	// Try to find the previous instance
	if (m_hMutexApplication!=NULL && !m_strName.IsEmpty())
		return FindInstance(m_strName);
	else
		return NULL;
}

HWND CSingleInstance::FindInstance(PCTSTR pszName)
{
	// Need to get the shared memory file
	HANDLE hSharedMemFile;
	hSharedMemFile = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FormatInternalName(pszName));
	if (!hSharedMemFile)
		// Nothing here
		return NULL;

	// Get a pointer to the data
	HWND hWnd = NULL;
	HWND *pWnd = static_cast<HWND*>(::MapViewOfFile(hSharedMemFile,FILE_MAP_READ,0,0,sizeof(HWND)));
	if (pWnd)
	{
		// Get the mutex to protect accessing the data
		HANDLE hMutex = NULL;
		bool bCreated;
		CreateMutex(pszName,hMutex,bCreated);
		if (hMutex)	
		{
			// Lock the data prior to reading from it
			VERIFY(::WaitForSingleObject(hMutex,INFINITE)==WAIT_OBJECT_0);			
			ASSERT_POINTER(pWnd,HWND);
			hWnd = *pWnd;
			::ReleaseMutex(hMutex);
			::CloseHandle(hMutex);
		}

		//Unmap the MMF we were using
		VERIFY(::UnmapViewOfFile(pWnd));
	}

	::CloseHandle(hSharedMemFile);
	return hWnd;
}

bool CSingleInstance::ActivatePrevInstance()
{
	ASSERT(m_hMutexApplication!=NULL && !m_strName.IsEmpty());

	// Try to find the previous instance
	HWND hWnd = FindInstance(m_strName);
	if (!hWnd)
		return false;

	// Only if the window handle is still valid
	if (::IsWindow(hWnd))
	{
		// does it have any popups?
		hWnd = ::GetLastActivePopup(hWnd);
		// Bring the main window to the top
		if (::IsIconic(hWnd))                 
			// If iconic, restore the main window
			::ShowWindow(hWnd,SW_RESTORE);     
		// bring it to top       		
		::SetForegroundWindow(hWnd);
		return true;
	}
	else
		return false;
}

void CSingleInstance::SetInstanceWindow(HWND hWnd)
{
	// Register must be called first and there should be a name
	ASSERT(m_hMutexApplication && !m_strName.IsEmpty());
	ASSERT(::IsWindow(hWnd));
	if (!m_hSharedMemFile)	
		// Try to open the memory mapped file 
		m_hSharedMemFile = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(HWND), FormatInternalName(m_strName));

	if (m_hSharedMemFile)
	{
		// Get a pointer to the data
		HWND* pWnd = (HWND*)::MapViewOfFile(m_hSharedMemFile, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, sizeof(HWND));
		if (pWnd)
		{
			// Lock the data prior to writing to from it
			VERIFY(::WaitForSingleObject(m_hMutexApplication,INFINITE)==WAIT_OBJECT_0);			
			ASSERT_POINTER(pWnd,HWND);
			*pWnd = hWnd;
			::ReleaseMutex(m_hMutexApplication);

			// Unmap
			VERIFY(::UnmapViewOfFile(pWnd));
		}
	}
}

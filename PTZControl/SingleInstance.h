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

#pragma once

/////////////////////////////////////////////////////////////////////////////
//	CSingleInstance	
//		Limits an application to a single instance
	
class CSingleInstance
{
//-------------private functions----------------------------------------
private:
// Private Construction
	CSingleInstance();
	~CSingleInstance();

//-------------public functions-----------------------------------------
public:
// Accessor
	static CSingleInstance& Instance();
	
// Operations:
// Register causes a mutex to be created
	bool Register();	
	void Unregister();

// Attributes
	void SetInstanceName(PCTSTR pszName);

// Save the current instance HWND in a memory mapped file (optinal)
	void SetInstanceWindow(CWnd *pWnd=NULL)
		{ SetInstanceWindow(pWnd!=NULL ? pWnd->m_hWnd : AfxGetMainWnd()->GetSafeHwnd()); }
	void SetInstanceWindow(HWND hWnd);

// Find previous instance
	HWND FindInstance();

// launch a previous running instance
	bool ActivatePrevInstance();

// The name is free to allow other programs to find the instance.  
// Find previous instance. Note that the name is converted with MakeUpper.
// Because a mutex is case sensitive.
	static HWND FindInstance(PCTSTR pszName);

//-------------protected member functions-------------------------------
protected:
	static CString FormatInternalName(PCTSTR pszName=NULL);
	static void CreateMutex(PCTSTR pszName, HANDLE &hMutex, bool &bCreated);

//-------------data member----------------------------------------------
protected:
	HANDLE	m_hMutexApplication;	// Mutex to lock the Application
									// and to control access to the data
	HANDLE	m_hSharedMemFile;		// memory mapped file to share the data
	CString m_strName;				// Name of the instance (may be file name of the module)
};

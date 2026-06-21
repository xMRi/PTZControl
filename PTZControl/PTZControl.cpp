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

// PTZControl.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "SingleInstance.h"
#include "framework.h"
#include "PTZControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
//	CAgvipCommandLineInfo
//		The program must be started with the parameters /conn:<connectionfile>
//		and /user:<userid>. For the Debug mode special Options are prepared.

class CPTZControlCommandLineInfo : public CCommandLineInfo
{
public:
	// Construction
	CPTZControlCommandLineInfo() 
		: m_bNoReset(false)
		, m_bNoGuard(false)
		, m_bShowDevices(false)
	{

	}

	// Overwritten virtual
	virtual void ParseParam(const TCHAR* pszParam,BOOL bFlag,BOOL bLast);
#ifdef _UNICODE
	virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast);
#endif

public:
	// Data
	CString m_strDevName;		// Device name from the command line to search for
	bool	m_bNoReset;			// No Reset of web cam
	bool	m_bNoGuard;			// Prevent a guard thread
	bool	m_bShowDevices;		// SHow message box with devicenames on open.

	// Command line options that allow controlling the cameras.
	int		m_iNumCamera{ 0 };		// If no camera is defined we use camer 0
	int		m_iZoom{ 0 };			// Zoom 1 (Zoom in), -1 (zoom out)
	int		m_iRestorePreset{ -1 };	// Restore to memory position 0-7
	int		m_iStorePreset{ -1 };	// Store to memory position 0-7
	int		m_iMovePan{ 0 };		// Move pan direction: 1 (right), -1 (left)
	int 	m_iMoveTilt{ 0 };		// Move tilt direction: -1 (down), 1 (up)	
	int		m_iMoveHome{ 0 };		// Move Home: 1 
	int		m_iNumSteps{ 1 };	// Number of steps to move (for move up/down/left/right)

	// Currently not used (may be used if we ant yes/no/undefined)
	enum class Mode
	{
		False = 0,
		True = 1,
		Undefined = -1,
	};
};

#ifdef _UNICODE
void CPTZControlCommandLineInfo::ParseParam(const TCHAR* pszParam,BOOL bFlag,BOOL bLast)
{
	ParseParam(CT2CA(pszParam),bFlag,bLast);
}
#endif

void CPTZControlCommandLineInfo::ParseParam(const char* pszParam,BOOL bFlag,BOOL bLast)
{
	if (bFlag)
	{
		if (_strnicmp(pszParam,"device:",7)==0)
		{
			// Get address set name
			pszParam += 7;
			m_strDevName = pszParam;
			::PathUnquoteSpaces(CStrBuf(m_strDevName,0));
		}
		else if (_stricmp(pszParam, "noreset")==0)
		{
			m_bNoReset = true;
		}
		else if (_stricmp(pszParam, "noguard") == 0)
		{
			m_bNoGuard = true;
		}
		else if (_stricmp(pszParam, "showdevices") == 0)
		{
			m_bShowDevices = true;
		}
		else if (isdigit(*pszParam))
		{
			// We have a number, so we set the camera number to use
			m_iNumCamera = atoi(pszParam)-1;
			if (m_iNumCamera<0 || m_iNumCamera>=CPTZControlDlg::NUM_MAX_WEBCAMS)
				theApp.SetRC(8);	// Command line error
		}
		else if (_strnicmp(pszParam, "n:", 2)==0)
		{
			if (isdigit(pszParam[2]))
			{
				m_iNumSteps = atoi(pszParam+2);
				if (m_iNumSteps<0)
					m_iNumSteps = 1;
			}
			else
				theApp.SetRC(8);	// Command line error
		}
		else if (_strnicmp(pszParam, "restore:", 8)==0)
		{
			if (isdigit(pszParam[8]))
			{
				m_iRestorePreset = atoi(pszParam+8)-1;
				if (m_iRestorePreset<0 || m_iRestorePreset>=CWebcamController::NUM_PRESETS)
					theApp.SetRC(8);	// Command line error
			}
			else
				theApp.SetRC(8);	// Command line error
		}
		else if (_strnicmp(pszParam, "store:", 6)==0)
		{
			if (isdigit(pszParam[6]))
			{
				m_iStorePreset = atoi(pszParam+6)-1;
				if (m_iStorePreset<0 || m_iStorePreset>=CWebcamController::NUM_PRESETS)
					theApp.SetRC(8);	// Command line error
			}
			else
				theApp.SetRC(8);	// Command line error
		}
		else if (_strnicmp(pszParam, "move", 4)==0)
		{
			pszParam += 4;
			if (_stricmp(pszParam, "_up")==0)
			{
				m_iMoveTilt = 1;
			}
			else if (_stricmp(pszParam, "_down")==0)
			{
				m_iMoveTilt = -1;
			}
			else if (_stricmp(pszParam, "_left")==0)
			{
				m_iMovePan = -1;
			}
			else if (_stricmp(pszParam, "_right")==0)
			{
				m_iMovePan = 1;
			}
			else if (_stricmp(pszParam, "_home")==0)
			{
				m_iMoveHome = 1;
			}
			else
				theApp.SetRC(8);	// Command line error
		}
		else if (_strnicmp(pszParam, "zoom", 4)==0)
		{
			pszParam += 4;
			if (_stricmp(pszParam, "_in")==0 ||
				_stricmp(pszParam, "+")==0)
			{
				m_iZoom = 1;
			}
			else if (_stricmp(pszParam, "_in")==0 ||
					 _stricmp(pszParam, "+")==0)
			{
				m_iZoom = -1;
			}
			else 
				theApp.SetRC(8);	// Command line error
		}
		else
			ParseParamFlag(pszParam);
	}
	else
		ParseParamNotFlag(pszParam);

	// Standard implementation
	ParseLast(bLast);
}


//////////////////////////////////////////////////////////////////////////
// CPTZControlApp

BEGIN_MESSAGE_MAP(CPTZControlApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
// CPTZControlApp construction

CPTZControlApp::CPTZControlApp()
	: m_bNoReset(false)
	, m_bNoGuard(false)
	, m_bShowDevices(false)
{
}


// The one and only CPTZControlApp object

CPTZControlApp theApp;


//////////////////////////////////////////////////////////////////////////
// CPTZControlApp initialization

BOOL CPTZControlApp::InitInstance()
{
	CWinApp::InitInstance();

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Taken from Logitech
	// PTZDemo\ConferenceCamPTZDemoDlg.cpp
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

//-------------Registry-------------------------------------------------

	SetRegistryKey(_T("MRi-Software"));

//-------------Commandline parsing--------------------------------------

// Parse command line for standard shell commands, DDE, file open
	CPTZControlCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	m_strDevName = cmdInfo.m_strDevName;

	// Registry is overruled command line
	m_bNoReset = GetProfileInt(REG_OPTIONS,REG_NORESET,FALSE)!=0 || cmdInfo.m_bNoReset;
	m_bNoGuard = GetProfileInt(REG_OPTIONS,REG_NOGUARD,FALSE)!=0 || cmdInfo.m_bNoGuard;
	m_bShowDevices = cmdInfo.m_bShowDevices;

//-------------Main ----------------------------------------------------

	auto& instance = CSingleInstance::Instance();

	// Check if we know an instance already. If not, we create the dialog. 
	// If there is an instance, we will try to execute a command on it.
	if (instance.Register())
	{
		// Create the Dialog
		m_pDlg = new CPTZControlDlg();
		if (m_pDlg->Create(CPTZControlDlg::IDD))
			m_pMainWnd = m_pDlg;
		else
			return FALSE;
	} 

	// If we have an instance we try to execute a command on it, if there is one.
	auto hWnd = instance.FindInstance(); 
	if (hWnd)
	{
		// We already have a window, so we check if we want to execute a command on it. 
		// Try to execute a command 
		if (cmdInfo.m_iMoveHome!=0)
		{
			if (::SendMessage(hWnd, WM_APP_COMMAND, cmdInfo.m_iNumCamera, 'H')==0)
				theApp.SetRC(16);	// Command error
		}
		else if (cmdInfo.m_iMovePan!=0)
		{
			if (::SendMessage(hWnd, WM_APP_COMMAND, cmdInfo.m_iNumCamera, MAKELPARAM(cmdInfo.m_iMovePan<0 ? 'L' : 'R', cmdInfo.m_iNumSteps))==0)
				theApp.SetRC(16);	// Command error
		}
		else if (cmdInfo.m_iMoveTilt!=0)
		{
			if (::SendMessage(hWnd, WM_APP_COMMAND, cmdInfo.m_iNumCamera, MAKELPARAM(cmdInfo.m_iMoveTilt<0 ? 'D' : 'U', cmdInfo.m_iNumSteps))==0)
				theApp.SetRC(16);	// Command error
		}
		else if (cmdInfo.m_iZoom!=0)
		{
			if (::SendMessage(hWnd, WM_APP_COMMAND, cmdInfo.m_iNumCamera, MAKELPARAM(cmdInfo.m_iZoom>0 ? '+' : '-', cmdInfo.m_iNumSteps))==0)
				theApp.SetRC(16);	// Command error
		}
		else if (cmdInfo.m_iRestorePreset!=-1)
		{
			if (::SendMessage(hWnd, WM_APP_COMMAND, cmdInfo.m_iNumCamera, MAKELPARAM('M', cmdInfo.m_iRestorePreset))==0)
				theApp.SetRC(16);	// Command error
		}
		else if (cmdInfo.m_iStorePreset!=-1)
		{
			if (::SendMessage(hWnd, WM_APP_COMMAND, cmdInfo.m_iNumCamera, MAKELPARAM('S', cmdInfo.m_iStorePreset))==0)
				theApp.SetRC(16);	// Command error
		}
	}

	// Succeeded
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

int CPTZControlApp::ExitInstance()
{
#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// Set a returncode if we executed a command.
	int iRC = __super::ExitInstance();
	if (m_iRC!= -1)
		iRC = m_iRC;
	return iRC;
}

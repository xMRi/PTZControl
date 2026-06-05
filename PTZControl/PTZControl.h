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

// PTZControl.h : main header file for the PROJECT_NAME application
//
#pragma once

#include "PTZControlDlg.h"

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

//////////////////////////////////////////////////////////////////////////

#define REG_WINDOW _T("Window")
#define REG_WINDOW_POSX		_T("X")
#define REG_WINDOW_POSY		_T("Y")
#define REG_TOOLTIP			_T("Tooltip%d")

#define REG_DEVICE						_T("Device")
#define REG_USELOGOTECHMOTIONCONTROL		_T("LogitechMotionControl")
#define REG_MOTORINTERVALTIMER				_T("MotorIntervalTimer")
#define REG_DEVICENAME						_T("DeviceName")

#define REG_OPTIONS	_T("Options")
#define REG_NORESET		_T("NoReset")
#define REG_NOGUARD		_T("NoGuard")

#define TIMER_FOCUS_CHECK			4711
#define TIMER_AUTO_REPEAT			4712
#define TIMER_CLEAR_MEMORY			4713
#define TIMER_HOTKEY_SWITCH_TO		4714	// Used for Hotkeys sequence
#define TIMER_HOTKEY_POSITION		4715
#define TIMER_HOTKEY_SWITCH_BACK	4716

#define TIMER_HOTKEY_DELAY_1		10		// Switch to camera
#define TIMER_HOTKEY_DELAY_2		10		// Set position
#define TIMER_HOTKEY_DELAY_3		750		// Switch back

#define FOCUS_CHECK_DELAY			250		// After 250msec we move the focus away from a button.
#define AUTO_REPEAT_DELAY			50		// Autorepeat is on the fastest possible delay of 50msec
#define AUTO_REPEAT_INITIAL_DELAY	500		// after 1/2 second we start autorepeat
#define CLEAR_MEMORY_DELAY			5000	// After 5 seconds clear the memory

#define COLOR_GREEN				RGB(0,240,0)
#define COLOR_RED				RGB(240,0,0)
#define COLOR_ORANGE			RGB(255,140,0)

#define WM_APP_COMMAND				(WM_APP+1)	
											// wParam : camera number, lParam: command and position
											// Commands: LOWORD
											//  ZoomIn= +, ZoomOut = -, 
											//  PanLeft = L, PanRight = R, 
											//  TiltUp = U, TiltDown = D, 
											//  Home = H, 
											//  Restore MemoryPos = M (HIWORD = memory position),
											//  Store MemoryPos = S (HIWORD = memory position)


//////////////////////////////////////////////////////////////////////////
// CPTZControlApp:
// See PTZControl.cpp for the implementation of this class
//

class CPTZControlApp : public CWinApp
{
public:
	CPTZControlApp();

	// Overrides
public:
	virtual BOOL InitInstance();

	void SetRC(int iRC)
	{
		if (iRC != -1)
			m_iRC = iRC;
	}

// Implementation

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();

public:
	// command line flags
	CString m_strDevName;		// Device name from the command line to search for
	bool	m_bNoReset;			// No Reset of web cam
	bool	m_bNoGuard;			// Prevent a guard thread
	bool	m_bShowDevices;

private:
	CPTZControlDlg* m_pDlg{};
	int		m_iRC{ -1 };		// Return code to return from the application. Default is 0, but can be set to other values on error.		
								// Command line errors have retcode 8. -1 used to use the default retcod
};


extern CPTZControlApp theApp;

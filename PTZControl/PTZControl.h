
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

// Allow the cameras with the following tags in the device name.
// This will match all Logitech PRT Pro, PTZ Pro 2 and Rally camers.
#define DEFAULT_DEVICE_NAME_1 _T("PTZ Pro")
#define DEFAULT_DEVICE_NAME_2 _T("Logi Rally")
#define DEFAULT_DEVICE_NAME_3 _T("Logi Group Camera")

#define TIMER_FOCUS_CHECK			4711
#define TIMER_AUTO_REPEAT			4712
#define TIMER_CLEAR_MEMORY			4713

#define FOCUS_CHECK_DELAY			250		// After 250msec we move the focus away from a button.
#define AUTO_REPEAT_DELAY			50		// Autorepeat is on the fastest possible delay of 50msec
#define AUTO_REPEAT_INITIAL_DELAY	500		// after 1/2 second we start autorepeat
#define CLEAR_MEMORY_DELAY			5000	// After 5 seconds clear the memory

#define COLOR_GREEN				RGB(0,240,0)
#define COLOR_RED				RGB(240,0,0)
#define COLOR_ORANGE			RGB(255,140,0)

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
	CPTZControlDlg* m_pDlg;
};

extern CPTZControlApp theApp;


// PTZControl.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
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

	// Create the Dialog
	m_pDlg = new CPTZControlDlg();
	if (m_pDlg->Create(CPTZControlDlg::IDD))
		m_pMainWnd = m_pDlg;
	else
		return FALSE;

	// Succeeded
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

int CPTZControlApp::ExitInstance()
{
#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	return __super::ExitInstance();
}

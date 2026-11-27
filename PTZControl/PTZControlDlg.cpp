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

// PTZControlDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "SingleInstance.h"

#include "PTZControl.h"
#include "PTZControlDlg.h"
#include "SettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//	Array of supported cameras
//		Allow the cameras with the following tags in the device name.
//		This will match all Logitech PRT Pro, PTZ Pro 2 and Rally cameras.
//		Also the ConferenceCam CC3000e Camera will be detected.
//		Remember: Just a partial token must match the name.
static const LPCTSTR g_aCameras[] = 
{
	_T("PTZ Pro"),
	_T("Logi Rally"),
	_T("ConferenceCam"),
	_T("Logi Group Camera"),
};

//////////////////////////////////////////////////////////////////////////////////////////
// We have several layouts for the Buttons
// 0-1 cameras (dialog will shrink by one column)
//		E |
//		S |
// 2 cameras
//		S E 
//		- -
//		1 2
// 3 cameras
//		1 E 
//		2 S
//		3 

struct SPosition
{
	UINT	nId;					// Id of button 
	bool	bShow;					// Show or hide
	int		x, y;					// outside raster, left two columns
};

struct SLayout
{
	int cxDelta;					// Num to shrink / grow the dialog
	const SPosition* pButtons;		// List of entries terminated with nId==0.
};

static const SPosition layoutBtns1[]	// 0 or 1 camera (uses only 1 column)
{
	IDC_BT_EXIT,		true,  0, 0,	// one columns
	IDC_BT_SETTINGS,	true,  0, 1,
	// Invisible
	IDC_BT_WEBCAM1,		false, 0, 0,
	IDC_BT_WEBCAM2,		false, 0, 0,
	IDC_BT_WEBCAM3,		false, 0, 0,
	0
};

static const SPosition layoutBtns2[]	// 2 cameras (uses 2 columns)
{
	IDC_BT_SETTINGS,	true,	0, 0,	// Top row
	IDC_BT_EXIT,		true,	1, 0,
	IDC_BT_WEBCAM1,		true,	0, 2,	// bottom row
	IDC_BT_WEBCAM2,		true,	1, 2,
	// Invisible
	IDC_BT_WEBCAM3,		false,  0, 0,
	0
};

static const SPosition layoutBtns3[]	// 3 cameras
{
	IDC_BT_EXIT,		true,	1, 0,	// Outer left column
	IDC_BT_SETTINGS,	true,	1, 1,
	IDC_BT_WEBCAM1,		true,	0, 0,	// all in one column left (top down)
	IDC_BT_WEBCAM2,		true,	0, 1,
	IDC_BT_WEBCAM3,		true,   0, 2,
	0
};

const SLayout g_layout[CPTZControlDlg::NUM_MAX_WEBCAMS+1] = 
{
	-1,	layoutBtns1,		// 0 or 1 camera (shrink dialog)
	-1,	layoutBtns1,		// 0 or 1 camera (shrink dialog)
	0,  layoutBtns2,		// 2 cameras	 (keep size)
	0,  layoutBtns3,		// 3 cameras	 (keep size)
};


//////////////////////////////////////////////////////////////////////////////////////////
//	Special new notification 

#define ON_BN_UNPUSHED(id, memberFxn) \
	ON_CONTROL(BN_UNPUSHED, id, memberFxn)

//////////////////////////////////////////////////////////////////////////////////////////
//	CPTZButton

IMPLEMENT_DYNAMIC(CPTZButton,CMFCButton)

CPTZButton::CPTZButton() 
	: m_bAutoRepeat(false)
	, m_uiSent(0)
{

}

void CPTZButton::PreSubclassWindow()
{
	__super::PreSubclassWindow();
	SetTooltip(_T(""));
	// Setting delay time doesn't seam to work.
	GetToolTipCtrl().SetDelayTime(0);
}

void CPTZButton::SetScalableImage(HICON hIcon)
{
	// Remember the button size at which the source image is drawn at its original size.
	CRect rectClient;
	GetClientRect(rectClient);
	m_sizeImageBaseClient = rectClient.Size();
	SetImage(hIcon);
}

void CPTZButton::OnDraw(CDC* pDC, const CRect& rect, UINT uiState)
{
	// Keep MFC's drawing for text buttons and buttons without a scalable image.
	if (m_sizeImageBaseClient.cx <= 0 || m_sizeImageBaseClient.cy <= 0 ||
		m_sizeImage.cx <= 0 || m_sizeImage.cy <= 0 || GetWindowTextLength() != 0)
	{
		__super::OnDraw(pDC, rect, uiState);
		return;
	}

	if (rect.IsRectEmpty())
		return;

	// Use one scale factor in both directions and fit inside the themed button border.
	CRect rectClient;
	GetClientRect(rectClient);
	const double scale = (std::min)({
		static_cast<double>(rectClient.Width()) / m_sizeImageBaseClient.cx,
		static_cast<double>(rectClient.Height()) / m_sizeImageBaseClient.cy,
		static_cast<double>(rect.Width()) / m_sizeImage.cx,
		static_cast<double>(rect.Height()) / m_sizeImage.cy
	});
	const CSize sizeImage(
		(std::max)(1, static_cast<int>(m_sizeImage.cx * scale + 0.5)),
		(std::max)(1, static_cast<int>(m_sizeImage.cy * scale + 0.5)));
	if (sizeImage == m_sizeImage)
	{
		__super::OnDraw(pDC, rect, uiState);
		return;
	}

	// Keep MFC's image selection and disabled drawing for each button state.
	const BOOL bGray = (uiState & ODS_DISABLED) && m_bGrayDisabled;
	CMFCToolBarImages& checkedImage = bGray && m_ImageCheckedDisabled.GetCount() != 0 ? m_ImageCheckedDisabled :
		m_bHighlighted && m_ImageCheckedHot.GetCount() != 0 ? m_ImageCheckedHot : m_ImageChecked;
	CMFCToolBarImages& normalImage = bGray && m_ImageDisabled.GetCount() != 0 ? m_ImageDisabled :
		m_bHighlighted && m_ImageHot.GetCount() != 0 ? m_ImageHot : m_Image;
	const bool useCheckedImage = m_bChecked && checkedImage.GetCount() != 0;
	CMFCToolBarImages& image = useCheckedImage ? checkedImage : normalImage;
	const BOOL bDrawGray = bGray && (useCheckedImage ? m_ImageCheckedDisabled : m_ImageDisabled).GetCount() == 0;

	// Stretch directly from MFC's original image and center it in the drawing area.
	CAfxDrawState drawState;
	if (image.PrepareDrawImage(drawState, sizeImage))
	{
		const int x = rect.left + (rect.Width() - sizeImage.cx) / 2;
		const int y = rect.top + (rect.Height() - sizeImage.cy) / 2;
		image.Draw(pDC, x, y, 0, FALSE, bDrawGray);
		image.EndDrawImage(drawState);
	}
}

void CPTZButton::OnDrawBorder(CDC* pDC, CRect& rectClient, UINT uiState)
{
	__super::OnDrawBorder(pDC,rectClient,uiState);

	// Give the button our button face. But only when the mouse isn't over
	if (m_bWinXPTheme && m_clrFace!=COLORREF(-1)/* && !m_bHover && !m_bHighlighted*/)
		pDC->FillSolidRect(rectClient, m_clrFace);
}

void CPTZButton::OnDrawFocusRect(CDC* pDC, const CRect& rectClient)
{
	UNUSED_ALWAYS(pDC); UNUSED_ALWAYS(rectClient);
}

void CPTZButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bAutoRepeat)
	{
		SetTimer(TIMER_AUTO_REPEAT, AUTO_REPEAT_INITIAL_DELAY, NULL);
		m_uiSent = 0;
	}
	__super::OnLButtonDown(nFlags, point);
}

void CPTZButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bAutoRepeat)
	{
		KillTimer(TIMER_AUTO_REPEAT);

		if (GetCapture() != NULL)
		{
			// If we never sent a message we do it once.
			if (m_uiSent==0 && (GetState() & BST_PUSHED) != 0)
				GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
			else
				GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_UNPUSHED), (LPARAM)m_hWnd);
			// release capture
			ReleaseCapture();
		}
	}
	else
		// Never call the default in auto repeat
		__super::OnLButtonUp(nFlags, point);
}

void CPTZButton::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent==TIMER_AUTO_REPEAT)
	{
		if (m_bAutoRepeat)
		{
			if ((GetState() & BST_PUSHED) == 0)
				return;
			++m_uiSent;
			SetTimer(TIMER_AUTO_REPEAT, AUTO_REPEAT_DELAY, NULL);
			GetParent()->SendMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
		}
	}

	__super::OnTimer(nIDEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////

static void AdjustVisibleWindowRect(LPRECT lpRect)
{
	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	const POINT pt{lpRect->left, lpRect->top};
	if (!::GetMonitorInfo(::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), &mi))
		return;

	const RECT& rcBase = mi.rcWork;

	// Check if rect is visible
	if (lpRect->bottom>rcBase.bottom)
		::OffsetRect(lpRect,0,rcBase.bottom-lpRect->bottom);
	if (lpRect->top<rcBase.top)
		::OffsetRect(lpRect,0,rcBase.top-lpRect->top);
	if (lpRect->right>rcBase.right)
		::OffsetRect(lpRect,rcBase.right-lpRect->right,0);
	if (lpRect->left<rcBase.left)
		::OffsetRect(lpRect,rcBase.left-lpRect->left,0);


}

//////////////////////////////////////////////////////////////////////////////////////////
// CPTZControlDlg dialog

CPTZControlDlg::CPTZControlDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PTZCONTROL_DIALOG, pParent)
	, m_hAccel(NULL)
	, m_iCurrentWebCam(0)
	, m_iNumWebCams(0)
	, m_evTerminating(FALSE,TRUE)
	, m_pGuardThread(nullptr)
	, m_iHotKeyCurrentCam{ 0 }
	, m_iHotKeyNextCam{ 0 }
	, m_iHotKeyPosition{ 0 }
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hAccel = ::LoadAccelerators(AfxFindResourceHandle(IDR_ACCELERATOR, RT_ACCELERATOR), MAKEINTRESOURCE(IDR_ACCELERATOR));
}

CPTZControlDlg::~CPTZControlDlg()
{
	for (auto &webcam : m_aWebCams)
		webcam.CloseDevice();
}

void CPTZControlDlg::PostNcDestroy()
{
	__super::PostNcDestroy();

	// Cleanup the guard thread.
	m_evTerminating.SetEvent();
	if (m_pGuardThread)
		::WaitForSingleObject(m_pGuardThread->m_hThread,INFINITE);
	m_pGuardThread = nullptr;

	// Finally delete the application.
	delete this;
}

BOOL CPTZControlDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return __super::OnCommand(wParam,lParam);
}


BOOL CPTZControlDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message>=WM_KEYFIRST && pMsg->message<=WM_KEYLAST)
	{
		if (m_hAccel)
		{
			if (::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
				return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CPTZControlDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BT_ZOOM_IN, m_btZoomIn);
	DDX_Control(pDX, IDC_BT_ZOOM_OUT, m_btZoomOut);
	DDX_Control(pDX, IDC_BT_UP, m_btUp);
	DDX_Control(pDX, IDC_BT_DOWN, m_btDown);
	DDX_Control(pDX, IDC_BT_HOME, m_btHome);
	DDX_Control(pDX, IDC_BT_LEFT, m_btLeft);
	DDX_Control(pDX, IDC_BT_RIGHT, m_btRight);
	DDX_Control(pDX, IDC_BT_MEMORY, m_btMemory);
	DDX_Control(pDX, IDC_BT_PRESET1, m_btPreset[0]);
	DDX_Control(pDX, IDC_BT_PRESET2, m_btPreset[1]);
	DDX_Control(pDX, IDC_BT_PRESET3, m_btPreset[2]);
	DDX_Control(pDX, IDC_BT_PRESET4, m_btPreset[3]);
	DDX_Control(pDX, IDC_BT_PRESET5, m_btPreset[4]);
	DDX_Control(pDX, IDC_BT_PRESET6, m_btPreset[5]);
	DDX_Control(pDX, IDC_BT_PRESET7, m_btPreset[6]);
	DDX_Control(pDX, IDC_BT_PRESET8, m_btPreset[7]);
	DDX_Control(pDX, IDC_BT_EXIT, m_btExit);
	DDX_Control(pDX, IDC_BT_SETTINGS, m_btSettings);
	DDX_Control(pDX, IDC_BT_WEBCAM1, m_btWebCam[0]);
	DDX_Control(pDX, IDC_BT_WEBCAM2, m_btWebCam[1]);
	DDX_Control(pDX, IDC_BT_WEBCAM3, m_btWebCam[2]);
}

BEGIN_MESSAGE_MAP(CPTZControlDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCHITTEST()
	ON_WM_SIZING()
	ON_WM_GETMINMAXINFO()
	ON_BN_CLICKED(IDC_BT_EXIT, &CPTZControlDlg::OnBtExit)
	ON_WM_SETFOCUS()
	ON_BN_CLICKED(IDC_BT_MEMORY, &CPTZControlDlg::OnBtMemory)
	ON_COMMAND_EX(IDC_BT_WEBCAM1, &CPTZControlDlg::OnBtWebCam)
	ON_COMMAND_EX(IDC_BT_WEBCAM2, &CPTZControlDlg::OnBtWebCam)
	ON_COMMAND_EX(IDC_BT_WEBCAM3, &CPTZControlDlg::OnBtWebCam)
	ON_COMMAND_EX(IDC_BT_PRESET1, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET2, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET3, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET4, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET5, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET6, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET7, &CPTZControlDlg::OnBtPreset)
	ON_COMMAND_EX(IDC_BT_PRESET8, &CPTZControlDlg::OnBtPreset)
	ON_BN_CLICKED(IDC_BT_ZOOM_IN, &CPTZControlDlg::OnBtZoomIn)
	ON_BN_CLICKED(IDC_BT_ZOOM_OUT, &CPTZControlDlg::OnBtZoomOut)
	ON_BN_CLICKED(IDC_BT_UP, &CPTZControlDlg::OnBtUp)
	ON_BN_CLICKED(IDC_BT_DOWN, &CPTZControlDlg::OnBtDown)
	ON_BN_CLICKED(IDC_BT_LEFT, &CPTZControlDlg::OnBtLeft)
	ON_BN_CLICKED(IDC_BT_HOME, &CPTZControlDlg::OnBtHome)
	ON_BN_CLICKED(IDC_BT_RIGHT, &CPTZControlDlg::OnBtRight)
	ON_BN_CLICKED(IDC_BT_SETTINGS, &CPTZControlDlg::OnBtSettings)
	ON_BN_UNPUSHED(IDC_BT_UP, &CPTZControlDlg::OnBtUnpushed)
	ON_BN_UNPUSHED(IDC_BT_DOWN, &CPTZControlDlg::OnBtUnpushed)
	ON_BN_UNPUSHED(IDC_BT_LEFT, &CPTZControlDlg::OnBtUnpushed)
	ON_BN_UNPUSHED(IDC_BT_RIGHT, &CPTZControlDlg::OnBtUnpushed)
	ON_WM_TIMER()
	ON_WM_HOTKEY()
	ON_MESSAGE(WM_APP_COMMAND, &CPTZControlDlg::OnAppCommand)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////////////////////
// CPTZControlDlg message handlers

void CPTZControlDlg::ResetAllColors()
{
	for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetWindow(GW_HWNDNEXT))
	{
		auto *pButton = DYNAMIC_DOWNCAST(CPTZButton, pWnd);

		// Don't reset color for web cam button
		bool bIsWebCamBtn = false;
		for (auto &btn : m_btWebCam)
			bIsWebCamBtn |= pButton==&btn;

		// Reset color for all other buttons
		if (pButton && !bIsWebCamBtn)
			pButton->SetFaceColor(COLORREF(-1), TRUE);
	}
}

void CPTZControlDlg::ResetMemButton()
{
	// Clear the mem button
	KillTimer(TIMER_CLEAR_MEMORY);
	m_btMemory.SetCheck(0);
	m_btMemory.SetFaceColor(COLORREF(-1));
}

CWebcamController& CPTZControlDlg::GetCurrentWebCam()
{
	return m_aWebCams[m_iCurrentWebCam];
}

void CPTZControlDlg::SetActiveCam(int iCam)
{
	if (iCam >= 0 && iCam < m_iNumWebCams)
	{
		// Clear mem button
		ResetMemButton();

		// Save the colors of the current buttons and set the old ones
		for (CWnd* pWnd = GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetWindow(GW_HWNDNEXT))
		{
			auto* pButton = DYNAMIC_DOWNCAST(CPTZButton, pWnd);
			if (pButton)
			{
				// Save the color of the current buttons for the current web cam and get 
				// the saved color from the map we have for the new cam.
				auto nId = pButton->GetDlgCtrlID();
				m_aMapBtnColors[m_iCurrentWebCam][nId] = pButton->GetFaceColor();
				auto it = m_aMapBtnColors[iCam].find(nId);
				if (it!=m_aMapBtnColors[iCam].end())
					pButton->SetFaceColor(it->second);
			}
		}

		// Set the tooltips
		for (int i=0; i<CWebcamController::NUM_PRESETS; ++i)
			m_btPreset[i].SetTooltip(m_strTooltips[iCam][i]);

		// Set the new webcam
		m_iCurrentWebCam = iCam;
		auto Enable = [&](CPTZButton &btn, bool bActive)
		{
			btn.SetCheck(bActive);
			btn.SetFaceColor(bActive ? COLOR_ORANGE : -1, TRUE);
		};
		int iWebCam = 0;
		for (auto &btn : m_btWebCam)
			Enable(btn,m_iCurrentWebCam==iWebCam++);
	}
}

//////////////////////////////////////////////////////////////////////////
//	It seams that in some cases a camera my block.
//	This thread should help that the blocking thread is detected. and the
//	application terminates.

UINT AFX_CDECL CPTZControlDlg::GuardThread(LPVOID p)
{
	auto *pWnd = static_cast<CPTZControlDlg*>(p);

	// This thread end when the application exists.
	// For ever all 2000 seconds check if the application still runs and accepts messages.
	for (;;)
	{
		// Check if we have an event, wait for 1 sec
		CSingleLock lock(&pWnd->m_evTerminating,FALSE);
		if (lock.Lock(1000))
			// Event is set.
			return 0;
		
		DWORD_PTR dwResult = 0;		
		// Check if the application is blocking
		if (::SendMessageTimeout(pWnd->GetSafeHwnd(), WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_NORMAL, 1000, &dwResult)==0)
		{
			// Exit the current process
			::ExitProcess(10);
		}			
	}
}

BOOL CPTZControlDlg::OnInitDialog()
{
	__super::OnInitDialog();

	// Resource layout entries would otherwise react to the initial dialog resize.
	EnableDynamicLayout(FALSE);

	//---------------------------------------------------------------------
	// INIT MFC STUFF

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Load the bitmap
	{
		CMFCToolBarImages	images;
		images.SetImageSize(CSize(16, 16));
		images.Load(IDB_BUTTONS);

		int iImage = 0;
		CPTZButton* apButtons[] =
		{
			&m_btDown,
			&m_btLeft,
			&m_btRight,
			&m_btUp,
			&m_btHome,
			&m_btZoomIn,
			&m_btZoomOut,
			&m_btMemory,
			&m_btPreset[0],
			&m_btPreset[1],
			&m_btPreset[2],
			&m_btPreset[3],
			&m_btPreset[4],
			&m_btPreset[5],
			&m_btPreset[6],
			&m_btPreset[7],
			&m_btExit,
			&m_btSettings,
			&m_btWebCam[0],
			&m_btWebCam[1],
			&m_btWebCam[2],
		};
		for (auto* pBtn : apButtons)
		{
			HICON hIcon = images.ExtractIcon(iImage++);
			// Keep the resource text if the image could not be loaded.
			if (hIcon == nullptr)
				continue;

			pBtn->SetScalableImage(hIcon);
			pBtn->SetWindowText(_T(""));
		}
	}

	// Set auto repeat buttons
	m_btLeft.SetAutoRepeat(true);
	m_btRight.SetAutoRepeat(true);
	m_btDown.SetAutoRepeat(true);
	m_btUp.SetAutoRepeat(true);
	m_btZoomIn.SetAutoRepeat(true);
	m_btZoomOut.SetAutoRepeat(true);

	// This is a check box style
	m_btMemory.SetCheckStyle();
	for (auto &btn : m_btWebCam)
		btn.SetCheckStyle();

	//----------------------------------------------------------------------
	// Save the current instance

	CSingleInstance& instance = CSingleInstance::Instance();
	instance.SetInstanceWindow(this);

	//---------------------------------------------------------------------
	// INIT AND FIND WEB CAMS
	// 
	// Try to find the Device list
	CStringArray aDevices;
	CWebcamController::ListDevices(aDevices);

	auto OpenWebCam = [](CWebcamController& webCam, CString strDevToken)->bool
	{
		HRESULT hr = webCam.OpenDevice(CComBSTR(strDevToken), 0, 0);
		if (FAILED(hr))
		{
			AfxMessageBox(IDP_ERR_OPENFAILED);
			return false;
		}

		//	Load the default setting
		webCam.UseLogitechMotionControl(theApp.GetProfileInt(REG_DEVICE, REG_USELOGOTECHMOTIONCONTROL, FALSE) != 0);
		webCam.SetMotorIntervalTimer(theApp.GetProfileInt(REG_DEVICE, REG_MOTORINTERVALTIMER, DEFAULT_MOTOR_INTERVAL_TIMER));
		return true;
	};

	// Find the devices to search for. We have some default devices if no other is set in 
	// the registry o on the command line.
	CStringArray aStrCameraNameToSearch;
	for (const auto *p : g_aCameras)
		aStrCameraNameToSearch.Add(p);
	CString strCameraNameToSearch = theApp.GetProfileString(REG_DEVICE, REG_DEVICENAME, _T(""));
	if (!strCameraNameToSearch.IsEmpty())
		aStrCameraNameToSearch.Add(strCameraNameToSearch);
	if (!theApp.m_strDevName.IsEmpty())
		aStrCameraNameToSearch.Add(theApp.m_strDevName);

	// Search for the PTZ PRO 2
	CString strDevToken, strCameras;
	for (int i = 0; i<aDevices.GetCount() && m_iNumWebCams<NUM_MAX_WEBCAMS; ++i)
	{
		CString strDevice = aDevices[i], strCameraName, strCameraDevice;
		int iPos = strDevice.Find(_T('\t'));
		if (iPos != -1)
		{
			// Get name and device token
			strCameraName = strDevice.Left(iPos);
			strDevToken = strDevice.Mid(iPos + 1);

			// Build list of names found
			if (!strCameras.IsEmpty())
				strCameras += _T("\r\n");
			strCameras += strCameraName;

			// Check if the name matches
			for (int j=0; j<aStrCameraNameToSearch.GetCount(); ++j)
			{
				// Check if we have a asterisk. Or a partial name match.
				if (aStrCameraNameToSearch[j]==_T("*") || strCameraName.Find(aStrCameraNameToSearch[j]) != -1)
				{
					if (OpenWebCam(m_aWebCams[m_iNumWebCams], strDevToken))
						++m_iNumWebCams;
				}
			}
		}
	}
	
	// save the camera names.
	m_strCameraDeviceNames = strCameras;

	// On SHIFT and CTRL key down we show all found camera device names.
	if (theApp.m_bShowDevices || (::GetAsyncKeyState(VK_SHIFT) & 0x8000) && (::GetAsyncKeyState(VK_CONTROL) & 0x8000))
	{
		CString strMsg(MAKEINTRESOURCE(IDP_TXT_CAMERAS)), strText;
		strText.FormatMessage(strMsg, strCameras.GetString());
		AfxMessageBox(strText);
	}

	// Check how many web cams we found
	if (m_iNumWebCams==0)
	{
		// If we have no cam, show message
		AfxMessageBox(IDP_ERR_NO_CAMERA, MB_ICONERROR);
	}

	//-------------Register hotkeys-----------------------------------------
	
	for (int iWebCam = 0; iWebCam<(m_iNumWebCams==0 ? 1 : m_iNumWebCams); ++iWebCam)
	{
		for (int iPreset = 0; iPreset<CWebcamController::NUM_PRESETS; ++iPreset)
		{
			if (::RegisterHotKey(
					GetSafeHwnd(),
					10*(iWebCam+1)+iPreset,
					(iWebCam==0 ? (MOD_WIN) : 			 
					 iWebCam==1 ? (MOD_WIN|MOD_CONTROL) :
					 iWebCam==2 ? (MOD_WIN|MOD_ALT) : 0),
					 VK_NUMPAD0+iPreset)==0)
			{
				auto dwError = ::GetLastError();
				TRACE(_T(__FUNCTION__) _T(" RegisterHotKey failed with %d"),dwError);
			}
		}
	}

	//---------------------------------------------------------------------
	// ADJUST THE DIALOG

	// Calculate the with an position of the Grid.
	CRect rectBtn11, rectBtn12, rectBtn21;
	m_btWebCam[0].GetWindowRect(rectBtn11);
	m_btWebCam[1].GetWindowRect(rectBtn12);
	m_btExit.GetWindowRect(rectBtn21);
	ScreenToClient(rectBtn11);
	ScreenToClient(rectBtn12);
	ScreenToClient(rectBtn21);

	CPoint pointBase { rectBtn11.TopLeft() };
	CSize sizeRaster { rectBtn21.left-rectBtn11.left, rectBtn12.top-rectBtn11.top };

	// Get the required layout
	const auto &layout = g_layout[m_iNumWebCams];

	for (const auto* pLayoutBtn = layout.pButtons; pLayoutBtn->nId; ++pLayoutBtn)
	{
		CWnd *pWnd = GetDlgItem(pLayoutBtn->nId);
		// Move the button and hide or show the button
		pWnd->SetWindowPos(nullptr, 
						   pointBase.x + pLayoutBtn->x * sizeRaster.cx, pointBase.y + pLayoutBtn->y * sizeRaster.cy, 
						   0, 0, SWP_NOSIZE|SWP_NOZORDER|(pLayoutBtn->bShow ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)			
		);
		pWnd->EnableWindow(pLayoutBtn->bShow);
	}

	// Keep the native resize frame without a caption and preserve the client size.
	CRect rectClientBefore, rectClientAfter;
	GetClientRect(rectClientBefore);
	ModifyStyle(WS_CAPTION, WS_THICKFRAME, SWP_FRAMECHANGED);
	GetClientRect(rectClientAfter);

	// First Center
	CenterWindow();

	// Adjust the window
	CRect rect;
	GetWindowRect(rect);
	CPoint pt = rect.TopLeft();
	rect.OffsetRect(-pt);
	pt.x = theApp.GetProfileInt(REG_WINDOW, REG_WINDOW_POSX, pt.x);
	pt.y = theApp.GetProfileInt(REG_WINDOW, REG_WINDOW_POSY, pt.y);
	rect.OffsetRect(pt);
	// Apply the camera-dependent width and compensate for the frame change.
	rect.right += layout.cxDelta * sizeRaster.cx + rectClientBefore.Width() - rectClientAfter.Width();
	rect.bottom += rectClientBefore.Height() - rectClientAfter.Height();
	AdjustVisibleWindowRect(rect);

	// Move it
	SetWindowPos(&CWnd::wndTopMost, rect.left, rect.top, rect.Width(), rect.Height(), 0);

	// Set the tooltips for all presets
	for (int i = 0; i < CWebcamController::NUM_PRESETS; ++i)
	{
		for (int j = 0; j < CPTZControlDlg::NUM_MAX_WEBCAMS; ++j)
		{
			CString str;
			str.Format(REG_TOOLTIP, i + 1 + j*100);
			m_strTooltips[j][i] = theApp.GetProfileString(REG_WINDOW, str);
		}
	}

	EnableToolTips(TRUE);

	// Start a guard thread that takes care about a blocking app.
	if (!theApp.m_bNoGuard)	
		m_pGuardThread = AfxBeginThread(&GuardThread,this);

	// Set a time to move the focus to the parent window
	SetTimer(TIMER_FOCUS_CHECK,FOCUS_CHECK_DELAY,nullptr);
	SetFocus();

	// Move all cams to home position. And WebCam 0 will be the active one.
	if (theApp.m_bNoReset)
	{
		// Activate camera 0.
		if (m_iNumWebCams>0)
			SetActiveCam(0);
	}
	else
	{
		// Reset the cameras to home position. and leave the first camera 
		// (index 0) the active one ((loop backwards).
		for (int i = m_iNumWebCams; i > 0; --i)
		{
			SetActiveCam(i - 1);
			OnBtHome();
		}
	}

	// Register the buttons at the default size before applying the saved size.
	InitButtonLayout();
	RestoreWindowSize();
	return FALSE;
}

void CPTZControlDlg::InitButtonLayout()
{
	CRect rectClient;
	GetClientRect(rectClient);
	if (rectClient.IsRectEmpty())
		return;

	// Keep the default client size for scaling and the outer size for the resize limit.
	m_sizeLayoutClient = rectClient.Size();
	CRect rectWindow;
	GetWindowRect(rectWindow);
	m_sizeMinWindow = rectWindow.Size();

	// Start with a fresh layout manager after the startup layout is complete.
	EnableDynamicLayout(TRUE);
	CMFCDynamicLayout* pLayout = GetDynamicLayout();
	if (!pLayout->Create(this))
		return;

	// Walk the child controls and register each button for proportional movement and resizing.
	for (HWND hChild = ::GetWindow(m_hWnd, GW_CHILD); hChild != nullptr; hChild = ::GetWindow(hChild, GW_HWNDNEXT))
	{
		// Keep other controls out of the button layout, and include hidden camera buttons.
		TCHAR szClass[16];
		if (!::GetClassName(hChild, szClass, _countof(szClass)) ||
			_tcsicmp(szClass, _T("Button")) != 0)
		{
			continue;
		}

		CRect rectButton;
		::GetWindowRect(hChild, rectButton);
		// Use the dialog client origin for the button position.
		ScreenToClient(rectButton);

		// Derive the MFC percentages from the button bounds in the default layout.
		const int moveX = MulDiv(rectButton.left, 100, rectClient.Width());
		const int moveY = MulDiv(rectButton.top, 100, rectClient.Height());
		// Round the edges so neighboring buttons scale their shared edge alike.
		const int sizeX = MulDiv(rectButton.right, 100, rectClient.Width()) - moveX;
		const int sizeY = MulDiv(rectButton.bottom, 100, rectClient.Height()) - moveY;

		// Register the button by its window handle.
		VERIFY(pLayout->AddItem(hChild,
			CMFCDynamicLayout::MoveHorizontalAndVertical(moveX, moveY),
			CMFCDynamicLayout::SizeHorizontalAndVertical(sizeX, sizeY)));
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon. For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPTZControlDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		__super::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPTZControlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPTZControlDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// Let Windows calculate the standard resize border first.
	// For both forms of WM_NCCALCSIZE the proposed RECT comes first.
	RECT& rectClient = *reinterpret_cast<RECT*>(lpncsp);
	const RECT rectWindow = rectClient;
	__super::OnNcCalcSize(bCalcValidRects, lpncsp);

	// Keep the native border and reclaim any extra strip above the client area.
	if (GetStyle() & WS_THICKFRAME)
	{
		const LONG topBorder = (std::max)(1, MulDiv(1, ::GetDpiForWindow(m_hWnd), 96));
		rectClient.top = (std::min)(rectClient.top, rectWindow.top + topBorder);
	}
}

LRESULT CPTZControlDlg::OnNcHitTest(CPoint point)
{
	// Preserve any hit outside the client area reported by Windows.
	const LRESULT hit = __super::OnNcHitTest(point);
	if (hit != HTCLIENT)
		return hit;

	// The thin top border needs a wider resize grip inside the client area.
	if (GetStyle() & WS_THICKFRAME)
	{
		CRect rect;
		GetWindowRect(rect);
		const int grip = (std::max)(1, MulDiv(4, ::GetDpiForWindow(m_hWnd), 96));
		if (point.y < rect.top + grip)
			return HTTOP;
	}

	// Drag the window from the background between buttons.
	return HTCAPTION;
}

void CPTZControlDlg::RestoreWindowSize()
{
	// Read the last client size. Older settings may contain only the window position.
	const int cxSaved = theApp.GetProfileInt(REG_WINDOW, REG_WINDOW_CLIENT_WIDTH, 0);
	const int cySaved = theApp.GetProfileInt(REG_WINDOW, REG_WINDOW_CLIENT_HEIGHT, 0);
	if (cxSaved <= 0 || cySaved <= 0 || m_sizeLayoutClient.cx <= 0 || m_sizeLayoutClient.cy <= 0)
		return;

	int cxClient = cxSaved;
	int cyClient = cySaved;
	// Check whether the saved size fits the current aspect ratio.
	const int cyForSavedWidth = MulDiv(cxSaved, m_sizeLayoutClient.cy, m_sizeLayoutClient.cx);
	if (cyForSavedWidth == -1)
		return;

	// A different camera layout can change the aspect ratio. Keep the saved height.
	if (std::abs(cyForSavedWidth - cySaved) > 1)
		cxClient = MulDiv(cySaved, m_sizeLayoutClient.cx, m_sizeLayoutClient.cy);

	// Keep the default size if the saved size is too small for this layout.
	if (cxClient < m_sizeLayoutClient.cx || cyClient < m_sizeLayoutClient.cy)
		return;

	CRect rect;
	GetWindowRect(rect);
	// Add the client size difference to the outer window rectangle.
	rect.right += cxClient - m_sizeLayoutClient.cx;
	rect.bottom += cyClient - m_sizeLayoutClient.cy;
	AdjustVisibleWindowRect(rect);
	SetWindowPos(nullptr, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void CPTZControlDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	__super::OnGetMinMaxInfo(lpMMI);
	// Use the larger of the Windows minimum and the default dialog size.
	lpMMI->ptMinTrackSize.x = (std::max)(lpMMI->ptMinTrackSize.x, m_sizeMinWindow.cx);
	lpMMI->ptMinTrackSize.y = (std::max)(lpMMI->ptMinTrackSize.y, m_sizeMinWindow.cy);
}

void CPTZControlDlg::OnSizing(UINT nSide, LPRECT lpRect)
{
	__super::OnSizing(nSide, lpRect);

	if (m_sizeLayoutClient.cx <= 0 || m_sizeLayoutClient.cy <= 0)
		return;

	CRect rectWindow, rectClient;
	GetWindowRect(rectWindow);
	GetClientRect(rectClient);
	const int cxFrame = rectWindow.Width() - rectClient.Width();
	const int cyFrame = rectWindow.Height() - rectClient.Height();
	// Convert the proposed outer dimensions to client dimensions before scaling.
	const int cxProposed = lpRect->right - lpRect->left - cxFrame;
	const int cyProposed = lpRect->bottom - lpRect->top - cyFrame;

	const double cxBase = m_sizeLayoutClient.cx;
	const double cyBase = m_sizeLayoutClient.cy;
	// Use the dragged dimension for an edge, or both dimensions for a corner.
	double scale;
	if (nSide == WMSZ_LEFT || nSide == WMSZ_RIGHT)
		scale = cxProposed / cxBase;
	else if (nSide == WMSZ_TOP || nSide == WMSZ_BOTTOM)
		scale = cyProposed / cyBase;
	else
	{
		// Project the dragged corner onto the line of proportional sizes.
		scale = (cxProposed * cxBase + cyProposed * cyBase) /
			(cxBase * cxBase + cyBase * cyBase);
	}

	// Respect both the default size and the Windows tracking limits.
	const double minScaleX = (GetSystemMetrics(SM_CXMINTRACK) - cxFrame) / cxBase;
	const double minScaleY = (GetSystemMetrics(SM_CYMINTRACK) - cyFrame) / cyBase;
	scale = (std::max)({scale, 1.0, minScaleX, minScaleY});

	// Convert the scaled client size back to outer window dimensions.
	const int cxWindow = static_cast<int>(cxBase * scale + 0.5) + cxFrame;
	const int cyWindow = static_cast<int>(cyBase * scale + 0.5) + cyFrame;

	// Move the dragged edges and leave the opposite corner anchored.
	if (nSide == WMSZ_LEFT || nSide == WMSZ_TOPLEFT || nSide == WMSZ_BOTTOMLEFT)
		lpRect->left = lpRect->right - cxWindow;
	else
		lpRect->right = lpRect->left + cxWindow;

	if (nSide == WMSZ_TOP || nSide == WMSZ_TOPLEFT || nSide == WMSZ_TOPRIGHT)
		lpRect->top = lpRect->bottom - cyWindow;
	else
		lpRect->bottom = lpRect->top + cyWindow;
}

void CPTZControlDlg::OnClose()
{
	CRect rect;
	GetWindowRect(rect);
	// Save the position and client size for the next start.
	theApp.WriteProfileInt(REG_WINDOW,REG_WINDOW_POSX,rect.left);
	theApp.WriteProfileInt(REG_WINDOW,REG_WINDOW_POSY,rect.top);

	CRect rectClient;
	GetClientRect(rectClient);
	if (!rectClient.IsRectEmpty())
	{
		// Save client dimensions so the frame does not affect the restored size.
		theApp.WriteProfileInt(REG_WINDOW, REG_WINDOW_CLIENT_WIDTH, rectClient.Width());
		theApp.WriteProfileInt(REG_WINDOW, REG_WINDOW_CLIENT_HEIGHT, rectClient.Height());
	}

//-------------Register hotkeys-----------------------------------------

	for (int iWebCam = 0; iWebCam<(m_iNumWebCams==0 ? 1 : m_iNumWebCams); ++iWebCam)
	{
		for (int iPreset = 0; iPreset<CWebcamController::NUM_PRESETS; ++iPreset)
		{
			::UnregisterHotKey(
				GetSafeHwnd(),
				10*(iWebCam+1)+iPreset
			);
		}
	}

	DestroyWindow();
}

void CPTZControlDlg::OnBtExit()
{
	PostMessage(WM_CLOSE);
}


void CPTZControlDlg::OnOK()
{
	// Ignore
}


void CPTZControlDlg::OnCancel()
{
	// Ignore
}



void CPTZControlDlg::OnBtMemory()
{
	ResetAllColors();
	bool bCheck = !m_btMemory.GetCheck();
	m_btMemory.SetCheck(bCheck);
	if (bCheck)
	{
		// Remember the mem button, but clear it after some delay.
		m_btMemory.SetFaceColor(COLOR_RED, TRUE);
		SetTimer(TIMER_CLEAR_MEMORY,CLEAR_MEMORY_DELAY,nullptr);
	}
	else
		m_btMemory.SetFaceColor(COLORREF(-1),TRUE);
}

BEGIN_MESSAGE_MAP(CPTZButton, CMFCButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


BOOL CPTZControlDlg::OnBtPreset(UINT nId)
{
	UINT uiPreset = 0;
	while (uiPreset<CWebcamController::NUM_PRESETS)
	{
		if (static_cast<UINT>(m_btPreset[uiPreset].GetDlgCtrlID())==nId)
			break;
		++uiPreset;
	}

	if (uiPreset<CWebcamController::NUM_PRESETS)
	{
		ResetAllColors();
		bool bStore = m_btMemory.GetCheck();
		if (bStore)
		{
			// Save as new preset
			GetCurrentWebCam().SavePreset(uiPreset);
			ResetMemButton();
		}
		else
			GetCurrentWebCam().GotoPreset(uiPreset);

		// Set color
		STATIC_DOWNCAST(CPTZButton,GetDlgItem(nId))->SetFaceColor(COLOR_GREEN,TRUE);
	}
	return TRUE;
}

void CPTZControlDlg::OnHotKey(UINT nId, UINT /*nMod*/, UINT /*nKey*/)
{
	m_iHotKeyCurrentCam = m_iCurrentWebCam;
	m_iHotKeyNextCam = nId/10-1; // Base 10 = webcam 1
	m_iHotKeyPosition = nId%10;  // Position 0 = home, 1=mem pos1, etc.	
	SetTimer(TIMER_HOTKEY_SWITCH_TO, TIMER_HOTKEY_DELAY_1,nullptr);
}

BOOL CPTZControlDlg::OnBtWebCam(UINT nId)
{
	SetActiveCam(nId==IDC_BT_WEBCAM1 ? 0 :
				 nId==IDC_BT_WEBCAM2 ? 1 : 2);
	return 1;
}

void CPTZControlDlg::OnBtHome()
{
	ResetAllColors();
	GetCurrentWebCam().GotoHome();
	m_btHome.SetFaceColor(COLOR_GREEN,TRUE);
}


void CPTZControlDlg::OnBtZoomIn()
{
	ResetAllColors();
	GetCurrentWebCam().Zoom(1);
}


void CPTZControlDlg::OnBtZoomOut()
{
	ResetAllColors();
	GetCurrentWebCam().Zoom(-1);
}


void CPTZControlDlg::OnBtDown()
{
	ResetAllColors();
	if (m_btDown.InAutoRepeat())
		GetCurrentWebCam().Tilt(-1);
	else
		GetCurrentWebCam().MoveTilt(-1);
}

void CPTZControlDlg::OnBtUp()
{
	ResetAllColors();
	if (m_btUp.InAutoRepeat())
		GetCurrentWebCam().Tilt(1);
	else
		GetCurrentWebCam().MoveTilt(1);
}


void CPTZControlDlg::OnBtLeft()
{
	ResetAllColors();
	if (m_btLeft.InAutoRepeat())
		GetCurrentWebCam().Pan(-1);
	else
		GetCurrentWebCam().MovePan(-1);
}


void CPTZControlDlg::OnBtRight()
{
	ResetAllColors();
	if (m_btRight.InAutoRepeat())
		GetCurrentWebCam().Pan(1);
	else
		GetCurrentWebCam().MovePan(1);
}


void CPTZControlDlg::OnSetFocus(CWnd* pOldWnd)
{
	// Do nothing. Never set the focus to a child.
	UNUSED_ALWAYS(pOldWnd);
}



void CPTZControlDlg::OnTimer(UINT_PTR nIDEvent)
{
	// Timer seuqnce 
	// - TIMER_HOTKEY_SWITCH_TO
	// - TIMER_HOTKEY_POSITION
	// - TIMER_HOTKEY_SWITCH_BACK
	if (nIDEvent==TIMER_HOTKEY_SWITCH_TO)
	{
		KillTimer(TIMER_HOTKEY_SWITCH_TO);
		SetActiveCam(m_iHotKeyNextCam);
		SetTimer(TIMER_HOTKEY_POSITION,TIMER_HOTKEY_DELAY_2,nullptr);
	}
	else if (nIDEvent==TIMER_HOTKEY_POSITION)
	{
		KillTimer(TIMER_HOTKEY_POSITION);
		if (m_iHotKeyPosition>=0 && m_iHotKeyPosition<CWebcamController::NUM_PRESETS+1)
		{
			if (m_iHotKeyPosition==0)
				CPTZControlDlg::OnBtHome();
			else
				CPTZControlDlg::OnBtPreset(m_btPreset[m_iHotKeyPosition-1].GetDlgCtrlID());
		}
		SetTimer(TIMER_HOTKEY_SWITCH_BACK,TIMER_HOTKEY_DELAY_3,nullptr);
	}
	else if (nIDEvent==TIMER_HOTKEY_SWITCH_BACK)
	{
		KillTimer(TIMER_HOTKEY_SWITCH_BACK);
		SetActiveCam(m_iHotKeyCurrentCam);
	}
	else if (nIDEvent == TIMER_FOCUS_CHECK)
	{
		CWnd* pWndFocus = GetFocus();
		if (pWndFocus && 
			pWndFocus->GetParent()==this && 
			(GetAsyncKeyState(VK_LBUTTON) & 0x8000)==0)
			// only move the focus, when a button has the focus and the mouse is not down.
			SetFocus();
	}
	else if (nIDEvent == TIMER_CLEAR_MEMORY)
	{
		// Clear the mem button after some delay
		ResetMemButton();
	}
	
	__super::OnTimer(nIDEvent);
}

void CPTZControlDlg::OnBtUnpushed()
{
	GetCurrentWebCam().Pan(0);
	GetCurrentWebCam().Tilt(0);
}

void CPTZControlDlg::OnBtSettings()
{
	CSettingsDlg dlg;
	dlg.m_strCameraName = m_strCameraDeviceNames;
	dlg.m_strCameraName.Replace(_T("\r\n"), _T(", "));
	dlg.m_bLogitechCameraControl = GetCurrentWebCam().UseLogitechMotionControl();
	dlg.m_iMotorIntervalTimer = GetCurrentWebCam().GetMotorIntervalTimer();

	// Get a copy of the tooltips
	for (int i = 0; i < CWebcamController::NUM_PRESETS; ++i)
	{
		for (int j = 0; j < CPTZControlDlg::NUM_MAX_WEBCAMS; ++j)
		{
			dlg.m_strTooltip[j][i] = m_strTooltips[j][i];
		}
	}

	if (dlg.DoModal()!=IDOK)
		return;

	// Copy back and save
	for (int i = 0; i < CWebcamController::NUM_PRESETS; ++i)
	{
		for (int j = 0; j < CPTZControlDlg::NUM_MAX_WEBCAMS; ++j)
		{
			m_strTooltips[j][i] = dlg.m_strTooltip[j][i];
			CString str;
			str.Format(REG_TOOLTIP, i + 1 + j*100);
			theApp.WriteProfileString(REG_WINDOW, str, m_strTooltips[j][i]);
		}
	}

	// Camera control
	GetCurrentWebCam().UseLogitechMotionControl(dlg.m_bLogitechCameraControl!=0);
	theApp.WriteProfileInt(REG_DEVICE, REG_USELOGOTECHMOTIONCONTROL, dlg.m_bLogitechCameraControl);
	GetCurrentWebCam().SetMotorIntervalTimer(dlg.m_iMotorIntervalTimer);
	theApp.WriteProfileInt(REG_DEVICE, REG_USELOGOTECHMOTIONCONTROL, dlg.m_bLogitechCameraControl);

	// Set tooltips again
	SetActiveCam(m_iCurrentWebCam);
}

LRESULT CPTZControlDlg::OnAppCommand(WPARAM wParam, LPARAM lParam)
{
	// Check if we know this camera.
	int iCamera = LOWORD(wParam);
	if (iCamera<0 || iCamera>=m_iNumWebCams)
		return 0;

	// Preset must not exceed
	UINT uiOption = HIWORD(lParam);
	if (LOWORD(lParam)=='M' || LOWORD(lParam)=='S')
	{
		// Memory position. We have one for each preset button. Must not exceed the number of presets.
		if (uiOption>=CWebcamController::NUM_PRESETS)
			return 0;
	}
	else
	{
		// Other operations may use a count. We limit it to 10.
		if (uiOption==0)
			uiOption = 1; // Default is 1 time
		else if (uiOption>10)
			uiOption = 10; // Max 10 times)
	}

	// Save the current camera. We will restore it at the end of this function.
	// And select the one we want to control.
	int iActiveCam = m_iCurrentWebCam;
	SetActiveCam(iCamera);

	// Check the command (lParam)
	//  ZoomIn= +, ZoomOut = -, 
	//  PanLeft = L, PanRight = R, 
	//  TiltUp = U, TiltDown = D, 
	//  Home = H, 
	//  Restore MemoryPos = M (HIWORD = memory position),
	//  Store MemoryPos = S (HIWORD = memory position)
	int iRC = 1;

	switch (LOWORD(lParam))
	{
	case '+':
		while (uiOption--)
			OnBtZoomIn();
		break;
	case '-':
		while (uiOption--)
			OnBtZoomOut();
		break;
	case 'L':
		while (uiOption--)
			OnBtLeft();
		break;
	case 'R':
		while (uiOption--)
			OnBtRight();
		break;
	case 'U':
		while (uiOption--)
			OnBtUp();
		break;
	case 'D':
		while (uiOption--)
			OnBtDown();
		break;
	case 'H':
		OnBtHome();
		break;
	case 'M':
		OnBtPreset(m_btPreset[uiOption].GetDlgCtrlID());
		break;
	case 'S':
		OnBtMemory();
		OnBtPreset(m_btPreset[uiOption].GetDlgCtrlID());
		break;
	default:
		// Unknown command
		iRC = 0;
		break;
	}

	// Restore old camera
	SetActiveCam(iActiveCam);

	return iRC;
}


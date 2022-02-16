#pragma once

#include "ExtensionUnit.h"
#include "PTZControlDlg.h"

// CSettingsDlg dialog

class CSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSettingsDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS };
#endif
public:
	CString m_strCameraName;
	CString m_strTooltip[CPTZControlDlg::NUM_MAX_WEBCAMS][CWebcamController::NUM_PRESETS];

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bLogitechCameraControl;
	int m_iMotorIntervalTimer;
	afx_msg void OnChLogitechcontrol();
	CEdit m_edMotorInterval;
	CButton m_chLogitechControl;
	virtual BOOL OnInitDialog();
};

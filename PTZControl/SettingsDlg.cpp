// SettingsDlg.cpp : implementation file
//

#include "pch.h"
#include "PTZControl.h"
#include "SettingsDlg.h"
#include "afxdialogex.h"


// CSettingsDlg dialog

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTINGS, pParent)
	, m_bLogitechCameraControl(FALSE)
	, m_iMotorIntervalTimer(0)
{

}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ED_CAMERA, m_strCameraName);
	DDX_Text(pDX, IDC_ED_TOOLTIP_1_1, m_strTooltip[0][0]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_1_2, m_strTooltip[0][1]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_1_3, m_strTooltip[0][2]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_1_4, m_strTooltip[0][3]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_1_5, m_strTooltip[0][4]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_1_6, m_strTooltip[0][5]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_1_7, m_strTooltip[0][6]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_1_8, m_strTooltip[0][7]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_2_1, m_strTooltip[1][0]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_2_2, m_strTooltip[1][1]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_2_3, m_strTooltip[1][2]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_2_4, m_strTooltip[1][3]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_2_5, m_strTooltip[1][4]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_2_6, m_strTooltip[1][5]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_2_7, m_strTooltip[1][6]);
	DDX_Text(pDX, IDC_ED_TOOLTIP_2_8, m_strTooltip[1][7]);
	DDX_Check(pDX, IDC_CH_LOGITECHCONTROL, m_bLogitechCameraControl);
	DDX_Text(pDX, IDC_ED_MOTORTIME, m_iMotorIntervalTimer);
	DDX_Control(pDX, IDC_ED_MOTORTIME, m_edMotorInterval);
	DDX_Control(pDX, IDC_CH_LOGITECHCONTROL, m_chLogitechControl);
	if (pDX->m_bSaveAndValidate)
		m_iMotorIntervalTimer = min(max(10,m_iMotorIntervalTimer),1000);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CH_LOGITECHCONTROL, &CSettingsDlg::OnChLogitechcontrol)
END_MESSAGE_MAP()


// CSettingsDlg message handlers


void CSettingsDlg::OnChLogitechcontrol()
{
	m_edMotorInterval.EnableWindow(!m_chLogitechControl.GetCheck());
}


BOOL CSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	OnChLogitechcontrol();

	CenterWindow();
	return TRUE;  				  
}

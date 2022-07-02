// DeepScanSettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Arbiter.h"
#include "DeepScanSettingDlg.h"


// CDeepScanSettingDlg 对话框

IMPLEMENT_DYNAMIC(CDeepScanSettingDlg, CDialog)

CDeepScanSettingDlg::CDeepScanSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDeepScanSettingDlg::IDD, pParent)
	, m_defaultsize(5)
	, m_defaultpath(L"")
{

}

CDeepScanSettingDlg::~CDeepScanSettingDlg()
{
}

void CDeepScanSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DEFAULT_SIZE, m_defaultsize);
	DDX_Text(pDX, IDC_DEFAULT_PATH, m_defaultpath);
}


BEGIN_MESSAGE_MAP(CDeepScanSettingDlg, CDialog)
	ON_BN_CLICKED(IDC_BROWSE, &CDeepScanSettingDlg::OnBnClickedBrowse)
	ON_BN_CLICKED(IDC_CONFIRM, &CDeepScanSettingDlg::OnBnClickedConfirm)
	ON_EN_CHANGE(IDC_DEFAULT_SIZE, &CDeepScanSettingDlg::OnEnChangeDefaultSize)
END_MESSAGE_MAP()


// CDeepScanSettingDlg 消息处理程序

BOOL CDeepScanSettingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();	

	return TRUE;
}

void CDeepScanSettingDlg::OnBnClickedBrowse()
{
	CBrowseFolder browsefolder;
	if(browsefolder.DoModal(this, NULL) == IDOK)
	{
		m_defaultpath = browsefolder.GetDirPath();

		if(m_defaultpath.GetAt(m_defaultpath.GetLength() - 1) == L'\\')
			m_defaultpath = m_defaultpath.Left(m_defaultpath.GetLength() - 1);

		UpdateData(0);
	}
}

void CDeepScanSettingDlg::OnEnChangeDefaultSize()
{
	UpdateData();
}

void CDeepScanSettingDlg::OnBnClickedConfirm()
{
	if(m_defaultpath.GetLength() > 0)
	{
		CBasicContext::m_strPath = m_defaultpath;
	}

	if(m_defaultsize > 0)
	{
		CBasicContext::m_ullArgument3 = m_defaultsize;
	}

	CDialog::OnOK();
}
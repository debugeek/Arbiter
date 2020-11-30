#pragma once


// CDeepScanSettingDlg 对话框

class CDeepScanSettingDlg : public CDialog
{
	DECLARE_DYNAMIC(CDeepScanSettingDlg)

public:
	CDeepScanSettingDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDeepScanSettingDlg();

// 对话框数据
	enum { IDD = IDD_DEEPSCAN_SETTING_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	ULONGLONG m_defaultsize;
	CString m_defaultpath;

	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedConfirm();
	afx_msg void OnEnChangeDefaultSize();
};

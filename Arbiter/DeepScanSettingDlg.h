#pragma once


// CDeepScanSettingDlg �Ի���

class CDeepScanSettingDlg : public CDialog
{
	DECLARE_DYNAMIC(CDeepScanSettingDlg)

public:
	CDeepScanSettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDeepScanSettingDlg();

// �Ի�������
	enum { IDD = IDD_DEEPSCAN_SETTING_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	ULONGLONG m_defaultsize;
	CString m_defaultpath;

	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedConfirm();
	afx_msg void OnEnChangeDefaultSize();
};

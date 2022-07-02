#pragma once

#include "DeepScanSettingDlg.h"

// CPartitionDlg �Ի���

class CPartitionDlg : public CDialog
{
	DECLARE_DYNAMIC(CPartitionDlg)

public:
	CPartitionDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CPartitionDlg();

// �Ի�������
	enum { IDD = IDD_PARTITION_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();
	
	afx_msg void OnBnClickedPartitionRead();
	afx_msg void OnBnClickedPartitionAnalysis();
	afx_msg void OnBnClickedPartitionRestore();
	afx_msg void OnBnClickedPartitionDeepscan();

	afx_msg void OnNMClickPartitionList(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

public:	
	void ReadPartition();
	void AnalysisPartition();
	void RestorePartition();

	void LoadPartition();

	CListCtrl m_partitionlist;
};

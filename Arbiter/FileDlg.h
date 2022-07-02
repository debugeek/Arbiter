#pragma once

#include "BasicContext.h"

// CFileDlg �Ի���

class CFileDlg : public CDialog
{
	DECLARE_DYNAMIC(CFileDlg)

public:
	CFileDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CFileDlg();

// �Ի�������
	enum { IDD = IDD_FILE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnSelchangedFolderTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickFileList(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

public:
	void LoadFolder();

public:
	CListCtrl						m_filelist;	
	CjTreeCtrl						m_foldertree;
	CImageList						m_imagelist;

protected:
	void FileRecover();
	
	void LoadNTFSFolderList();
	void LoadNTFSFileList(TCHAR* pPath);
	void LoadFAT32FolderList();
	void LoadFAT32FileList(TCHAR* pPath);

public:
	afx_msg void OnBnClickedTest();
};

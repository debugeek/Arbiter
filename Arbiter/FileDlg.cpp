// FileDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Arbiter.h"
#include "FileDlg.h"

#include "PartitionDlg.h"

// CFileDlg 对话框

IMPLEMENT_DYNAMIC(CFileDlg, CDialog)

CFileDlg::CFileDlg(CWnd* pParent)
	: CDialog(CFileDlg::IDD, pParent)
{
}

CFileDlg::~CFileDlg()
{
}

void CFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE_LIST, m_filelist);
	DDX_Control(pDX, IDC_FOLDER_TREE, m_foldertree);
}


BEGIN_MESSAGE_MAP(CFileDlg, CDialog)
	ON_COMMAND(IDM_FILE_RECOVER, FileRecover)

	ON_NOTIFY(TVN_SELCHANGED, IDC_FOLDER_TREE, &CFileDlg::OnTvnSelchangedFolderTree)
	ON_NOTIFY(NM_RCLICK, IDC_FILE_LIST, &CFileDlg::OnNMRClickFileList)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CFileDlg 消息处理程序
BOOL CFileDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	//////////////////////ImageList
	m_imagelist.Create (16, 16, ILC_COLOR32|ILC_MASK, 5, 1);
	m_imagelist.Add(AfxGetApp()->LoadIcon(IDI_ROOT));
	m_imagelist.Add(AfxGetApp()->LoadIcon(IDI_FOLDER_NORMAL));
	m_imagelist.Add(AfxGetApp()->LoadIcon(IDI_FOLDER_DELETED));
	m_imagelist.Add(AfxGetApp()->LoadIcon(IDI_FILE_NORMAL));
	m_imagelist.Add(AfxGetApp()->LoadIcon(IDI_FILE_DELETED));

	//////////////////////FolderTree
	DWORD dwStyle = GetWindowLong(m_foldertree.m_hWnd,GWL_STYLE);
	dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
	SetWindowLong(m_foldertree.m_hWnd,GWL_STYLE,dwStyle);
	m_foldertree.SetImageList(&m_imagelist, TVSIL_NORMAL);

	//////////////////////FileList
	m_filelist.SetExtendedStyle(LVS_EX_FLATSB |LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	m_filelist.InsertColumn(0, _T("文件名"), LVCFMT_LEFT, 161);
	m_filelist.InsertColumn(1, _T("文件大小"), LVCFMT_LEFT, 60);
	m_filelist.InsertColumn(2, _T("创建日期"), LVCFMT_LEFT, 110);
	m_filelist.InsertColumn(3, _T("文件状态"), LVCFMT_LEFT, 60);
	m_filelist.SetImageList(&m_imagelist, LVSIL_SMALL);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFileDlg::LoadNTFSFolderList()
{
	std::vector<NTFS_ITEM*>::iterator iter;
	for(iter = CBasicContext::m_NTFSItemList.begin();
		iter != CBasicContext::m_NTFSItemList.end();
		iter++)
	{
		NTFS_ITEM* pItem = *iter;
		if(pItem != NULL)
		{
			CString strPath = pItem->Name;

			m_foldertree.InsertTreeItemByPath(strPath);
		}
	}
}

void CFileDlg::LoadNTFSFileList(TCHAR* pPath)
{
	if(pPath != NULL)
	{
		m_filelist.DeleteAllItems();
		m_filelist.SetRedraw(FALSE);

		ULONG ulRecord = 0;
		std::vector<NTFS_ITEM*>::iterator iter;
		for(iter = CBasicContext::m_NTFSItemList.begin();
			iter != CBasicContext::m_NTFSItemList.end();
			iter++, ulRecord++)
		{
			NTFS_ITEM* pItem = *iter;
			if(pItem != NULL)
			{
				int nPathLen = wcslen(pPath);
				if(wmemcmp(pItem->Name, pPath, nPathLen) == 0)
				{
					CString str = pItem->Name;
					int nLen = str.GetLength();
					str = str.Right(str.GetLength() - str.ReverseFind(_T('\\')) - 1);
					int nNameLen = str.GetLength();

					if(nLen == nNameLen + nPathLen + 1)
					{
						int nItem = m_filelist.GetItemCount();

						if(pItem->IsDeleted == TRUE)
						{
							m_filelist.InsertItem(nItem, str, 4);
							m_filelist.SetItemText(nItem, 3, L"被删除");
						}
						else
						{
							m_filelist.InsertItem(nItem, str, 3);
							m_filelist.SetItemText(nItem, 3, L"--");
						}

						str.Format(_T("%I64d"), pItem->Size);
						m_filelist.SetItemText(nItem, 1, str);

						m_filelist.SetItemText(nItem, 2, pItem->CreateDate);

						m_filelist.SetItemData(nItem, ulRecord);
					}
				}
			}
		}
		m_filelist.SetRedraw(TRUE);
	}
}

void CFileDlg::LoadFAT32FolderList()
{
	std::vector<FAT32_ITEM*>::iterator iter;
	for(iter = CBasicContext::m_FAT32ItemList.begin();
		iter != CBasicContext::m_FAT32ItemList.end();
		iter++)
	{
		FAT32_ITEM* pItem = *iter;
		if(pItem != NULL)
		{
			CString strPath = pItem->Name;

			m_foldertree.InsertTreeItemByPath(strPath);
		}
	}
}

void CFileDlg::LoadFAT32FileList(TCHAR* pPath)
{
	if(pPath != NULL)
	{
		m_filelist.DeleteAllItems();
		m_filelist.SetRedraw(FALSE);

		ULONG ulRecord = 0;
		std::vector<FAT32_ITEM*>::iterator iter;
		for(iter = CBasicContext::m_FAT32ItemList.begin();
			iter != CBasicContext::m_FAT32ItemList.end();
			iter++, ulRecord++)
		{
			FAT32_ITEM* pItem = *iter;
			if(pItem != NULL)
			{
				int nPathLen = wcslen(pPath);
				if(wmemcmp(pItem->Name, pPath, nPathLen) == 0)
				{
					CString str = pItem->Name;
					int nLen = str.GetLength();
					str = str.Right(str.GetLength() - str.ReverseFind(_T('\\')) - 1);
					int nNameLen = str.GetLength();

					if(nLen == nNameLen + nPathLen + 1)
					{
						int nItem = m_filelist.GetItemCount();

						if(pItem->IsDeleted == TRUE)
						{
							m_filelist.InsertItem(nItem, str, 4);
							m_filelist.SetItemText(nItem, 3, L"被删除");
						}
						else
						{
							m_filelist.InsertItem(nItem, str, 3);
							m_filelist.SetItemText(nItem, 3, L"--");
						}

						str.Format(_T("%d"), pItem->Size);
						m_filelist.SetItemText(nItem, 1, str);

						m_filelist.SetItemText(nItem, 2, pItem->CreateDate);

						m_filelist.SetItemData(nItem, ulRecord);
					}
				}
			}
		}
		m_filelist.SetRedraw(TRUE);
	}
}

void CFileDlg::LoadFolder()
{
	m_filelist.DeleteAllItems();
	m_foldertree.DeleteAllItems();
	m_foldertree.Clear();

	int nItemCount = 0;
	switch(CBasicContext::m_PartitionType)
	{
	case FS_NTFS:
		LoadNTFSFolderList();
		LoadNTFSFileList(L"根目录");
		nItemCount = CBasicContext::m_NTFSItemList.size();
		break;
	case FS_FAT32:
		LoadFAT32FolderList();
		LoadFAT32FileList(L"根目录");
		nItemCount = CBasicContext::m_FAT32ItemList.size();
		break;
	}

	CString str;
	str.Format(L"找到%d项文件", nItemCount);
	CBasicContext::WriteStatus(str);
}

void CFileDlg::FileRecover()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, L"All Files(*.*)|*.*||", AfxGetMainWnd());
	if(dlg.DoModal() == IDOK)
	{
		CBasicContext::m_strPath = dlg.GetPathName();
		CBasicContext::AddTask(TASK_RECOVER_FILE);
	}
}

void CFileDlg::OnTvnSelchangedFolderTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	HTREEITEM hItem = m_foldertree.GetSelectedItem();
	CString strItemPath = m_foldertree.GetTreeItemPath(hItem);
	if(strItemPath.GetLength() > 0)
	{
		if(CBasicContext::m_PartitionType == FS_NTFS)
			LoadNTFSFileList(strItemPath.GetBuffer());
		else if(CBasicContext::m_PartitionType == FS_FAT32)
			LoadFAT32FileList(strItemPath.GetBuffer());
	}
}

void CFileDlg::OnNMRClickFileList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if(pNMListView->iItem != -1 && pNMListView->iSubItem != -1)
	{
		int nItem = m_filelist.GetSelectionMark();
		int nRecordNumber = m_filelist.GetItemData(nItem);

		if(CBasicContext::m_PartitionType == FS_NTFS)
		{
			NTFS_ITEM* pItem = CBasicContext::GetNTFSItem(nRecordNumber);
			if(pItem != NULL)
			{
				CBasicContext::m_ullArgument1 = pItem->FileReferenceNumber;
				CBasicContext::m_ullArgument2 = pItem->Size;
			}
			else
			{
				CBasicContext::m_ullArgument1 = 0;
				CBasicContext::m_ullArgument2 = 0;
			}
		}
		else if(CBasicContext::m_PartitionType == FS_FAT32)
		{
			FAT32_ITEM* pItem = CBasicContext::GetFAT32Item(nRecordNumber);
			if(pItem != NULL)
			{
				CBasicContext::m_ullArgument1 = pItem->Sector;
				CBasicContext::m_ullArgument2 = pItem->Size;
			}
			else
			{
				CBasicContext::m_ullArgument1 = 0;
				CBasicContext::m_ullArgument2 = 0;
			}
		}

		CPoint pt;
		GetCursorPos(&pt);
		CMenu menu;
		menu.LoadMenu(IDR_FILE_MENU);
		CMenu* pMenu = menu.GetSubMenu(0);
		pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
	}
	*pResult = 0;
}

void CFileDlg::OnBnClickedTest()
{
	// TODO: 在此添加控件通知处理程序代码
	//ReadPartition();
}
// PatitionDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "Arbiter.h"
#include "PartitionDlg.h"


// CPartitionDlg 对话框

IMPLEMENT_DYNAMIC(CPartitionDlg, CDialog)

CPartitionDlg::CPartitionDlg(CWnd* pParent)
	: CDialog(CPartitionDlg::IDD, pParent)
{
}

CPartitionDlg::~CPartitionDlg()
{
}

void CPartitionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PARTITION_LIST, m_partitionlist);
}


BEGIN_MESSAGE_MAP(CPartitionDlg, CDialog)
	ON_BN_CLICKED(IDC_PARTITION_ANALYSIS, &CPartitionDlg::OnBnClickedPartitionAnalysis)
	ON_NOTIFY(NM_CLICK, IDC_PARTITION_LIST, &CPartitionDlg::OnNMClickPartitionList)
	ON_BN_CLICKED(IDC_PARTITION_RESTORE, &CPartitionDlg::OnBnClickedPartitionRestore)
	ON_BN_CLICKED(IDC_PARTITION_DEEPSCAN, &CPartitionDlg::OnBnClickedPartitionDeepscan)
	ON_BN_CLICKED(IDC_PARTITION_READ, &CPartitionDlg::OnBnClickedPartitionRead)
END_MESSAGE_MAP()


// CPartitionDlg 消息处理程序
BOOL CPartitionDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_partitionlist.SetExtendedStyle(LVS_EX_FLATSB |LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	m_partitionlist.InsertColumn(0, _T("分区卷标"), LVCFMT_LEFT, 80);
	m_partitionlist.InsertColumn(1, _T("分区类型"), LVCFMT_LEFT, 96);
	m_partitionlist.InsertColumn(2, _T("分区大小"), LVCFMT_LEFT, 110);

	
	ReadPartition();
		
	return TRUE;
}

void CPartitionDlg::LoadPartition()
{
	CString str;

	std::vector<PARTITION_ITEM*>::iterator iter;
	for(iter = CBasicContext::m_PartitionItemList.begin();
		iter != CBasicContext::m_PartitionItemList.end();
		iter++)
	{
		PARTITION_ITEM* pItem = *iter;
		if(pItem != NULL)
		{
			int nItem = m_partitionlist.GetItemCount();

			m_partitionlist.InsertItem(nItem, pItem->Volume);

			if(pItem->PartitionType == FS_NTFS)
				m_partitionlist.SetItemText(nItem, 1, L"NTFS");
			else if(pItem->PartitionType == FS_FAT32)
				m_partitionlist.SetItemText(nItem, 1, L"FAT32");

			str.Format(L"%d(G)", pItem->SectorCount>>21);
			m_partitionlist.SetItemText(nItem, 2, str);
		}
	}

	str.Format(L"找到%d个分区", CBasicContext::m_PartitionItemList.size());
	CBasicContext::WriteStatus(str);
}

//
// 解析某一个分区
//
void CPartitionDlg::AnalysisPartition()
{
	CBasicContext::ClearFAT32Items();
	CBasicContext::ClearNTFSItems();
	CBasicContext::AddTask(TASK_ANALYSIS_PARTITION);
}

void CPartitionDlg::ReadPartition()
{
	m_partitionlist.DeleteAllItems();
	CBasicContext::ClearPartitionItems();	
	CBasicContext::ClearFAT32Items();
	CBasicContext::ClearNTFSItems();

	CBasicContext::m_ullArgument1 = MODE_NORMAL;
	CBasicContext::AddTask(TASK_READ_PARTITION);
}

void CPartitionDlg::RestorePartition()
{
	m_partitionlist.DeleteAllItems();
	CBasicContext::ClearPartitionItems();	
	CBasicContext::ClearFAT32Items();
	CBasicContext::ClearNTFSItems();

	
	CBasicContext::m_ullArgument1 = MODE_RESTORE;
	CBasicContext::AddTask(TASK_READ_PARTITION);
}

void CPartitionDlg::OnBnClickedPartitionRead()
{
	ReadPartition();
}

void CPartitionDlg::OnBnClickedPartitionAnalysis()
{
	int nItem = m_partitionlist.GetSelectionMark();
	if(nItem >= 0)
	{
		AnalysisPartition();
	}
}

void CPartitionDlg::OnBnClickedPartitionRestore()
{
	RestorePartition();
}

void CPartitionDlg::OnBnClickedPartitionDeepscan()
{	
	CDeepScanSettingDlg dlg;
	if(dlg.DoModal() == IDOK)
	{
		if(CBasicContext::m_strPath.GetLength() <= 0 || CBasicContext::m_ullArgument3 <= 0)
			CBasicContext::WriteStatus(L"深度扫描参数设置错误");
		else
			CBasicContext::AddTask(TASK_DEEPSCAN);
	}
}

void CPartitionDlg::OnNMClickPartitionList(NMHDR *pNMHDR, LRESULT *pResult)
{
	int nItem = m_partitionlist.GetSelectionMark();
	if(nItem >= 0)
	{
		PARTITION_ITEM* pItem = CBasicContext::m_PartitionItemList.at(nItem);
		if(pItem != NULL)
		{
			CBasicContext::SetDevice(pItem->DeviceHandle);			
			CBasicContext::m_ullArgument1 = pItem->StartSector;
			CBasicContext::m_ullArgument2 = pItem->SectorCount;
			CBasicContext::m_PartitionType = pItem->PartitionType;
		}
	}
	else
	{
		CBasicContext::SetDevice(INVALID_HANDLE_VALUE);
		CBasicContext::m_ullArgument1 = 0;
		CBasicContext::m_ullArgument2 = 0;
		CBasicContext::m_PartitionType = FS_UNKNOWN;
	}
}
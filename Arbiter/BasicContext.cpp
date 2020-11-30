#include "stdafx.h"
#include "BasicContext.h"

HWND CBasicContext::m_hMainWnd;
BOOL CBasicContext::m_bIsFSEngineInitialized = FALSE;

INITIALIZEFSENGINE CBasicContext::m_pInitializeFSEngine = NULL;
UNINITIALIZEFSENGINE CBasicContext::m_pUninitializeFSEngine = NULL;
SETHWND CBasicContext::m_pSetHwnd = NULL;
SETDEVICE CBasicContext::m_pSetDevice = NULL;
ADDTASK CBasicContext::m_pAddTask = NULL;

ULONGLONG CBasicContext::m_ullArgument1 = 0;
ULONGLONG CBasicContext::m_ullArgument2 = 0;
ULONGLONG CBasicContext::m_ullArgument3 = 0;
CString CBasicContext::m_strPath = 0;
PARTITION_TYPE CBasicContext::m_PartitionType = FS_UNKNOWN;

std::vector<PARTITION_ITEM*> CBasicContext::m_PartitionItemList;
std::vector<NTFS_ITEM*> CBasicContext::m_NTFSItemList;
std::vector<FAT32_ITEM*> CBasicContext::m_FAT32ItemList;

//
// 初始化
//
void CBasicContext::InitializeContext(HWND hWnd)
{
	CBasicContext::m_hMainWnd = hWnd;

	//
	// InitializeFileSystemEngine
	//
	TCHAR tcDllPath[MAX_PATH] = L"\0";
	::GetModuleFileName(NULL, tcDllPath, MAX_PATH);
	CString strDllPath;
	strDllPath.Format(_T("%s"), tcDllPath);
	strDllPath = strDllPath.Left(strDllPath.ReverseFind('\\') + 1 );
	strDllPath += L"ArbiterFSEngine.dll";

	HMODULE hModule = ::LoadLibrary(strDllPath);
	if(hModule != NULL)
	{
		m_pInitializeFSEngine = (INITIALIZEFSENGINE)::GetProcAddress(hModule, "InitializeFSEngine");
		m_pUninitializeFSEngine = (UNINITIALIZEFSENGINE)::GetProcAddress(hModule, "UninitializeFSEngine");
		m_pSetHwnd = (SETHWND)::GetProcAddress(hModule, "SetHwnd");
		m_pSetDevice = (SETDEVICE)::GetProcAddress(hModule, "SetDevice");
		m_pAddTask = (ADDTASK)::GetProcAddress(hModule, "AddTask");
		if(m_pInitializeFSEngine != NULL
			&& m_pUninitializeFSEngine != NULL
			&& m_pSetDevice != NULL
			&& m_pSetDevice != NULL
			&& m_pAddTask != NULL)
		{
			m_bIsFSEngineInitialized = TRUE;
		}
	}

	if(m_bIsFSEngineInitialized == TRUE)
	{
		InitializeFSEngine();
		SetHwnd(m_hMainWnd);
	}
}

void CBasicContext::UninitializeContext()
{
	if(m_bIsFSEngineInitialized == TRUE)
	{
		UninitializeFSEngine();
		
		ClearPartitionItems();
		ClearFAT32Items();
		ClearNTFSItems();
	}
}

void CBasicContext::WriteStatus(LPCTSTR lpszText)
{
	::SendMessage(m_hMainWnd, WM_USER_MSG, MSG_UPDATE_STATUS, (LPARAM)lpszText);

}

//
// 初始化文件系统模块，向模块传入对话框窗口句柄以供模块投递消息
//
void CBasicContext::InitializeFSEngine()
{
	if(m_pInitializeFSEngine != NULL)
		m_pInitializeFSEngine();
}

//
// 卸载文件系统模块，不想造成内存泄露的话就试着在程序退出之前调用
//
void CBasicContext::UninitializeFSEngine()
{
	if(m_pUninitializeFSEngine != NULL)
		m_pUninitializeFSEngine();
}

void CBasicContext::SetHwnd(HWND hWnd)
{
	if(m_pSetHwnd != NULL)
		m_pSetHwnd(hWnd);
}

//
// 对文件系统模块进行设备设置
//
void CBasicContext::SetDevice(HANDLE hDevice)
{
	if(m_pSetDevice != NULL)
		m_pSetDevice(hDevice);
}

//
// 向文件系统模块投递任务
//
void CBasicContext::AddTask(TASK_TOKEN TaskToken)
{
	if(m_pAddTask != NULL)
		m_pAddTask(TaskToken, m_ullArgument1, m_ullArgument2, m_ullArgument3, m_strPath.GetBuffer());
}


//
// 保存分区项
//
void CBasicContext::AddPartitionItem(PARTITION_ITEM* pPartitionItem)
{
	if(pPartitionItem != NULL)
	{
		PARTITION_ITEM* pItem = new PARTITION_ITEM;

		pItem->PartitionType = pPartitionItem->PartitionType;
		pItem->StartSector = pPartitionItem->StartSector;
		pItem->SectorCount = pPartitionItem->SectorCount;
		pItem->DeviceHandle = pPartitionItem->DeviceHandle;

		int nVolumeLen = wcslen(pPartitionItem->Volume);
		pItem->Volume = new TCHAR[nVolumeLen + 1];
		wmemset(pItem->Volume, 0, nVolumeLen + 1);
		wmemcpy(pItem->Volume, pPartitionItem->Volume, nVolumeLen);

		m_PartitionItemList.push_back(pItem);
	}
}

//
// 保存NTFS文件项
//
void CBasicContext::AddNTFSItem(NTFS_ITEM* pNTFSItem)
{
	if(pNTFSItem != NULL)
	{
		NTFS_ITEM* pItem = new NTFS_ITEM;

		pItem->IsDeleted = pNTFSItem->IsDeleted;
		pItem->FileReferenceNumber = pNTFSItem->FileReferenceNumber;
		pItem->Size = pNTFSItem->Size;

		wmemset(pItem->CreateDate, 0, 20);
		wmemcpy(pItem->CreateDate, pNTFSItem->CreateDate, 20);

		int nNameLen = wcslen(pNTFSItem->Name);
		pItem->Name = new TCHAR[nNameLen + 1];
		wmemset(pItem->Name, 0, nNameLen + 1);
		wmemcpy(pItem->Name, pNTFSItem->Name, nNameLen);

		m_NTFSItemList.push_back(pItem);
	}
}

//
// 根据NTFS文件的记录号取得相应的文件项
//
NTFS_ITEM* CBasicContext::GetNTFSItem(int nRecordNumber)
{
	NTFS_ITEM* pItem = NULL;

	int nSize = m_NTFSItemList.size();
	if(nRecordNumber >= 0 && nRecordNumber < nSize)
		pItem = m_NTFSItemList.at(nRecordNumber);

	return pItem;
}

//
// 保存FAT32文件项
//
void CBasicContext::AddFAT32Item(FAT32_ITEM* pFAT32Item)
{
	if(pFAT32Item != NULL)
	{
		FAT32_ITEM* pItem = new FAT32_ITEM;

		pItem->IsDeleted = pFAT32Item->IsDeleted;
		pItem->Sector = pFAT32Item->Sector;
		pItem->Size = pFAT32Item->Size;

		wmemset(pItem->CreateDate, 0, 20);
		wmemcpy(pItem->CreateDate, pFAT32Item->CreateDate, 20);

		int nNameLen = wcslen(pFAT32Item->Name);
		pItem->Name = new TCHAR[nNameLen + 1];
		wmemset(pItem->Name, 0, nNameLen + 1);
		wmemcpy(pItem->Name, pFAT32Item->Name, nNameLen);

		m_FAT32ItemList.push_back(pItem);	
	}
}

//
// 根据FAT32文件的记录号取得相应的文件项
//
FAT32_ITEM* CBasicContext::GetFAT32Item(int nRecordNumber)
{
	FAT32_ITEM* pItem = NULL;

	int nSize = m_FAT32ItemList.size();
	if(nRecordNumber >= 0 && nRecordNumber < nSize)
		pItem = m_FAT32ItemList.at(nRecordNumber);

	return pItem;
}

//
// 清理所有分区项
//
void CBasicContext::ClearPartitionItems()
{
	std::vector<PARTITION_ITEM*>::iterator iter;
	for(iter = m_PartitionItemList.begin();
		iter != m_PartitionItemList.end();
		iter++)
	{
		PARTITION_ITEM* pItem = *iter;
		if(pItem != NULL)
		{
			if(pItem->Volume != NULL)
				delete[] pItem->Volume;
			delete pItem;
		}
	}
	m_PartitionItemList.clear();
}

//
// 清理所有FAT32文件项
//
void CBasicContext::ClearFAT32Items()
{
	std::vector<FAT32_ITEM*>::iterator iter;
	for(iter = m_FAT32ItemList.begin();
		iter != m_FAT32ItemList.end();
		iter++)
	{
		FAT32_ITEM* pItem = *iter;
		if(pItem != NULL)
		{
			if(pItem->Name != NULL)
				delete[] pItem->Name;
			delete pItem;
		}
	}
	m_FAT32ItemList.clear();
}

//
// 清理所有NTFS文件项
//
void CBasicContext::ClearNTFSItems()
{
	std::vector<NTFS_ITEM*>::iterator iter;
	for(iter = m_NTFSItemList.begin();
		iter != m_NTFSItemList.end();
		iter++)
	{
		NTFS_ITEM* pItem = *iter;
		if(pItem != NULL)
		{
			if(pItem->Name != NULL)
				delete[] pItem->Name;
			delete pItem;
		}
	}
	m_NTFSItemList.clear();
}
#pragma once

#include <winsvc.h>
#include <aclapi.h>
#include <tlhelp32.h>

#include <vector>
#include <map>

#include "../Common/globaldefs.h"

typedef void (*INITIALIZEFSENGINE)();
typedef void (*UNINITIALIZEFSENGINE)();
typedef void (*SETHWND)(HWND);
typedef void (*SETDEVICE)(HANDLE);
typedef void (*ADDTASK)(TASK_TOKEN, ULONGLONG, ULONGLONG, ULONGLONG, TCHAR*);

class CBasicContext
{
public:
	static void InitializeContext(HWND hWnd);
	static void UninitializeContext();

	static void WriteStatus(LPCTSTR lpszText);

	static BOOL m_bIsFSEngineInitialized;

private:	
	static HWND m_hMainWnd;

public:	
	static void InitializeFSEngine();
	static void UninitializeFSEngine();
	static void SetHwnd(HWND hWnd);
	static void SetDevice(HANDLE hDevice);
	static void AddTask(TASK_TOKEN TaskToken);

	static void AddPartitionItem(PARTITION_ITEM* pPartitionItem);
	static void AddNTFSItem(NTFS_ITEM* pNTFSItem);
	static void AddFAT32Item(FAT32_ITEM* pFAT32Item);

	static NTFS_ITEM* GetNTFSItem(int nRecordNumber);
	static FAT32_ITEM* GetFAT32Item(int nRecordNumber);

	static void ClearPartitionItems();
	static void ClearFAT32Items();
	static void ClearNTFSItems();

	static std::vector<PARTITION_ITEM*> m_PartitionItemList;
	static std::vector<NTFS_ITEM*> m_NTFSItemList;
	static std::vector<FAT32_ITEM*> m_FAT32ItemList;

protected:
	static INITIALIZEFSENGINE m_pInitializeFSEngine;
	static UNINITIALIZEFSENGINE m_pUninitializeFSEngine;
	static SETHWND m_pSetHwnd;
	static SETDEVICE m_pSetDevice;
	static ADDTASK m_pAddTask;

public:
	static ULONGLONG m_ullArgument1;
	static ULONGLONG m_ullArgument2;
	static ULONGLONG m_ullArgument3;
	static PARTITION_TYPE m_PartitionType;
	static CString m_strPath;
};
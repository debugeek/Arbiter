#pragma once

#include "macros.h"

DLL_EXPORT void InitializeFSEngine();
DLL_EXPORT void UninitializeFSEngine();
DLL_EXPORT void SetHwnd(HWND hWnd);
DLL_EXPORT void SetDevice(HANDLE hPhysicalDrive);
DLL_EXPORT void AddTask(
						TASK_TOKEN TaskToken,
						ULONGLONG ullArg1,
						ULONGLONG ullArg2 = 0,
						ULONGLONG ullArg3 = 0,
						TCHAR* pArg4 = NULL);

//
//
//
void QueryTaskQueue();
static DWORD WINAPI KernelThreadProc(LPVOID lpvoid);

void Notify(WPARAM wParam, LPARAM lParam, BOOL bAsyncFlag);

//
// Global
//
void ReadPartition(ULONGLONG ullMode);
void AnalysisPartition(ULONGLONG ullStartSector);

void ReadDPT(ULONGLONG ullSector, ULONGLONG ullOffset);
void ReadDBR(ULONGLONG ullSector);
void RestoreDBR(ULONGLONG ullMode);

ULONG ReadSector(ULONGLONG ullSector, ULONGLONG ullSectorCount, LPBYTE lpSectorBuffer);

ULONGLONG GetCylinderSector(ULONGLONG ullCylinder);
ULONGLONG GetTrackSector(ULONGLONG ullTrack);

PARTITION_TYPE GetPartitionType(ULONGLONG ullSector);

BOOL IsPartitionRecorded(ULONGLONG ullSector);
void ClearPartitionItems();
void ClearDeviceItems();

void DeepScan(ULONGLONG ullStartSector, ULONGLONG ullSectorCount, ULONGLONG ullMaxSectors, TCHAR* pPath);
BOOL IsMatched(LPBYTE lpBuffer);

HWND m_hMainWnd;
HANDLE m_hPhysicalDrive;
HANDLE m_hFile;
PARTITION_TYPE m_PartitionType;
DISK_GEOMETRY m_DiskGeometry;

ULONG m_ulBytesPerSector;
ULONGLONG m_ullStartSector;
ULONGLONG m_ullSectorCount;

HANDLE m_hKernelThread;
HANDLE m_hTaskEvent;
std::queue<TASK_RECORD*> m_TaskQueue;

std::vector<PARTITION_ITEM*> m_PartitionItemList;
std::set<ULONGLONG> m_PartitionSet;
std::set<HANDLE> m_DeviceSet;

//
// NTFS
//
void GetNTFSPartitionInfo(ULONGLONG ullStartSector);
void AnalysisNTFSDBR(ULONGLONG ullStartSector);
void AnalysisMFT(ULONG ulFileReferenceNumber, ULONG ulFileRecordCount);
void CopyNTFSFile(ULONG ulFileReferenceNumber);

ULONG ReadFileRecord(ULONG ulFileReferenceNumber, ULONG ulFileRecordCount, LPBYTE lpFileRecordBuffer);
ULONG GetFileRecordCount();
ULONG GetFileRecordSize();

ULONGLONG GetStartFileRecordOffset();
ULONGLONG GetFileRecordOffset(ULONG ulFileReferenceNumber);

TCHAR* ReadNTFSVolume(ULONGLONG ullStartSector);
TCHAR* GetFileRecordPath(ULONG ulFileReferenceNumber);

void ClearNTFSItems();

ULONG m_ulFileRecordCount;
ULONG m_ulFileRecordSize;
ULONGLONG m_ullStartFileRecordOffset;
NTFS_ROOT_SECTOR m_NTFSRootSector;

std::vector<FILE_RECORD_BLOCK*> m_FileRecordBlock;
std::map<ULONG, FILE_RECORD*> m_FileRecordMap;

//
// FAT32
//
void GetFAT32PartitionInfo(ULONGLONG ullStartSector);
void AnalysisFAT32DBR(ULONGLONG ullStartSector);
void AnalysisEntry(ULONGLONG ullSector, TCHAR* pPath);
void CopyFAT32File(ULONGLONG ullSector, ULONGLONG ullSize);

TCHAR* ReadFAT32Volume(ULONGLONG ullStartSector);

ULONGLONG GetFatSector();	
ULONGLONG GetDataSector();	
ULONGLONG GetClusterSector(ULONGLONG ullCluster);

void ClearFAT32Items();

FAT32_ROOT_SECTOR m_FAT32RootSector;

std::map<ULONGLONG, TCHAR*> m_EntryMap;
std::vector	<ENTRY_BLOCK*> m_EntryBlock;
std::set<ULONGLONG> m_EntrySet;

//
//
//
char* TCHARTOCHAR(TCHAR* pSrcBuffer)
{
	char* pDesBuffer = NULL;
	if(pSrcBuffer != NULL)
	{
		ULONG ulLen= WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pSrcBuffer, -1, NULL, 0, NULL, NULL);
		if(ulLen > 0)
		{
			pDesBuffer = new char[ulLen + 1];
			memset(pDesBuffer, 0, ulLen + 1);
			WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pSrcBuffer, -1, pDesBuffer, ulLen, NULL, NULL);
		}
	}
	return pDesBuffer;
}

TCHAR* CHARTOTCHAR(char* pSrcBuffer)
{
	TCHAR* pDesBuffer = NULL;
	if(pSrcBuffer != NULL)
	{
        ULONG ulLen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pSrcBuffer, -1, NULL, 0);
		if(ulLen > 0)
		{
			pDesBuffer = new TCHAR[ulLen + 1];
			wmemset(pDesBuffer, 0, ulLen + 1);
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pSrcBuffer, -1, pDesBuffer, ulLen);
		}
    }
	return pDesBuffer;
}

char* CHARTOSTRING(char* pSrcBuffer, ULONG ulMaxLen)
{
	char* pDesBuffer = NULL;
	if(pSrcBuffer != NULL)
	{
		ULONG ulDesLen = 0;
		while(pSrcBuffer[ulDesLen] != 0x20 && ++ulDesLen < ulMaxLen);

		pDesBuffer = new char[ulDesLen + 1];
		memset(pDesBuffer, 0, ulDesLen + 1);
		memcpy(pDesBuffer, pSrcBuffer, ulDesLen);
	}

	return pDesBuffer;
}
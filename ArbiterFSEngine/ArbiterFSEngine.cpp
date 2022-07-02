#include "ArbiterFSEngine.h"

//
//
// Interface
//
//
DLL_EXPORT void InitializeFSEngine()
{
	m_hMainWnd = NULL;
	m_hPhysicalDrive = INVALID_HANDLE_VALUE;

	m_hTaskEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hKernelThread = CreateThread(NULL, 0, KernelThreadProc, NULL, 0, NULL);
}

DLL_EXPORT void UninitializeFSEngine()
{
	DWORD dwExitCode = 0;
	GetExitCodeThread(m_hKernelThread, &dwExitCode);
	TerminateThread(m_hKernelThread, dwExitCode);
	Sleep(100);

	ClearPartitionItems();
	ClearDeviceItems();
	ClearNTFSItems();
	ClearFAT32Items();
}

DLL_EXPORT void SetHwnd(HWND hWnd)
{
	m_hMainWnd = hWnd;
}

DLL_EXPORT void SetDevice(HANDLE hPhysicalDrive)
{
	if(hPhysicalDrive != INVALID_HANDLE_VALUE)
	{
		DWORD dwBytesRet = 0;
		DeviceIoControl(
			hPhysicalDrive,
			IOCTL_DISK_GET_DRIVE_GEOMETRY,
			NULL,
			0,
			&m_DiskGeometry,
			sizeof(m_DiskGeometry),
			&dwBytesRet,
			NULL);

		m_ulBytesPerSector = m_DiskGeometry.BytesPerSector;		
		m_hPhysicalDrive = hPhysicalDrive;
	}
}

DLL_EXPORT void AddTask(
						TASK_TOKEN TaskToken, 
						ULONGLONG ullArg1, 
						ULONGLONG ullArg2, 
						ULONGLONG ullArg3,
						TCHAR* pArg4)
{
	TASK_RECORD* pTaskRecord = new TASK_RECORD;
	pTaskRecord->TaskToken = TaskToken;
	pTaskRecord->Argument1 = ullArg1;
	pTaskRecord->Argument2 = ullArg2;
	if(pArg4 != NULL)
	{
		ULONG ulArg4Len = wcslen(pArg4);
		pTaskRecord->Argument4= new TCHAR[ulArg4Len + 1];
		wmemset(pTaskRecord->Argument4, 0, ulArg4Len + 1);
		wmemcpy(pTaskRecord->Argument4, pArg4, ulArg4Len);
	}
	m_TaskQueue.push(pTaskRecord);

	SetEvent(m_hTaskEvent);
}

//
//
// Kernel
//
//
void QueryTaskQueue()
{
	if(!m_TaskQueue.empty())
	{
		TASK_RECORD* pTaskRecord = m_TaskQueue.front();

		switch(pTaskRecord->TaskToken)
		{
		case TASK_READ_PARTITION:
			{
				ReadPartition(pTaskRecord->Argument1);
			}
			break;
		case TASK_ANALYSIS_PARTITION:
			{
				AnalysisPartition(pTaskRecord->Argument1);
			}
			break;
		case TASK_RECOVER_FILE:
			{
				if(pTaskRecord->Argument3 != NULL)
				{
					{
						m_hFile = CreateFile(
							pTaskRecord->Argument4, 
							GENERIC_WRITE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							0,
							CREATE_ALWAYS,
							0,
							0);
						if(m_hFile != INVALID_HANDLE_VALUE)
						{
							if(m_PartitionType == FS_FAT32)
								CopyFAT32File(pTaskRecord->Argument1, pTaskRecord->Argument2);
							else if(m_PartitionType == FS_NTFS)
								CopyNTFSFile(pTaskRecord->Argument1);
						}
					}
				}
			}
			break;
		case TASK_DEEPSCAN:
			{
				DeepScan(pTaskRecord->Argument1, pTaskRecord->Argument2, pTaskRecord->Argument3, pTaskRecord->Argument4);
			}
			break;	
		}

		m_TaskQueue.pop();
	}
}

ULONG WINAPI KernelThreadProc(LPVOID lpvoid)
{
	while(1)
	{
		WaitForSingleObject(m_hTaskEvent, INFINITE);

		QueryTaskQueue();

		Sleep(10);
	}

	return -1;
}

void Notify(WPARAM wParam, LPARAM lParam, BOOL bAsyncFlag)
{
	if(bAsyncFlag == FALSE)
		::SendMessage(m_hMainWnd, WM_USER_MSG, wParam, lParam);
	else
		::PostMessage(m_hMainWnd, WM_USER_MSG, wParam, lParam);
}

//
//
// Global
//
//
void ReadPartition(ULONGLONG ullMode)
{
	ClearPartitionItems();
	ClearDeviceItems();

	TCHAR tcDevice[100];
	wmemset(tcDevice, 0, 100);
	HANDLE hPhysicalDrive = INVALID_HANDLE_VALUE;
	for(ULONG ulPos = 0; ulPos < 26; ulPos++)
	{
		wsprintf(tcDevice, L"\\\\.\\PHYSICALDRIVE%d", ulPos);
		hPhysicalDrive = CreateFile(
			tcDevice,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			0,
			OPEN_EXISTING,
			0,
			0);
		if(hPhysicalDrive != INVALID_HANDLE_VALUE)
		{
			m_DeviceSet.insert(hPhysicalDrive);

			DWORD dwBytesRet = 0;
			DeviceIoControl(
				hPhysicalDrive,
				IOCTL_DISK_GET_DRIVE_GEOMETRY,
				NULL,
				0,
				&m_DiskGeometry,
				sizeof(m_DiskGeometry),
				&dwBytesRet,
				NULL);

			m_ulBytesPerSector = m_DiskGeometry.BytesPerSector;
			m_hPhysicalDrive = hPhysicalDrive;

			if(m_DiskGeometry.MediaType == FixedMedia)
				ReadDPT(0, 0);
			else
				ReadDBR(0);

			//
			//
			
			if(ullMode != MODE_NORMAL)
				RestoreDBR(ullMode);
		}
	}

	std::vector<PARTITION_ITEM*>::iterator iter;
	for(iter = m_PartitionItemList.begin();
		iter != m_PartitionItemList.end();
		iter++)
	{
		PARTITION_ITEM* pItem = *iter;
		if(pItem != NULL)
			Notify(MSG_PARTITION, (LPARAM)pItem, FALSE);
	}

	Notify(MSG_DONE_INITIALIZE_PARTITION, 0, FALSE);
}

void AnalysisPartition(ULONGLONG ullStartSector)
{
	m_PartitionType = GetPartitionType(ullStartSector);
	switch(m_PartitionType)
	{
	case FS_NTFS:
		AnalysisNTFSDBR(ullStartSector);
		break;
	case FS_FAT32:
		AnalysisFAT32DBR(ullStartSector);
		break;
	}
}

void ReadDPT(ULONGLONG ullSector, ULONGLONG ullOffset)
{
	LPBYTE lpMBRBuffer = new BYTE[m_ulBytesPerSector];
	if(lpMBRBuffer != NULL)
	{
		if(ReadSector(ullSector, 1, lpMBRBuffer))
		{
			LPBYTE lpBuffer = lpMBRBuffer + 446;
			for(ULONG ulPos = 0; ulPos < 4; ulPos++, lpBuffer += 16)
			{
				PARTITION_TABLE table = {0};
				memcpy(&table, lpBuffer, 16);
				if(table.PartitionType != 0x00)
				{
					//
					// 0x0F或者0x05代表分区表，递归解析
					//
					if(table.PartitionType == 0x0F || table.PartitionType == 0x05)
					{
						ullSector = ullOffset + table.ReservedSectors;
						if(ullOffset == 0) 
							ullOffset = table.ReservedSectors;
						ReadDPT(ullSector, ullOffset);
					}
					else
					{
						ReadDBR(ullSector + table.ReservedSectors);

						//if(table.PartitionType == 0x07)
						//{
						//	GetNTFSDriveInfo(ullSector + table.ReservedSectors, table.SectorsCount);
						//}
						//else if(table.PartitionType == 0x0B || table.PartitionType == 0x0C)
						//{
						//	GetFAT32DriveInfo(ullSector + table.ReservedSectors, table.SectorsCount);
						//}
					}
				}
			}
		}
		
		delete[] lpMBRBuffer;
	}
}

//
// 通过遍历磁盘扇区的方式搜索分区
//
void RestoreDBR(ULONGLONG ullMode)
{
	Notify(MSG_UPDATE_STATUS, (LPARAM)L"正在搜索历史分区", FALSE);

	switch(ullMode)
	{
	case MODE_RESTORE:
		{
			ULONGLONG ullCylinderCount = m_DiskGeometry.Cylinders.QuadPart;
			for(ULONGLONG ullCylinder = 0; ullCylinder <= ullCylinderCount; ullCylinder++)
			{
				Notify(MSG_UPDATE_PROGRESS, (LPARAM)ullCylinder*100/ullCylinderCount, TRUE);
				ULONGLONG ullSector = GetCylinderSector(ullCylinder);
				for(ULONG ulPos = 0; ulPos < 2; ulPos++, ullSector += ulPos*m_DiskGeometry.SectorsPerTrack)
					ReadDBR(ullSector);
			}
		}
		break;
	case MODE_INTELLECT:
		{

		}
		break;
	}
}

//
// 解析分区DBR
//
void ReadDBR(ULONGLONG ullSector)
{
	PARTITION_TYPE PartitionType = GetPartitionType(ullSector);
	switch(PartitionType)
	{
	case FS_NTFS:
		GetNTFSPartitionInfo(ullSector);
		break;
	case FS_FAT32:
		GetFAT32PartitionInfo(ullSector);
		break;
	}

	////
	//// 搜索分区的备份DBR，以此可以恢复出损坏的分区、历史分区
	////
	//if(ReadSector(ullSector - 1, 1, lpSectorBuffer))
	//{
	//	if(*(WORD*)(lpSectorBuffer + 510) == 0xAA55) //找到某个
	//	{
	//		if(lpSectorBuffer[0x00] == 0xEB && lpSectorBuffer[0x01] == 0x52 && 
	//			lpSectorBuffer[0x02] == 0x90 && lpSectorBuffer[0x03] == 0x4E &&
	//			lpSectorBuffer[0x04] == 0x54   && lpSectorBuffer[0x05] == 0x46 &&
	//			lpSectorBuffer[0x06] == 0x53)
	//		{
	//			//
	//			// llSector - 1 - *(ULONGLONG*)(lpSectorBuffer + 0x28) + 1
	//			//
	//			GetNTFSDriveInfo(ullSector - *(ULONGLONG*)(lpSectorBuffer + 0x28), *(ULONGLONG*)(lpSectorBuffer + 0x28) + 1);
	//		}
	//	}
	//}

	//if(ReadSector(ullSector + 6, 1, lpSectorBuffer))
	//{
	//	if(*(WORD*)(lpSectorBuffer + 510) == 0xAA55) //找到某个
	//	{
	//		if(lpSectorBuffer[0x00] == 0xEB && lpSectorBuffer[0x01] == 0x58 &&
	//			lpSectorBuffer[0x02] == 0x90 && lpSectorBuffer[0x03] == 0x4D &&
	//			lpSectorBuffer[0x04] == 0x53   && lpSectorBuffer[0x52] == 0x46 &&
	//			lpSectorBuffer[0x53] == 0x41 && lpSectorBuffer[0x54] == 0x54)
	//		{
	//			GetFAT32DriveInfo(ullSector - *(ULONGLONG*)(lpSectorBuffer + 0x20), *(ULONGLONG*)(lpSectorBuffer + 0x20));
	//		}
	//	}
	//}
}

void DeepScan(ULONGLONG ullStartSector, ULONGLONG ullSectorCount, ULONGLONG ullMaxSectors, TCHAR* pPath)
{
	Notify(MSG_UPDATE_STATUS, (LPARAM)L"正在进行深度扫描", FALSE);

	ULONG ulPathLen = wcslen(pPath);

	ULONG ulFileFound = 0;
	ULONG ulSectorsPerBlock = 256;
	ULONGLONG ullSectorRead = 0;
	ULONGLONG ullTotalSectors = ullSectorCount;

	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector*ulSectorsPerBlock];
	if(lpSectorBuffer != NULL)
	{
		BOOL bIsRun = FALSE;
		ULONGLONG ullDataStartSector = 0;
		ULONGLONG ullDataSectorCount = 0;

		while(ullSectorCount > ulSectorsPerBlock)
		{
			memset(lpSectorBuffer, 0, m_ulBytesPerSector*ulSectorsPerBlock);
			if(ReadSector(ullStartSector, ulSectorsPerBlock, lpSectorBuffer))
			{
				LPBYTE lpBuffer = lpSectorBuffer;

				for(ULONGLONG ullPos = 0;
					ullPos < ulSectorsPerBlock;
					ullPos++)
				{
					BOOL bIsMatched = IsMatched(lpBuffer);

					//
					// 首次匹配到
					//
					if(bIsRun == FALSE && bIsMatched > -1)
					{
						bIsRun = TRUE;
						ullDataStartSector = ullStartSector + ullPos;
						ullDataSectorCount = 1;
					}

					//
					// 搜索过程中再次匹配到另一个特征码，或者搜索到的数据长度已经达到上限便结束当前搜索
					// 表示上一个文件的数据搜索完毕，从当前位置起搜索另一个文件的数据
					//
					else if(bIsRun == TRUE && (bIsMatched > -1 || ullDataSectorCount >= 10240/*DATA_MAX_SECTORS*/))
					{
						bIsRun = TRUE;
						ulFileFound++;

						//
						// Path + \\ + DeepScanFileXXX.XXX + \0
						//
						TCHAR* pFilePath = new TCHAR[ulPathLen + 21];
						wmemset(pFilePath, 0, ulPathLen + 21);
						wsprintf(pFilePath, L"%ws\\DeepScanFile%003d.%ws", pPath, ulFileFound, FileSignature[bIsMatched].Extension);

						HANDLE hFile = CreateFile(
							pFilePath, 
							GENERIC_WRITE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							0,
							CREATE_ALWAYS,
							0,
							0);
						if(hFile != INVALID_HANDLE_VALUE)
						{
							LPBYTE lpDataBuffer = new BYTE[m_ulBytesPerSector];
							for(ULONGLONG ullPos = 0; ullPos < ullDataSectorCount; ullPos++)
							{
								DWORD dwBytesWrite = 0;
								DWORD dwBytesRead = ReadSector(ullDataStartSector + ullPos, 1, lpDataBuffer);

								DWORD dwFileSizeHigh;
								DWORD dwFileSizeLow;
								dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
								SetFilePointer(hFile, dwFileSizeLow, (PLONG)&dwFileSizeHigh, FILE_BEGIN);
								WriteFile(hFile, lpDataBuffer, dwBytesRead, &dwBytesWrite, NULL);
							}

							delete[] lpDataBuffer;
							CloseHandle(hFile);
						}

						delete[] pFilePath;

						//
						// 结束是因为找到下一个特征码
						//
						if(bIsMatched > -1)
						{
							bIsRun = TRUE;

							ullDataStartSector = ullStartSector + ullPos;
							ullDataSectorCount = 1;
						}
						//
						// 结束是因为长度达到上限
						//
						else
						{
							bIsRun = FALSE;

							ullDataStartSector = 0;
							ullDataSectorCount = 0;
						}
					}
					//
					// 匹配失败，但是正在搜索
					// 表示正在搜索一个文件的数据，扇区数+1
					//
					else if(bIsRun == TRUE && bIsMatched == -1)
						ullDataSectorCount++;

					lpBuffer += m_ulBytesPerSector;
				}

				ullStartSector += ulSectorsPerBlock;
				ullSectorCount -= ulSectorsPerBlock;
				ullSectorRead += ulSectorsPerBlock;
				Notify(MSG_UPDATE_PROGRESS, (LPARAM)ullSectorRead*100/ullTotalSectors, TRUE);
			}
		}

		if(ullSectorCount > 0)
		{
			memset(lpSectorBuffer, 0, m_ulBytesPerSector*ulSectorsPerBlock);
			if(ReadSector(ullSectorRead, ullSectorCount, lpSectorBuffer))
			{
				LPBYTE lpBuffer = lpSectorBuffer;

				for(ULONGLONG ullPos = 0;
					ullPos < ullSectorCount;
					ullPos++)
				{
					BOOL bIsMatched = IsMatched(lpBuffer);

					if(bIsRun == FALSE && bIsMatched > -1)
					{
						bIsRun = TRUE;
						ullDataStartSector = ullSectorRead + ullPos;
						ullDataSectorCount = 1;					
					}

					else if(bIsRun == TRUE && (bIsMatched > -1 || ullDataSectorCount >= 10240/*DATA_MAX_SECTORS*/))
					{
						bIsRun = TRUE;
						ulFileFound++;
						
						TCHAR* pFilePath = new TCHAR[ulPathLen + 21];
						wmemset(pFilePath, 0, ulPathLen + 21);
						wsprintf(pFilePath, L"%ws\\DeepScanFile%003d.%ws", pPath, ulFileFound, FileSignature[bIsMatched].Extension);

						HANDLE hFile = CreateFile(
							pFilePath, 
							GENERIC_WRITE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							0,
							CREATE_ALWAYS,
							0,
							0);
						if(hFile != INVALID_HANDLE_VALUE)
						{
							LPBYTE lpDataBuffer = new BYTE[m_ulBytesPerSector];
							for(ULONGLONG ullPos = 0; ullPos < ullDataSectorCount; ullPos++)
							{
								DWORD dwBytesWrite = 0;
								DWORD dwBytesRead = ReadSector(ullDataStartSector + ullPos, 1, lpDataBuffer);

								DWORD dwFileSizeHigh;
								DWORD dwFileSizeLow;
								dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
								SetFilePointer(hFile, dwFileSizeLow, (PLONG)&dwFileSizeHigh, FILE_BEGIN);
								WriteFile(hFile, lpDataBuffer, dwBytesRead, &dwBytesWrite, NULL);
							}

							delete[] lpDataBuffer;
							CloseHandle(hFile);
						}

						delete[] pFilePath;

						if(bIsMatched > -1)
						{
							bIsRun = TRUE;

							ullDataStartSector = ullSectorRead + ullPos;
							ullDataSectorCount = 1;
						}
						//
						// 结束是因为长度达到上限
						//
						else
						{
							bIsRun = FALSE;

							ullDataStartSector = 0;
							ullDataSectorCount = 0;
						}
					}

					else if(bIsRun == TRUE && bIsMatched == -1)
						ullDataSectorCount++;

					lpBuffer += m_ulBytesPerSector;
				}

				ullSectorRead += ulSectorsPerBlock;
				ullSectorCount -= ulSectorsPerBlock;				
				ullSectorRead += ulSectorsPerBlock;
				Notify(MSG_UPDATE_PROGRESS, (LPARAM)ullSectorRead*100/ullTotalSectors, TRUE);
			}
		}

		delete[] lpSectorBuffer;
	}
}

BOOL IsMatched(LPBYTE lpBuffer)
{
	BOOL Signature = -1;

	for(ULONG ulPos = 0; ulPos < ulFileSignatureCnt; ulPos++)
	{
		CRegexpT <BYTE> regexp(FileSignature[ulPos].SignatureHead);
		MatchResult result = regexp.Match(lpBuffer + FileSignature[ulPos].SignatureHeadOffset);

		if(result.IsMatched() == TRUE)
		{
			Signature = ulPos;
			break;
		}
	}

	return Signature;
}

ULONG ReadSector(ULONGLONG ullSector, ULONGLONG ullSectorCount, LPBYTE lpSectorBuffer)
{
	DWORD dwBytesRead = 0;
	LARGE_INTEGER li;
	li.QuadPart = ullSector*m_ulBytesPerSector;

	OVERLAPPED Overlapped = {0, 0, 0, 0, NULL};
	Overlapped.Offset = li.LowPart;
	Overlapped.OffsetHigh = li.HighPart;

	ReadFile(m_hPhysicalDrive, lpSectorBuffer, m_DiskGeometry.BytesPerSector*ullSectorCount, &dwBytesRead, &Overlapped);

	return dwBytesRead;
}

ULONGLONG GetCylinderSector(ULONGLONG ullCylinder)
{
	ULONGLONG ullCylinderSector = ullCylinder*m_DiskGeometry.TracksPerCylinder*m_DiskGeometry.SectorsPerTrack;
	return ullCylinderSector;
}

ULONGLONG GetTrackSector(ULONGLONG ullTrack)
{
	ULONGLONG ullTrackSector = ullTrack*m_DiskGeometry.SectorsPerTrack;
	return ullTrackSector;
}

PARTITION_TYPE GetPartitionType(ULONGLONG ullSector)
{
	PARTITION_TYPE PartitionType = FS_UNKNOWN;
	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector];
	if(lpSectorBuffer != NULL)
	{
		if(ReadSector(ullSector, 1, lpSectorBuffer))
		{
			if(*(USHORT*)(lpSectorBuffer + 510) == 0xAA55)
			{
				if(lpSectorBuffer[0x00] == 0xEB && lpSectorBuffer[0x01] == 0x52 && 
					lpSectorBuffer[0x02] == 0x90 && lpSectorBuffer[0x03] == 0x4E &&
					lpSectorBuffer[0x04] == 0x54 && lpSectorBuffer[0x05] == 0x46 &&
					lpSectorBuffer[0x06] == 0x53)
				{
					PartitionType = FS_NTFS;
				}
				else if(lpSectorBuffer[0x00] == 0xEB && lpSectorBuffer[0x01] == 0x58 &&
					lpSectorBuffer[0x02] == 0x90 && lpSectorBuffer[0x03] == 0x4D &&
					lpSectorBuffer[0x04] == 0x53 && lpSectorBuffer[0x52] == 0x46 &&
					lpSectorBuffer[0x53] == 0x41 && lpSectorBuffer[0x54] == 0x54)
				{
					PartitionType = FS_FAT32;
				}			
			}
		}

		delete[] lpSectorBuffer;
	}

	return PartitionType;
}

BOOL IsPartitionRecorded(ULONGLONG ullSector)
{
	std::set<ULONGLONG>::iterator iter;
	iter = m_PartitionSet.find(ullSector);
	if(iter != m_PartitionSet.end())
		return TRUE;
	else
		return FALSE;
}

void ClearPartitionItems()
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
	m_PartitionSet.clear();
}

void ClearDeviceItems()
{
	std::set<HANDLE>::iterator iter;
	for(iter = m_DeviceSet.begin();
		iter != m_DeviceSet.end();
		iter++)
	{
		CloseHandle(*iter);
	}
	m_DeviceSet.clear();
}

//
//
// NTFS
//
//
void GetNTFSPartitionInfo(ULONGLONG ullStartSector)
{
	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector];
	if(lpSectorBuffer != NULL)
	{
		if(ReadSector(ullStartSector, 1, lpSectorBuffer))
		{
			if(memcmp(lpSectorBuffer + 3, "NTFS", 4) == 0)
			{
				memcpy(&m_NTFSRootSector, lpSectorBuffer, m_ulBytesPerSector);
				delete[] lpSectorBuffer;
		
				m_ullStartSector = ullStartSector;

				//
				// 真实的扇区数目要比DBR中记录的值多一个，因为还有一个扇区用来备份DBR了
				//
				m_ullSectorCount = m_NTFSRootSector.TotalSectors + 1;
				m_ulFileRecordSize = GetFileRecordSize();

				PARTITION_ITEM* pItem = new PARTITION_ITEM;
				pItem->PartitionType = FS_NTFS;
				pItem->StartSector = m_ullStartSector;
				pItem->SectorCount = m_ullSectorCount;
				pItem->DeviceHandle = m_hPhysicalDrive;
				pItem->Volume = ReadNTFSVolume(ullStartSector + m_NTFSRootSector.ReservedSectors);
				if(pItem->Volume == NULL)
				{
					pItem->Volume = new TCHAR[5];
					wmemset(pItem->Volume, 0, 5);
					wmemcpy(pItem->Volume, L"未初始化", 4);
				}

				m_PartitionItemList.push_back(pItem);
				m_PartitionSet.insert(ullStartSector);
			}
		}
	}		
}

void AnalysisNTFSDBR(ULONGLONG ullStartSector)
{
	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector];
	if(lpSectorBuffer != NULL)
	{
		if(ReadSector(ullStartSector, 1, lpSectorBuffer))
		{
			if(memcmp(lpSectorBuffer + 3, "NTFS", 4) == 0)
			{
				memcpy(&m_NTFSRootSector, lpSectorBuffer, m_ulBytesPerSector);
				delete[] lpSectorBuffer;
		
				m_ullStartSector = ullStartSector;
				//
				// 真实的扇区数目要比DBR中记录的值多一个，因为还有一个扇区用来备份DBR了
				//
				m_ullSectorCount = m_NTFSRootSector.TotalSectors + 1;

				ClearNTFSItems();
				//
				// 通过对R-Studio的逆向可以发现，它读取磁盘时每次读取256个扇区
				// 因此每次读取的MFT数目就是128个
				//
				ULONG ulRecordsPerBlock = 128;

				m_ulFileRecordSize = GetFileRecordSize();
				m_ullStartFileRecordOffset = GetStartFileRecordOffset();
				m_ulFileRecordCount = GetFileRecordCount();

				//
				// 分块解析MFT，减少磁盘IO
				//
				ULONG ulRecordRead = 0;
				Notify(MSG_UPDATE_STATUS, (LPARAM)L"正在初始化", FALSE);
				std::vector<FILE_RECORD_BLOCK*>::iterator iter_block;
				for(iter_block = m_FileRecordBlock.begin();
					iter_block != m_FileRecordBlock.end();
					iter_block++)
				{
					FILE_RECORD_BLOCK* pBlock = *iter_block;
					if(pBlock != NULL)
					{
						ULONG ulStartRecord = pBlock->StartRecord;
						ULONG ulRecordCount = pBlock->RecordCount;

						while(ulRecordCount > ulRecordsPerBlock)
						{
							AnalysisMFT(ulStartRecord, ulRecordsPerBlock);
							ulStartRecord += ulRecordsPerBlock;
							ulRecordCount -= ulRecordsPerBlock;
							ulRecordRead += ulRecordsPerBlock;
							Notify(MSG_UPDATE_PROGRESS, (LPARAM)ulRecordRead*100/m_ulFileRecordCount, FALSE);
						}
						if(ulRecordCount > 0)
						{
							AnalysisMFT(ulStartRecord, ulRecordCount);
							ulRecordCount -= ulRecordsPerBlock;
							ulRecordRead += ulRecordsPerBlock;
							Notify(MSG_UPDATE_PROGRESS, (LPARAM)ulRecordRead*100/m_ulFileRecordCount, FALSE);
						}

						assert(ulRecordCount == 0 && L"初始化失败");
					}
				}

				//
				// 所有MFT项已经解析完毕，相关数据均存放在m_FileRecordMap中
				//
				ulRecordRead = 0;
				Notify(MSG_UPDATE_STATUS, (LPARAM)L"正在解析分区", FALSE);
				std::map<ULONG, FILE_RECORD*>::iterator iter_record;
				for(iter_record = m_FileRecordMap.begin();
					iter_record != m_FileRecordMap.end();
					iter_record++)
				{
					FILE_RECORD* pRecord = iter_record->second;
					if(pRecord != NULL && pRecord->IsDirectory == FALSE)
					{
						NTFS_ITEM* pItem = new NTFS_ITEM;

						pItem->IsDeleted = pRecord->IsDeleted;
						pItem->Size = pRecord->Size;
						pItem->FileReferenceNumber = pRecord->FileReferenceNumber;
						wmemset(pItem->CreateDate, 0, 20);
						wmemcpy(pItem->CreateDate, pRecord->CreateTime, 20);
						pItem->Name = GetFileRecordPath(pRecord->FileReferenceNumber);

						Notify(MSG_NTFS_ITEM, (LPARAM)pItem, FALSE);

						if(pItem->Name != NULL)
							delete[] pItem->Name;
						delete pItem;
					}

					if(ulRecordRead++%500 == 0)
						Notify(MSG_UPDATE_PROGRESS, (LPARAM)ulRecordRead*100/m_ulFileRecordCount, FALSE);
				}

				Notify(MSG_DONE_SCAN_PARTITION, 0, FALSE);
			}
		}
	}
}

//
// 很关键，创建所有MFT的相关索引，增加了内存占用，但是大大减少了解析所需时间
// （整个解析过程的主要耗时用在了GetMFTPath这个函数上，还有优化的可能）
//
void AnalysisMFT(ULONG ulFileReferenceNumber, ULONG ulFileRecordCount)
{
	LPBYTE lpFileRecordBuffer = new BYTE[m_ulFileRecordSize*ulFileRecordCount];
	if(lpFileRecordBuffer != NULL)
	{
		if(ReadFileRecord(ulFileReferenceNumber, ulFileRecordCount, lpFileRecordBuffer))
		{
			LPBYTE lpBuffer = lpFileRecordBuffer;
			for(ULONGLONG ullPos = 0; ullPos < ulFileRecordCount; ullPos++, lpBuffer += m_ulFileRecordSize)
			{
				if(memcmp(lpBuffer, "FILE", 4) == 0)
				{
					MFT_INFO* pMftInfo = (MFT_INFO*)lpBuffer;

					USHORT usUpdateSequenceOffset = pMftInfo->UpdateSequenceOffset;
					USHORT usUpdateSequenceCount = pMftInfo->UpdateSequenceCount - 1;
					for(USHORT usPos = 0; usPos < usUpdateSequenceCount; usPos ++)
						*(USHORT*)(lpBuffer + m_ulBytesPerSector*usPos + 510) = *(USHORT*)(lpBuffer + usUpdateSequenceOffset + 2 + 2*usPos);

					MFT_ATTRIBUTE* pMftAttr = NULL;
					for(pMftAttr = (MFT_ATTRIBUTE*)(lpBuffer + pMftInfo->AttributeOffset);
						pMftAttr->AttributeType != ATTRIBUTE_FILENAME && pMftAttr->AttributeType != ATTRIBUTE_END;
						pMftAttr = (MFT_ATTRIBUTE*)((LPBYTE)pMftAttr + pMftAttr->AttributeLength));

					if(pMftAttr->AttributeType == ATTRIBUTE_FILENAME)
					{
						MFT_NAME* pMftName = (MFT_NAME*)((LPBYTE)pMftAttr + pMftAttr->RESIDENT.AttributeOffset);
						if(pMftName->NameSpace == 2)
						{
							pMftAttr = (MFT_ATTRIBUTE*)((LPBYTE)pMftAttr + pMftAttr->AttributeLength);
							pMftName = (MFT_NAME*)((LPBYTE)pMftAttr + pMftAttr->RESIDENT.AttributeOffset);
						}

						FILE_RECORD* pRecord = new FILE_RECORD();
						pRecord->FileReferenceNumber = pMftInfo->FileReferenceNumber;
						pRecord->DirectoryFileReferenceNumber = pMftName->DirectoryFileReferenceNumber;
						pRecord->IsDeleted = (pMftInfo->Flag == 0x00) ? TRUE : FALSE;
						pRecord->IsDirectory = (pMftInfo->Flag == 0x02 || pMftInfo->Flag == 0x03) ? TRUE : FALSE;

						//
						// 获取文件数据大小
						// 30属性中的数据大小可能是0，这种情况下必须从80属性中获取
						//
						if(pMftName->FileAttribute&0x10000000)
							pRecord->Size = 0;
						else
						{
							for(pMftAttr = (MFT_ATTRIBUTE*)(lpBuffer + pMftInfo->AttributeOffset);
								pMftAttr->AttributeType != ATTRIBUTE_DATA && pMftAttr->AttributeType != ATTRIBUTE_END;
								pMftAttr = (MFT_ATTRIBUTE*)((LPBYTE)pMftAttr + pMftAttr->AttributeLength));

							//
							// 其实这里应该有两种可能
							// 1、只存在80属性
							// 2、存在20属性
							// 第二种情况太纠结了，先不考虑
							//
							if(pMftAttr->AttributeType == ATTRIBUTE_DATA)
							{
								if(!pMftAttr->Symbol)
									pRecord->Size = pMftAttr->RESIDENT.AttributeLength;
								else
									pRecord->Size = pMftAttr->NONRESIDENT.RealSize;
							}
						}

						FILETIME FileTime = {0};
						memcpy(&FileTime, &pMftName->CreateTime, 8);
						SYSTEMTIME SystemTime = {0};
						FileTimeToSystemTime(&FileTime, &SystemTime);
						wmemset(pRecord->CreateTime, 0, 20);
						wsprintf(pRecord->CreateTime, L"%d.%d.%d %d:%d:%d",
							SystemTime.wYear, 
							SystemTime.wMonth, 
							SystemTime.wDay,
							SystemTime.wHour+8, 
							SystemTime.wMinute,
							SystemTime.wSecond);

						ULONG ulNameLen = pMftName->NameLength;
						pRecord->Name = new TCHAR[ulNameLen + 1];
						wmemset(pRecord->Name, 0, ulNameLen + 1);
						wmemcpy(pRecord->Name, pMftName->Name, ulNameLen);

						m_FileRecordMap[pMftInfo->FileReferenceNumber] = pRecord;
					}
				}
			}
		}
		
		delete[] lpFileRecordBuffer;
	}
}

void CopyNTFSFile(ULONG ulFileReferenceNumber)
{
	LPBYTE lpFileRecordBuffer = new BYTE[m_ulFileRecordSize];
	if(lpFileRecordBuffer != NULL)
	{
		if(ReadFileRecord(ulFileReferenceNumber, 1, lpFileRecordBuffer))
		{
			if(memcmp(lpFileRecordBuffer, "FILE", 4) == 0)
			{
				MFT_INFO* pMftInfo = (MFT_INFO*)lpFileRecordBuffer;

				USHORT usUpdateSequenceOffset = pMftInfo->UpdateSequenceOffset;
				USHORT usUpdateSequenceCount = pMftInfo->UpdateSequenceCount - 1;
				for(USHORT usPos = 0; usPos < usUpdateSequenceCount; usPos ++)
					*(USHORT*)(lpFileRecordBuffer + m_ulBytesPerSector*usPos + 510) = *(USHORT*)(lpFileRecordBuffer + usUpdateSequenceOffset + 2 + 2*usPos);

				MFT_ATTRIBUTE* pMftAttr = NULL;

				//
				// 解析20属性
				//
				for(pMftAttr = (MFT_ATTRIBUTE*)(lpFileRecordBuffer + pMftInfo->AttributeOffset);
					pMftAttr->AttributeType != ATTRIBUTE_ATTRIBUTELIST && pMftAttr->AttributeType != ATTRIBUTE_END;
					pMftAttr = (MFT_ATTRIBUTE*)((LPBYTE)pMftAttr + pMftAttr->AttributeLength));

				if(pMftAttr->AttributeType == ATTRIBUTE_ATTRIBUTELIST)
				{
					ULONG ulAttrListCount = (pMftAttr->AttributeLength - pMftAttr->RESIDENT.AttributeOffset)/32;
					LPBYTE lpBuffer = new BYTE[ulAttrListCount*32];
					if(lpBuffer != NULL)
					{
						memcpy(lpBuffer, (LPBYTE)pMftAttr + pMftAttr->RESIDENT.AttributeOffset, ulAttrListCount*32);
						for(ULONG ulPos = 0; ulPos < ulAttrListCount; ulPos++, lpBuffer +=32)
						{
							if(*lpBuffer&0x80)
							{
								ULONG ulDataRun = lpBuffer[16] | lpBuffer[17] << 8 | lpBuffer[18] << 16 | lpBuffer[19] <<24 ;
								if(ulDataRun != ulFileReferenceNumber)
									CopyNTFSFile(ulDataRun);
							}
						}

						delete[] lpBuffer;
					}
				}

				//
				// 解析80属性
				//
				for(pMftAttr = (MFT_ATTRIBUTE*)(lpFileRecordBuffer + pMftInfo->AttributeOffset);
					pMftAttr->AttributeType != ATTRIBUTE_DATA && pMftAttr->AttributeType != ATTRIBUTE_END;
					pMftAttr = (MFT_ATTRIBUTE*)((LPBYTE)pMftAttr + pMftAttr->AttributeLength));

				if(pMftAttr->AttributeType == ATTRIBUTE_DATA)
				{
					if(!pMftAttr->Symbol)
					{
						DWORD dwBytesWrite = 0;
						WriteFile(
							m_hFile, 
							(LPBYTE)pMftAttr + pMftAttr->RESIDENT.AttributeOffset, 
							pMftAttr->RESIDENT.AttributeLength, 
							&dwBytesWrite, 
							NULL);
						CloseHandle(m_hFile);
					}
					else
					{
						ULONG ulStartCluster = 0;
						ULONG ulClusterCount = 0;
						ULONG nDataRunsLen = pMftAttr->AttributeLength - 64;
						ULONG nDataRunsRead = 0;

						LPBYTE lpBuffer = (LPBYTE)pMftAttr + pMftAttr->NONRESIDENT.DataRunOffset;

						LPBYTE lpDataBuffer = new BYTE[m_ulBytesPerSector];
						if(lpDataBuffer != NULL)
						{
							while(nDataRunsRead < nDataRunsLen && *lpBuffer != 0x00)
							{
								//
								// LCN可为负
								//
								LONG lLCN = 0;
								ULONG ulLCNCount = 0;

								BYTE byLCNLen = lpBuffer[0]&0x0F;
								BYTE byLCNCountLen = (lpBuffer[0]&0xF0) >> 4;

								//
								// 带符号转换
								//
								lLCN = (char)(lpBuffer[byLCNLen + byLCNCountLen]);
								for(LONG lPos = byLCNLen +byLCNCountLen - 1; lPos > byLCNLen; --lPos)
									lLCN = (lLCN << 8) + lpBuffer[lPos];
								for(ULONG ulPos = byLCNLen; ulPos > 0; ulPos--)
									ulLCNCount = (ulLCNCount << 8) + lpBuffer[ulPos];

								ulStartCluster += lLCN;
								ulClusterCount = ulLCNCount;

								lpBuffer += byLCNLen + byLCNCountLen + 1;

								ULONGLONG ullStartSector = m_ullStartSector + ulStartCluster*m_NTFSRootSector.SectorsPerCluster;
								ULONGLONG ullSectorCount = ulClusterCount*m_NTFSRootSector.SectorsPerCluster;
								for(ULONGLONG ullPos = 0; ullPos < ullSectorCount; ullPos++)
								{
									DWORD dwBytesWrite = 0;
									DWORD dwBytesRead = ReadSector(ullStartSector + ullPos, 1, lpDataBuffer);

									DWORD dwFileSizeHigh;
									DWORD dwFileSizeLow;
									dwFileSizeLow = GetFileSize(m_hFile, &dwFileSizeHigh);
									SetFilePointer(m_hFile, dwFileSizeLow, (PLONG)&dwFileSizeHigh, FILE_BEGIN);
									WriteFile(m_hFile, lpDataBuffer, dwBytesRead, &dwBytesWrite, NULL);
								}
							}

							CloseHandle(m_hFile);
							delete[] lpDataBuffer;
						}
					}
				}
			}
		}
		
		delete[] lpFileRecordBuffer;
	}
}

ULONG ReadFileRecord(ULONG ulFileReferenceNumber, ULONG ulFileRecordCount, LPBYTE lpFileRecordBuffer)
{
	DWORD dwBytesRead = 0;
	LARGE_INTEGER li;
	li.QuadPart = GetFileRecordOffset(ulFileReferenceNumber);

	OVERLAPPED Overlapped = {0, 0, 0, 0, NULL};
	Overlapped.Offset = li.LowPart;
	Overlapped.OffsetHigh = li.HighPart;

	ReadFile(m_hPhysicalDrive, lpFileRecordBuffer, m_ulFileRecordSize*ulFileRecordCount, &dwBytesRead, &Overlapped);

	return dwBytesRead;
}

ULONG GetFileRecordCount()
{
	ULONG ulFileRecordCount = 0;
	LPBYTE lpFileRecordBuffer = new BYTE[m_ulFileRecordSize];
	if(lpFileRecordBuffer != NULL)
	{
		DWORD dwBytesRead = 0;
		LARGE_INTEGER li;
		li.QuadPart = m_ullStartFileRecordOffset;

		OVERLAPPED Overlapped = {0, 0, 0, 0, NULL};
		Overlapped.Offset = li.LowPart;
		Overlapped.OffsetHigh = li.HighPart;

		if(ReadFile(m_hPhysicalDrive, lpFileRecordBuffer, m_ulFileRecordSize, &dwBytesRead, &Overlapped))
		{
			if(memcmp(lpFileRecordBuffer,"FILE", 4) == 0)
			{
				MFT_INFO* pMftInfo = (MFT_INFO*)lpFileRecordBuffer;

				USHORT usUpdateSequenceOffset = pMftInfo->UpdateSequenceOffset;
				USHORT usUpdateSequenceCount = pMftInfo->UpdateSequenceCount - 1;
				for(USHORT usPos = 0; usPos < usUpdateSequenceCount; usPos ++)
					*(USHORT*)(lpFileRecordBuffer + m_ulBytesPerSector*usPos + 510) = *(USHORT*)(lpFileRecordBuffer + usUpdateSequenceOffset + 2 + 2*usPos);

				MFT_ATTRIBUTE* pMftAttr = NULL;
				for(pMftAttr = (MFT_ATTRIBUTE*)(lpFileRecordBuffer + pMftInfo->AttributeOffset);
					pMftAttr->AttributeType != ATTRIBUTE_DATA && pMftAttr->AttributeType != ATTRIBUTE_END;
					pMftAttr = (MFT_ATTRIBUTE*)((LPBYTE)pMftAttr + pMftAttr->AttributeLength));

				if(pMftAttr->AttributeType == ATTRIBUTE_DATA)
				{
					if(pMftAttr->Symbol)
					{
						ULONG ulStartCluster = 0;
						ULONG ulClusterCount = 0;
						ULONG nDataRunsLen = pMftAttr->AttributeLength - 64;
						ULONG nDataRunsRead = 0;

						LPBYTE lpBuffer = (LPBYTE)pMftAttr + pMftAttr->NONRESIDENT.DataRunOffset;
						while(nDataRunsRead < nDataRunsLen && *lpBuffer != 0x00)
						{
							LONG lLCN = 0;
							ULONG ulLCNCount = 0;

							BYTE byLCNLen = lpBuffer[0]&0x0F;
							BYTE byLCNCountLen = (lpBuffer[0]&0xF0) >> 4;

							//
							// 带符号转换
							//
							lLCN = (char)(lpBuffer[byLCNLen + byLCNCountLen]);
							for(LONG lPos = byLCNLen +byLCNCountLen - 1; lPos > byLCNLen; --lPos)
								lLCN = (lLCN << 8) + lpBuffer[lPos];
							for(ULONG ulPos = byLCNLen; ulPos > 0; ulPos--)
								ulLCNCount = (ulLCNCount << 8) + lpBuffer[ulPos];

							ulStartCluster += lLCN;
							ulClusterCount = ulLCNCount;
							
							lpBuffer += byLCNLen + byLCNCountLen + 1;

							FILE_RECORD_BLOCK* pBlock = new FILE_RECORD_BLOCK;
							pBlock->StartCluster = ulStartCluster;
							pBlock->RecordCount = ulClusterCount*4;
							pBlock->StartRecord = ulFileRecordCount;
							m_FileRecordBlock.push_back(pBlock);
							
							ulFileRecordCount += ulClusterCount*m_NTFSRootSector.SectorsPerCluster/2;
						}
					}
				}
			}
		}

		delete[] lpFileRecordBuffer;
	}
	return ulFileRecordCount;
}

ULONG GetFileRecordSize()
{
	return 0x01<<((-1)*((char)m_NTFSRootSector.ClustersPerFileRecord));
}

ULONGLONG GetFileRecordOffset(ULONG ulFileReferenceNumber)
{
int n1 = GetTickCount();
	ULONGLONG ullOffset = 0;
	std::vector<FILE_RECORD_BLOCK*>::iterator iter;
	for(iter = m_FileRecordBlock.begin();
		iter != m_FileRecordBlock.end();
		iter++)
	{
		FILE_RECORD_BLOCK* pBlock = *iter;
		if(pBlock != NULL)
		{
			ULONG ulStartCluster = pBlock->StartCluster;
			ULONG ulStartRecord = pBlock->StartRecord;
			ULONG ulRecordCount = pBlock->RecordCount;
			if(ulFileReferenceNumber < ulStartRecord + ulRecordCount)
			{
				ullOffset = (ulStartCluster*m_NTFSRootSector.SectorsPerCluster
					+ (ulFileReferenceNumber - ulStartRecord)*2
					+ m_ullStartSector)*m_ulBytesPerSector;

				break;
			}
		}
	}

	return ullOffset;
}

ULONGLONG GetStartFileRecordOffset()
{
	return (m_NTFSRootSector.MFTStartLCN*m_NTFSRootSector.SectorsPerCluster + (ULONGLONG)m_ullStartSector)*m_NTFSRootSector.BytesPerSector;
}

TCHAR* GetFileRecordPath(ULONG ulFileReferenceNumber)
{
	FILE_RECORD* pRecord = m_FileRecordMap[ulFileReferenceNumber];
	TCHAR* pMFTPath = NULL;
	TCHAR* pMFTName = NULL;
	std::wstring strMFTPath;
	do
	{
		TCHAR* pMFTName = pRecord->Name;
		if(pMFTName != NULL)
		{
			if(strMFTPath.length() > 0)
				strMFTPath.insert(0, L"\\");

			strMFTPath.insert(0, pMFTName);
		}

		pRecord = m_FileRecordMap[pRecord->DirectoryFileReferenceNumber];
	}while(pRecord != NULL && pRecord->FileReferenceNumber != 5 && pRecord->IsDirectory == TRUE);

	if(pRecord != NULL && pRecord->IsDirectory == FALSE)
		strMFTPath.insert(0, L"丢失的文件\\");
	else
		strMFTPath.insert(0, L"根目录\\");

	ULONG ulPathLen = strMFTPath.length();
	pMFTPath = new TCHAR[ulPathLen + 1];
	wmemset(pMFTPath, 0, ulPathLen + 1);
	wmemcpy(pMFTPath, strMFTPath.c_str(), strMFTPath.length());

	return pMFTPath;
}

TCHAR* ReadNTFSVolume(ULONGLONG ullStartSector)
{
	TCHAR* pVolume = NULL;
	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector];
	if(lpSectorBuffer != NULL)
	{
		if(ReadSector(ullStartSector, 1, lpSectorBuffer))
		{
			ULONGLONG ullStartFileRecordSector = ((((NTFS_ROOT_SECTOR*)lpSectorBuffer)->MFTStartLCN
				*((NTFS_ROOT_SECTOR*)lpSectorBuffer)->SectorsPerCluster + ullStartSector)
				*((NTFS_ROOT_SECTOR*)lpSectorBuffer)->BytesPerSector)/m_ulBytesPerSector;

			LPBYTE lpFileRecordBuffer = new BYTE[1024];
			if(lpFileRecordBuffer != NULL)
			{
				if(ReadSector(ullStartFileRecordSector + 6, 2, lpFileRecordBuffer))
				{
					if(memcmp(lpFileRecordBuffer, "FILE", 4) == 0)
					{
						MFT_INFO* pMftInfo = (MFT_INFO*)lpFileRecordBuffer;

						USHORT usUpdateSequenceOffset = pMftInfo->UpdateSequenceOffset;
						USHORT usUpdateSequenceCount = pMftInfo->UpdateSequenceCount - 1;
						for(USHORT usPos = 0; usPos < usUpdateSequenceCount; usPos ++)
							*(USHORT*)(lpFileRecordBuffer + m_ulBytesPerSector*usPos + 510) = *(USHORT*)(lpFileRecordBuffer + usUpdateSequenceOffset + 2 + 2*usPos);

						MFT_ATTRIBUTE* pMftAttr = NULL;
						for(pMftAttr = (MFT_ATTRIBUTE*)(lpFileRecordBuffer + pMftInfo->AttributeOffset);
							pMftAttr->AttributeType != ATTRIBUTE_VOLUMENAME && pMftAttr->AttributeType != ATTRIBUTE_END;
							pMftAttr = (MFT_ATTRIBUTE*)((LPBYTE)pMftAttr + pMftAttr->AttributeLength));

						if(pMftAttr->AttributeType == ATTRIBUTE_VOLUMENAME)
						{
							DWORD dwNameLen = pMftAttr->RESIDENT.AttributeLength;
							if(dwNameLen == 0)
							{
								pVolume = new TCHAR[10];
								wmemset(pVolume, 0, 10);
								wcscpy(pVolume, L"本地磁盘");
							}
							else
							{
								pVolume = new TCHAR[dwNameLen + 1];
								wmemset(pVolume, 0, dwNameLen + 1);
								wmemcpy(pVolume, (TCHAR*)((LPBYTE)pMftAttr + pMftAttr->RESIDENT.AttributeOffset), dwNameLen);
							}
						}
					}
				}

				delete[] lpFileRecordBuffer;
			}			
		}

		delete[] lpSectorBuffer;
	}

	return pVolume;
}

void ClearNTFSItems()
{
	std::vector<FILE_RECORD_BLOCK*>::iterator iter_block;
	for(iter_block = m_FileRecordBlock.begin();
		iter_block != m_FileRecordBlock.end();
		iter_block++)
	{
		FILE_RECORD_BLOCK* pBlock = *iter_block;
		if(pBlock != NULL)
			delete pBlock;
	}
	m_FileRecordBlock.clear();

	std::map<ULONG, FILE_RECORD*>::iterator iter_record;
	for(iter_record = m_FileRecordMap.begin();
		iter_record != m_FileRecordMap.end();
		iter_record++)
	{
		FILE_RECORD* pRecord = iter_record->second;
		if(pRecord->Name != NULL)
			delete[] pRecord->Name;
		delete pRecord;
	}
	m_FileRecordMap.clear();
}

//
//
// FAT32
//
//
void GetFAT32PartitionInfo(ULONGLONG ullStartSector)
{
	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector];
	if(lpSectorBuffer != NULL)
	{
		if(ReadSector(ullStartSector, 1, lpSectorBuffer))
		{
			memcpy(&m_FAT32RootSector, lpSectorBuffer, m_ulBytesPerSector);
			delete[] lpSectorBuffer;

			m_FAT32RootSector.Volume[10] = '\0';
			m_FAT32RootSector.Volume[7] = '\0';

			m_ullStartSector = ullStartSector;
			m_ullSectorCount = m_FAT32RootSector.TotalSectors;

			PARTITION_ITEM* pItem = new PARTITION_ITEM;

			pItem->PartitionType = FS_FAT32;
			pItem->StartSector = m_ullStartSector;
			pItem->SectorCount = m_ullSectorCount;
			pItem->DeviceHandle = m_hPhysicalDrive;;
			pItem->Volume = ReadFAT32Volume(m_ullStartSector);

			m_PartitionItemList.push_back(pItem);
			m_PartitionSet.insert(ullStartSector);
		}
	}
}

void AnalysisFAT32DBR(ULONGLONG ullStartSector)
{
	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector];
	if(lpSectorBuffer != NULL)
	{
		if(ReadSector(ullStartSector, 1, lpSectorBuffer))
		{
			memcpy(&m_FAT32RootSector, lpSectorBuffer, m_ulBytesPerSector);	
			delete[] lpSectorBuffer;

			m_FAT32RootSector.Volume[10] = '\0';
			m_FAT32RootSector.Volume[7] = '\0';
			
			ClearFAT32Items();

			m_ullStartSector = ullStartSector;
			m_ullSectorCount = m_FAT32RootSector.TotalSectors;

			TCHAR* pEntryPath = new TCHAR[7];
			wmemset(pEntryPath, 0, 7);
			wmemcpy(pEntryPath, L"根目录", 3);
			m_EntryMap[GetDataSector()] = pEntryPath;

			std::map<ULONGLONG, TCHAR*>::iterator iter;
			for(iter = m_EntryMap.begin();
				iter != m_EntryMap.end();
				iter++)
			{
				AnalysisEntry(iter->first, iter->second);
			}

			Notify(MSG_DONE_SCAN_PARTITION, 0, FALSE);
		}
	}
}

void AnalysisEntry(ULONGLONG ullSector, TCHAR* pPath)
{
	ULONG ulEntryRead = 0;
	LPBYTE lpEntryBuffer = new BYTE[m_ulBytesPerSector*m_FAT32RootSector.SectorsPerCluster];
	if(lpEntryBuffer != NULL)
	{
		if(ReadSector(ullSector++, m_FAT32RootSector.SectorsPerCluster, lpEntryBuffer))
		{
			LPBYTE lpBuffer = lpEntryBuffer;

			//
			// 一个神奇的夜晚，我神奇的梦到这一句必须被注释掉，可是当我醒来却想不起为什么
			// 相信第七感吧 
			//
			//if(*lpBuffer == 0x2E || *(lpBuffer + 11) == 0x08)
			{
				do
				{
					if(*lpBuffer == 0x00)
						break;

					else if(*lpBuffer != 0x2E)
					{
						BOOL bIsReadOnly, bIsHidden, bIsSystem, bIsVolume, bIsDeleted, bIsLongFileName;
						bIsVolume = ((lpBuffer[11]&0x08) != 0) ? TRUE : FALSE;						
						bIsReadOnly = ((lpBuffer[11]&0x01) != 0) ? TRUE : FALSE;
						bIsHidden = ((lpBuffer[11]&0x02) != 0) ? TRUE : FALSE;
						bIsSystem = ((lpBuffer[11]&0x04) != 0) ? TRUE : FALSE;
						bIsDeleted = (lpBuffer[0] == 0xE5) ? TRUE : FALSE;

						bIsLongFileName = bIsReadOnly && bIsHidden && bIsSystem && bIsVolume;

						//
						// 跳过Volume
						//
						if(!(bIsVolume && !bIsReadOnly && !bIsHidden && !bIsSystem))
						{
							TCHAR* pFileName = NULL;
							//
							// 长名
							//
							if(bIsLongFileName)
							{
								ULONG ulCount = 0;
								ULONG ulOffset = 0;

								//
								// 首先计算文件名长度
								//
								if(*lpBuffer == 0xE5)
								{
									while(*(lpBuffer + ulOffset + 11) == 0x0F)
									{
										ulCount++;
										ulOffset += 32;
									}
								}
								else if(*lpBuffer == 0x01)
								{
									ulCount = 1;
								}
								else
									ulCount = *lpBuffer ^ 0x40;

								ulEntryRead += ulCount;
								ulOffset = (ulCount - 1)*32;

								char* pTmpName = new char[ulCount*26 + 2];
								memset(pTmpName, 0, ulCount*26 + 2);
								for(ULONG ulPos = 0; ulPos < ulCount; ulPos++)
								{			
									memcpy(pTmpName + ulPos*26, lpBuffer + ulOffset + 1, 10);
									memcpy(pTmpName + 10 + ulPos*26, lpBuffer + ulOffset + 14, 12);
									memcpy(pTmpName + 22 + ulPos*26, lpBuffer + ulOffset + 28, 4);

									ulOffset -= 32;
								}

								ULONG ulNameLen = wcslen((TCHAR*)pTmpName);
								pFileName = new TCHAR[ulNameLen + 1];
								wmemset(pFileName, 0, ulNameLen + 1);
								wmemcpy(pFileName, (TCHAR*)pTmpName, ulNameLen);
								delete[] pTmpName;

								lpBuffer += ulCount *32;
							}

							//
							// 短名
							//
							else
							{
								char* pTmpName = NULL;
								char* pName = CHARTOSTRING(((FAT_FDT*)lpBuffer)->Name, 8);
								char* pExName = CHARTOSTRING(((FAT_FDT*)lpBuffer)->ExName, 3);

								if(strlen(pExName) > 0)
								{
									ULONG ulNameLen = strlen(pName) + strlen(pExName) + 1;
									pTmpName = new char[ulNameLen + 1];
									memset(pTmpName, 0, ulNameLen + 1);
									sprintf(pTmpName, "%s.%s", pName, pExName);
								}
								else
								{
									ULONG ulNameLen = strlen(pName);
									pTmpName = new char[ulNameLen + 1];
									memset(pTmpName, 0, ulNameLen + 1);
									memcpy(pTmpName, pName, ulNameLen);
								}

								if(bIsDeleted)
									*pTmpName = '_';

								pFileName = CHARTOTCHAR(pTmpName);

								delete[] pTmpName;
								delete[] pName;
								delete[] pExName;
							}

							BOOL bIsDirectory = ((lpBuffer[11]&0x10) != 0) ? TRUE : FALSE;

							//
							// 文件夹被删除后，起始簇的高32被清除，得到的信息是错误的
							// 所以不解析被删除的目录项
							//
							if(bIsDirectory && !bIsDeleted)
							{
								ULONGLONG ullEntrySector = GetClusterSector(((((FAT_FDT*)lpBuffer)->StartClusterHigh) << 16) + ((FAT_FDT*)lpBuffer)->StartClusterLow);

								//
								// m_EntrySet是一个集合，表示了某一个目录项扇区是否已经被解析
								// 这样避免了对某一个目录项进行天荒地老海枯石烂的解析
								//
								std::map<ULONGLONG, TCHAR*>::iterator iter;
								iter = m_EntryMap.find(ullEntrySector);

								//
								// 目录项没有被解析过，记录这个目录项
								//
								if(iter == m_EntryMap.end())
								{
									ULONG ulNameLen = wcslen(pFileName);
									ULONG ulPathLen = wcslen(pPath);

									TCHAR* pEntryPath = new TCHAR[ulNameLen + ulPathLen + 2];
									wmemset(pEntryPath, 0, ulNameLen + ulPathLen + 2);
									wsprintf(pEntryPath, L"%s\\%s", pPath, pFileName);
									
									m_EntryMap[ullEntrySector] = pEntryPath;
								}
							}
							else if(!bIsDirectory)
							{
								FAT32_ITEM* pItem = new FAT32_ITEM;

								pItem->IsDeleted = bIsDeleted;
								pItem->Sector = GetClusterSector(((((FAT_FDT*)lpBuffer)->StartClusterHigh) << 16) + ((FAT_FDT*)lpBuffer)->StartClusterLow);
								pItem->Size = ((FAT_FDT*)lpBuffer)->Size;

								wsprintf(pItem->CreateDate, L"%d.%d.%d %d:%d:%d",
									((FAT_FDT*)lpBuffer)->CreateDate.Year + 1980, 
									((FAT_FDT*)lpBuffer)->CreateDate.Month, 
									((FAT_FDT*)lpBuffer)->CreateDate.Day,
									((FAT_FDT*)lpBuffer)->CreateTime.Hour, 
									((FAT_FDT*)lpBuffer)->CreateTime.Minute,
									((FAT_FDT*)lpBuffer)->CreateTime.Second);

								ULONG ulNameLen = wcslen(pFileName);
								ULONG ulPathLen = wcslen(pPath);
								pItem->Name = new TCHAR[ulNameLen + ulPathLen + 2];
								wmemset(pItem->Name, 0, ulNameLen + ulPathLen + 2);
								wsprintf(pItem->Name, L"%s\\%s", pPath, pFileName);

								Notify(MSG_FAT32_ITEM, (LPARAM)pItem, FALSE);

								if(pItem->Name != NULL)
									delete[] pItem->Name;
								delete pItem;
							}

							if(pFileName != NULL)
							{
								delete[] pFileName;
								pFileName = NULL;
							}
						}
					}
					lpBuffer += 32;
					ulEntryRead++;
				}while(ulEntryRead < m_ulBytesPerSector*m_FAT32RootSector.SectorsPerCluster/32);
			}
		}

		delete[] lpEntryBuffer;
	}
}

void CopyFAT32File(ULONGLONG ullSector, ULONGLONG ullSize)
{
	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector];
	if(lpSectorBuffer != NULL)
	{
		//
		// 计算文件数据所占用的扇区总数
		//
		ULONGLONG ullSectorCnt = ullSize/m_ulBytesPerSector + (ullSize%m_ulBytesPerSector > 0);
		DWORD dwBytesRead = 0;
		DWORD dwBytesWrite = 0;

		for(ULONGLONG ullPos = 0; ullPos < ullSectorCnt; ullPos++, ullSector++)
		{
			dwBytesRead = ReadSector(ullSector, 1, lpSectorBuffer);

			DWORD dwFileSizeHigh;
			DWORD dwFileSizeLow;
			dwFileSizeLow = GetFileSize(m_hFile, &dwFileSizeHigh);
			SetFilePointer(m_hFile, dwFileSizeLow, (PLONG)&dwFileSizeHigh, FILE_BEGIN);
			WriteFile(m_hFile, lpSectorBuffer, dwBytesRead, &dwBytesWrite, NULL);
		}

		CloseHandle(m_hFile);
		delete[] lpSectorBuffer;
	}
}

ULONGLONG GetDataSector()
{
	return m_ullStartSector + m_FAT32RootSector.ReservedSectors 	+ m_FAT32RootSector.FATsCount*m_FAT32RootSector.SectorsPerFAT32;
}

ULONGLONG GetClusterSector(ULONGLONG ullCluster)
{
	return GetDataSector() + (ullCluster - 2)*m_FAT32RootSector.SectorsPerCluster;
}

TCHAR* ReadFAT32Volume(ULONGLONG ullStartSector)
{
	TCHAR* pVolume = NULL;
	LPBYTE lpSectorBuffer = new BYTE[m_ulBytesPerSector];
	if(lpSectorBuffer != NULL)
	{
		if(ReadSector(ullStartSector, 1, lpSectorBuffer))
		{
			ULONGLONG ullDataSector = ullStartSector 
				+ ((FAT32_ROOT_SECTOR*)lpSectorBuffer)->ReservedSectors 
				+ ((FAT32_ROOT_SECTOR*)lpSectorBuffer)->FATsCount*((FAT32_ROOT_SECTOR*)lpSectorBuffer)->SectorsPerFAT32;

			ULONG ulEntryRead = 0;

			LPBYTE lpEntryBuffer = new BYTE[m_ulBytesPerSector*m_FAT32RootSector.SectorsPerCluster];
			if(lpEntryBuffer != NULL)
			{
				if(ReadSector(ullDataSector, m_FAT32RootSector.SectorsPerCluster, lpEntryBuffer))
				{
					LPBYTE lpBuffer = lpEntryBuffer;
					do
					{
						if(*lpBuffer == 0x00)
							break;

						BOOL bIsReadOnly, bIsHidden, bIsSystem, bIsVolume;
						bIsVolume = ((lpBuffer[11]&0x08) != 0) ? TRUE : FALSE;						
						bIsReadOnly = ((lpBuffer[11]&0x01) != 0) ? TRUE : FALSE;
						bIsHidden = ((lpBuffer[11]&0x02) != 0) ? TRUE : FALSE;
						bIsSystem = ((lpBuffer[11]&0x04) != 0) ? TRUE : FALSE;

						if(bIsVolume && !bIsReadOnly && !bIsHidden && !bIsSystem)
						{
							pVolume = CHARTOTCHAR(CHARTOSTRING((char*)lpBuffer, 20));
							break;
						}
						lpBuffer += 32;
						ulEntryRead++;
					}while(ulEntryRead < m_ulBytesPerSector*m_FAT32RootSector.SectorsPerCluster/32);

					if(pVolume == NULL)
					{
						pVolume = new TCHAR[10];
						wmemset(pVolume, 0, 10);
						wmemcpy(pVolume, L"本地磁盘", 4);
					}
				}

				delete[] lpEntryBuffer;
			}
		}

		delete[] lpSectorBuffer;
	}

	return pVolume;
}

void ClearFAT32Items()
{
	m_EntrySet.clear();
}
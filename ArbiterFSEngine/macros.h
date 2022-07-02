#pragma once

#pragma warning(disable: 4996 4244 4309 4018)

#include <windows.h>
#include <assert.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>

#include "../Common/globaldefs.h"
#include "../Common/deelx.h"

#define DLL_EXPORT __declspec(dllexport)

typedef struct _TASK_RECORD
{
	TASK_TOKEN TaskToken;
	ULONGLONG Argument1;
	ULONGLONG Argument2;
	ULONGLONG Argument3;
	TCHAR* Argument4;
}TASK_RECORD, *PTASK_RECORD;

typedef struct _FILE_RECORD
{
	ULONG FileReferenceNumber;
	ULONG DirectoryFileReferenceNumber;
	BOOL IsDeleted;
	BOOL IsDirectory;
	ULONGLONG Size;
	TCHAR CreateTime[20];
	TCHAR* Name;
}FILE_RECORD, *PFILE_RECORD;

typedef struct _FILE_RECORD_BLOCK
{
	ULONG StartCluster;
	ULONG StartRecord;
	ULONG RecordCount;
}FILE_RECORD_BLOCK, *PFILE_RECORD_BLOCK;

typedef struct _ENTRY_BLOCK
{
	ULONGLONG EntrySector;
	TCHAR* EntryPath;
}ENTRY_BLOCK, *PENTRY_BLOCK;

//
// DPT
//
typedef struct _PARTITION_TABLE
{
	UCHAR BootIndicator;
	UCHAR StartPhysicalOffset[3];
	UCHAR PartitionType;
	UCHAR EndPhysicalOffset[3];
	UINT ReservedSectors;
	UINT SectorsCount;
}PARTITION_TABLE, *PPARTITION_TABLE;

//
// UCHAR:1
// USHORT:2
// ULONG:4
//
#pragma pack (1)
typedef struct _FAT32_ROOT_SECTOR
{
	UCHAR JMP[3];
	UCHAR Format[8];
	USHORT BytesPerSector;
	UCHAR SectorsPerCluster;
	USHORT ReservedSectors;
	UCHAR FATsCount;
	USHORT RootEntries;
	USHORT TotalSectors16;
	UCHAR MediaDescriptor;
	USHORT SectorsPerFAT16;
	USHORT SectorsPerTrack;
	USHORT Heads;
	ULONG HiddenSectors;
	ULONG TotalSectors;
	ULONG SectorsPerFAT32;
	USHORT ExtentedFlag;
	USHORT Version;
	ULONG RootCluster;
	USHORT SINFOSector;
	USHORT DBRSector;
	UCHAR Reserved1[12];
	UCHAR BIOSDrive;
	UCHAR Reserved2;
	UCHAR BootSignature;
	ULONG SerialNumber;
	UCHAR Volume[11];
	UCHAR FileSystem[8];
	UCHAR BootstrapCode[420];
	USHORT Signature;
}FAT32_ROOT_SECTOR, *PFAT32_ROOT_SECTOR;

typedef struct _NTFS_ROOT_SECTOR
{
	UCHAR JMP[3];
	UCHAR Format[8];
	USHORT BytesPerSector;
	UCHAR SectorsPerCluster;
	USHORT ReservedSectors;
	UCHAR Reserved1[5];
	UCHAR MediaDescriptor;
	USHORT Reserved2;
	USHORT SectorsPerTrack;
	USHORT Heads;
	ULONG HiddenSectors;
	ULONGLONG Reserved3;
	ULONGLONG TotalSectors;
	ULONGLONG MFTStartLCN;
	ULONGLONG MFT2StartLCN;
	ULONG ClustersPerFileRecord;
	ULONG ClustersPerIndexBlock;
	ULONGLONG SerialNumber;
	ULONG Checksum;
	UCHAR BootstrapCode[426];
	USHORT Signature;
}NTFS_ROOT_SECTOR, *PNTFS_ROOT_SECTOR;

typedef struct _OBJECT_PROPERTY
{
	UCHAR ReadOnly : 1;
	UCHAR Hidden : 1;
	UCHAR System : 1;
	UCHAR Volume : 1;
	UCHAR Directory : 1;
	UCHAR Save : 1;
	UCHAR Unused : 2;
}OBJECT_PROPERTY, *POBJECT_PROPERTY;

typedef struct _OBJECT_TIME
{
	USHORT Second : 5;
	USHORT Minute : 6;
	USHORT Hour : 5;
}OBJECT_TIME, *POBJECT_TIME;

typedef struct _OBJECT_DATE
{
	USHORT Day : 5;	
	USHORT Month : 4;
	USHORT Year : 7;	
}OBJECT_DATE, *POBJECT_DATE;

typedef struct _FAT_FDT
{
	CHAR Name[8];
	CHAR ExName[3];
	UCHAR Attribute;
	UCHAR byReserve;
	UCHAR CreateTimeInMS;
	OBJECT_TIME CreateTime;
	OBJECT_DATE CreateDate;
	OBJECT_TIME LastAccessTime;
	USHORT StartClusterHigh;
	OBJECT_TIME LastWriteTime;
	OBJECT_DATE LastWriteDate;
	USHORT StartClusterLow;
	ULONG Size;
}FAT_FDT, *PFAT_FDT;

//
// MFT_ATTRIBUTE
//
enum ATTRIBUTE_TYPE{
	ATTRIBUTE_STANDARDINFORMATION = 0x10,
	ATTRIBUTE_ATTRIBUTELIST = 0x20,
	ATTRIBUTE_FILENAME = 0x30,
	ATTRIBUTE_OBJECTID = 0x40,
	ATTRIBUTE_SECURITYDESCRIPTOR = 0x50,
	ATTRIBUTE_VOLUMENAME = 0x60,
	ATTRIBUTE_VOLUMEINFORMATION = 0x70,
	ATTRIBUTE_DATA = 0x80,
	ATTRIBUTE_INDEXROOT = 0x90,
	ATTRIBUTE_INDEXALLOCATION = 0xA0,
	ATTRIBUTE_BITMAP = 0xB0,

	ATTRIBUTE_END = 0xFFFFFFFF
};

typedef struct _MFT_INFO
{
	UCHAR Signature[4];
	USHORT UpdateSequenceOffset;
	USHORT UpdateSequenceCount;
	ULONGLONG LogfileSequenceNumber;
	USHORT SequenceNumber;	
	USHORT HardLinkCount;
	USHORT AttributeOffset;
	USHORT Flag;
	ULONG LogicalSize;
	ULONG PhysicalSize;
	ULONGLONG BaseRecord;
	USHORT NextAttribute;
	USHORT Reserved;
	ULONG FileReferenceNumber;
	UCHAR UpdateSequence[8];
}MFT_INFO, *PMFT_INFO;

typedef struct _MFT_ATTRIBUTE
{
	ULONG AttributeType;
	ULONG AttributeLength;
	UCHAR Symbol;
	UCHAR NameLength;
	USHORT NameOffset;
	USHORT Flag;
	USHORT AttributeNumber;

	union
	{
		struct _RESIDENT
		{
			ULONG AttributeLength;
			USHORT AttributeOffset;
			USHORT IndexedFlag;
		}RESIDENT;

		struct _NONRESIDENT
		{
			ULONGLONG StartVCN;
			ULONGLONG EndVCN;
			USHORT DataRunOffset;
			USHORT CompressionUnit;
			UCHAR Reserved[4];
			ULONGLONG AllocatedSize;
			ULONGLONG RealSize;
			ULONGLONG InitializedSize;
		}NONRESIDENT;
	};
}MFT_ATTRIBUTE, *PMFT_ATTRIBUTE;

typedef struct _MFT_NAME
{
	ULONGLONG DirectoryFileReferenceNumber;
	ULONGLONG CreateTime;
	ULONGLONG ModifyTime;
	ULONGLONG LastWriteTime;
	ULONGLONG LastAccessTime;
	ULONGLONG AllocatedSize;
	ULONGLONG RealSize;
	ULONG FileAttribute;
	ULONG Reversed;
	UCHAR NameLength;
	UCHAR NameSpace;
	TCHAR Name[260];
}MFT_NAME, *PMFT_NAME;
#pragma pack ()

typedef struct _DEEPSCAN_ITEM
{
	ULONGLONG DataStartSector;
	ULONGLONG DataSectorCount;
}DEEPSCAN_ITEM, *PDEEPSCAN_ITEM;

typedef struct _FILE_SIGNATURE
{
	UCHAR SignatureHead[100];	
	ULONG SignatureHeadOffset;
	ULONG DefaultSize;
	TCHAR Extension[10];
}FILE_SIGNATURE, *PFILE_SIGNATURE;

static FILE_SIGNATURE FileSignature[] = 
{
	{"Rar!\x1a\x07\x00", 0, 0, L"rar"},
	{"(PK\x03\x04)|(PK00)|(PK\x05\x06)", 0, 0, L"zip"},
	{"7z\xBC\xAF\x27", 0, 0, L"7z"},
	{"\x4D\x5A\x90\x00\x03\x00", 0, 0, 0, L"exe"},
	{"GIF8[79]a", 0, 0, L"gif"},
	{"\x89PNG\x0D\x0A\x1A\x0A", 0, 0, L"png"},
	//{"(\x49\x49\x2A\x00)|(\x4D\x4D\x00\x2A)", 0, 0, L"tif", L".tif"},	
	{"WAVEfmt", 8, 0, L"wav"},
	{"ID3[\x02-\x04]\x00\x00\x00", 0, 0, L"mp3"},
	{"\x30\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C", 0, 0, L"wmv"},
	{"MThd", 0, 0, L"mid"},
	{"\xFF\xD8\xFF[\xDB\xE0\xE1\xEB\xEE\xFE\xC4]", 0, 0, L"jpg"},
	{"\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 0, 0, L"doc"},
	{"%PDF\x2D\x31\x2E", 0, 0, L"pdf"},
	{"ITSF\x03\x00\x00\x00", 0, 0, L"chm"},
};
static ULONG ulFileSignatureCnt = sizeof(FileSignature)/sizeof(FileSignature[0]);
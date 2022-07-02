#include "stdafx.h"
#include "General.h"

CString CGeneral::GetModulePath(HMODULE hModule)
{
	TCHAR tcBuffer[MAX_PATH];
	CString strDir, strTemp;

	wmemset(tcBuffer, 0, MAX_PATH);
	::GetModuleFileName(hModule, tcBuffer, MAX_PATH);
	strTemp = tcBuffer;
	strDir = strTemp.Left(strTemp.ReverseFind('\\') + 1);
	return strDir;
}

char* CGeneral::TCHARTOCHAR(TCHAR* pSrcBuffer)
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

TCHAR* CGeneral::CHARTOTCHAR(char* pSrcBuffer)
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

char* CGeneral::CHARTOSTRING(char* pSrcBuffer, ULONG ulMaxLen)
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
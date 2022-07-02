#pragma once

class CGeneral
{
public:
	static CString GetModulePath(HMODULE hModule = NULL);

	static char* TCHARTOCHAR(TCHAR* pSrcBuffer);
	static TCHAR* CHARTOTCHAR(char* pSrcBuffer);
	static char* CHARTOSTRING(char* pSrcBuffer, ULONG ulMaxLen);
};
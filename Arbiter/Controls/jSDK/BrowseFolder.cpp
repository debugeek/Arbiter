#include "stdafx.h"
#include "BrowseFolder.h"

int CALLBACK CBrowseFolder::BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	CBrowseFolder *pBrowseFolder=(CBrowseFolder *) lpData;
	if(uMsg==BFFM_INITIALIZED)
		SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM) pBrowseFolder->m_pDirPath);
	return 0;
}

CBrowseFolder::CBrowseFolder()
{
	m_pDirPath=new TCHAR[MAX_PATH];
}

CBrowseFolder::~CBrowseFolder()
{
	delete m_pDirPath;
}

int CBrowseFolder::DoModal(CWnd *pParentWnd,const TCHAR *pStartPath)
{
	LPMALLOC pMalloc;
	if(SHGetMalloc(&pMalloc)!=NOERROR)
		return -1;
	if(pStartPath && IsValuePath(pStartPath))
	{
		TCHAR *pStr=m_pDirPath;
		while(*pStr++=*pStartPath++);
	}
	BROWSEINFO bInfo={pParentWnd->m_hWnd,NULL,0,_T("请选择文件夹，搜索到的文件将会保存在这里"),0,BrowseCallbackProc,(LPARAM) this,0};
	ITEMIDLIST *pItemList=SHBrowseForFolder(&bInfo);
	if(pItemList)
	{
		SHGetPathFromIDList(pItemList,m_pDirPath);
		pMalloc->Free(pItemList);
		pMalloc->Release();
		return TRUE;
	}
	pMalloc->Free(pItemList);
	pMalloc->Release();
	return FALSE;
}

TCHAR *CBrowseFolder::GetDirPath()
{
	return m_pDirPath;
}

BOOL CBrowseFolder::IsValuePath(const TCHAR *pDirPath)
{
	DWORD dFileAttrib=GetFileAttributes(pDirPath);
	return (dFileAttrib!=0xffffffff && dFileAttrib & FILE_ATTRIBUTE_DIRECTORY)?TRUE:FALSE;
}
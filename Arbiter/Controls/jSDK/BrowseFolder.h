#if !defined(AFX_BrowseFolder_H__62FFAC92_1DEE_11D1_B87A_0060979CDF6D__INCLUDED_)
#define AFX_BrowseFolder_H__62FFAC92_1DEE_11D1_B87A_0060979CDF6D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif

class CBrowseFolder
{
public:
	CBrowseFolder();
	virtual ~CBrowseFolder();
	TCHAR *GetDirPath();
	int DoModal(CWnd *pParentWnd,const TCHAR *pStartPath=NULL);

protected:
	static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);
	BOOL IsValuePath(const TCHAR *pDirPath);
	TCHAR *m_pDirPath;
};
#endif
#pragma once

#include "jSDK.h"

#include <map>

class CjTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CjTreeCtrl)

public:
	CjTreeCtrl();	
	virtual ~CjTreeCtrl();
	
protected:

	DECLARE_MESSAGE_MAP()

public:
	CString GetTreeItemPath(const HTREEITEM &hParentItem);

	void InsertTreeItemByPath(CString strItemPath);

	void Clear();

protected:
	ULONG CalcStringHash(LPCTSTR lpszString);

private:
	std::map<ULONG, HTREEITEM> m_TreeItemMap;
};

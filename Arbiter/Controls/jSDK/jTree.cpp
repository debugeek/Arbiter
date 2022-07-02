#include "stdafx.h"
#include "jTree.h"

IMPLEMENT_DYNAMIC(CjTreeCtrl, CTreeCtrl)
CjTreeCtrl::CjTreeCtrl()
{

}

CjTreeCtrl::~CjTreeCtrl()
{
}

BEGIN_MESSAGE_MAP(CjTreeCtrl, CTreeCtrl)
END_MESSAGE_MAP()


CString CjTreeCtrl::GetTreeItemPath(const HTREEITEM &hItem)
{
	CString strItemPath;
	HTREEITEM hParentItem = hItem;
	while(hParentItem)
	{
		CString pItemName = GetItemText(hParentItem);
		strItemPath.Insert(0, pItemName);

		hParentItem = GetParentItem(hParentItem);
		if(hParentItem != NULL)
			strItemPath.Insert(0, L"\\");
	}

	return strItemPath;
}

void CjTreeCtrl::InsertTreeItemByPath(CString strItemPath)
{
	CString strFront;
	HTREEITEM hItem;
	BOOL bIsRootItem = FALSE;
	std::map <ULONG, HTREEITEM>::iterator iter;
	do
	{
		CString strName = strItemPath.Left(strItemPath.Find(L"\\"));
		if(strName.GetLength() <= 0)
			break;
		
		if(strFront.GetLength() > 0)
		{
			bIsRootItem = FALSE;
			strFront += L"\\" + strName;
		}
		else
		{
			bIsRootItem = TRUE;
			strFront = strName;
		}
		strItemPath = strItemPath.Right(strItemPath.GetLength() - strName.GetLength() - 1);

		ULONG ulHash = CalcStringHash(strFront);

		iter = m_TreeItemMap.find(ulHash);
		if(iter != m_TreeItemMap.end())
			hItem = iter->second;
		else
		{
			if(bIsRootItem == FALSE)
				hItem = InsertItem(strName, 1, 1, hItem);
			else
				hItem = InsertItem(strName, 0, 0);

			m_TreeItemMap[ulHash] = hItem;
		}
	}while(strItemPath.GetLength() > 0);
}

ULONG CjTreeCtrl::CalcStringHash(LPCTSTR lpszString)
{
	ULONG ulSeed = 131;
	ULONG ulHash = 0;

	while(*lpszString)
	{
		ulHash = ulHash*ulSeed + (*lpszString++);
	}
	return (ulHash&0x7FFFFFFF);
}

void CjTreeCtrl::Clear()
{
	m_TreeItemMap.clear();
}

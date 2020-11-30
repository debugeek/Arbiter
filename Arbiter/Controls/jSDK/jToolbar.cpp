#include "stdafx.h"
#include "jToolbar.h"

IMPLEMENT_DYNAMIC(CjToolbarCtrl, CToolBar)
CjToolbarCtrl::CjToolbarCtrl()
{

}

CjToolbarCtrl::~CjToolbarCtrl()
{
}

BEGIN_MESSAGE_MAP(CjToolbarCtrl, CToolBar)
END_MESSAGE_MAP()

void CjToolbarCtrl::SetToolbarImage(ULONG ulBtnWidth, UINT uToolBar, UINT uToolBarHot, UINT uToolBarDisabled)
{
	if(!SetToolbar(TB_SETIMAGELIST, uToolBar, ulBtnWidth))
		return;
	
	if(uToolBarHot)
	{
		if (!SetToolbar(TB_SETHOTIMAGELIST, uToolBarHot, ulBtnWidth))
			return;
	}

	if(uToolBarDisabled)
	{
		if(!SetToolbar(TB_SETDISABLEDIMAGELIST, uToolBarDisabled, ulBtnWidth))
			return;
	}
}

BOOL CjToolbarCtrl::SetToolbar(UINT uToolBarType, UINT uToolBar, ULONG  ulBtnWidth)
{
	CImageList	cImageList;
	CBitmap		cBitmap;
	BITMAP		bmBitmap;
	
	if (!cBitmap.Attach(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(uToolBar), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION)) 
		|| !cBitmap.GetBitmap(&bmBitmap))
		return FALSE;

	CSize cSize(bmBitmap.bmWidth, bmBitmap.bmHeight); 
	ULONG ulNbBtn = cSize.cx/ulBtnWidth;
	RGBTRIPLE* rgb = (RGBTRIPLE*)(bmBitmap.bmBits);
	COLORREF rgbMask = RGB(rgb[0].rgbtRed, rgb[0].rgbtGreen, rgb[0].rgbtBlue);
	
	if(!cImageList.Create(ulBtnWidth, cSize.cy, ILC_COLOR24|ILC_MASK, ulNbBtn, 0))
		return FALSE;
	
	if(cImageList.Add(&cBitmap, rgbMask) == -1)
		return FALSE;

	SendMessage(uToolBarType, 0, (LPARAM)cImageList.m_hImageList);
	cImageList.Detach(); 
	cBitmap.Detach();
	
	return TRUE;
}
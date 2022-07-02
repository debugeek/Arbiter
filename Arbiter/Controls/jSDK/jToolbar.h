#pragma once

#include "jSDK.h"

class CjToolbarCtrl : public CToolBar
{
public:
	DECLARE_DYNAMIC(CjToolbarCtrl)

	CjToolbarCtrl();	
	virtual ~CjToolbarCtrl();
	
protected:

	DECLARE_MESSAGE_MAP()

public:
	void SetToolbarImage(ULONG ulBtnWidth, UINT uToolBar, UINT uToolBarHot = 0, UINT uToolBarDisabled = 0);
	
protected:
	BOOL SetToolbar(UINT uToolBarType, UINT uToolBar, ULONG ulBtnWidth);
};

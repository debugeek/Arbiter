// Arbiter.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CArbiterApp:
// �йش����ʵ�֣������ Arbiter.cpp
//

class CArbiterApp : public CWinApp
{
public:
	CArbiterApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CArbiterApp theApp;
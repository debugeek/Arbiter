// ArbiterDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Arbiter.h"
#include "ArbiterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CArbiterDlg 对话框




CArbiterDlg::CArbiterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CArbiterDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CArbiterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CArbiterDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_NCHITTEST()
	//}}AFX_MSG_MAP	

	ON_WM_CLOSE()

	ON_MESSAGE(WM_USER_MSG, OnReadMessage)
END_MESSAGE_MAP()


// CArbiterDlg 消息处理程序

BOOL CArbiterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	CBasicContext::InitializeContext(GetSafeHwnd());

	InitializeUI();
	SwitchDialog(PARTITION_DIALOG);
	

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CArbiterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CArbiterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CArbiterDlg::InitializeUI()
{
	m_DialogMap.clear();

	MoveWindow(0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, 0);
	SetWindowText(L"Arbiter - 老特拉福德球场指定专用产品");
	
	//
	// 状态栏
	//
	m_statusbar.Create(WS_CHILD|WS_VISIBLE|CCS_BOTTOM, CRect(0,0,0,0),  this,  IDR_STATUSBAR);
	int StatusParts[4]= {150, 700, -1};
	m_statusbar.SetParts(3, StatusParts);
	m_statusbar.SetText(_T("Ready"), 0, 0 );
	m_statusbar.SetText(_T("0 host exist."), 2, 0);

	m_FileDlg.Create(IDD_FILE_DIALOG, this);
	m_FileDlg.MoveWindow(
		CHILD_WINDOW_X,
		CHILD_WINDOW_Y,
		CHILD_WINDOW_WIDTH,
		CHILD_WINDOW_HEIGHT,
		0);
	m_DialogMap[FILE_DIALOG] = m_FileDlg.GetSafeHwnd();

	m_PartitionDlg.Create(IDD_PARTITION_DIALOG, this);
	m_PartitionDlg.MoveWindow(
		CHILD_WINDOW_X,
		CHILD_WINDOW_Y,
		CHILD_WINDOW_WIDTH,
		CHILD_WINDOW_HEIGHT,
		0);
	m_DialogMap[PARTITION_DIALOG] = m_PartitionDlg.GetSafeHwnd();

	m_hPrevWnd = NULL;
	m_hNextWnd = NULL;
}

void CArbiterDlg::SwitchDialog(DIALOG_TOKEN DialogToken)
{
	std::map<DIALOG_TOKEN, HWND>::iterator iter;
	iter = m_DialogMap.find(DialogToken);
	if(iter != m_DialogMap.end())
	{
		HWND hWnd = iter->second;
		if(m_hNextWnd != hWnd)
		{

			m_hPrevWnd = m_hNextWnd;
			m_hNextWnd = hWnd;

			::ShowWindow(m_hPrevWnd, SW_HIDE);
			::ShowWindow(m_hNextWnd, SW_SHOW);
		}
	}
}

LRESULT CArbiterDlg::OnReadMessage(WPARAM wParam, LPARAM lParam)
{
	USER_MSG UserMsg = (USER_MSG)wParam;
	switch(UserMsg)
	{
	case MSG_PARTITION:
		{
			PARTITION_ITEM* pItem = (PARTITION_ITEM*)lParam;
			if(pItem != NULL)
				CBasicContext::AddPartitionItem(pItem);
		}
		break;

	case MSG_NTFS_ITEM:
		{
			NTFS_ITEM* pItem = (NTFS_ITEM*)lParam;
			if(pItem != NULL)
				CBasicContext::AddNTFSItem(pItem);
		}
		break;

	case MSG_FAT32_ITEM:
		{
			FAT32_ITEM* pItem = (FAT32_ITEM*)lParam;
			if(pItem != NULL)
				CBasicContext::AddFAT32Item(pItem);
		}
		break;

	//
	// 分区读取完成
	//
	case MSG_DONE_INITIALIZE_PARTITION:
		{
			m_PartitionDlg.LoadPartition();
		}
		break;

	//
	// 对某一个分区的解析完成
	//
	case MSG_DONE_SCAN_PARTITION:
		{
			m_statusbar.SetText(L"正在构建目录", 0, 0);
			m_statusbar.SetText(L"", 2, 0);
			m_FileDlg.LoadFolder();
			SwitchDialog(FILE_DIALOG);
		}
		break;

	case MSG_UPDATE_STATUS:
		{
			LPCTSTR lpszText = (LPCTSTR)lParam;

			m_statusbar.SetText(lpszText, 0, 0 );
		}
		break;

	case MSG_UPDATE_PROGRESS:
		{
			ULONG ulPos = (ULONG)lParam;

			CString strPos;
			strPos.Format(L"%d%%", ulPos);
			m_statusbar.SetText(strPos, 0, 2);
		}
		break;
	}

	return 1;
}

LRESULT CArbiterDlg::OnNcHitTest(CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	RECT rt;
	GetClientRect(&rt);
	ScreenToClient(&rt);
	UINT nHit = CDialog::OnNcHitTest(point);
	return (nHit == HTCLIENT) ? HTCAPTION : nHit;
}

void CArbiterDlg::OnClose()
{
	ShowWindow(SW_HIDE);

	//不用特意去释放内存 进程结束系统会自动收回所有内存
	//CBasicContext::UninitializeContext();

	CDialog::OnClose();
}
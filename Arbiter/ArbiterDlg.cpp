// ArbiterDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "Arbiter.h"
#include "ArbiterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CArbiterDlg �Ի���




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


// CArbiterDlg ��Ϣ�������

BOOL CArbiterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	CBasicContext::InitializeContext(GetSafeHwnd());

	InitializeUI();
	SwitchDialog(PARTITION_DIALOG);
	

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CArbiterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CArbiterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CArbiterDlg::InitializeUI()
{
	m_DialogMap.clear();

	MoveWindow(0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, 0);
	SetWindowText(L"Arbiter - ������������ָ��ר�ò�Ʒ");
	
	//
	// ״̬��
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
	// ������ȡ���
	//
	case MSG_DONE_INITIALIZE_PARTITION:
		{
			m_PartitionDlg.LoadPartition();
		}
		break;

	//
	// ��ĳһ�������Ľ������
	//
	case MSG_DONE_SCAN_PARTITION:
		{
			m_statusbar.SetText(L"���ڹ���Ŀ¼", 0, 0);
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
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	RECT rt;
	GetClientRect(&rt);
	ScreenToClient(&rt);
	UINT nHit = CDialog::OnNcHitTest(point);
	return (nHit == HTCLIENT) ? HTCAPTION : nHit;
}

void CArbiterDlg::OnClose()
{
	ShowWindow(SW_HIDE);

	//��������ȥ�ͷ��ڴ� ���̽���ϵͳ���Զ��ջ������ڴ�
	//CBasicContext::UninitializeContext();

	CDialog::OnClose();
}
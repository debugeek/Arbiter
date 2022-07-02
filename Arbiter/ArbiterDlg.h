// ArbiterDlg.h : 头文件
//

#pragma once

#include "FileDlg.h"
#include "PartitionDlg.h"

#define MAIN_WINDOW_WIDTH 888
#define MAIN_WINDOW_HEIGHT 486

#define CHILD_WINDOW_X 10
#define CHILD_WINDOW_Y 20
#define CHILD_WINDOW_WIDTH 860 /* 573 */
#define CHILD_WINDOW_HEIGHT 406 /* 250 */

enum DIALOG_TOKEN
{
	FILE_DIALOG = 1,
	PARTITION_DIALOG = 2,
};

// CArbiterDlg 对话框
class CArbiterDlg : public CDialog
{
// 构造
public:
	CArbiterDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_ARBITER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	afx_msg LRESULT OnNcHitTest(CPoint point);

	afx_msg LRESULT OnReadMessage(WPARAM wParam ,LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	
public:
	CStatusBarCtrl m_statusbar;

	CFileDlg m_FileDlg;
	CPartitionDlg m_PartitionDlg;

protected:
	void InitializeUI();
	void SwitchDialog(DIALOG_TOKEN DialogToken);

private:
	HWND m_hPrevWnd, m_hNextWnd;
	std::map<DIALOG_TOKEN, HWND> m_DialogMap;
};

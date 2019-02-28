// NumProgrammerDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include <string>

// CNumProgrammerDlg �Ի���
class CNumProgrammerDlg : public CDialog
{
// ����
public:
	CNumProgrammerDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_NUMPROGRAMMER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnDevListMessage(UINT wParam, LPTSTR lParam);
	void IEEEAddressString2Num(std::string IEEEstring);
	std::string IEEEAddressNum2String(void);
	void IEEEAddressNum2EachValue();
	bool IEEEAddressEachValue2Num();
	wchar_t* AnsiToUnicode( const char* szStr );
	char* UnicodeToAnsi( const wchar_t* szStr );
	DECLARE_MESSAGE_MAP()
public:
	CListBox m_DevList;
	CString m_FlashFilePath;
	CComboBox m_FlashSize;
	CComboBox m_ProductType;
	unsigned int m_Channel;
	unsigned int m_ShapeMode;
	unsigned int m_CardNumber;
	afx_msg void OnBnClickedButtonRead();
	unsigned char m_IEEEAddr[8];
	unsigned int m_LotNumber;
	afx_msg void OnBnClickedButtonWrite();
	afx_msg void OnBnClickedButtonFilepath();
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedButtonProgram();
	afx_msg void OnEnChangeEdit3();
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnEnChangeEdit5();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnBnClickedButtonInc();
	afx_msg void OnBnClickedButtonDec();
	afx_msg void OnStnClickedStaticResult();
};

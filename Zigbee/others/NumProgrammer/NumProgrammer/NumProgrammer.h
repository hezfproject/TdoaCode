// NumProgrammer.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CNumProgrammerApp:
// �йش����ʵ�֣������ NumProgrammer.cpp
//

class CNumProgrammerApp : public CWinApp
{
public:
	CNumProgrammerApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CNumProgrammerApp theApp;
// NumProgrammerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "NumProgrammer.h"
#include "NumProgrammerDlg.h"
#include <afxdlgs.h>
#include <afxmt.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <afx.h>
//#include <winuser.h>
#include <windows.h>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DEVICE_CC2430	0
#define DEVICE_CC2530	1
#define DEVICE_INVALID -1

unsigned int device = DEVICE_INVALID;


static std::vector<string> gDevList;
static CMutex				gMutex;
static UINT __cdecl DevDetectProcess( LPVOID pParam );

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

    // 对话框数据
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    // 实现
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CNumProgrammerDlg 对话框




CNumProgrammerDlg::CNumProgrammerDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CNumProgrammerDlg::IDD, pParent)
    , m_FlashFilePath(_T(""))
    , m_Channel(0)
    , m_ShapeMode(0)//_T("")
    , m_CardNumber(0)
    , m_LotNumber(0)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CNumProgrammerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_DEVLIST, m_DevList);
    DDX_Text(pDX, IDC_EDIT_FILEPATH, m_FlashFilePath);
    DDX_Control(pDX, IDC_COMBO1, m_FlashSize);
    DDX_Control(pDX, IDC_COMBO2, m_ProductType);
    DDX_Text(pDX, IDC_EDIT3, m_Channel);
    DDX_Text(pDX, IDC_EDIT5, m_ShapeMode);
    DDX_Text(pDX, IDC_EDIT2, m_CardNumber);
    DDX_Text(pDX, IDC_EDIT4, m_LotNumber);
}

BEGIN_MESSAGE_MAP(CNumProgrammerDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_DEVLIST_MESSAGE,OnDevListMessage)
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BUTTON_READ, &CNumProgrammerDlg::OnBnClickedButtonRead)
    ON_BN_CLICKED(IDC_BUTTON_FILEPATH, &CNumProgrammerDlg::OnBnClickedButtonFilepath)
    ON_BN_CLICKED(IDC_CHECK2, &CNumProgrammerDlg::OnBnClickedCheck2)
    ON_BN_CLICKED(IDC_BUTTON_PROGRAM, &CNumProgrammerDlg::OnBnClickedButtonProgram)
    ON_EN_CHANGE(IDC_EDIT3, &CNumProgrammerDlg::OnEnChangeEdit3)
    ON_CBN_SELCHANGE(IDC_COMBO2, &CNumProgrammerDlg::OnCbnSelchangeCombo2)
    ON_EN_CHANGE(IDC_EDIT4, &CNumProgrammerDlg::OnEnChangeEdit4)
    ON_EN_CHANGE(IDC_EDIT5, &CNumProgrammerDlg::OnEnChangeEdit5)
    ON_EN_CHANGE(IDC_EDIT2, &CNumProgrammerDlg::OnEnChangeEdit2)
    ON_BN_CLICKED(IDC_BUTTON_INC, &CNumProgrammerDlg::OnBnClickedButtonInc)
    ON_BN_CLICKED(IDC_BUTTON_DEC, &CNumProgrammerDlg::OnBnClickedButtonDec)
	//ON_LBN_SELCHANGE(IDC_LIST_DEVLIST, &CNumProgrammerDlg::OnLbnSelchangeListDevlist)
	ON_STN_CLICKED(IDC_STATIC_RESULT, &CNumProgrammerDlg::OnStnClickedStaticResult)
END_MESSAGE_MAP()

// CNumProgrammerDlg 消息处理程序

BOOL CNumProgrammerDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // 将“关于...”菜单项添加到系统菜单中。

    // IDM_ABOUTBOX 必须在系统命令范围内。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO: 在此添加额外的初始化代码
    // 用一个新线程检测device
    AfxBeginThread(DevDetectProcess,(void *)this);

    m_FlashSize.InsertString(0,CString("128k"));
    m_FlashSize.InsertString(1,CString("64k"));
    m_FlashSize.InsertString(2,CString("32k"));

    m_ProductType.InsertString(0,CString("地面人员卡"));
    m_ProductType.InsertString(1,CString("地面设备卡"));
    //m_ProductType.InsertString(2,CString("射频板"));
    //m_ProductType.InsertString(3,CString("瓦检仪"));
    //m_ProductType.InsertString(4,CString("定位器"));
    //m_ProductType.InsertString(5,CString("读卡器"));

    ((CEdit*)GetDlgItem(IDC_EDIT2))->SetLimitText(5);//8
    ((CEdit*)GetDlgItem(IDC_EDIT3))->SetLimitText(2);
    ((CEdit*)GetDlgItem(IDC_EDIT4))->SetLimitText(3);
    ((CEdit*)GetDlgItem(IDC_EDIT5))->SetLimitText(3);//2

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CNumProgrammerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CNumProgrammerDlg::OnPaint()
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
HCURSOR CNumProgrammerDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}
LRESULT CNumProgrammerDlg::OnDevListMessage(UINT wParam, LPTSTR lParam)
{
    if(gDevList.size() != m_DevList.GetCount())  // if device list changed
    {

        while(m_DevList.GetCount())
            m_DevList.DeleteString(0);

        for (unsigned int i=0; i<gDevList.size(); i++)
        {
            m_DevList.AddString(CString(gDevList[i].c_str()));
        }

        GetDlgItem( IDC_LIST_DEVLIST)->UpdateData(false);

        if (m_DevList.GetCount()==0)
        {
            GetDlgItem(IDC_BUTTON_PROGRAM)->EnableWindow(false);
            GetDlgItem(IDC_BUTTON_READ)->EnableWindow(false);
        }
        else
        {
            GetDlgItem(IDC_BUTTON_PROGRAM)->EnableWindow(true);
            GetDlgItem(IDC_BUTTON_READ)->EnableWindow(true);
        }
    }

    bool isAvoidStupid = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck()>0 ? true:false;
    if(device == DEVICE_CC2430 && !isAvoidStupid)
    {
        GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
    }
    else if (device == DEVICE_CC2530)
    {
        GetDlgItem(IDC_COMBO1)->EnableWindow(FALSE);
    }
    return 0;
}
static UINT __cdecl DevDetectProcess( LPVOID pParam )
{
    CNumProgrammerDlg *pThis = (CNumProgrammerDlg *)pParam;

    // generate .bat file
    FILE *fpexec;
    fopen_s(&fpexec,"DevList.bat","w");
    if(fpexec == NULL)
    {
        AfxMessageBox(_T("open DevList.bat error!"));
        exit(1);
    }

    fprintf(fpexec,"SmartRFProgConsole.exe X > DevList.txt 2>&1\n");
    fclose(fpexec);

    //run .bat file
    while (1)
    {
        string LineBuf;

        gMutex.Lock();
        int flag = (int)ShellExecute(NULL,_T("open"),_T("DevList.bat"),NULL,NULL,SW_HIDE);

        if (flag ==0)
        {
            AfxMessageBox(_T("exex DevList.bat error!"));
            exit(1);
        }
        gMutex.Unlock();

        // read result
        Sleep(500);
        ifstream fpDevList;
        fpDevList.open("DevList.txt",ios::in);
        if(!fpDevList)
        {
            //AfxMessageBox(_T("open DevList.txt error!"));
            //exit(1);
            continue;
        }

        gDevList.clear();
        device = DEVICE_INVALID;
        while (getline(fpDevList,LineBuf))
        {
            if (LineBuf.find("No supported devices detected")!=string::npos) //no device found
            {
                break;
            }
            else if(LineBuf.find("Chip:-")!=string::npos || LineBuf.find("Chip:Unknown Chip")!=string::npos) // wrong device
            {
                break;
            }
            else if (LineBuf.find("Device:")!=string::npos) // find a device
            {
                if (LineBuf.find("2430")!=string::npos)
                {
                    device = DEVICE_CC2430;
                    gDevList.push_back(LineBuf);
                }
                else if (LineBuf.find("2530")!=string::npos)
                {
                    device = DEVICE_CC2530;
                    gDevList.push_back(LineBuf);
                }
            }
        }

        SendMessage(pThis->m_hWnd,WM_DEVLIST_MESSAGE,0,0);
        if(gDevList.size() > 1)
        {
            AfxMessageBox(_T("检测到多个仿真器，本版本只支持一个仿真器!"));
        }
        fpDevList.close();
    }
}

void CNumProgrammerDlg::OnBnClickedButtonRead()
{
    // TODO: 在此添加控件通知处理程序代码

    // generate .bat file
    if(device == DEVICE_INVALID)
    {
        AfxMessageBox(_T("无设备"));
        return;
    }

    int real_flashSizeidx;
    if(device == DEVICE_CC2430)
    {
        int flashSizeidx;
        if(m_FlashSize.GetCurSel()>= 0 && m_FlashSize.GetCurSel()<3)
        {
            flashSizeidx = m_FlashSize.GetCurSel();  //0:128k 1:64k 0:32k
            switch(flashSizeidx)
            {
            case 0:
                real_flashSizeidx=128;
                break;
            case 1:
                real_flashSizeidx=64;
                break;
            case 2:
                real_flashSizeidx=32;
                break;
            default:
                break;
            }
        }
        else
        {
            AfxMessageBox(_T("请选择Flash空间大小!"));
            return;
        }
    }

    FILE *fpexec;
    fopen_s(&fpexec,"ReadIEEE.bat","w");
    if(fpexec == NULL)
    {
        AfxMessageBox(_T("open ReadIEEE.bat error!"));
        exit(1);
    }

    if(device == DEVICE_CC2530)
    {
        fprintf(fpexec,"SmartRFProgConsole.exe S RI(F=%d,[SEC]) > ReadIEEE.txt 2>&1\n",0);
    }
    else if(device == DEVICE_CC2430)
    {
        fprintf(fpexec,"SmartRFProgConsole.exe S RI(F=%d) > ReadIEEE.txt 2>&1\n",real_flashSizeidx);
    }

    fclose(fpexec);

    fopen_s(&fpexec,"CleanRead.bat","w");
    if(fpexec == NULL)
    {
        AfxMessageBox(_T("open CleanRead.bat error!"));
        exit(1);
    }

    fprintf(fpexec,"del ReadIEEE.txt\n");
    fclose(fpexec);

// run
    int flag = (int)ShellExecute(NULL,_T("open"),_T("CleanRead.bat"),NULL,NULL,SW_HIDE);
    if(flag == 0)
    {
        AfxMessageBox(_T("exec ReadIEEE.bat error!"));
        exit(1);
    }

    gMutex.Lock();
    flag = (int)ShellExecute(NULL,_T("open"),_T("ReadIEEE.bat"),NULL,NULL,SW_HIDE);
    if(flag == 0)
    {
        AfxMessageBox(_T("exec ReadIEEE.bat error!"));
        exit(1);
    }
    gMutex.Unlock();

// read result
    Sleep(500);
    ifstream fpResult;
    string LineBuf;
    fpResult.open("ReadIEEE.txt",ios::in);
    if(!fpResult)
    {
        AfxMessageBox(_T("open ReadIEEE.txt error!"));
        exit(1);
    }

    bool read_success = FALSE;
    while (getline(fpResult,LineBuf))
    {
        int pos = LineBuf.find("IEEE address:");
        if (pos!=string::npos) //no device found
        {
            string sub_str = LineBuf.substr(LineBuf.rfind(':')+2,string::npos);
            GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(sub_str.c_str()));

            IEEEAddressString2Num(sub_str);
            IEEEAddressNum2EachValue();
            read_success = TRUE;
        }
    }

    fpResult.close();
    if(!read_success)
    {
        AfxMessageBox(_T("IEEE地址读取失败!"));
    }
}
void CNumProgrammerDlg::IEEEAddressString2Num(string IEEEstring)
{
    stringstream stream(IEEEstring);
    string sub_str;

    for (int i=0; i<8; i++)
    {
        if (getline(stream,sub_str,'.'))
        {
            sscanf_s(sub_str.c_str(),"%X",m_IEEEAddr+i);
        }
        else
        {
            AfxMessageBox(_T("IEEE 地址错误!"));
            exit(1);
        }
    }

    return;
}
string CNumProgrammerDlg::IEEEAddressNum2String(void)
{
    string str;
    for (int i=0; i<8; i++)
    {
        char buf[32];
        sprintf_s(buf,32,"%X",(m_IEEEAddr[i]>>4) & 0xF);
        str += buf;
        sprintf_s(buf,32,"%X",(m_IEEEAddr[i] & 0xF) );
        str += buf;
        str.push_back('.');
    }
    str.erase(str.size()-1,1);
    return str;
}
void CNumProgrammerDlg::IEEEAddressNum2EachValue()
{
	unsigned char buf[5];

    m_Channel = m_IEEEAddr[5];

    unsigned int producttypeidx = m_IEEEAddr[6]-3;
    if (producttypeidx < (unsigned int)m_ProductType.GetCount())
    {
        m_ProductType.SetCurSel(producttypeidx);
    }
    else
    {
        m_ProductType.SetCurSel(-1);
    }

	buf[0] = (m_IEEEAddr[0]>>4)&0x0F;
	buf[1] = m_IEEEAddr[0]&0x0F;
	buf[2] = (m_IEEEAddr[1]>>4)&0x0F;
    m_LotNumber = buf[0]*100+buf[1]*10+buf[2];

	buf[0] = m_IEEEAddr[1]&0x0F;
	buf[1] = (m_IEEEAddr[2]>>4)&0x0F;
	buf[2] = m_IEEEAddr[2]&0x0F;
	buf[3] = (m_IEEEAddr[3]>>4)&0x0F;
	buf[4] = m_IEEEAddr[3]&0x0F;
	m_CardNumber = buf[0]*10000+buf[1]*1000+buf[2]*100+buf[3]*10+buf[4];
    //m_ShapeMode.Empty();
    //m_ShapeMode += m_IEEEAddr[4];
    //m_ShapeMode += m_IEEEAddr[5];

    m_ShapeMode = m_IEEEAddr[4];

    //m_CardNumber = (((unsigned int)m_IEEEAddr[6])*256 + (unsigned int)m_IEEEAddr[7]);
    UpdateData(false);
    return;
}

bool CNumProgrammerDlg::IEEEAddressEachValue2Num()
{
	unsigned char buf[14];

    if (GetDlgItem(IDC_EDIT2)->GetWindowTextLength() <=0
            || GetDlgItem(IDC_EDIT3)->GetWindowTextLength() <=0
            || GetDlgItem(IDC_EDIT4)->GetWindowTextLength() <=0
            || GetDlgItem(IDC_EDIT5)->GetWindowTextLength() <=0)
    {

        return false;
    }
    UpdateData(true);

    if (m_Channel < 11 || m_Channel> 26
            || m_ProductType.GetCurSel() < 0
            || m_LotNumber > 999
            || m_CardNumber > 65000 || m_CardNumber == 0
			|| m_ShapeMode > 255
            //|| m_ShapeMode.GetLength() < 2
       )
    {

        return false;

    }
    if(device ==DEVICE_CC2430)
    {
        if(m_FlashSize.GetCurSel() < 0)
        {
            return false;
        }

    }


    for (int i=0; i<8; i++)
    {
        m_IEEEAddr[i] = 0xFF;
    }

	buf[0] = m_LotNumber/100;
	buf[1] = (m_LotNumber%100)/10;
	buf[2] = m_LotNumber%10;

	buf[3] = m_CardNumber/10000;
	buf[4] = (m_CardNumber%10000)/1000;
	buf[5] = (m_CardNumber%1000)/100;
	buf[6] = (m_CardNumber%100)/10;
	buf[7] = (m_CardNumber%10)/1;

    //m_IEEEAddr[7]保留
	m_IEEEAddr[6] = m_ProductType.GetCurSel() + 3;//0x03：人员卡 0x04：设备卡
    m_IEEEAddr[5] = m_Channel;
    m_IEEEAddr[4] = (unsigned char)m_ShapeMode;//MODE
    m_IEEEAddr[3] = (buf[6]<<4)|buf[7];//(unsigned char)m_ShapeMode[0];
    m_IEEEAddr[2] = (buf[4]<<4)|buf[5];//(unsigned char)m_ShapeMode[1];
    m_IEEEAddr[1] = (buf[2]<<4)|buf[3];//(m_CardNumber>>8) & 0x000000FF;
    m_IEEEAddr[0] = (buf[0]<<4)|buf[1];//(m_CardNumber) & 0x000000FF;

/*
    m_IEEEAddr[1] = m_Channel;
    m_IEEEAddr[2] = m_ProductType.GetCurSel() + 1;
    m_IEEEAddr[3] = m_LotNumber;
    m_IEEEAddr[4] = (unsigned char)m_ShapeMode[0];
    m_IEEEAddr[5] = (unsigned char)m_ShapeMode[1];
    m_IEEEAddr[6] = (m_CardNumber>>8) & 0x000000FF;
    m_IEEEAddr[7] = (m_CardNumber) & 0x000000FF;
*/
    return true;
}


void CNumProgrammerDlg::OnBnClickedButtonFilepath()
{
    // TODO: 在此添加控件通知处理程序代码
    CFileDialog filedlg(TRUE,
                        NULL,
                        m_FlashFilePath,
                        OFN_EXPLORER|OFN_NOCHANGEDIR,
                        NULL,
                        this );
    filedlg.DoModal();
    m_FlashFilePath = filedlg.GetPathName();
    UpdateData(false);
}


void CNumProgrammerDlg::OnBnClickedCheck2()
{
    // TODO: 在此添加控件通知处理程序代码
    if (((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck())
    {
        GetDlgItem(IDC_COMBO1)->EnableWindow(false);
        GetDlgItem(IDC_COMBO2)->EnableWindow(false);
        GetDlgItem(IDC_EDIT2)->EnableWindow(false);
        GetDlgItem(IDC_EDIT3)->EnableWindow(false);
        GetDlgItem(IDC_EDIT4)->EnableWindow(false);
        GetDlgItem(IDC_EDIT5)->EnableWindow(false);
        //GetDlgItem(IDC_CHECK1)->EnableWindow(false);
        GetDlgItem(IDC_CHECK3)->EnableWindow(false);
        GetDlgItem(IDC_EDIT_FILEPATH)->EnableWindow(false);
        GetDlgItem(IDC_BUTTON_FILEPATH)->EnableWindow(false);
    }
    else
    {
        GetDlgItem(IDC_COMBO1)->EnableWindow(true);
        GetDlgItem(IDC_COMBO2)->EnableWindow(true);
        GetDlgItem(IDC_EDIT2)->EnableWindow(true);
        GetDlgItem(IDC_EDIT3)->EnableWindow(true);
        GetDlgItem(IDC_EDIT4)->EnableWindow(true);
        GetDlgItem(IDC_EDIT5)->EnableWindow(true);
        //GetDlgItem(IDC_CHECK1)->EnableWindow(true);
        GetDlgItem(IDC_CHECK3)->EnableWindow(true);
        GetDlgItem(IDC_EDIT_FILEPATH)->EnableWindow(true);
        GetDlgItem(IDC_BUTTON_FILEPATH)->EnableWindow(true);
    }
}

void CNumProgrammerDlg::OnBnClickedButtonProgram()
{
    // read result
    ifstream fpResult;
    string LineBuf;
    ofstream fpLog;


    /* disable the entire window */
    EnableWindow(false);

//    bool isRetainIEEE = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck()>0 ? true:false;
    bool isRetainIEEE = false;
    bool isFlashLock  = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck()>0 ? true:false;
    bool isAvoidStupid = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck()>0 ? true:false;

    if (!isRetainIEEE && !IEEEAddressEachValue2Num())
    {
        AfxMessageBox(_T("IEEE地址错误!"));
        goto err;
    }

    if (m_FlashFilePath.Find(_T(".hex")) < 0)
    {
        AfxMessageBox(_T("hex文件错误!"));
        goto err;
    }

    int real_flashSizeidx;
    if(device == DEVICE_INVALID)
    {
        AfxMessageBox(_T("设备错误!"));
        goto err;
    }
    else if(device == DEVICE_CC2430)
    {
        switch(m_FlashSize.GetCurSel())
        {
        case(0):
            real_flashSizeidx=128;
            break;
        case(1):
            real_flashSizeidx=64;
            break;
        case(2):
            real_flashSizeidx=32;
            break;

        }
    }

    // generate .bat file
    FILE *fpexec;
    fpexec = fopen("ExecProgram.bat","w");
    if(fpexec == NULL)
    {
        AfxMessageBox(_T("open ExecProgram.bat error!"));
        exit(1);
    }

    if (isRetainIEEE && isFlashLock)
    {
        char* filestr = UnicodeToAnsi((LPCTSTR) m_FlashFilePath);
        if(device==DEVICE_CC2430)
        {
            fprintf(fpexec,"SmartRFProgConsole.exe S CE > ExecProgram.txt 2>&1\n");
            fprintf(fpexec,"SmartRFProgConsole.exe S EPV F=\"%s\" KI(F=%d) LA(7) >> ExecProgram.txt 2>&1\n",filestr,real_flashSizeidx);
        }
        else
        {
            fprintf(fpexec,"SmartRFProgConsole.exe S CE > ExecProgram.txt 2>&1\n");
            fprintf(fpexec,"SmartRFProgConsole.exe S EPV F=\"%s\" KI(F=%d) LD >> ExecProgram.txt 2>&1\n",filestr,0);  //m_FlashSize.GetCurSel() real_flashSizeidx
        }
        free(filestr);
    }
    else if (isRetainIEEE && !isFlashLock)
    {
        char* filestr = UnicodeToAnsi((LPCTSTR) m_FlashFilePath);
        if(device==DEVICE_CC2430)
        {
            fprintf(fpexec,"SmartRFProgConsole.exe S CE > ExecProgram.txt 2>&1\n");
            fprintf(fpexec,"SmartRFProgConsole.exe S EPV F=\"%s\" KI(F=%d) >> ExecProgram.txt 2>&1\n",filestr,real_flashSizeidx);
        }
        else
        {
            fprintf(fpexec,"SmartRFProgConsole.exe S CE > ExecProgram.txt 2>&1\n");
            fprintf(fpexec,"SmartRFProgConsole.exe S EPV F=\"%s\" KI(F=%d) >> ExecProgram.txt 2>&1\n",filestr,0);
        }
        free(filestr);
    }
    else if (!isRetainIEEE && isFlashLock)
    {
        string addrstr = IEEEAddressNum2String();
        char* filestr = UnicodeToAnsi((LPCTSTR) m_FlashFilePath);
        if(device == DEVICE_CC2430)
        {
            fprintf(fpexec,"SmartRFProgConsole.exe S CE > ExecProgram.txt 2>&1\n");
            fprintf(fpexec,"SmartRFProgConsole.exe S EPV F=\"%s\" WI(F=%d,I=%s) LA(7) >> ExecProgram.txt 2>&1\n",filestr,real_flashSizeidx,addrstr.c_str());
        }
        else
        {
            fprintf(fpexec,"SmartRFProgConsole.exe S CE > ExecProgram.txt 2>&1\n");
            fprintf(fpexec,"SmartRFProgConsole.exe S EPV F=\"%s\" WI(F=%d,I=%s) LD >> ExecProgram.txt 2>&1\n",filestr,0,addrstr.c_str());
        }
        free(filestr);
    }
    else
    {
        string addrstr = IEEEAddressNum2String();
        char* filestr = UnicodeToAnsi((LPCTSTR) m_FlashFilePath);
        if(device == DEVICE_CC2430)
        {
            fprintf(fpexec,"SmartRFProgConsole.exe S CE > ExecProgram.txt 2>&1\n");
            fprintf(fpexec,"SmartRFProgConsole.exe S EPV F=\"%s\" WI(F=%d,I=%s) >> ExecProgram.txt 2>&1\n",filestr,real_flashSizeidx,addrstr.c_str());
        }
        else
        {
            fprintf(fpexec,"SmartRFProgConsole.exe S CE > ExecProgram.txt 2>&1\n");
            fprintf(fpexec,"SmartRFProgConsole.exe S EPV F=\"%s\" WI(F=%d,I=%s) >> ExecProgram.txt 2>&1\n",filestr,0,addrstr.c_str());
        }
        free(filestr);
    }
    fclose(fpexec);

    fopen_s(&fpexec,"CleanProg.bat","w");
    if(fpexec == NULL)
    {
        AfxMessageBox(_T("open CleanProg.bat error!"));
        exit(1);
    }

    fprintf(fpexec,"del ExecProgram.txt\n");
    fclose(fpexec);

    // run  .bat file
    (GetDlgItem(IDC_STATIC_RESULT))->SetWindowText(_T("正在烧写"));
    (GetDlgItem(IDC_STATIC_RESULT))->UpdateData(false);
    ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->SetRange(0,10);
    ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->SetStep(1);
    ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->SetPos(0);

    int flag = (int)ShellExecute(NULL,_T("open"),_T("CleanProg.bat"),NULL,NULL,SW_HIDE);   // clean old results
    if(flag == 0)
    {
        AfxMessageBox(_T("run CleanProg.bat error!"));
        exit(1);
    }
    gMutex.Lock();
    flag = (int)ShellExecute(NULL,_T("open"),_T("ExecProgram.bat"),NULL,NULL,SW_HIDE);    // run programing
    if(flag == 0)
    {
        AfxMessageBox(_T("run ExecProgram.bat error!"));
        exit(1);
    }
    gMutex.Unlock();

    fpLog.clear();
    fpLog.open(_T("log.txt"),ios::out);
    for (int i=0; i<40; i++)
    {
        string logstr("------------- try read --------------\n");
        fpLog<<logstr<<endl;

        Sleep(500);
        if(((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->GetPos()<10)
        {
            ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->StepIt();
        }
        else
        {
            ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->SetPos(0);
        }

        ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->UpdateData(false);
        /*
        int flag = (int)ShellExecute(NULL,_T("open"),_T("MoveExec.bat"),NULL,NULL,SW_HIDE);    // move ExecProgram.txt to ExecProgram.txt.bak
        if(flag == 0)
        {
        AfxMessageBox(_T("run MoveExec.bat error!"));
        exit(1);
        }
        */

        fpResult.clear();
        fpResult.open("ExecProgram.txt",ios::in);
        if(!fpResult)
        {
            string logstr("open failed\n");
            fpLog<<logstr<<endl;
            continue;
        }
        else
        {
            string logstr("open success\n");
            fpLog<<logstr<<endl;
        }

        bool isFileEmpty;
        isFileEmpty = true;

        while (getline(fpResult,LineBuf))
        {
            fpLog<<LineBuf<<endl; // print all lines to log

            if (LineBuf.empty())
            {
                continue;
            }
            isFileEmpty = false;
            if (LineBuf.find("verify OK")!=string::npos)      //Erase, program and verify OK
            {
                fpResult.close();
                fpLog.close();

                (GetDlgItem(IDC_STATIC_RESULT))->SetWindowText(_T("烧写成功"));
                ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->SetPos(10);
                EnableWindow(true);
                if (isAvoidStupid)
                {
                    m_CardNumber++;
                    UpdateData(false);
                    if (!IEEEAddressEachValue2Num())
                    {
                        AfxMessageBox(_T("IEEE地址错误!"));
                        goto err;
                    }
                    string str = IEEEAddressNum2String();
                    GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(str.c_str()));
                }
                return;
            }
            else if (LineBuf.find("failed")!=string::npos
                     || LineBuf.find("error")!=string::npos
                     || LineBuf.find("Error")!=string::npos
                     || LineBuf.find("No System-on-Chip was detected")!=string::npos
                     || LineBuf.find("Could not")!=string::npos
                     || LineBuf.find("Not able")!=string::npos
                     || LineBuf.find("Unsupported")!=string::npos
                     || LineBuf.find("locked")!=string::npos
                    )  //it is failed
            {
                goto err;
            }
        }

        fpResult.close();
    }

err:
    fpResult.close();
    fpLog.close();
    (GetDlgItem(IDC_STATIC_RESULT))->SetWindowText(_T("烧写失败"));

	MessageBox(_T("     烧写失败！"),NULL,MB_OK);

    ((CProgressCtrl*)GetDlgItem(IDC_PROGRESS1))->SetPos(10);
    EnableWindow(true);
}

void CNumProgrammerDlg::OnEnChangeEdit3()
{
    // TODO:  如果该控件是 RICHEDIT 控件，则它将不会
    // 发送该通知，除非重写 CDialog::OnInitDialog()
    // 函数并调用 CRichEditCtrl().SetEventMask()，
    // 同时将 ENM_CHANGE 标志“或”运算到掩码中。

    // TODO:  在此添加控件通知处理程序代码
    if (!IEEEAddressEachValue2Num())
    {
        return;
    }
    string str = IEEEAddressNum2String();
    GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(str.c_str()));
}

void CNumProgrammerDlg::OnCbnSelchangeCombo2()
{
    // TODO: 在此添加控件通知处理程序代码
    if (!IEEEAddressEachValue2Num())
    {
        return;
    }
    string str = IEEEAddressNum2String();
    GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(str.c_str()));
}

void CNumProgrammerDlg::OnEnChangeEdit4()
{
    // TODO:  如果该控件是 RICHEDIT 控件，则它将不会
    // 发送该通知，除非重写 CDialog::OnInitDialog()
    // 函数并调用 CRichEditCtrl().SetEventMask()，
    // 同时将 ENM_CHANGE 标志“或”运算到掩码中。

    // TODO:  在此添加控件通知处理程序代码
    if (!IEEEAddressEachValue2Num())
    {
        return;
    }
    string str = IEEEAddressNum2String();
    GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(str.c_str()));
}

void CNumProgrammerDlg::OnEnChangeEdit5()
{
    // TODO:  如果该控件是 RICHEDIT 控件，则它将不会
    // 发送该通知，除非重写 CDialog::OnInitDialog()
    // 函数并调用 CRichEditCtrl().SetEventMask()，
    // 同时将 ENM_CHANGE 标志“或”运算到掩码中。

    // TODO:  在此添加控件通知处理程序代码
    if (!IEEEAddressEachValue2Num())
    {
        return;
    }
    string str = IEEEAddressNum2String();
    GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(str.c_str()));
}

void CNumProgrammerDlg::OnEnChangeEdit2()
{
    // TODO:  如果该控件是 RICHEDIT 控件，则它将不会
    // 发送该通知，除非重写 CDialog::OnInitDialog()
    // 函数并调用 CRichEditCtrl().SetEventMask()，
    // 同时将 ENM_CHANGE 标志“或”运算到掩码中。

    // TODO:  在此添加控件通知处理程序代码
    if (!IEEEAddressEachValue2Num())
    {
        return;
    }
    string str = IEEEAddressNum2String();
    GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(str.c_str()));
}

wchar_t* CNumProgrammerDlg::AnsiToUnicode( const char* szStr )
{
    int nLen = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szStr, -1, NULL, 0 );
    if (nLen == 0)
    {
        return NULL;
    }
    wchar_t* pResult = new wchar_t[nLen];
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szStr, -1, pResult, nLen );
    return pResult;
}
char* CNumProgrammerDlg::UnicodeToAnsi( const wchar_t* szStr )
{
    int nLen = WideCharToMultiByte( CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL );
    if (nLen == 0)
    {
        return NULL;
    }
    char* pResult = new char[nLen];
    WideCharToMultiByte( CP_ACP, 0, szStr, -1, pResult, nLen, NULL, NULL );
    return pResult;
}

void CNumProgrammerDlg::OnBnClickedButtonInc()
{
    // TODO: 在此添加控件通知处理程序代码

    if(m_CardNumber<99999)
    {
        m_CardNumber++;
    }
    else
    {
        m_CardNumber = 0;
    }
    UpdateData(false);
    if (!IEEEAddressEachValue2Num())
    {
        AfxMessageBox(_T("IEEE地址错误!"));
        return;
    }
    string str = IEEEAddressNum2String();
    GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(str.c_str()));
}

void CNumProgrammerDlg::OnBnClickedButtonDec()
{
    // TODO: 在此添加控件通知处理程序代码
    if (m_CardNumber>0)
    {
        m_CardNumber--;
    }
    else
    {
        m_CardNumber = 99999;
    }

    UpdateData(false);
    if (!IEEEAddressEachValue2Num())
    {
        AfxMessageBox(_T("IEEE地址错误!"));
        return;
    }
    string str = IEEEAddressNum2String();
    GetDlgItem(IDC_EDIT_IEEE)->SetWindowText(CString(str.c_str()));
}

void CNumProgrammerDlg::OnStnClickedStaticResult()
{
	// TODO: 在此添加控件通知处理程序代码
}

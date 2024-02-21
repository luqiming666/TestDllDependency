
// TestDllDependencyDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "TestDllDependency.h"
#include "TestDllDependencyDlg.h"
#include "afxdialogex.h"
#include <iostream>

#include "LateLoad_DllBWrapper.h"

#include "..\Win32DllC\Win32DllCAPIs.h"
#pragma comment(lib, "Win32DllC.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 打开一个控制台窗口，方便查看调试信息
#define DEBUG_WINDOW_IS_OPEN	1


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTestDllDependencyDlg 对话框



CTestDllDependencyDlg::CTestDllDependencyDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TESTDLLDEPENDENCY_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestDllDependencyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTestDllDependencyDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CTestDllDependencyDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CTestDllDependencyDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CTestDllDependencyDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CTestDllDependencyDlg 消息处理程序

BOOL CTestDllDependencyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
#if DEBUG_WINDOW_IS_OPEN
	// 打开一个控制台查看日志
	if (::GetConsoleWindow() == NULL)
	{
		if (::AllocConsole())
		{
			FILE* stream;
			freopen_s(&stream, "CONOUT$", "w", stdout);
		}
	}
#endif

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTestDllDependencyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTestDllDependencyDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTestDllDependencyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Test: Dynamically-load DLL-B, which loads DLL-A
//	DLL-A: MFC，通过宏导出一个类
//	DLL-B: MFC，通过宏或者.def文件导出函数
void CTestDllDependencyDlg::OnBnClickedButton1()
{
	CDLLBWrapper dllBWrapper;
	int result = dllBWrapper.AddThreeNumbers(1, 2, 3);
	std::cout << "EXE -> DLL-B -> DLL-A: " << result << std::endl;
}

// Test: call a method in DLL C（注：通过.lib静态链接）
// 测试方法：将Win32DllC.dll删除，然后直接运行TestDllDependency.exe
// 现象：App无法运行，主界面无法展现！报错：
//	TestDllDependency.exe - 系统错误
//	由于找不到Win32DllC.dll，无法继续执行代码。重新安装程序可能会解决此问题。
void CTestDllDependencyDlg::OnBnClickedButton2()
{
	dllcapi_init();

	int result = dllcapi_add_four_numbers(1, 2, 3, 4);
	std::cout << "EXE -> Win32 standard DLL-C: " << result << std::endl;

	dllcapi_release();
}

// Test: Dynamically-load DLL-B
//	重点测试DLL-B中带有namespace的函数调用
void CTestDllDependencyDlg::OnBnClickedButton3()
{
	CDLLBWrapper dllBWrapper;
	int result = dllBWrapper.dllbapi_add(3, 4);
	std::cout << "EXE -> DLL-B >> simple API: " << result << std::endl;

	// DLL中经过 extern "C" 修饰过的 namespace 被作废了！
	result = dllBWrapper.dllbapi_minus(10, 2);
	std::cout << "EXE -> DLL-B >> API with extern C namespace: " << result << std::endl;


	///////////////////////////////////////////////////
	// 不能使用LateLoad.h了...
	HINSTANCE hDLL = LoadLibrary("DLLB.dll");
	if (hDLL == NULL) {
		std::cout << "Failed to load DLL" << std::endl;
		return;
	}

	// 获取函数指针
	// 注意：在C++中，函数名会被编译器进行名称修饰（name mangling），导致实际的函数名与源代码中的函数名不一样。
	//	因此，在使用GetProcAddress获取函数指针时，需要使用实际的修饰后的函数名。
	//	或者在DLL开发时，使用extern "C"来告诉编译器不要进行名称修饰
	// 【技巧】可以通过如下命令查看DLL的导出函数名
	//		dumpbin /EXPORTS YourDLL.dll
	typedef int (*MyFunctionPtr)(int, int);
	MyFunctionPtr myFunction = (MyFunctionPtr)GetProcAddress(hDLL, "?dllbapi_power@MySpace@@YAHHH@Z"); // 不能使用MySpace::dllbapi_power
	if (myFunction == NULL) {
		std::cout << "Failed to get function pointer" << std::endl;
		return;
	}

	result = myFunction(2, 3);
	std::cout << "EXE -> DLL-B >> API with namespace: " << result << std::endl;

	// 释放DLL
	FreeLibrary(hDLL);
}

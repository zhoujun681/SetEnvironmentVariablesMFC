
// SetEnvironmentVariablesMFC2Dlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "SetEnvironmentVariablesMFC2.h"
#include "SetEnvironmentVariablesMFC2Dlg.h"
#include "afxdialogex.h"
#include "windows.h"
#include <shlobj.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool isModule = 1;  //是否使用GoModule模式

const int REGMOD = 0; //设置注册表模式,推荐
const int SETXMOD = 1; //调用系统setx模式,缺点：无法保留通配符
int setMod = SETXMOD;

wchar_t* pathev = NULL;


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

/*
把普通string转为宽字符string
*/
std::wstring string2wstring(std::string str)
{
	std::wstring result;
	//获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
	TCHAR* buffer = new TCHAR[len + 1];
	//多字节编码转换成宽字节编码  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
	buffer[len] = '\0';             //添加字符串结尾  
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

/*
读取注册表
参数:
HKEY hKey  主键值
LPCSTR lpSubKey  子健路径
LPCSTR key  路径下的键名

*/
std::string RegQueryBb(_In_ HKEY hKey,_In_opt_ LPCSTR lpSubKey, LPCSTR key) {
    HKEY retKey; //用于关联打开的注册表实例
	LONG ret;	//查询返回状态值 ERROR_SUCCESS（0）为成功
	char retKeyVal[MAX_PATH * 8] = { 0 };//查询结果值，MAX_PATH：编译器所支持的最长全路径名的长度。
	DWORD nSize = MAX_PATH * 8; //查询出的结果的缓存空间大小
	std::string lpVal = ""; //查询结果的string值

	/*
	作用，打开注册表
	LONG RegOpenKeyEx(
    HKEY hKey, // 需要打开的主键的名称
    LPCTSTR lpSubKey, //需要打开的子键的名称
    DWORD ulOptions, // 保留，设为0
    REGSAM samDesired, // 安全访问标记，也就是权限
    PHKEY phkResult // 指向新创建或打开的键的句柄的指针
) 
ulOptions可用值：
　  KEY_CREATE_LINK 准许生成符号键
　　KEY_CREATE_SUB_KEY 准许生成子键
　　KEY_ENUMERATE_SUB_KEYS 准许生成枚举子键
　　KEY_EXECUTE 准许进行读操作
　　KEY_NOTIFY 准许更换通告
　　KEY_QUERY_VALUE 准许查询子键
　　KEY_ALL_ACCESS 提供完全访问，是上面数值的组合
　　KEY_READ 是下面数值的组合：
　　KEY_QUERY_VALUE、KEY_ENUMERATE_SUB_KEYS、KEY_NOTIFY
　　KEY_SET_VALUE 准许设置子键的数值
　　KEY_WRITE 是下面数值的组合：
　　KEY_SET_VALUE、KEY_CREATE_SUB_KEY
	*/
	ret = RegOpenKeyExA(hKey, lpSubKey, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &retKey); 
	if (ret != ERROR_SUCCESS) {
		return FALSE;
	}

	//查询键值
	/*
	LONG WINAPI RegQueryValueEx(
    HKEY hKey,            // handle to key
    LPCTSTR lpValueName,  // value name
    LPDWORD lpReserved,   // reserved   0
    LPDWORD lpType,       // type buffer
    LPBYTE lpData,        // data buffer
    LPDWORD lpcbData      // size of data buffer
);
	*/
	ret = RegQueryValueExA(retKey, key, NULL, NULL, (LPBYTE)retKeyVal, &nSize);
	RegCloseKey(retKey);

	if (ret != ERROR_SUCCESS)
	{
		return NULL;
	}
	lpVal = retKeyVal;
	return lpVal;
}

/*
设置注册表键值  
  HKEY hKey,           // handle to key
  LPCSTR lpSubKey    //子路径
  LPCTSTR lpValueName // value name
  DWORD dwType // value type  
  const char *data // value data
*/
int RegSetValueBb(_In_ HKEY hKey, _In_opt_ LPCSTR lpSubKey, LPCSTR lpValueName, DWORD dwType,const char *data) {
	HKEY retKey;

	LONG ret;
	bool isExist = TRUE; //是否存在键值
	LPBYTE datas = (LPBYTE)data;
	int dataLen = strlen(data) + 1;  //需要用strlen才能正确获取长度
	//打开注册表
	ret = RegOpenKeyExA(hKey, lpSubKey, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY | KEY_WRITE, &retKey);  //打开注册表链接，使用retKey来操作注册表。加上KEY_WRITE才能写注册表
	//如果键值不存在则创建,新版本RegOpenKeyExA不存在默认会创建目录，所以这一段用不到
	/*
    HKEY tmpRetKey;
	if (ret != ERROR_SUCCESS) { 
		isExist = FALSE;
		ret = RegCreateKeyA(hKey, lpSubKey, &tmpRetKey);
		ret = RegSetValueExA(tmpRetKey, KeyName, NULL, REG_EXPAND_SZ, datas, dataLen);	//设置键值
		RegCloseKey(retKey);
		if (ret != ERROR_SUCCESS) {
			return -1;
		}
		return 0;
	}
	*/

	/*
	LONG RegSetValueEx(
  HKEY hKey,           // handle to key
  LPCTSTR lpValueName, // value name
  DWORD Reserved,      // reserved  must be 0
  DWORD dwType,        // value type  
  CONST BYTE *lpData,  // value data
  DWORD cbData         // size of value data
);
hkey： 当前打开的密钥或以下预定义密钥之一的句柄:

HKEY_CLASSES_ROOT
HKEY_CURRENT_CONFIG
HKEY_CURRENT_USER
HKEY_LOCAL_MACHINE
HKEY_USERS
Windows NT/2000/XP: HKEY_PERFORMANCE_DATA
Windows 95/98/Me: HKEY_DYN_DATA

lpValueName：指向一个字符串的指针，该字符串包含要设置的值的名称。如果键中不存在这个名称的值，函数将其添加到键中。如果lpValueName是NULL或空字符串""，函数将为键的未命名或默认值设置类型和数据。

Reserved：保留;必须是零。

dwType：指定由lpData参数指向的数据类型的代码。有关可能类型代码的列表，请参见注册表值类型。

Ipdata：指向一个缓冲区的指针，该缓冲区包含以指定值名存储的数据。对于基于字符串的数据类型，如REG_SZ，字符串必须为空终止。对于REG_MULTI_SZ数据类型，字符串必须以双null结尾。

cbData：指定lpData参数指向的信息的大小(以字节为单位)。如果数据类型为REG_SZ、REG_EXPAND_SZ或REG_MULTI_SZ，则cbData必须包括终止null字符或字符的大小。

返回值

如果函数成功，返回值为ERROR_SUCCESS。如果函数失败，返回值是Winerror.h中定义的非零错误代码。您可以使用FormatMessage函数和FORMAT_MESSAGE_FROM_SYSTEM标志来获得错误的通用描述。
*/
	ret = RegSetValueExA(retKey, lpValueName, NULL, dwType, datas, dataLen);	//设置键值


	RegCloseKey(retKey);

	if (ret != ERROR_SUCCESS)
	{
		return 0;
	}
	return 1;
}

std::vector<std::string> Split(std::string input_string, std::string search_string)
{
	std::list<int> search_hit_list;
	std::vector<std::string> word_list;
	size_t search_position, search_start = 0;

	// Find start positions of every substring occurence and store positions to a hit list.
	while ((search_position = input_string.find(search_string, search_start)) != std::string::npos) {
		search_hit_list.push_back(search_position);
		search_start = search_position + search_string.size();
	}

	// Iterate through hit list and reconstruct substring start and length positions
	int character_counter = 0;
	int start, length;

	for (auto hit_position : search_hit_list) {

		// Skip over substrings we are splitting with. This also skips over repeating substrings.
		if (character_counter == hit_position) {
			character_counter = character_counter + search_string.size();
			continue;
		}

		start = character_counter;
		character_counter = hit_position;
		length = character_counter - start;
		word_list.push_back(input_string.substr(start, length));
		character_counter = character_counter + search_string.size();
	}

	// If the search string is not found in the input string, then return the whole input_string.
	if (word_list.size() == 0) {
		word_list.push_back(input_string);
		return word_list;
	}
	// The last substring might be still be unprocessed, get it.
	if (character_counter < input_string.size()) {
		word_list.push_back(input_string.substr(character_counter, input_string.size() - character_counter));
	}

	return word_list;
}



DWORD StringToDword(std::string val)
{
	DWORD cur_dword;
	sscanf_s(val.c_str(), "%ul", &cur_dword);
	return cur_dword;
}


CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSetEnvironmentVariablesMFC2Dlg 对话框



CSetEnvironmentVariablesMFC2Dlg::CSetEnvironmentVariablesMFC2Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETENVIRONMENTVARIABLESMFC2_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void CSetEnvironmentVariablesMFC2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_cb1);
}

BEGIN_MESSAGE_MAP(CSetEnvironmentVariablesMFC2Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT1, &CSetEnvironmentVariablesMFC2Dlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_BUTTON1, &CSetEnvironmentVariablesMFC2Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CSetEnvironmentVariablesMFC2Dlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_CHECK1, &CSetEnvironmentVariablesMFC2Dlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON3, &CSetEnvironmentVariablesMFC2Dlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CSetEnvironmentVariablesMFC2Dlg 消息处理程序

BOOL CSetEnvironmentVariablesMFC2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//GetDlgItem(IDC_EDIT1)->SetWindowText(str);
	((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);
	SetDlgItemText(IDC_EDIT1, _T("c:\\go"));
	SetDlgItemText(IDC_EDIT2, _T("D:\\project_files\\go"));

	/*
	setMod初始化
	*/
	//m_cb1.AddString(_T("系统命令模式"));  //无序方式，系统自动排序
	//m_cb1.AddString(_T("注册表模式"));//无序方式，系统自动排序
	m_cb1.InsertString(0, _T("注册表模式"));
	m_cb1.InsertString(1, _T("系统命令模式"));
	//m_cb1.FindStringExact(0, _T("注册表模式"));//从索引0开始查找选项的索引值
	m_cb1.SetCurSel(0); 

	

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

	//ShowWindow(SW_MAXIMIZE);

	//ShowWindow(SW_MINIMIZE);

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSetEnvironmentVariablesMFC2Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CSetEnvironmentVariablesMFC2Dlg::OnPaint()
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
HCURSOR CSetEnvironmentVariablesMFC2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CSetEnvironmentVariablesMFC2Dlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。


	// TODO:  在此添加控件通知处理程序代码
}

std::string CString2string(CString csStrData)
{
	std::string strRet;
	char ss[2048];
	memset(ss, 0, sizeof(char) * 2048);
	sprintf_s(ss, "%s", csStrData);

	strRet = ss;
	return strRet;
}

char* wchar2char(const wchar_t* wchar)
{
	char* m_char;
	int len = WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), NULL, 0, NULL, NULL);
	m_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), m_char, len, NULL, NULL);
	m_char[len] = '\0';
	return m_char;
}

wchar_t* char2wchar(const char* cchar)
{
	wchar_t* m_wchar = new wchar_t[strlen(cchar) + 1];
	MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), m_wchar, strlen(cchar));
	m_wchar[strlen(cchar)] = L'\0';
	return m_wchar;
}

std::string stringDISTINCT(const char* c, std::string sep) {
	std::vector<std::string> evs = Split(c, sep);
	sort(evs.begin(), evs.end());
	std::vector<std::string>::iterator iter = std::unique(evs.begin(), evs.end());
	evs.erase(iter, evs.end());

	std::string s = "";
	for (int i = 0; i < evs.size(); ++i) {
		s += evs[i];
		s += sep;
	}
	s += '\0';
	return s;
}


/*
Set按钮
*/
void CSetEnvironmentVariablesMFC2Dlg::OnBnClickedButton1()
{
	bool isSucess = 1;
	std::string regstr;
	std::wstring regRes;
	std::string adVaule;
	CString str = NULL;
	int results[8] = { 0 };
	setMod = m_cb1.GetCurSel(); //获取当前选择值
	switch (setMod) {
	case REGMOD:
		// TODO: 在此添加控件通知处理程序代码
		SetDlgItemText(IDC_STATIC, _T("注册表模式正在执行中，请稍候..."));
		Sleep(300);
		regstr = RegQueryBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "Path");
		//备份PATH
		isSucess = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "Path_Reg_BAK", REG_EXPAND_SZ, regstr.c_str());
		//设置PATH
		GetDlgItemText(IDC_EDIT1, str);
		results[0] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "GOROOT", REG_EXPAND_SZ, std::string(CT2A(str.GetString())).c_str());
		GetDlgItemText(IDC_EDIT2, str);
		results[5] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "GOPATH", REG_EXPAND_SZ, std::string(CT2A(str.GetString())).c_str());
		results[1] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "GOPROXY", REG_EXPAND_SZ, "https://goproxy.io");
		results[2] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "GOARCH", REG_EXPAND_SZ,"amd64");
		results[3] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "GOOS", REG_EXPAND_SZ, "Windows");
		results[4] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "GOTOOLS", REG_EXPAND_SZ, "%GOROOT%\\pkg\\tool");

		if (isModule) {
			results[6] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "GO111MODULE", REG_EXPAND_SZ, "on");
		}
		else {
			results[6] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "GO111MODULE", REG_EXPAND_SZ, "off");
		}
	
		
		regRes = string2wstring(regstr);
		adVaule = regstr + ";%GOROOT%\\bin;%GOPATH%\\bin";
		adVaule = stringDISTINCT(adVaule.c_str(), ";"); //排序并去重
		results[7] = RegSetValueBb(HKEY_LOCAL_MACHINE, "SYSTEM\\ControlSet001\\Control\\Session Manager\\Environment", "Path", REG_EXPAND_SZ, adVaule.c_str());
		for (int result : results) {
			if (result ==0) {
				isSucess = 0;
				break;
			}
		}
		break;
	case SETXMOD:
		SetDlgItemText(IDC_STATIC, _T("系统命令模式正在执行中，请稍候..."));
		Sleep(300);
		GetDlgItemText(IDC_EDIT1, str);
		results[0] = WinExec(CA2W("cmd.exe /c C:\\Windows\\System32\\setx.exe -m GOROOT \"") + str + "\"", SW_HIDE);//隐藏控制
		GetDlgItemText(IDC_EDIT2, str);
		results[1] = WinExec(CA2W("cmd.exe /c C:\\Windows\\System32\\setx.exe -m GOPATH \"") + str + "\"", SW_HIDE);//隐藏控制
		results[2] = WinExec("cmd.exe /c C:\\Windows\\System32\\setx.exe -m  GOPROXY https://goproxy.io", SW_HIDE);//隐藏控制
		results[3] = WinExec("cmd.exe /c C:\\Windows\\System32\\setx.exe -m  GOARCH amd64", SW_HIDE);//隐藏控制
		results[4] = WinExec("cmd.exe /c C:\\Windows\\System32\\setx.exe -m  GOOS Windows", SW_HIDE);//隐藏控制
		results[5] = WinExec("cmd.exe /c C:\\Windows\\System32\\setx.exe -m  GOTOOLS ^%GOROOT^%\\pkg\\tool", SW_HIDE);//隐藏控制
		results[6] = WinExec("cmd.exe /c C:\\Windows\\System32\\setx.exe -m  PATH %PATH%;^%GOROOT^%\\bin;^%GOPATH^%\\bin", SW_HIDE);//隐藏控制
		if (isModule) {
			results[7] = WinExec("cmd.exe /c C:\\Windows\\System32\\setx.exe -m  GO111MODULE on", SW_HIDE);//隐藏控制
		}
		else {
			results[7] = WinExec("cmd.exe /c C:\\Windows\\System32\\setx.exe -m  GO111MODULE off", SW_HIDE);//隐藏控制
		}

		//改变path,去重，问题：%%通配符消失
		char* pathvar;
		size_t len;
		std::string pathstr = "";
		errno_t err = _dupenv_s(&pathvar, &len, "PATH"); //获得PATH的内容
		//备份Path
		std::string const& bak = "cmd.exe /c C:\\Windows\\System32\\setx.exe -m  PATH_SETX_BAK \"" + std::string(pathvar) + "\"";
		results[6] = WinExec(bak.c_str(), SW_HIDE);//隐藏控制
	
		//设置Path
		pathstr = stringDISTINCT(pathvar, ";"); //去重
		char* pathvar2 = new char[strlen(pathstr.c_str()) + 1];
		strcpy_s(pathvar2, strlen(pathstr.c_str()) + 1, pathstr.c_str());
		int num = MultiByteToWideChar(0, 0, pathvar2, -1, NULL, 0);
		wchar_t* wide = new wchar_t[num];
		MultiByteToWideChar(0, 0, pathvar2, -1, wide, num);
		std::string const& cc = "cmd.exe /c C:\\Windows\\System32\\setx.exe -m  PATH \"" + std::string(wchar2char(wide)) + "\"";
		results[6] = WinExec(cc.c_str(), SW_HIDE);//隐藏控制
		
	
		for (int result : results) {
			if (!result > 31) {
				isSucess = 0;
				break;
			}
		}
		delete pathvar2;
		delete wide;
	}
	if (isSucess) {
		SetDlgItemText(IDC_STATIC, _T("执行成功。。。"));
	}
	else
	{
		SetDlgItemText(IDC_STATIC, _T("执行失败。。。"));
	}
}



/*
GoRoot选择按钮
*/
void CSetEnvironmentVariablesMFC2Dlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	// 呼出文件夹
	BROWSEINFO bi = {0};
	ITEMIDLIST* pidl;
	TCHAR szPath[MAX_PATH];


	bi.hwndOwner = m_hWnd;
	bi.pszDisplayName = szPath;
	bi.lpszTitle = TEXT("请选择GoRoot的路径：");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;


	if (pidl = SHBrowseForFolder(&bi))
	{
		SHGetPathFromIDList(pidl, szPath);
		//return ;
	}
	if (NULL == pidl) // 如果没有选择文件路径，
	{
		return;
	}
	SetDlgItemText(IDC_EDIT1, szPath);


	UpdateData(FALSE);
	// TODO: 在此添加控件通知处理程序代码

}




/*
GoPath选择按钮
*/
void CSetEnvironmentVariablesMFC2Dlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
		// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	// 呼出文件夹
	BROWSEINFO bi = { 0 };
	ITEMIDLIST* pidl;
	TCHAR szPath[MAX_PATH];


	bi.hwndOwner = m_hWnd;
	bi.pszDisplayName = szPath;
	bi.lpszTitle = TEXT("请选择GoPath的路径：");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;


	if (pidl = SHBrowseForFolder(&bi))
	{
		SHGetPathFromIDList(pidl, szPath);
		//return ;
	}
	if (NULL == pidl) // 如果没有选择文件路径，
	{
		return;
	}
	SetDlgItemText(IDC_EDIT2, szPath);


	UpdateData(FALSE);
	// TODO: 在此添加控件通知处理程序代码
}

void CSetEnvironmentVariablesMFC2Dlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	if (IsDlgButtonChecked(IDC_CHECK1)) {
		isModule = 1;
	}
	else
	{
		isModule = 0;
	}
}

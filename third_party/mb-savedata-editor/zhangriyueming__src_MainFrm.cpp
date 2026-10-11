
// MainFrm.cpp : CMainFrame 类的实现
//

#include "stdafx.h"
#include "紫禁神器 之 战团SuperMS.h"

#include "MainFrm.h"
#include "MainCore.h"

#include "WSE.h"

#include "ZJ_Algorithm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CALLBACK GUICallBackMainView(UINT Event, int ID, CZJGUIControl* pCtrl)
{
    ((CMainFrame*)theApp.m_pMainWnd)->GUICallBackMainView(Event, ID, pCtrl);
}

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // 状态行指示器
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame 构造/析构

CMainFrame::CMainFrame()
{
	m_pd3d = NULL;
    m_pd3dDevice = NULL;
}

CMainFrame::~CMainFrame()
{
    Shutdown();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	//BOOL bNameValid;

	// 设置用于绘制所有用户界面元素的视觉管理器
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));

	// 设置视觉管理器使用的视觉样式
	CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("未能创建菜单栏\n");
		return -1;      // 未能创建
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// 防止菜单栏在激活时获得焦点
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	// 创建一个视图以占用框架的工作区
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("未能创建视图窗口\n");
		return -1;
	}

    /*
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME))
	{
		TRACE0("未能创建工具栏\n");
		return -1;      // 未能创建
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
    */

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("未能创建状态栏\n");
		return -1;      // 未能创建
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: 如果您不希望工具栏和菜单栏可停靠，请删除这五行
    /*
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);


	// 启用 Visual Studio 2005 样式停靠窗口行为
	CDockingManager::SetDockingMode(DT_SMART);
	// 启用 Visual Studio 2005 样式停靠窗口自动隐藏行为
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// 启用工具栏和停靠窗口菜单替换
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// 启用快速(按住 Alt 拖动)工具栏自定义
	CMFCToolBar::EnableQuickCustomization();
    */


    HICON hIcon = theApp.LoadIcon(IDR_MAINFRAME);
    SetIcon(hIcon, TRUE);
    SetIcon(hIcon, FALSE);


	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		 | WS_MINIMIZEBOX | WS_SYSMENU;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

// CMainFrame 诊断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame 消息处理程序

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// 将焦点前移到视图窗口
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// 让视图第一次尝试该命令
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// 否则，执行默认处理
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* 扫描菜单*/);
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}


void CMainFrame::InitDevice(void)
{
}


void CMainFrame::InitGeometry(void)
{
    m_Manager.Create(m_pd3dDevice);

    InitGUI();

    // Mesh
    HDC hdc = CreateCompatibleDC(NULL);
    HFONT hFont = CreateFontW(20,
        0,
        0,
        0,
        FW_BOLD,
        false,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_NATURAL_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"楷体");
    HFONT hFontOld = (HFONT)SelectObject(hdc, hFont);
    D3DXCreateTextW(m_pd3dDevice, hdc, L"紫禁神器之战团",
        0.001f, 0.2f, &m_pTitleMesh, NULL, NULL);
    SelectObject(hdc, hFontOld);
    DeleteObject(hFont);
    DeleteDC(hdc);

    // MatrixPre
    int numVertices = m_pTitleMesh->GetNumVertices();
    stVertexXYZNor *obj = new stVertexXYZNor[numVertices];
    void *ptr;

    m_pTitleMesh->LockVertexBuffer(D3DLOCK_READONLY, &ptr);
    memcpy(obj, ptr, sizeof(stVertexXYZNor) * numVertices);
    m_pTitleMesh->UnlockVertexBuffer();

    float min = 1000.0f, max = -1000.0f;
    for (int i = 0; i < numVertices; ++i)
    {
        if (obj[i].XYZ.x < min)
            min = obj[i].XYZ.x;
        if (obj[i].XYZ.x > max)
            max = obj[i].XYZ.x;
    }

    D3DXMATRIX matTrans;
    D3DXMatrixTranslation(&matTrans, -(max - min) / 2, 0.0f, 0.0f);
    m_matTitlePre = matTrans;

    // Material
    ZeroMemory(&m_mtrlTitle, sizeof(m_mtrlTitle));
    m_mtrlTitle.Diffuse.r = m_mtrlTitle.Ambient.r = 69.0f / 255;
    m_mtrlTitle.Diffuse.g = m_mtrlTitle.Ambient.g = 45.0f / 255;
    m_mtrlTitle.Diffuse.b = m_mtrlTitle.Ambient.b = 13.0f / 255;
    m_mtrlTitle.Diffuse.a = m_mtrlTitle.Ambient.a = 1.0f;

    // Meshes
    //theWSE.meshes[0].Create(m_pd3dDevice, L"pitch_fork");
}


void CMainFrame::InitFonts(void)
{
    m_Manager.CreateFont(_T("楷体"), 18, &IDF_K18);
    m_Manager.CreateFont(_T("楷体"), 24, &IDF_K24);
    m_Manager.CreateFont(_T("楷体"), 20, &IDF_K20);
    m_Manager.CreateFont(_T("楷体"), 17, &IDF_K17);
    m_Manager.CreateFont(_T("楷体"), 15, &IDF_K15);
    m_Manager.CreateFont(_T("楷体"), 14, &IDF_K14);


    m_Manager.CreateFont(_T("宋体"), 18, &IDF_S18);
}


void CMainFrame::InitTextures(void)
{
    CString strTexturePath = theCore.m_strGamePath + _T("Textures\\");

    m_Manager.CreateTexture(strTexturePath + _T("mp_ui_host.dds"), &IDT_MP_UI_HOST);
    m_Manager.CreateTexture(strTexturePath + _T("button_drop.dds"), &IDT_BUTTON_DROP);
    m_Manager.CreateTexture(strTexturePath + _T("interface.dds"), &IDT_INTERFACE);
    m_Manager.CreateTexture(strTexturePath + _T("main_menu_nord.dds"), &IDT_MAINMENU);
    m_Manager.CreateTexture(strTexturePath + _T("mp_ui_bg.dds"), &IDT_MP_UI_BG);
    m_Manager.CreateTexture(strTexturePath + _T("character_window.dds"), &IDT_CHARACTER_WINDOW);
    m_Manager.CreateTexture(strTexturePath + _T("bg2.dds"), &IDT_BG2);
    m_Manager.CreateTexture(strTexturePath + _T("inventory_window.dds"), &IDT_INVENTORY_WINDOW);
    m_Manager.CreateTexture(strTexturePath + _T("loadscreen.dds"), &IDT_LOADSCREEN);

    m_Manager.CreateTexture(0, MAKEINTRESOURCE(IDB_PNG1), &IDT_PREMENU_BUTTON1);
    m_Manager.CreateTexture(0, MAKEINTRESOURCE(IDB_PNG2), &IDT_PREMENU_BUTTON2);
    m_Manager.CreateTexture(0, MAKEINTRESOURCE(IDB_PNG3), &IDT_PREMENU_BUTTON3);
    m_Manager.CreateTexture(0, MAKEINTRESOURCE(IDB_PNG6), &IDT_LOGO1);
    m_Manager.CreateTexture(0, MAKEINTRESOURCE(IDB_PNG5), &IDT_LOGO2);
    m_Manager.CreateTexture(0, MAKEINTRESOURCE(IDB_PNG4), &IDT_LOGO3);
}



void RenderTitle()
{
    ((CMainFrame*)theApp.m_pMainWnd)->RenderTitle();
}


void PreRenderLoadScreen()
{
    ((CMainFrame*)theApp.m_pMainWnd)->PreRenderLoadScreen();
}



void CMainFrame::InitGUI(void)
{
    m_GUI.Create(&m_Manager);

    InitFonts();
    InitTextures();

    CZJGUIView *pView;
    CZJGUIElement e;
    RECT rcTexture, rcPadding, rcTrack;

    CZJGUIScrollBar *pScrollBar;
    CZJGUIListBox *pListBox;
    //CZJGUIComboBox *pComboBox;

    //-------------------------------------------------------------------
    m_GUI.AddView(0, 19, 800, 600, &IDV_TOPVIEW, 0, NULL, &pView);
    ZeroMemory(&e, sizeof(CZJGUIElement));
    SetRect(&rcTexture, 0, 0, 128, 64);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_LOGO1, &rcTexture);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_LOGO2, &rcTexture);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_LOGO3, &rcTexture);
    m_GUI.CreateElement(&e, &IDE_LOGO);

    pView->AddButton(IDC_LOGO, NULL, 695, 15, 88, 33, IDE_LOGO);

    pView->SetCallBack(::GUICallBackMainView);

    m_GUI.SetView(IDV_TOPVIEW, false, 0.3f);
    m_GUI.SetTopView(IDV_TOPVIEW, true);
    //-------------------------------------------------------------------
    m_GUI.AddView(0, 19, 800, 600, &IDV_MAIN, IDT_MP_UI_HOST, NULL, &pView);

    ZeroMemory(&e, sizeof(CZJGUIElement));
    SetRect(&rcTexture, 986, 14, 1006, 687);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    m_GUI.CreateElement(&e, &IDE_SCROLLBAR);

    ZeroMemory(&e, sizeof(CZJGUIElement));
    SetRect(&rcTexture, 954, 120, 966, 441);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    m_GUI.CreateElement(&e, &IDE_SCROLLBAR_THUMB);

    SetRect(&rcPadding, 0, 0, 0, 0);
    SetRect(&rcTrack, 515, 232, 515 + 15, 232 + 176);
    pView->AddScrollBar(0, 510, 230, 25, 180, &rcTrack, IDE_SCROLLBAR, IDE_SCROLLBAR_THUMB, &pScrollBar);

    ZeroMemory(&e, sizeof(CZJGUIElement));
    SetRect(&rcTexture, 148, 195, 510, 250);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_BUTTON_DROP, &rcTexture);
    e.SetTexture(ZJGUI_STATE_SELECTED, IDT_BUTTON_DROP, &rcTexture);
    SetRect(&rcTexture, 148, 130, 510, 185);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_BUTTON_DROP, &rcTexture);
    SetRect(&rcTexture, 148, 260, 510, 315);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_BUTTON_DROP, &rcTexture);

    e.SetFont(IDF_K18);
    e.SetFontColor(ZJGUI_STATE_NORMAL);
    e.SetFontColor(ZJGUI_STATE_SELECTED);
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER);
    e.SetFontColor(ZJGUI_STATE_PRESSED);
    m_GUI.CreateElement(&e, &IDE_COMBOBOX_ITEM);

    SetRect(&rcPadding, 0, 0, 0, 0);
    pView->AddListBox(0, 270, 230, 240, 180, &rcPadding, 0, 240, 30, 1, IDE_COMBOBOX_ITEM, pScrollBar, &pListBox);
    //pListBox->UpdateScrollBar();

    CFileFind ff;
    BOOL bFind = ff.FindFile(theCore.m_strGamePath + _T("Modules\\*.*"));
    int i;
    while (bFind)
    {
        bFind = ff.FindNextFile();
        if (ff.IsDirectory() && !ff.IsDots())
        {
            //CString tmp = ff.GetFilePath() + _T("module.ini");
            if (PathFileExists(ff.GetFilePath() + _T("\\module.ini")))
            {
                i = theCore.m_Modules.GetSize();
                theCore.m_Modules.Increase();
                theCore.m_Modules[i].Path = ff.GetFilePath();
                pListBox->AddItem(ff.GetFileName());
            }
        }
    }
    pListBox->UpdateScrollBar();

    ZeroMemory(&e, sizeof(CZJGUIElement));
    SetRect(&rcTexture, 148, 390, 510, 445);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_BUTTON_DROP, &rcTexture);
    e.SetTexture(ZJGUI_STATE_SELECTED, IDT_BUTTON_DROP, &rcTexture);
    SetRect(&rcTexture, 148, 325, 510, 380);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_BUTTON_DROP, &rcTexture);
    SetRect(&rcTexture, 148, 455, 510, 510);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_BUTTON_DROP, &rcTexture);

    e.SetFont(IDF_K18);
    e.SetFontColor(ZJGUI_STATE_NORMAL);
    e.SetFontColor(ZJGUI_STATE_SELECTED);
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER);
    e.SetFontColor(ZJGUI_STATE_PRESSED);
    m_GUI.CreateElement(&e, &IDE_COMBOBOX);

    pView->AddComboBox(0, 270, 200, 254, 30, IDE_COMBOBOX, pListBox, 0, &m_pModulesComboBox);

    ZeroMemory(&e, sizeof(CZJGUIElement));
    e.SetFont(IDF_K18, DT_NOCLIP);
    e.SetFontColor(ZJGUI_STATE_NORMAL);
    m_GUI.CreateElement(&e, &IDE_STATIC1);

    pView->AddStaticText(0, _T("选择剧本："), 230, 170, 0, 0, IDE_STATIC1);


    ZeroMemory(&e, sizeof(CZJGUIElement));
    SetRect(&rcTexture, 0, 0, 128, 64);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_PREMENU_BUTTON1, &rcTexture);
    SetRect(&rcTexture, 0, 64, 128, 128);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_PREMENU_BUTTON1, &rcTexture);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_PREMENU_BUTTON1, &rcTexture);
    m_GUI.CreateElement(&e, &IDE_PREMENU_BUTTON1);

    ZeroMemory(&e, sizeof(CZJGUIElement));
    SetRect(&rcTexture, 0, 0, 128, 64);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_PREMENU_BUTTON2, &rcTexture);
    SetRect(&rcTexture, 0, 64, 128, 128);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_PREMENU_BUTTON2, &rcTexture);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_PREMENU_BUTTON2, &rcTexture);
    m_GUI.CreateElement(&e, &IDE_PREMENU_BUTTON2);

    ZeroMemory(&e, sizeof(CZJGUIElement));
    SetRect(&rcTexture, 0, 0, 128, 64);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_PREMENU_BUTTON3, &rcTexture);
    SetRect(&rcTexture, 0, 64, 128, 128);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_PREMENU_BUTTON3, &rcTexture);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_PREMENU_BUTTON3, &rcTexture);
    m_GUI.CreateElement(&e, &IDE_PREMENU_BUTTON3);

    pView->AddButton(IDC_PREMENU_ENTER, 0, 335, 245, 125, 45, IDE_PREMENU_BUTTON1);
    pView->AddButton(0, 0, 335, 290, 125, 45, IDE_PREMENU_BUTTON2);
    pView->AddButton(IDC_QUIT, 0, 335, 335, 125, 45, IDE_PREMENU_BUTTON3);

    pView->SetCallBack(::GUICallBackMainView);

    m_GUI.SetView(IDV_MAIN, true);

    //--------------------------------------------------------------

    m_GUI.AddView(0, 19, 800, 600, &IDV_MAINMENU, IDT_MAINMENU, NULL, &pView);

    ZeroMemory(&e, sizeof(CZJGUIElement));
    e.SetFont(IDF_K18);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(64, 31, 13));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(246, 216, 22));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(246, 216, 22));
    e.SetFontColor(ZJGUI_STATE_DISABLED, D3DCOLOR_XRGB(115, 102, 72));
    m_GUI.CreateElement(&e, &IDE_MAINMENU_BUTTON);

    pView->AddButton(0, _T("访问主页"), 55, 174, 125, 20, IDE_MAINMENU_BUTTON);
    pView->AddButton(0, _T("访问中文站帖子"), 55, 204, 125, 20, IDE_MAINMENU_BUTTON);
    pView->AddButton(IDC_LOADSAVEFILE, _T("载入存档"), 55, 234, 125, 20, IDE_MAINMENU_BUTTON);
    pView->AddButton(0, _T("保存"), 55, 264, 125, 20, IDE_MAINMENU_BUTTON, 0, &m_WSEOptions[0]);
    pView->AddButton(0, _T("另存为"), 55, 294, 125, 20, IDE_MAINMENU_BUTTON, 0, &m_WSEOptions[1]);
    pView->AddButton(IDC_MODIFY_SAVEDATA, _T("修改存档"), 55, 324, 125, 20, IDE_MAINMENU_BUTTON, 0, &m_WSEOptions[2]);
    pView->AddButton(0, _T("修改剧本"), 55, 354, 125, 20, IDE_MAINMENU_BUTTON);
    pView->AddButton(IDC_QUIT, _T("退出"), 55, 384, 125, 20, IDE_MAINMENU_BUTTON);
    //pView->AddStaticText(0, _T("OK"), 335, 245, 125, 45, IDE_STATIC1);

    pView->SetExtraRender(::RenderTitle);
    pView->SetCallBack(::GUICallBackMainView);
    //-----------------------------------------------------------------------
    SetRect(&rcTexture, 0, 0, 1024, 1024 - 188);
    m_GUI.AddView(0, 19, 800, 600, &IDV_MODIFY_SAVEDATA_MENU, IDT_BG2, &rcTexture, &pView);

    ZeroMemory(&e, sizeof(CZJGUIElement));
    e.SetFont(IDF_K18);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(0, 0, 0));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(93, 233, 79));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(93, 233, 79));
    m_GUI.CreateElement(&e, &IDE_GAMEMENU_ITEM);

    pView->AddButton(IDC_MODIFY_CHARACTER, _T("修改人物"), 350, 300, 100, 20, IDE_GAMEMENU_ITEM);
    pView->AddButton(0, _T("修改物品"), 350, 330, 100, 20, IDE_GAMEMENU_ITEM);
    pView->AddButton(IDC_BACKTOMAINMENU, _T("返回"), 350, 360, 100, 20, IDE_GAMEMENU_ITEM, VK_TAB);

    pView->SetCallBack(::GUICallBackMainView);
    //-------------------------------------------------------------------------

    m_GUI.AddView(0, 19, 800, 600, &IDV_MODIFY_CHARACTER, IDT_CHARACTER_WINDOW, NULL, &pView);

    ZeroMemory(&e, sizeof(e));
    e.SetFont(IDF_K20, DT_NOCLIP);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(64, 31, 13));
    m_GUI.CreateElement(&e, &IDE_STATIC2);

    pView->AddStaticText(0, _T("等级"), 285, 66, 0, 0, IDE_STATIC2);
    pView->AddStaticText(0, _T("经验值"), 285, 86, 0, 0, IDE_STATIC2);

    pView->AddStaticText(IDC_LEVEL, NULL, 285 + 60, 66, 0, 0, IDE_STATIC2);
    pView->AddStaticText(IDC_XP, NULL, 285 + 60 + 20, 66 + 20, 0, 0, IDE_STATIC2);

    e.SetFont(IDF_K17, DT_NOCLIP);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(25, 37, 65));
    m_GUI.CreateElement(&e, &IDE_STATIC3);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(58, 82, 36));
    m_GUI.CreateElement(&e, &IDE_STATIC4);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(91, 62, 39));
    m_GUI.CreateElement(&e, &IDE_STATIC5);

    pView->AddStaticText(0, _T("属性"), 66, 388, 0, 0, IDE_STATIC3);
    pView->AddStaticText(0, _T("技能"), 315, 175, 0, 0, IDE_STATIC4);
    pView->AddStaticText(0, _T("人物列表"), 600, 258, 0, 0, IDE_STATIC5);

    e.SetFont(IDF_K15, DT_NOCLIP);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(25, 37, 65));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(37, 46, 209));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(37, 46, 209));
    m_GUI.CreateElement(&e, &IDE_ATTRIBUTES);

    pView->AddButton(0, _T("力："), 40, 418, 180, 15, IDE_ATTRIBUTES);
    pView->AddButton(0, _T("敏："), 40, 418 + 25, 180, 15, IDE_ATTRIBUTES);
    pView->AddButton(0, _T("智："), 40, 418 + 50, 180, 15, IDE_ATTRIBUTES);
    pView->AddButton(0, _T("魅："), 40, 418 + 75, 180, 15, IDE_ATTRIBUTES);
    
    e.SetFont(IDF_K14, DT_NOCLIP);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(58, 82, 36));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(32, 175, 25));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(32, 175, 25));
    m_GUI.CreateElement(&e, &IDE_SKILLS);

    //theCore.ModuleData.m_numSkls
    int x = 278, y = 200;
    for (int i = 0; i < sizeof(tmp_Skls) / sizeof(tmp_Skls[0]); ++i)
    {
        pView->AddButton(0, tmp_Skls[i], x, y, 240, 14, IDE_SKILLS);
        y += 14;
    }

    pView->AddStaticText(0, _T("属性点"), 48, 535, 0, 0, IDE_STATIC3);
    pView->AddStaticText(0, _T("技能点"), 310, 535, 0, 0, IDE_STATIC4);
    pView->AddStaticText(0, _T("武器点"), 310, 555, 0, 0, IDE_STATIC5);

    ZeroMemory(&e, sizeof(e));
    SetRect(&rcTexture, 12, 545, 602, 610);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_INTERFACE, &rcTexture);
    OffsetRect(&rcTexture, 0, -100 + 1);
    e.SetTexture(ZJGUI_STATE_FOCUSED, IDT_INTERFACE, &rcTexture);
    e.SetFont(IDF_S18);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(6, 5, 4));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(6, 5, 4));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(6, 5, 4));
    e.SetFontColor(ZJGUI_STATE_FOCUSED, D3DCOLOR_XRGB(6, 5, 4));
    m_GUI.CreateElement(&e, &IDE_EDITBOX1);

    pView->AddSimpleEditBox(IDC_NAME, _T("Zi-Jin.com"), 245, 15, 310, 35, IDE_EDITBOX1, false, 14);

    ZeroMemory(&e, sizeof(e));
    SetRect(&rcTexture, 330, 115, 565, 195);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_INTERFACE, &rcTexture);
    OffsetRect(&rcTexture, 0, -100);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_INTERFACE, &rcTexture);

    e.SetFont(IDF_K20);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(9, 8, 7));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(24, 127, 17));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(24, 127, 17));
    m_GUI.CreateElement(&e, &IDE_BUTTON1);

    pView->AddButton(IDC_BACKTOMODIFYMENU, _T("完成"), 685, 555, 100, 35, IDE_BUTTON1, VK_TAB);

    ZeroMemory(&e, sizeof(e));
    SetRect(&rcTexture, 757, 220, 931, 284);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_FOCUSED, IDT_INTERFACE, &rcTexture);

    e.SetFont(IDF_K14);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(255, 255, 255));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(255, 255, 255));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(255, 255, 255));
    e.SetFontColor(ZJGUI_STATE_FOCUSED, D3DCOLOR_XRGB(255, 255, 255));
    m_GUI.CreateElement(&e, &IDE_EDITBOX2);

    x = 470;
    y = 200;
    for (int i = 0, j = IDC_SKILLVAL_BEGIN; i < sizeof(tmp_Skls) / sizeof(tmp_Skls[0]); ++i, ++j)
    {
        pView->AddSimpleEditBox(j, NULL, x, y, 48, 14, IDE_EDITBOX2, true, 2);
        y += 14;
    }

    pView->AddSimpleEditBox(IDC_STR, NULL, 120, 418, 96, 15, IDE_EDITBOX2, true, 10);
    pView->AddSimpleEditBox(IDC_AGI, NULL, 120, 418 + 25, 96, 15, IDE_EDITBOX2, true, 10);
    pView->AddSimpleEditBox(IDC_INT, NULL, 120, 418 + 50, 96, 15, IDE_EDITBOX2, true, 10);
    pView->AddSimpleEditBox(IDC_CHA, NULL, 120, 418 + 75, 96, 15, IDE_EDITBOX2, true, 10);

    pView->AddSimpleEditBox(IDC_ATTPOINT, NULL, 48 + 65, 535 + 2, 96, 15, IDE_EDITBOX2, true, 10);
    pView->AddSimpleEditBox(IDC_SKLPOINT, NULL, 310 + 65, 535 + 2, 96, 15, IDE_EDITBOX2, true, 10);
    pView->AddSimpleEditBox(IDC_WPNPOINT, NULL, 310 + 65, 555 + 2, 96, 15, IDE_EDITBOX2, true, 10);

    ZeroMemory(&e, sizeof(e));
    SetRect(&rcTexture, 650, 135, 680, 180);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_INTERFACE, &rcTexture);

    e.SetFont(IDF_K20, DT_NOCLIP | DT_VCENTER);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(64, 31, 13));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(64, 31, 13));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(64, 31, 13));
    
    m_GUI.CreateElement(&e, &IDE_CHECKBOX1);

    SetRect(&rcTexture, 682, 135, 712, 180);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_MOUSEOVER, IDT_INTERFACE, &rcTexture);
    e.SetTexture(ZJGUI_STATE_PRESSED, IDT_INTERFACE, &rcTexture);
    
    m_GUI.CreateElement(&e, &IDE_CHECKBOX1_CHECKED);

    pView->AddCheckBox(IDC_CANNOTQUITWITHOUTSAVE, _T("直面人生"), 595, 66, 120, 45, 30, 45, IDE_CHECKBOX1, IDE_CHECKBOX1_CHECKED);

    ZeroMemory(&e, sizeof(e));
    e.SetFont(IDF_K15, DT_NOCLIP);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(91, 62, 39));
    e.SetFontColor(ZJGUI_STATE_MOUSEOVER, D3DCOLOR_XRGB(214, 48, 27));
    e.SetFontColor(ZJGUI_STATE_PRESSED, D3DCOLOR_XRGB(214, 48, 27));
    e.SetFontColor(ZJGUI_STATE_SELECTED, D3DCOLOR_XRGB(91, 62, 39));
    m_GUI.CreateElement(&e, &IDE_NPCLIST_ITEM);

    SetRect(&rcTrack, 575 + 192 + 2, 287 + 3, 575 + 192 + 10 - 2, 287 + 15 * 16 - 3);
    pView->AddScrollBar(0, 575 + 192, 287, 10, 15 * 16, &rcTrack, IDE_SCROLLBAR, IDE_SCROLLBAR_THUMB, &pScrollBar);
    pView->AddListBox(IDC_NPCLIST, 575, 287, 192, 15 * 16, NULL, 0, 192, 15, 1, IDE_NPCLIST_ITEM, pScrollBar, &theWSE.m_pNPCList);

    pView->SetCallBack(::GUICallBackMainView);
    //-----------------------------------------------------------------------------
        
    m_GUI.AddView(0, 19, 800, 600, &IDV_MODIFY_ITEM, IDT_INVENTORY_WINDOW, NULL, &pView);


    //-----------------------------------------------------------------------------
        
    m_GUI.AddView(0, 19, 800, 600, &IDV_LOADSCREEN, IDT_LOADSCREEN, NULL, &pView);

    ZeroMemory(&e, sizeof(e));
    SetRect(&rcTexture, 657 + 5, 1024 - 266, 914 - 3, 1024 - 248);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    m_GUI.CreateElement(&e, &IDE_PROGRESSBAR);

    SetRect(&rcTexture, 605, 55, 860, 75);
    e.SetTexture(ZJGUI_STATE_NORMAL, IDT_INTERFACE, &rcTexture);
    m_GUI.CreateElement(&e, &IDE_PROGRESSHANDLE);

    SetRect(&rcPadding, 3, 4, 5, 4);
    pView->AddProgressBar(0, 74, 516, 654, 25, &rcPadding, IDE_PROGRESSBAR, IDE_PROGRESSHANDLE, &theCore.m_pProgressBar);

    ZeroMemory(&e, sizeof(e));
    e.SetFont(IDF_K24);
    e.SetFontColor(ZJGUI_STATE_NORMAL, D3DCOLOR_XRGB(178, 162, 34));
    m_GUI.CreateElement(&e, &IDE_LOADSCREENTEXT);

    pView->AddStaticText(0, NULL, 74, 546, 654, 21, IDE_LOADSCREENTEXT, &theCore.m_pLoadingFileText);

#if defined(DEBUG)
    pView->AddStaticText(0, NULL, 628, 92, 0, 0, IDE_STATIC1, &theCore.m_pCostTimeText);
#endif

    pView->SetPreRender(::PreRenderLoadScreen);
    //------------------------------------------------------------------------------
    theCore.SetMainFrm(this);
    CheckWSEStatus();
}


void CMainFrame::Shutdown(void)
{
}


void CMainFrame::Update(bool bSaveAndValidate)
{
    CDataExchange DX(this, bSaveAndValidate), *pDX;
    pDX = &DX;
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_STR, theWSE.pCurTroop->attSTR);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_AGI, theWSE.pCurTroop->attAGI);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_INT, theWSE.pCurTroop->attINT);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_CHA, theWSE.pCurTroop->attCHA);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_ATTPOINT, theWSE.pCurTroop->nAttributePoints);

    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_SKLPOINT, theWSE.pCurTroop->nSkillPoints);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 37, theWSE.pCurTroop->sklTrade);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 36, theWSE.pCurTroop->sklLeadership);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 35, theWSE.pCurTroop->sklPManagement);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT19, theWSE.pCurTroop->sklR[1]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT20, theWSE.pCurTroop->sklR[2]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT21, theWSE.pCurTroop->sklR[3]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT22, theWSE.pCurTroop->sklR[4]);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 34, theWSE.pCurTroop->sklPersuation);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 33, theWSE.pCurTroop->sklEngineer);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 32, theWSE.pCurTroop->sklFirstAid);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 31, theWSE.pCurTroop->sklSurgery);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 30, theWSE.pCurTroop->sklWTreatment);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 29, theWSE.pCurTroop->sklIManagement);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 28, theWSE.pCurTroop->sklSpotting);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 27, theWSE.pCurTroop->sklPathfinding);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 26, theWSE.pCurTroop->sklTactics);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 25, theWSE.pCurTroop->sklTracking);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 24, theWSE.pCurTroop->sklTrainer);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT34, theWSE.pCurTroop->sklR[5]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT35, theWSE.pCurTroop->sklR[6]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT36, theWSE.pCurTroop->sklR[7]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT37, theWSE.pCurTroop->sklR[8]);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 23, theWSE.pCurTroop->sklLooting);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 22, theWSE.pCurTroop->sklHArchery);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 21, theWSE.pCurTroop->sklRiding);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 20, theWSE.pCurTroop->sklAthletics);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 19, theWSE.pCurTroop->sklShield);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 18, theWSE.pCurTroop->sklWMaster);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT44, theWSE.pCurTroop->sklR[9]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT45, theWSE.pCurTroop->sklR[10]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 20, theWSE.pCurTroop->sklR[11]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 19, theWSE.pCurTroop->sklR[12]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 18, theWSE.pCurTroop->sklR[13]);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 17, theWSE.pCurTroop->sklPDraw);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 16, theWSE.pCurTroop->sklPThrow);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 15, theWSE.pCurTroop->sklPStrike);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, 14, theWSE.pCurTroop->sklIronflesh);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT53, theWSE.pCurTroop->sklR[14]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT54, theWSE.pCurTroop->sklR[15]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT55, theWSE.pCurTroop->sklR[16]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT56, theWSE.pCurTroop->sklR[17]);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT57, theWSE.pCurTroop->sklR[18]);

    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_WPNPOINT, theWSE.pCurTroop->nWeaponPoints);

    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT14, *theWSE.pCurTroop->pszFaceKey1);
    //ZJGDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_EDIT15, *theWSE.pCurTroop->pszFaceKey2);

    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_NAME, *theWSE.pCurTroop->pName);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_LEVEL, theWSE.pCurTroop->nLevel);
    DDX_Text(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_XP, theWSE.pCurTroop->nXP);

    DDX_Check(pDX, m_GUI, IDV_MODIFY_CHARACTER, IDC_CANNOTQUITWITHOUTSAVE, theWSE.m_bCannotQuitWithoutSave);

    //DDX_Text(pDX, m_guiItemWindow, IDC_PLAYERNAME, *theWSE.trpPlayer.pName);
    //DDX_Text(pDX, m_guiItemWindow, IDC_MONEY, theWSE.trpPlayer.nMoney);

    CheckWSEStatus();
}


void CMainFrame::CheckWSEStatus()
{
    for (int i = 0; i < 3; ++i)
    {
        m_WSEOptions[i]->Enable(!theWSE.IsEmpty());
    }
}


LRESULT CMainFrame::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_SETCURSOR)
    {
        SetCursor(theApp.LoadCursorW(MAKEINTRESOURCE(IDC_CURSOR1)));
        return TRUE;
    }

    LRESULT lr = CFrameWndEx::WindowProc(msg, wParam, lParam);

    switch (msg)
    {
    case WM_SHOWWINDOW:
        
        break;
    }

    return lr;
}

void CMainFrame::GUICallBackMainView(UINT Event, int ID, CZJGUIControl* pCtrl)
{
    if (Event == EVENT_BUTTON_PRESSED || Event == EVENT_LISTBOX_ITEM_PRESSED)
    {
        PlaySound(MAKEINTRESOURCE(IDR_WAVE1), NULL, SND_RESOURCE | SND_ASYNC);
    }

    switch (ID)
    {
    case IDC_PREMENU_ENTER:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            theCore.LoadModule(m_pModulesComboBox->GetSelectedIndex());
            return;
        }
        break;
    case IDC_QUIT:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            PostQuitMessage(0);
            return;
        }
        break;
    case IDC_LOADSAVEFILE:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            theWSE.OnFileOpen();
            return;
        }
        break;
    case IDC_MODIFY_SAVEDATA:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            m_GUI.TurnView(IDV_MODIFY_SAVEDATA_MENU);
            return;
        }
        break;
    case IDC_MODIFY_CHARACTER:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            m_GUI.TurnView(IDV_MODIFY_CHARACTER);
            return;
        }
        break;
    case IDC_BACKTOMODIFYMENU:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            Update(true);
            m_GUI.TurnView(IDV_MODIFY_SAVEDATA_MENU);
            return;
        }
        break;
    case IDC_BACKTOMAINMENU:
        if (Event == EVENT_BUTTON_CLICKED)
        {
            m_GUI.TurnView(IDV_MAINMENU);
            return;
        }
        break;

    case IDC_NPCLIST:
        if (Event == EVENT_LISTBOX_SELECTION)
        {
            Update(true);
            if (theWSE.m_pNPCList->GetSelectedIndex() == 0)
                theWSE.pCurTroop = &theWSE.trpPlayer;
            else
                theWSE.pCurTroop = &theWSE.trpNpc[theWSE.m_pNPCList->GetSelectedIndex() - 1];
            Update(false);
        }
        break;
    }
}


void CMainFrame::RenderTitle()
{
    D3DXMATRIX matView;
    D3DXVECTOR3 Eye(0.0f, 0.0f, -5.0f);
    D3DXVECTOR3 At(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 Up(0.0f, 1.0f, 0.0f);

    D3DXMatrixLookAtLH(&matView, &Eye, &At, &Up);
    m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

    RECT rc;
    ::GetClientRect(m_wndView, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    D3DXMATRIX proj;
    D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI * 0.25f, float(width) / height, 0.1f, 1000.0f);
    m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &proj);
    m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
    m_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);//允许绘制背面

    D3DXMATRIXA16 matWorld, matScale, matTrans, matRotate;
    D3DXMatrixIdentity(&matWorld);
    
    m_pd3dDevice->SetMaterial(&m_mtrlTitle);
    
    float a = ((float)D3DX_PI) / 6 * sinf(GetTickCount() / 1000.0f);
    
    D3DXMatrixScaling(&matScale, 0.13f, 0.13f, 0.13f);
    D3DXMatrixTranslation(&matTrans, -1.9f, 1.1f, 0.0f);

    D3DXVECTOR3 axis(0.0f, 1.0f, 0.0f);
    D3DXVECTOR4 vec4;
    D3DXVec3Transform(&vec4, &axis, &matTrans);
    memcpy(&axis, &vec4, sizeof(axis));
    D3DXMatrixRotationAxis(&matRotate, &axis, a);
    matWorld = m_matTitlePre * matScale * matTrans * matRotate;

    m_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
    
    // Light
    D3DXCOLOR color = D3DCOLOR_XRGB(255, 255, 255);
    D3DLIGHT9 light;
    ZeroMemory(&light, sizeof(light));
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Ambient = color * 0.6f;
    light.Diffuse = color;
    light.Specular = color * 0.6f;
    light.Direction = D3DXVECTOR3(0.0f, -0.5f, 1.0f);
    light.Position = D3DXVECTOR3(12.0f, -0.2f, -4.0f);

    m_pd3dDevice->SetLight(0, &light);
    m_pd3dDevice->LightEnable(0, TRUE);

    m_pTitleMesh->DrawSubset(0);
}


void CMainFrame::PreRenderLoadScreen()
{

#if defined(DEBUG)

    wchar_t *wstr = utow(GetTickCount() - theCore.m_dwStartTime);
    theCore.m_pCostTimeText->SetText(wstr);
    delete[] wstr;

#endif

}
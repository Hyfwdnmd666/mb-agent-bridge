
#include "MainWnd.h"

#include "ZBL/Algorithm/KMP.h"
#pragma comment(lib, "ws2_32.lib")

LRESULT CALLBACK       WndProc(HWND, UINT, WPARAM, LPARAM);

cMainWnd::cMainWnd() 
{
    m_pDevice = NULL;
    m_pD3D = NULL;
    
#if 0
    // IniSet
    FILE *fp;
    fopen_s(&fp, "data\\ini\\config.ini", "rb");
    if (fp == NULL)
    {
        fopen_s(&fp, "data\\ini\\config.ini", "wb");

        const char _iniConfig[] = 
            ";������֮�䡷�����ļ�\r\n"
            ";  �����ļ������ڣ������ʹ��Ĭ�����ã����Զ��������ļ�\r\n"
            "\r\n"
            "\r\n"
            "[display]\r\n"
            "Version = 0.00.28252.0\r\n"
            "\r\n"
            ";\r\n"
            ";DirectX10\r\n"
            ";  Default:  0 - (ʹ��DirectX9)\r\n"
            ";            1 - (ʹ��DirectX10)\r\n"
            "\r\n"
            ";DirectX10 = 0 (��ȡ����ѡ��)\r\n"
            "\r\n"
            ";\r\n"
            ";DeviceType\r\n"
            ";  Default:  1 - (Ӳ����Ⱦ)\r\n"
            ";            0 - (������Ⱦ���ٶȽ�����������ڲ�֧��Ӳ����Ⱦʱʹ��)\r\n"
            "\r\n"
            "DeviceType = 1\r\n"
            "\r\n"
            ";\r\n"
            ";FullScreen\r\n"
            ";  Default:  0 - (����ģʽ)\r\n"
            ";            1 - (ȫ��ģʽ)\r\n"
            "\r\n"
            "FullScreen = 0\r\n"
            "\r\n"
            ";\r\n"
            ";DisplayMode\r\n"
            ";  Default:  0 - (800x600)\r\n"
            ";            1 - (1024x768)\r\n"
            ";            2 - (1280x800)\r\n"
            "\r\n"
            "DisplayMode = 0\r\n"
            "\r\n"
            "\r\n"
            "[sound]\r\n"
            "Version = 0.00.28252.0\r\n"
            "\r\n"
            ";\r\n"
            ";Silence\r\n"
            ";  Default:  0\r\n"
            ";            1 - (����)\r\n"
            "\r\n"
            "Silence = 0\r\n"
            "\r\n"
            "\r\n"
            "[save]\r\n"
            "Version = 0.00.28252.0\r\n"
            "\r\n"
            ";\r\n"
            ";UseMyDocumentsFolder\r\n"
            // ÿ���浵һ���ļ���
            ";  Default:  1 - (�浵Ŀ¼Ϊ �ҵ��ĵ�/Zi-Jin.com/����֮��/savegames)\r\n"
            ";            0 - (�浵Ŀ¼Ϊ data/savegames)\r\n"
            "\r\n"
            "UseMyDocumentsFolder = 1\r\n"
            "\r\n"
            "\r\n";

        fwrite(_iniConfig, 1, strlen(_iniConfig), fp);
        fclose(fp);

        m_iniConfig.m_bAllDefault = true;
    }
    else
    {
        fclose(fp);
        m_iniConfig.LoadFile("data\\ini\\config.ini");
    }
#endif
    // For Test
#if 0
    cIniSet iniConfig;
    iniConfig.LoadFile("data\\ini\\config.ini");
    //FILE *fp;
    fopen_s(&fp, "data\\ini\\test.txt", "wb");
    char *str = iniConfig.m_Tables[0]->m_strName;
    fwrite(str, 1, strlen(str), fp);
    char _rn[3] = "\r\n";
    fwrite(_rn, 1, 2, fp);
    for (int i = 0; i < iniConfig.m_Tables[0]->m_Nodes.GetSize(); ++i)
    {
        str = iniConfig.m_Tables[0]->m_Nodes[i]->m_strName;
        fwrite(str, 1, strlen(str), fp);
        fwrite(_rn, 1, 2, fp);
        str = iniConfig.m_Tables[0]->m_Nodes[i]->m_strVal;
        //ASSERT(str == iniConfig.GetText(iniConfig.m_Tables[0]->m_strName, iniConfig.m_Tables[0]->m_Nodes[i]->m_strName, NULL));
        fwrite(str, 1, strlen(str), fp);
        fwrite(_rn, 1, 2, fp);
        fwrite(_rn, 1, 2, fp);
    }
    fclose(fp);
#endif

    
    // Styles Goes here

    m_strWindowName = "�����￳�޸���";

    m_dwClsStyle = CS_HREDRAW | CS_VREDRAW;
    m_lpfnWndProc = ::WndProc;

#if 1
    m_nWidth = 800;
    m_nHeight = 600;
#else
    switch (m_iniConfig.GetInt("display", "DisplayMode", 0))
    {
    case 0:
        m_nWidth = 800;
        m_nHeight = 600;
        break;

    case 1:
        m_nWidth = 1024;
        m_nHeight = 768;
        break;

    case 2:
        m_nWidth = 1280;
        m_nHeight = 800;
        break;
    }
#endif

#if 0
    m_bFullScreen = m_iniConfig.GetInt("display", "FullScreen", 0);

    if (m_bFullScreen)
    {
        m_dwStyle = WS_POPUP;
        m_dwExStyle = WS_EX_TOPMOST;
    }
#endif
    /*
    // Cursor

    const char *strCursorFileName = "hsl3anicursor";
    char strPath[MAX_PATH];
    GetTempPath(MAX_PATH - strlen(strCursorFileName), strPath);
    strcat(strPath, strCursorFileName);
    
    HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(IDR_CURSOR1), "FILE");

    HGLOBAL pData = LoadResource(NULL, hResInfo);
    WriteFile(strPath, (char *)pData, SizeofResource(NULL, hResInfo));
    GlobalFree(pData);
    
    m_hCursor = LoadCursorFromFile(strPath);
    DeleteFile(strPath);
    */

    // Icon
    m_hIcon = LoadIcon(m_hInst, MAKEINTRESOURCE(IDI_ICON1));
    //m_hIconSm = LoadIcon((HINSTANCE)m_hInst, MAKEINTRESOURCE(IDI_ICON1));
        //LoadCursor((HINSTANCE)m_hInst, MAKEINTRESOURCE(IDC_CURSOR1));
}


cMainWnd::~cMainWnd()
{
    m_Input.Shutdown();
}



//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  WM_COMMAND	- ����Ӧ�ó���˵�
//  WM_PAINT	- ����������
//  WM_DESTROY	- �����˳���Ϣ������
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int cMainWnd::CreateWindow()
{
    int cxscreen = GetSystemMetrics(SM_CXSCREEN);
    int cyscreen = GetSystemMetrics(SM_CYSCREEN);
    
#if 0
    if (m_bFullScreen)
    {
        DEVMODE DevMode = {0};
        DevMode.dmSize = sizeof(DEVMODE);

        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DevMode);

        if (m_nWidth != cxscreen || m_nHeight != cyscreen)
        {
            DevMode.dmPelsWidth = m_nWidth;
            DevMode.dmPelsHeight = m_nHeight;

            long lr = ChangeDisplaySettings(&DevMode, 0);
            if (lr == DISP_CHANGE_SUCCESSFUL)
            {
                ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
            }
            else
            {
                ChangeDisplaySettings(NULL, 0);
                MessageBox(NULL, _T("����ȫ��ʧ�ܣ���ʹ�ô���ģʽ���У�"), _T("�޷�ȫ��"), MB_OK);
                m_bFullScreen = false;
                m_dwStyle = WS_SYSMENU | WS_CAPTION;
                m_dwExStyle = 0;
            }
        }
    }
#endif

    if (cWnd::CreateWindow(0, 0, false) == -1)
        return -1;
    //if (!m_bFullScreen)
    //{
        RECT rc;
        GetClientRect(m_hWnd, &rc);
        int w = m_nWidth;
        int h = m_nHeight;
        if (w != rc.right)
            w += (m_nWidth - rc.right);
        if (h != rc.bottom)
            h += (m_nHeight - rc.bottom);
        int x = ((cxscreen - w) >> 1);
        int y = ((cyscreen - h) >> 1);
        MoveWindow(m_hWnd, x, y, w, h, FALSE);
    //}
    ShowWindow();

    m_Input.Initialize(m_hWnd, (HINSTANCE)m_hInst, true);//false);//true);
    m_Input.Acquire();

    return 0;
}

bool cMainWnd::InitializeDevice()
{
    // HRESULT hr;

    D3DDEVTYPE DeviceType = D3DDEVTYPE_HAL;

    D3DDISPLAYMODE d3ddm;
    D3DCAPS9 d3dcaps;

    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_pD3D)
        return false;

    if (FAILED( m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm) ))
        return false;

    if (FAILED( m_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, DeviceType, &d3dcaps) ))
        return false;

    DWORD VertexProcessing = (d3dcaps.VertexProcessingCaps ? D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE :
                                                             D3DCREATE_SOFTWARE_VERTEXPROCESSING);

    
    D3DPRESENT_PARAMETERS d3dpp = {0};
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = m_nWidth;
    d3dpp.BackBufferHeight = m_nHeight;
    d3dpp.hDeviceWindow = m_hWnd;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format; //D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    if (FAILED( m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                            DeviceType,
                            m_hWnd,
                            VertexProcessing,
                            &d3dpp,
                            &m_pDevice) ))
    {
        MessageBox(NULL, _T("����D3D�豸ʧ��"), _T("����D3D�豸ʧ��"), MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}


bool cMainWnd::InitializeRes()
{
    CreateD3D9Renderer(m_pDevice, &m_pRenderer);
    //m_Manager.Create(m_pDevice);

    m_pRenderer->CreateFont("����", 18, &IDF_K18);
    
    // Textures
    m_Textures.m_pRenderer = m_pRenderer;
    m_Textures.m_strCommonPath = m_GameData.m_strPath + "/Textures/";
    m_Textures.RequestSize(32); // Ԥ��
        
    cString str;
    GetTempPath(261, str.BeginBuffer(261));
    str.EndBuffer();
    str += "/t.png";
    GetWeiboPic(str);
    m_Textures.Load("weibo", str);

    m_Textures.LoadCommon("cursor", "cursor.dds");
    m_Textures.LoadCommon("mp_ui_host", "mp_ui_host.dds");
    m_Textures.LoadCommon("button_drop", "button_drop.dds");
    m_Textures.LoadCommon("interface", "interface.dds");
    m_Textures.LoadCommon("main_menu_nord", "main_menu_nord.dds");
    m_Textures.LoadCommon("mp_ui_bg", "mp_ui_bg.dds");
    m_Textures.LoadCommon("character_window", "character_window.dds");
    m_Textures.LoadCommon("bg2", "bg2.dds");
    m_Textures.LoadCommon("inventory_window", "inventory_window.dds");
    m_Textures.LoadCommon("loadscreen", "loadscreen.dds");
    //m_Textures.LoadCommon(

    m_Textures.Load("premenu_button1", 0, MAKEINTRESOURCEA(IDB_PNG1));
    m_Textures.Load("premenu_button2", 0, MAKEINTRESOURCEA(IDB_PNG2));
    m_Textures.Load("premenu_button3", 0, MAKEINTRESOURCEA(IDB_PNG3));
    m_Textures.Load("logo1", 0, MAKEINTRESOURCEA(IDB_PNG4));
    m_Textures.Load("logo2", 0, MAKEINTRESOURCEA(IDB_PNG5));
    m_Textures.Load("logo3", 0, MAKEINTRESOURCEA(IDB_PNG6));
    m_Textures.Load("weibob", 0, MAKEINTRESOURCEA(IDB_PNG7));

    m_Textures.ReleaseSpare();
    m_Textures.Sort(PAFD_strcmp);
    // end of Textures

    if (!InitModData())
        return false;

    return true;
}


bool cMainWnd::SetupGeometry()
{
    if (!SetupGUI())
        return false;

    cAnimation::m_pRenderer = m_pRenderer;

    m_aniCursor.CreateFromTexture(m_Textures.Get("cursor"), 1, 1);

    m_Cursor.SetSize(32, 32);
    m_Cursor.SetAnimation(&m_aniCursor);

    //if (!SetupWorld())
    //    return false;

    return true;
}

int cMainWnd::MainLoop()
{
    ProcessInput();
    
    //
    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);
    m_pDevice->BeginScene();

    m_GUI.Render();
    m_Cursor.Render();

    m_pDevice->EndScene();
    if (m_pDevice->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST)
    {
        if (m_pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
        {
            // m_pDevice->Reset(); Maybe later..
        }
    }
    //

    return 0;
}


void cMainWnd::ProcessInput()
{
    m_Input.Update();

    //----------------------------------

    if (m_Input.m_ptMousePos.x < 0)
        m_Input.m_ptMousePos.x = 0;
    if (m_Input.m_ptMousePos.x > m_nWidth)
        m_Input.m_ptMousePos.x = m_nWidth;
    if (m_Input.m_ptMousePos.y < 0)
        m_Input.m_ptMousePos.y = 0;
    if (m_Input.m_ptMousePos.y > m_nHeight)
        m_Input.m_ptMousePos.y = m_nHeight;

    // Cursor
    
    m_Cursor.SetPos(m_Input.m_ptMousePos.x - 3, m_Input.m_ptMousePos.y - 3);


    // GUI

    MSG msg;
    while (m_Input.GetMsg(&msg))
    {
        m_GUI.MsgProc(msg.message, msg.wParam, msg.lParam);
    }

    //POINT pt = m_Input.MousePos();
    //m_pButton->SetLocation(pt.x, pt.y);
    //m_pButton->UpdateRects();
}


bool cMainWnd::InitModData()
{
#if 0
    m_ModData.m_pWnd = this;

    if (!m_ModData.LoadImages())
        return false;

    if (!m_ModData.LoadAnimations())
        return false;

    if (!m_ModData.LoadMaps("data\\compiled\\maps.pak"))
        return false;
#endif

    return true;
}


bool cMainWnd::GetWeiboPic(const char *strSavePath)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1, 2), &wsaData))
    {
        return false;
    }

    hostent* phe = gethostbyname("sign.t.sina.com.cn");
    if (phe == NULL)
        return false;

    in_addr ip_addr;
    *(DWORD*)&ip_addr = *(DWORD*)phe->h_addr_list[0];

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr = ip_addr;

    const int maxsize = 1024 * 1024;
    char *pBuf;
    SOCKET sck = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sck, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        return false;
    
    pBuf = new char[maxsize];
    char request[] = 
            "GET   http://sign.t.sina.com.cn/widget/qmd/2319683162/039d33aa/4.png   HTTP/1.1\r\n"   
            "Host:sign.t.sina.com.cn\r\n"
            "Accept:*/*\r\n"
            "User-Agent:Mozilla/4.0   (compatible;   MSIE   5.00;   Windows   98)\r\n"
            "Pragma:   no-cache\r\n"
            "Cache-Control:   no-cache\r\n"
            "Keep-Alive: timeout=600, max=100\r\n"
            "Connection: Close\r\n\r\n";//"Connection: Keep-Alive\r\n\r\n";
    
    if (send(sck, request, strlen(request), 0) == SOCKET_ERROR)
            return false;

    int size = 0, tsize;
    while (true)
    {
        tsize = recv(sck, pBuf + size, maxsize - size, 0);
        if (tsize <= 0)
            break;
        else
            size += tsize;
    }

    closesocket(sck);

    if (size == 0)
        return false;

    pBuf[size] = '\0';

    bool bChunked = (KMP(pBuf, "Transfer-Encoding: chunked", 0, true) > -1);

    int i = KMP(pBuf, "Content-Type:", 0, true);

    while (pBuf[i++] != '\n');
    while (pBuf[i++] != '\n');

    if (bChunked)
    {
        sscanf(pBuf + i, "%x", &size);
        //while (pBuf[i++] != '\n');
    }
    else
    {
        size = size - i;
    }

    i = KMP(pBuf, "\x89\x50\x4E\x47\x0D");
    if (i < 0)
        return false;

    FILE *fp;
    fopen_s(&fp, strSavePath, "wb");
    fwrite(pBuf + i, size, 1, fp);
    fclose(fp);

    delete[] pBuf;

    WSACleanup();

    return true;
}